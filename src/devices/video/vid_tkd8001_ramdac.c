/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Trident TKD8001 RAMDAC emulation.
 *
 * Version:	@(#)vid_tkd8001_ramdac.c	1.0.2	2018/05/06
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
#include <wchar.h>
#include "../../emu.h"
#include "../../mem.h"
#include "video.h"
#include "vid_svga.h"
#include "vid_tkd8001_ramdac.h"


void tkd8001_ramdac_out(uint16_t addr, uint8_t val, tkd8001_ramdac_t *ramdac, svga_t *svga)
{
        switch (addr)
        {
                case 0x3C6:
                if (ramdac->state == 4)
                {
                        ramdac->state = 0;
                        ramdac->ctrl = val;
                        switch (val >> 5)
                        {
                                case 0: case 1: case 2: case 3:
                                svga->bpp = 8;
                                break;
                                case 5:
                                svga->bpp = 15;
                                break;
                                case 6:
                                svga->bpp = 24;
                                break;
                                case 7:
                                svga->bpp = 16;
                                break;
                        }
                        return;
                }
                break;
                case 0x3C7: case 0x3C8: case 0x3C9:
                ramdac->state = 0;
                break;
        }
        svga_out(addr, val, svga);
}

uint8_t tkd8001_ramdac_in(uint16_t addr, tkd8001_ramdac_t *ramdac, svga_t *svga)
{
        switch (addr)
        {
                case 0x3C6:
                if (ramdac->state == 4)
                {
                        return ramdac->ctrl;
                }
                ramdac->state++;
                break;
                case 0x3C7: case 0x3C8: case 0x3C9:
                ramdac->state = 0;
                break;
        }
        return svga_in(addr, svga);
}
