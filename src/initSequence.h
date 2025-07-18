/**
 * @file      initSequence.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-05-29
 *
 */
#pragma once

#include <stdint.h>

typedef struct {
    uint32_t addr;
    uint8_t param[20];
    uint32_t len;
} lcd_cmd_t;

#define AMOLED_DEFAULT_BRIGHTNESS               175

#define RM67162_INIT_SEQUENCE_LENGTH            12
extern const lcd_cmd_t rm67162_cmd[RM67162_INIT_SEQUENCE_LENGTH];
#define RM67162_WIDTH                           240
#define RM67162_HEIGHT                          536
#define RM67162_MADCTL_MY                       0x80
#define RM67162_MADCTL_MX                       0x40
#define RM67162_MADCTL_MV                       0x20
#define RM67162_MADCTL_ML                       0x10
#define RM67162_MADCTL_RGB                      0x00
#define RM67162_MADCTL_MH                       0x04
#define RM67162_MADCTL_BGR                      0x08

#define RM690B0_INIT_SEQUENCE_LENGTH             13
extern const lcd_cmd_t rm690b0_cmd[RM690B0_INIT_SEQUENCE_LENGTH];
#define RM690B0_WIDTH                            600
#define RM690B0_HEIGHT                           450
#define RM690B0_MADCTL_MY                       0x80
#define RM690B0_MADCTL_MX                       0x40
#define RM690B0_MADCTL_MV                       0x20
#define RM690B0_MADCTL_ML                       0x10
#define RM690B0_MADCTL_RGB                      0x00
#define RM690B0_MADCTL_MH                       0x04
#define RM690B0_MADCTL_BGR                      0x08

/*
#define JD9613_INIT_SEQUENCE_LENGTH             88
extern const lcd_cmd_t jd9613_cmd[JD9613_INIT_SEQUENCE_LENGTH];
#define JD9613_WIDTH                            294
#define JD9613_HEIGHT                           126
*/

#define RM67162_INIT_SPI_SEQUENCE_LENGTH        18
extern const lcd_cmd_t rm67162_spi_cmd[RM67162_INIT_SPI_SEQUENCE_LENGTH];









