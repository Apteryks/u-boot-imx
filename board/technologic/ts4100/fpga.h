/*
 * Copyright (C) 2016 Technologic Systems
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FPGA_H__
#define __FPGA_H__

/* FPGA IO */
#define EN_USB_HOST_5V_PAD	22
#define EN_SD_POWER_PAD		24
#define OFF_BD_RESET_PADN	25
#define EN_SW_3V3_PAD		26
#define GREEN_LED_PADN		27
#define RED_LED_PADN		28
#define DIO_20			57
#define DIO_05			42

void fpga_gpio_output(int io, int value);
int fpga_gpio_input(int io);
int fpga_get_rev(void);

#endif // __FPGA_H__