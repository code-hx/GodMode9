// Based on xerpi's CODEC driver for Linux
// Original comment follows:
/*
 * nintendo3ds_codec.c
 *
 * Copyright (C) 2016 Sergi Granell (xerpi)
 * Copyright (C) 2017 Paul LaMendola (paulguy)
 * based on ad7879-spi.c
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>
#include <types.h>

#include <hid_map.h>

#include "hw/codec.h"
#include "hw/spi.h"

#define CODEC_SPI_DEV	3

#define CPAD_THRESH	(150)

/* SPI stuff */
static void CODEC_WriteRead(u32 *txb, u8 txl, u32 *rxb, u8 rxl)
{
	SPI_XferInfo xfers[2];

	xfers[0].buf = txb;
	xfers[0].len = txl;
	xfers[0].read = false;

	xfers[1].buf = rxb;
	xfers[1].len = rxl;
	xfers[1].read = true;

	SPI_DoXfer(CODEC_SPI_DEV, xfers, 2);
}

static void CODEC_RegSelect(u8 reg)
{
	SPI_XferInfo xfer;
	u32 cmd;

	cmd = reg << 8;

	xfer.buf = &cmd;
	xfer.len = 2;
	xfer.read = false;

	SPI_DoXfer(CODEC_SPI_DEV, &xfer, 1);
}

static u8 CODEC_RegRead(u8 reg)
{
	u32 cmd, ret;
	cmd = (reg << 1) | 1;
	CODEC_WriteRead(&cmd, 1, &ret, 1);
	return ret;
}

static void CODEC_RegWrite(u8 reg, u8 val)
{
	SPI_XferInfo xfer;
	u32 cmd;

	cmd = (val << 8) | (reg << 1);

	xfer.buf = &cmd;
	xfer.len = 2;
	xfer.read = false;

	SPI_DoXfer(CODEC_SPI_DEV, &xfer, 1);
}

static void CODEC_RegReadBuf(u8 reg, u32 *out, u8 size)
{
	u32 cmd = (reg << 1) | 1;
	CODEC_WriteRead(&cmd, 1, out, size);
}

static void CODEC_RegMask(u8 reg, u8 mask0, u8 mask1)
{
	CODEC_RegWrite(reg, (CODEC_RegRead(reg) & ~mask1) | (mask0 & mask1));
}

void CODEC_Init(void)
{
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x24, 0x98);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x26, 0x00);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x25, 0x43);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x24, 0x18);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x17, 0x43);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x19, 0x69);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x1B, 0x80);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x27, 0x11);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x26, 0xEC);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x24, 0x18);
	CODEC_RegSelect(0x67);
	CODEC_RegWrite(0x25, 0x53);

	CODEC_RegSelect(0x67);
	CODEC_RegMask(0x26, 0x80, 0x80);
	CODEC_RegSelect(0x67);
	CODEC_RegMask(0x24, 0x00, 0x80);
	CODEC_RegSelect(0x67);
	CODEC_RegMask(0x25, 0x10, 0x3C);
}

void CODEC_GetRawData(u32 *buffer)
{
	CODEC_RegSelect(0x67);
	CODEC_RegRead(0x26);
	CODEC_RegSelect(0xFB);
	CODEC_RegReadBuf(1, buffer, 0x34);
}

void CODEC_Get(CODEC_Input *input)
{
	u32 raw_data_buf[0x34 / 4];
	u8 *raw_data = (u8*)raw_data_buf;
	s16 cpad_x, cpad_y;
	bool ts_pressed;

	CODEC_GetRawData(raw_data_buf);

	cpad_x = ((raw_data[0x24] << 8 | raw_data[0x25]) & 0xFFF) - 2048;
	cpad_y = ((raw_data[0x14] << 8 | raw_data[0x15]) & 0xFFF) - 2048;

	// X axis is inverted
	input->cpad_x = (abs(cpad_x) > CPAD_THRESH) ? -cpad_x : 0;
	input->cpad_y = (abs(cpad_y) > CPAD_THRESH) ? cpad_y : 0;

	ts_pressed = !(raw_data[0] & BIT(4));
	if (ts_pressed) {
		input->ts_x = (raw_data[0] << 8) | raw_data[1];
		input->ts_y = (raw_data[10] << 8) | raw_data[11];
	} else {
		input->ts_x = 0xFFFF;
		input->ts_y = 0xFFFF;
	}
}
