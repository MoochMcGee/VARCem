/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		IBM VGA emulation.
 *
 * Version:	@(#)vid_vga.c	1.0.6	2018/05/06
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
#include "../../io.h"
#include "../../mem.h"
#include "../../rom.h"
#include "../../device.h"
#include "video.h"
#include "vid_svga.h"


typedef struct vga_t
{
        svga_t svga;
        
        rom_t bios_rom;
} vga_t;

void vga_out(uint16_t addr, uint8_t val, void *p)
{
        vga_t *vga = (vga_t *)p;
        svga_t *svga = &vga->svga;
        uint8_t old;

        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;

        switch (addr)
        {
                case 0x3D4:
                svga->crtcreg = val & 0x1f;
                return;
                case 0x3D5:
                if ((svga->crtcreg < 7) && (svga->crtc[0x11] & 0x80))
                        return;
                if ((svga->crtcreg == 7) && (svga->crtc[0x11] & 0x80))
                        val = (svga->crtc[7] & ~0x10) | (val & 0x10);
                old = svga->crtc[svga->crtcreg];
                svga->crtc[svga->crtcreg] = val;
                if (old != val)
                {
                        if (svga->crtcreg < 0xe || svga->crtcreg > 0x10)
                        {
                                svga->fullchange = changeframecount;
                                svga_recalctimings(svga);
                        }
                }
                break;
        }
        svga_out(addr, val, svga);
}

uint8_t vga_in(uint16_t addr, void *p)
{
        vga_t *vga = (vga_t *)p;
        svga_t *svga = &vga->svga;
        uint8_t temp;

        if (((addr & 0xfff0) == 0x3d0 || (addr & 0xfff0) == 0x3b0) && !(svga->miscout & 1)) 
                addr ^= 0x60;
             
        switch (addr)
        {
                case 0x3D4:
                temp = svga->crtcreg;
                break;
                case 0x3D5:
                temp = svga->crtc[svga->crtcreg];
                break;
                default:
                temp = svga_in(addr, svga);
                break;
        }
        return temp;
}


static void *vga_init(const device_t *info)
{
        vga_t *vga = malloc(sizeof(vga_t));
        memset(vga, 0, sizeof(vga_t));

        rom_init(&vga->bios_rom, L"video/ibm/vga/ibm_vga.bin", 0xc0000, 0x8000, 0x7fff, 0x2000, MEM_MAPPING_EXTERNAL);

        svga_init(&vga->svga, vga, 1 << 18, /*256kb*/
                   NULL,
                   vga_in, vga_out,
                   NULL,
                   NULL);

        io_sethandler(0x03c0, 0x0020, vga_in, NULL, NULL, vga_out, NULL, NULL, vga);

        vga->svga.bpp = 8;
        vga->svga.miscout = 1;
        
        return vga;
}


#ifdef DEV_BRANCH
static void *trigem_unk_init(const device_t *info)
{
        vga_t *vga = malloc(sizeof(vga_t));
        memset(vga, 0, sizeof(vga_t));

        rom_init(&vga->bios_rom, L"video/ibm/vga/ibm_vga.bin", 0xc0000, 0x8000, 0x7fff, 0x2000, MEM_MAPPING_EXTERNAL);

        svga_init(&vga->svga, vga, 1 << 18, /*256kb*/
                   NULL,
                   vga_in, vga_out,
                   NULL,
                   NULL);

        io_sethandler(0x03c0, 0x0020, vga_in, NULL, NULL, vga_out, NULL, NULL, vga);

	io_sethandler(0x22ca, 0x0002, svga_in, NULL, NULL, vga_out, NULL, NULL, vga);
	io_sethandler(0x22ce, 0x0002, svga_in, NULL, NULL, vga_out, NULL, NULL, vga);
	io_sethandler(0x32ca, 0x0002, svga_in, NULL, NULL, vga_out, NULL, NULL, vga);

        vga->svga.bpp = 8;
        vga->svga.miscout = 1;
        
        return vga;
}
#endif

/*PS/1 uses a standard VGA controller, but with no option ROM*/
void *ps1vga_init(const device_t *info)
{
        vga_t *vga = malloc(sizeof(vga_t));
        memset(vga, 0, sizeof(vga_t));
       
        svga_init(&vga->svga, vga, 1 << 18, /*256kb*/
                   NULL,
                   vga_in, vga_out,
                   NULL,
                   NULL);

        io_sethandler(0x03c0, 0x0020, vga_in, NULL, NULL, vga_out, NULL, NULL, vga);

        vga->svga.bpp = 8;
        vga->svga.miscout = 1;
        
        return vga;
}

static int vga_available(void)
{
        return rom_present(L"video/ibm/vga/ibm_vga.bin");
}

void vga_close(void *p)
{
        vga_t *vga = (vga_t *)p;

        svga_close(&vga->svga);
        
        free(vga);
}

void vga_speed_changed(void *p)
{
        vga_t *vga = (vga_t *)p;
        
        svga_recalctimings(&vga->svga);
}

void vga_force_redraw(void *p)
{
        vga_t *vga = (vga_t *)p;

        vga->svga.fullchange = changeframecount;
}

void vga_add_status_info(char *s, int max_len, void *p)
{
        vga_t *vga = (vga_t *)p;
        
        svga_add_status_info(s, max_len, &vga->svga);
}

const device_t vga_device =
{
        "VGA",
        DEVICE_ISA,
	0,
        vga_init,
        vga_close,
	NULL,
        vga_available,
        vga_speed_changed,
        vga_force_redraw,
        vga_add_status_info
};
#ifdef DEV_BRANCH
const device_t trigem_unk_device =
{
        "VGA",
        DEVICE_ISA,
	0,
        trigem_unk_init,
        vga_close,
	NULL,
        vga_available,
        vga_speed_changed,
        vga_force_redraw,
        vga_add_status_info
};
#endif
const device_t ps1vga_device =
{
        "PS/1 VGA",
        0,
	0,
        ps1vga_init,
        vga_close,
	NULL,
        vga_available,
        vga_speed_changed,
        vga_force_redraw,
        vga_add_status_info
};
