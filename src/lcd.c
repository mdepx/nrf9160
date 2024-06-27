/*-
 * Copyright (c) 2018-2024 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/of.h>
#include <sys/thread.h>
#include <sys/malloc.h>

#include <arm/nordicsemi/nrf9160.h>

#include <dev/gpio/gpio.h>

#include "lcd.h"

static mdx_device_t gpio_dev;

#define	JDI_CMD_UPDATE		0x90
#define	JDI_CMD_ALL_CLEAR	0x20
#define	JDI_CMD_NO_UPDATE	0x00
#define	JDI_CMD_INVERSION	0x14
#define	JDI_CMD_VCOM		0x40

#define	LCD_BLINKMODE_NONE	0x00
#define	LCD_BLINKMODE_WHITE	0x01
#define	LCD_BLINKMODE_BLACK	0x02
#define	LCD_BLINKMODE_INVERSE	0x03

#define	LCD_COLOR_CMD_UPDATE		0x90 /* 4 bit. */
#define	LCD_COLOR_CMD_ALL_CLEAR		0x20
#define	LCD_COLOR_CMD_NO_UPDATE		0x00
#define	LCD_COLOR_CMD_BLINKING_WHITE	0x18
#define	LCD_COLOR_CMD_BLINKING_BLACK	0x10
#define	LCD_COLOR_CMD_INVERSION		0x14

void setBlinkMode(uint8_t mode);

static uint8_t vcom = 0;

static void
mdx_usleep2(int count)
{
	int i, j;

	for (i = 0; i < count; i++)
		for (j = 0; j < 1; j++);
}

static void
pin_set(int pin)
{

	mdx_gpio_set(gpio_dev, 0, pin, 1);
}

static void
pin_clr(int pin)
{

	mdx_gpio_set(gpio_dev, 0, pin, 0);
}

static void
toggle_vcom(void)
{

	if (vcom) {
		pin_clr(LCD_EXTCOMIN);
		vcom = 0;
	} else {
		pin_set(LCD_EXTCOMIN);
		vcom = 1;
	}
}

static void
lcd_reset(void)
{

	pin_clr(LCD_CS);
	pin_clr(LCD_DISP);
	mdx_usleep2(10000);
	pin_set(LCD_DISP);
	mdx_usleep2(120000);
}

static void
Write_Data(uint8_t dh)
{
	uint8_t i;

	for (i = 0; i < 8; i++) {
		pin_clr(LCD_SCK);
		if (dh & 0x80)
			pin_set(LCD_MOSI);
		else
			pin_clr(LCD_MOSI);
		pin_set(LCD_SCK);
		dh = dh << 1;
	}
	pin_clr(LCD_SCK);
}

static void
ClearScreen(int bColor)
{
	uint8_t reg;

	pin_set(LCD_CS);
	reg = JDI_CMD_ALL_CLEAR;
	Write_Data(reg);
	Write_Data(0x00);
	pin_clr(LCD_CS);
}

static void
invertDisplay(void)
{
	uint8_t reg;

	pin_set(LCD_CS);
	reg = JDI_CMD_INVERSION;
	Write_Data(reg);
	Write_Data(0x00);
	pin_clr(LCD_CS);
}

void
setBlinkMode(uint8_t mode)
{
	uint8_t blink_cmd;

	switch (mode) {
	case LCD_BLINKMODE_NONE:
		blink_cmd = LCD_COLOR_CMD_NO_UPDATE;
		break;
	case LCD_BLINKMODE_WHITE:
		blink_cmd = LCD_COLOR_CMD_BLINKING_WHITE;
		break;
	case LCD_BLINKMODE_BLACK:
		blink_cmd = LCD_COLOR_CMD_BLINKING_BLACK;
		break;
	case LCD_BLINKMODE_INVERSE:
		blink_cmd = LCD_COLOR_CMD_INVERSION;
		break;
	default:
		blink_cmd = LCD_COLOR_CMD_NO_UPDATE;
		break;
	}

	pin_set(LCD_CS);
	Write_Data(blink_cmd);
	Write_Data(0x00);
	pin_clr(LCD_CS);
}

static void
set_line(uint8_t line)
{
	uint8_t reg;
	int i;

	reg = LCD_COLOR_CMD_UPDATE;
	pin_set(LCD_CS);
	Write_Data(reg);
	Write_Data(line + 1);
	for (i = 0; i < 44; i++)
		Write_Data(0xcc);
	for (i = 0; i < 44; i++)
		Write_Data(0x33);
	Write_Data(0x0); /* Dummy. */
	Write_Data(0x0); /* Dummy. */
	pin_clr(LCD_CS);
}

void
lcd_init(void)
{
	mdx_device_t dev;
	int i;

	dev = mdx_device_lookup_by_name("nrf_gpio", 0);
	if (!dev)
		panic("gpio dev not found");

	gpio_dev = dev;

	mdx_gpio_configure(dev, 0, LCD_EXTMODE, MDX_GPIO_OUTPUT);
	mdx_gpio_configure(dev, 0, LCD_EXTCOMIN, MDX_GPIO_OUTPUT);
	pin_set(LCD_EXTMODE);
	pin_set(LCD_EXTCOMIN);
	mdx_gpio_configure(dev, 0, LCD_CS, MDX_GPIO_OUTPUT);
	mdx_gpio_configure(dev, 0, LCD_SCK, MDX_GPIO_OUTPUT);
	mdx_gpio_configure(dev, 0, LCD_MOSI, MDX_GPIO_OUTPUT);
	mdx_gpio_configure(dev, 0, LCD_DISP, MDX_GPIO_OUTPUT);

	lcd_reset();

	printf("display initialized\n");

	ClearScreen(0);
	toggle_vcom();
	if (1 == 0) {
		invertDisplay();
		toggle_vcom();
	}

	for (i = 0; i < 176; i++) {
		set_line(i);
		toggle_vcom();
	}

	while (1) {
		setBlinkMode(LCD_BLINKMODE_NONE);
		toggle_vcom();
		mdx_usleep2(2000000);
		printf("blink\n");
		break;
	}

	printf("screen cleared\n");
}
