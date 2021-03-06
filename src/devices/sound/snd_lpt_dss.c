/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Implementation of the LPT-based DSS sound device.
 *
 * Version:	@(#)snd_lpt_dss.c	1.0.7	2018/05/06
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Sarah Walker, <tommowalker@tommowalker.co.uk>
 *
 *		Copyright 2017,2018 Fred N. van Kempen.
 *		Copyright 2016-2018 Miran Grca.
 *		Copyright 2008-2018 Sarah Walker.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free  Software  Foundation; either  version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is  distributed in the hope that it will be useful, but
 * WITHOUT   ANY  WARRANTY;  without  even   the  implied  warranty  of
 * MERCHANTABILITY  or FITNESS  FOR A PARTICULAR  PURPOSE. See  the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *   Free Software Foundation, Inc.
 *   59 Temple Place - Suite 330
 *   Boston, MA 02111-1307
 *   USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "../../emu.h"
#include "../../cpu/cpu.h"
#include "../../machines/machine.h"
#include "../../timer.h"
#include "../ports/parallel_dev.h"
#include "sound.h"
#include "filters.h"
#include "snd_lpt_dss.h"


typedef struct {
    uint8_t	fifo[16];
    int8_t	read_idx,
		write_idx;

    uint8_t	dac_val;

    int64_t	time;

    int16_t	buffer[SOUNDBUFLEN];
    int		pos;

    const char	*name;
} dss_t;


static void
dss_update(dss_t *dev)
{
    for (; dev->pos < sound_pos_global; dev->pos++)
	dev->buffer[dev->pos] = (int8_t)(dev->dac_val ^ 0x80) * 0x40;
}


static void
dss_write_data(uint8_t val, void *priv)
{
    dss_t *dev = (dss_t *)priv;

    timer_clock();

    if ((dev->write_idx - dev->read_idx) < 16) {
	dev->fifo[dev->write_idx & 15] = val;
	dev->write_idx++;
    }
}


static void
dss_write_ctrl(uint8_t val, void *priv)
{
}


static uint8_t
dss_read_status(void *priv)
{
    dss_t *dev = (dss_t *)priv;

    if ((dev->write_idx - dev->read_idx) >= 16)
		return 0x40;
    return 0;
}


static void
dss_get_buffer(int32_t *buffer, int len, void *priv)
{
    dss_t *dev = (dss_t *)priv;
    int16_t val;
    int c;

    dss_update(dev);

    for (c = 0; c < len*2; c += 2) {
	val = (int16_t)dss_iir((float)dev->buffer[c >> 1]);

	buffer[c] += val;
	buffer[c+1] += val;
    }

    dev->pos = 0;
}


static void
dss_callback(void *priv)
{
    dss_t *dev = (dss_t *)priv;

    dss_update(dev);

    if ((dev->write_idx - dev->read_idx) > 0) {
	dev->dac_val = dev->fifo[dev->read_idx & 15];
	dev->read_idx++;
    }

    dev->time += (int64_t) (TIMER_USEC * (1000000.0 / 7000.0));
}


static void *
dss_init(const lpt_device_t *info)
{
    dss_t *dev = malloc(sizeof(dss_t));

pclog("SOUND: LPT device '%s' initializing!\n", info->name);
    memset(dev, 0x00, sizeof(dss_t));
    dev->name = info->name;

    sound_add_handler(dss_get_buffer, dev);

    timer_add(dss_callback, &dev->time, TIMER_ALWAYS_ENABLED, dev);
	
    return dev;
}


static void
dss_close(void *priv)
{
    dss_t *dev = (dss_t *)priv;

    pclog("SOUND: LPT device '%s' closed!\n", dev->name);

    free(dev);
}


const lpt_device_t dss_device = {
    "Disney Sound Source",
    0,
    dss_init,
    dss_close,
    dss_write_data,
    dss_write_ctrl,
    dss_read_status
};
