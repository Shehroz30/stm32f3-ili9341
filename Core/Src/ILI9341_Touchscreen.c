// MIT License
//
// Copyright (c) 2017 Matej Artnak
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------
// ILI9341 Touchscreen library for STM32
// -----------------------------------
// Very simple Touchscreen library for ILI9341.
// Extremely basic reading of position. No runtime calibration, No prediction, basic noise removal. Simple but stupid.
// Basic hardcoded calibration values saved in .h file
//
// Library is written for STM32 HAL library and supports STM32CubeMX. To use the library with Cube software
// you need to tick the box that generates peripheral initialization code in their own respective .c and .h file
//
// Touchpad GPIO defines
//   Outputs: CLK, MOSI, CS
//   Inputs : IRQ, MISO
//
// - Touchpad library bitbangs SPI interface and only requires basic GPIOs.
// - Setting GPIOs as FREQ_VERY_HIGH is recommended
// - Warning! Library is written for "ILI9341_Set_Rotation(SCREEN_VERTICAL_1)"
//   If using a different layout, you will need to re-map X and Y coordinates
//
// - NO_OF_POSITION_SAMPLES reduces noise but increases read time
//
// -------- EXAMPLE -------------
// if (TP_Touchpad_Pressed()) {
//     uint16_t x_pos = 0;
//     uint16_t y_pos = 0;
//     uint16_t position_array[2];
//     if (TP_Read_Coordinates(position_array) == TOUCHPAD_DATA_OK) {
//         x_pos = position_array[0];
//         y_pos = position_array[1];
//     }
// }
// ------------------------------

#include "ILI9341_Touchscreen.h"
#include "stm32f3xx_hal.h"

// Internal Touchpad command, do not call directly
uint16_t TP_Read(void) {
    uint8_t i = 16;
    uint16_t value = 0;

    while (i > 0x00) {
        value <<= 1;

        HAL_GPIO_WritePin(TP_CLK_PORT, TP_CLK_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(TP_CLK_PORT, TP_CLK_PIN, GPIO_PIN_RESET);

        if (HAL_GPIO_ReadPin(TP_MISO_PORT, TP_MISO_PIN) != 0) {
            value++;
        }

        i--;
    }

    return value;
}

// Internal Touchpad command, do not call directly
void TP_Write(uint8_t value) {
    uint8_t i = 0x08;

    HAL_GPIO_WritePin(TP_CLK_PORT, TP_CLK_PIN, GPIO_PIN_RESET);

    while (i > 0) {
        if ((value & 0x80) != 0x00) {
            HAL_GPIO_WritePin(TP_MOSI_PORT, TP_MOSI_PIN, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(TP_MOSI_PORT, TP_MOSI_PIN, GPIO_PIN_RESET);
        }

        value <<= 1;
        HAL_GPIO_WritePin(TP_CLK_PORT, TP_CLK_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(TP_CLK_PORT, TP_CLK_PIN, GPIO_PIN_RESET);

        i--;
    }
}

// Read coordinates of touchscreen press. Position[0] = X, Position[1] = Y
uint8_t TP_Read_Coordinates(uint16_t Coordinates[2]) {
    HAL_GPIO_WritePin(TP_CLK_PORT, TP_CLK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TP_MOSI_PORT, TP_MOSI_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TP_CS_PORT, TP_CS_PIN, GPIO_PIN_SET);

    uint32_t avg_x = 0, avg_y = 0;
    uint16_t rawx = 0, rawy = 0;
    uint32_t calculating_x = 0, calculating_y = 0;

    uint32_t samples = NO_OF_POSITION_SAMPLES;
    uint32_t counted_samples = 0;

    HAL_GPIO_WritePin(TP_CS_PORT, TP_CS_PIN, GPIO_PIN_RESET);

    while ((samples > 0) && (HAL_GPIO_ReadPin(TP_IRQ_PORT, TP_IRQ_PIN) == 0)) {
        TP_Write(CMD_RDY);
        rawy = TP_Read();
        avg_y += rawy;
        calculating_y += rawy;

        TP_Write(CMD_RDX);
        rawx = TP_Read();
        avg_x += rawx;
        calculating_x += rawx;

        samples--;
        counted_samples++;
    }

    HAL_GPIO_WritePin(TP_CS_PORT, TP_CS_PIN, GPIO_PIN_SET);

    if ((counted_samples == NO_OF_POSITION_SAMPLES) &&
        (HAL_GPIO_ReadPin(TP_IRQ_PORT, TP_IRQ_PIN) == 0)) {

        calculating_x /= counted_samples;
        calculating_y /= counted_samples;

        rawx = calculating_x;
        rawy = calculating_y;

        rawx *= -1;
        rawy *= -1;

        // Convert 16-bit values to screen coordinates
        Coordinates[0] = ((240 - (rawx / X_TRANSLATION)) - X_OFFSET) * X_MAGNITUDE;
        Coordinates[1] = ((rawy / Y_TRANSLATION) - Y_OFFSET) * Y_MAGNITUDE;

        return TOUCHPAD_DATA_OK;
    } else {
        Coordinates[0] = 0;
        Coordinates[1] = 0;
        return TOUCHPAD_DATA_NOISY;
    }
}

// Check if Touchpad was pressed. Returns TOUCHPAD_PRESSED (1) or TOUCHPAD_NOT_PRESSED (0)
uint8_t TP_Touchpad_Pressed(void) {
    if (HAL_GPIO_ReadPin(TP_IRQ_PORT, TP_IRQ_PIN) == 0) {
        return TOUCHPAD_PRESSED;
    } else {
        return TOUCHPAD_NOT_PRESSED;
    }
}
