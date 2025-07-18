/**
 * @file      initSequence.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-05-29
 *
 */

#include "initSequence.h"

const lcd_cmd_t rm67162_cmd[RM67162_INIT_SEQUENCE_LENGTH] = {
    {0xFE, {0x00}, 0x01}, //SET APGE 00H
    {0x11, {0x00}, 0x80}, // Sleep Out

    {0xFE, {0x05}, 0x01}, //SET APGE
    {0x05, {0x05}, 0x01}, //OVSS control set elvss -3.95v

    {0xFE, {0x01}, 0x01}, //SET APGE
    {0x73, {0x25}, 0x01}, //set OVSS voltage level.= -4.0V

    {0xFE, {0x00}, 0x01}, //SET APGE 00H
    // {0x44, {0x01, 0x66},0x02}, //Set_Tear_Scanline
    // {0x35, {0x00},        0x00}, //TE ON
    // {0x34, {0x00},        0x00}, //TE OFF
    // {0x36, {0x00},        0x01}, //Scan Direction Control
    {0x36, {0x60}, 0x01}, //
    {0x3A, {0x55}, 0x01}, // Interface Pixel Format 16bit/pixel
    // {0x3A, {0x66},        0x01}, //Interface Pixel Format    18bit/pixel
    // {0x3A, {0x77},        0x01}, //Interface Pixel Format    24bit/pixel
    {0x51, {0x00}, 0x01}, // Write Display Brightness MAX_VAL=0XFF
    {0x29, {0x00}, 0x80}, // Display on
    {0x51, {AMOLED_DEFAULT_BRIGHTNESS}, 0x01} // Write Display Brightness   MAX_VAL=0XFF
};

const lcd_cmd_t rm67162_spi_cmd[RM67162_INIT_SPI_SEQUENCE_LENGTH] = {
    {0xFE, {0x04}, 0x01}, //SET APGE3
    {0x6A, {0x00}, 0x01},
    {0xFE, {0x05}, 0x01}, //SET APGE4
    {0xFE, {0x07}, 0x01}, //SET APGE6
    {0x07, {0x4F}, 0x01},
    {0xFE, {0x01}, 0x01}, //SET APGE0
    {0x2A, {0x02}, 0x01},
    {0x2B, {0x73}, 0x01},
    {0xFE, {0x0A}, 0x01}, //SET APGE9
    {0x29, {0x10}, 0x01},
    {0xFE, {0x00}, 0x01},
    {0x51, {AMOLED_DEFAULT_BRIGHTNESS}, 0x01},
    {0x53, {0x20}, 0x01},
    {0x35, {0x00}, 0x01},

    {0x3A, {0x75}, 0x01}, // Interface Pixel Format 16bit/pixel
    {0xC4, {0x80}, 0x01},
    {0x11, {0x00}, 0x01 | 0x80},
    {0x29, {0x00}, 0x01 | 0x80},
};

const lcd_cmd_t rm690b0_cmd[RM690B0_INIT_SEQUENCE_LENGTH] = {
    {0xFE, {0x20}, 0x01},           //SET PAGE
    {0x26, {0x0A}, 0x01},           //MIPI OFF
    {0x24, {0x80}, 0x01},           //SPI write RAM
    {0x5A, {0x51}, 0x01},           //! 230918:SWIRE FOR BV6804
    {0x5B, {0x2E}, 0x01},           //! 230918:SWIRE FOR BV6804
    {0xFE, {0x00}, 0x01},           //SET PAGE
    {0x3A, {0x55}, 0x01},           //Interface Pixel Format    16bit/pixel
    {0xC2, {0x00}, 0x21},           //delay_ms(10);
    {0x35, {0x00}, 0x01},           //TE ON
    {0x51, {0x00}, 0x01},           //Write Display Brightness  MAX_VAL=0XFF
    {0x11, {0x00}, 0x80},           //Sleep Out delay_ms(120);
    {0x29, {0x00}, 0x20},           //Display on delay_ms(10);
    {0x51, {0xFF}, 0x01},           //Write Display Brightness  MAX_VAL=0XFF
};

/*
const lcd_cmd_t jd9613_cmd[JD9613_INIT_SEQUENCE_LENGTH] = {
    {0xfe, {0x01}, 0x02},
    {0xf7, {0x96, 0x13, 0xa9}, 0x04},
    {0x90, {0x01}, 0x02},
    {0x2c, {0x19, 0x0b, 0x24, 0x1b, 0x1b, 0x1b, 0xaa, 0x50, 0x01, 0x16, 0x04, 0x04, 0x04, 0xd7}, 0x0f},
    {0x2d, {0x66, 0x56, 0x55}, 0x04},
    {0x2e, {0x24, 0x04, 0x3f, 0x30, 0x30, 0xa8, 0xb8, 0xb8, 0x07}, 0x0a},
    {0x33, {0x03, 0x03, 0x03, 0x19, 0x19, 0x19, 0x13, 0x13, 0x13, 0x1a, 0x1a, 0x1a}, 0x0d},
    {0x10, {0x0b, 0x08, 0x64, 0xae, 0x0b, 0x08, 0x64, 0xae, 0x00, 0x80, 0x00, 0x00, 0x01}, 0x0e},
    {0x11, {0x01, 0x1e, 0x01, 0x1e, 0x00}, 0x06},
    {0x03, {0x93, 0x1c, 0x00, 0x01, 0x7e}, 0x06},
    {0x19, {0x00}, 0x02},
    {0x31, {0x1b, 0x00, 0x06, 0x05, 0x05, 0x05}, 0x07},
    {0x35, {0x00, 0x80, 0x80, 0x00}, 0x05},
    {0x12, {0x1b}, 0x02},
    {0x1a, {0x01, 0x20, 0x00, 0x08, 0x01, 0x06, 0x06, 0x06}, 0x09},
    {0x74, {0xbd, 0x00, 0x01, 0x08, 0x01, 0xbb, 0x98}, 0x08},
    {0x6c, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x01, 0x30, 0x08, 0x00}, 0x0a},
    {0x6d, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x02, 0x30, 0x08, 0x00}, 0x0a},
    {0x76, {0xda, 0x00, 0x02, 0x20, 0x39, 0x80, 0x80, 0x50, 0x05}, 0x0a},
    {0x6e, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x02, 0x4f, 0x02, 0x00}, 0x0a},
    {0x6f, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x01, 0x4f, 0x02, 0x00}, 0x0a},
    {0x80, {0xbd, 0x00, 0x01, 0x08, 0x01, 0xbb, 0x98}, 0x08},
    {0x78, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x01, 0x30, 0x08, 0x00}, 0x0a},
    {0x79, {0xdc, 0x08, 0x02, 0x01, 0x08, 0x02, 0x30, 0x08, 0x00}, 0x0a},
    {0x82, {0xda, 0x40, 0x02, 0x20, 0x39, 0x00, 0x80, 0x50, 0x05}, 0x0a},
    {0x7a, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x02, 0x4f, 0x02, 0x00}, 0x0a},
    {0x7b, {0xdc, 0x00, 0x02, 0x01, 0x00, 0x01, 0x4f, 0x02, 0x00}, 0x0a},
    {0x84, {0x01, 0x00, 0x09, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19}, 0x0b},
    {0x85, {0x19, 0x19, 0x19, 0x03, 0x02, 0x08, 0x19, 0x19, 0x19, 0x19}, 0x0b},
    {0x20, {0x20, 0x00, 0x08, 0x00, 0x02, 0x00, 0x40, 0x00, 0x10, 0x00, 0x04, 0x00}, 0x0d},
    {0x1e, {0x40, 0x00, 0x10, 0x00, 0x04, 0x00, 0x20, 0x00, 0x08, 0x00, 0x02, 0x00}, 0x0d},
    {0x24, {0x20, 0x00, 0x08, 0x00, 0x02, 0x00, 0x40, 0x00, 0x10, 0x00, 0x04, 0x00}, 0x0d},
    {0x22, {0x40, 0x00, 0x10, 0x00, 0x04, 0x00, 0x20, 0x00, 0x08, 0x00, 0x02, 0x00}, 0x0d},
    {0x13, {0x63, 0x52, 0x41}, 0x04},
    {0x14, {0x36, 0x25, 0x14}, 0x04},
    {0x15, {0x63, 0x52, 0x41}, 0x04},
    {0x16, {0x36, 0x25, 0x14}, 0x04},
    {0x1d, {0x10, 0x00, 0x00}, 0x04},
    {0x2a, {0x0d, 0x07}, 0x03},
    {0x27, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}, 0x07},
    {0x28, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}, 0x07},
    {0x26, {0x01, 0x01}, 0x03},
    {0x86, {0x01, 0x01}, 0x03},
    {0xfe, {0x02}, 0x02},
    {0x16, {0x81, 0x43, 0x23, 0x1e, 0x03}, 0x06},
    {0xfe, {0x03}, 0x02},
    {0x60, {0x01}, 0x02},
    {0x61, {0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x0d, 0x26, 0x5a, 0x80, 0x80, 0x95, 0xf8, 0x3b, 0x75}, 0x10},
    {0x62, {0x21, 0x22, 0x32, 0x43, 0x44, 0xd7, 0x0a, 0x59, 0xa1, 0xe1, 0x52, 0xb7, 0x11, 0x64, 0xb1}, 0x10},
    {0x63, {0x54, 0x55, 0x66, 0x06, 0xfb, 0x3f, 0x81, 0xc6, 0x06, 0x45, 0x83}, 0x0c},
    {0x64, {0x00, 0x00, 0x11, 0x11, 0x21, 0x00, 0x23, 0x6a, 0xf8, 0x63, 0x67, 0x70, 0xa5, 0xdc, 0x02}, 0x10},
    {0x65, {0x22, 0x22, 0x32, 0x43, 0x44, 0x24, 0x44, 0x82, 0xc1, 0xf8, 0x61, 0xbf, 0x13, 0x62, 0xad}, 0x10},
    {0x66, {0x54, 0x55, 0x65, 0x06, 0xf5, 0x37, 0x76, 0xb8, 0xf5, 0x31, 0x6c}, 0x0c},
    {0x67, {0x00, 0x10, 0x22, 0x22, 0x22, 0x00, 0x37, 0xa4, 0x7e, 0x22, 0x25, 0x2c, 0x4c, 0x72, 0x9a}, 0x10},
    {0x68, {0x22, 0x33, 0x43, 0x44, 0x55, 0xc1, 0xe5, 0x2d, 0x6f, 0xaf, 0x23, 0x8f, 0xf3, 0x50, 0xa6}, 0x10},
    {0x69, {0x65, 0x66, 0x77, 0x07, 0xfd, 0x4e, 0x9c, 0xed, 0x39, 0x86, 0xd3}, 0x0c},
    {0xfe, {0x05}, 0x02},
    {0x61, {0x00, 0x31, 0x44, 0x54, 0x55, 0x00, 0x92, 0xb5, 0x88, 0x19, 0x90, 0xe8, 0x3e, 0x71, 0xa5}, 0x10},
    {0x62, {0x55, 0x66, 0x76, 0x77, 0x88, 0xce, 0xf2, 0x32, 0x6e, 0xc4, 0x34, 0x8b, 0xd9, 0x2a, 0x7d}, 0x10},
    {0x63, {0x98, 0x99, 0xaa, 0x0a, 0xdc, 0x2e, 0x7d, 0xc3, 0x0d, 0x5b, 0x9e}, 0x0c},
    {0x64, {0x00, 0x31, 0x44, 0x54, 0x55, 0x00, 0xa2, 0xe5, 0xcd, 0x5c, 0x94, 0xcf, 0x09, 0x4a, 0x72}, 0x10},
    {0x65, {0x55, 0x65, 0x66, 0x77, 0x87, 0x9c, 0xc2, 0xff, 0x36, 0x6a, 0xec, 0x45, 0x91, 0xd8, 0x20}, 0x10},
    {0x66, {0x88, 0x98, 0x99, 0x0a, 0x68, 0xb0, 0xfb, 0x43, 0x8c, 0xd5, 0x0e}, 0x0c},
    {0x67, {0x00, 0x42, 0x55, 0x55, 0x55, 0x00, 0xcb, 0x62, 0xc5, 0x09, 0x44, 0x72, 0xa9, 0xd6, 0xfd}, 0x10},
    {0x68, {0x66, 0x66, 0x77, 0x87, 0x98, 0x21, 0x45, 0x96, 0xed, 0x29, 0x90, 0xee, 0x4b, 0xb1, 0x13}, 0x10},
    {0x69, {0x99, 0xaa, 0xba, 0x0b, 0x6a, 0xb8, 0x0d, 0x62, 0xb8, 0x0e, 0x54}, 0x0c},
    {0xfe, {0x07}, 0x02},
    {0x3e, {0x00}, 0x02},
    {0x42, {0x03, 0x10}, 0x03},
    {0x4a, {0x31}, 0x02},
    {0x5c, {0x01}, 0x02},
    {0x3c, {0x07, 0x00, 0x24, 0x04, 0x3f, 0xe2}, 0x07},
    {0x44, {0x03, 0x40, 0x3f, 0x02}, 0x05},
    {0x12, {0xaa, 0xaa, 0xc0, 0xc8, 0xd0, 0xd8, 0xe0, 0xe8, 0xf0, 0xf8}, 0x0b},
    {0x11, {0xaa, 0xaa, 0xaa, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8}, 0x10},
    {0x10, {0xaa, 0xaa, 0xaa, 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58}, 0x10},
    {0x14, {0x03, 0x1f, 0x3f, 0x5f, 0x7f, 0x9f, 0xbf, 0xdf, 0x03, 0x1f, 0x3f, 0x5f, 0x7f, 0x9f, 0xbf, 0xdf}, 0x11},
    {0x18, {0x70, 0x1a, 0x22, 0xbb, 0xaa, 0xff, 0x24, 0x71, 0x0f, 0x01, 0x00, 0x03}, 0x0d},
    {0xfe, {0x00}, 0x02},
    {0x3a, {0x55}, 0x02},
    {0xc4, {0x80}, 0x02},
    {0x2a, {0x00, 0x00, 0x00, 0x7d}, 0x05},
    {0x2b, {0x00, 0x00, 0x01, 0x25}, 0x05},
    {0x35, {0x00}, 0x02},
    {0x53, {0x28}, 0x02},
    {0x51, {0xff}, 0x02},
    {0, {0}, 0xff},
};
*/



