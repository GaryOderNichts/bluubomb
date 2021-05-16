/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "bdaddr.h"

static int transient = 0;

static int generic_reset_device(int dd)
{
	bdaddr_t bdaddr;
	int err;

	err = hci_send_cmd(dd, 0x03, 0x0003, 0, NULL);
	if (err < 0)
		return err;

	return hci_read_bd_addr(dd, &bdaddr, 10000);
}

#define OCF_ERICSSON_WRITE_BD_ADDR	0x000d
typedef struct {
	bdaddr_t	bdaddr;
} __attribute__ ((packed)) ericsson_write_bd_addr_cp;

static int ericsson_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
	struct hci_request rq;
	ericsson_write_bd_addr_cp cp;

	memset(&cp, 0, sizeof(cp));
	bacpy(&cp.bdaddr, bdaddr);

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = OCF_ERICSSON_WRITE_BD_ADDR;
	rq.cparam = &cp;
	rq.clen   = sizeof(cp);
	rq.rparam = NULL;
	rq.rlen   = 0;

	if (hci_send_req(dd, &rq, 1000) < 0)
		return -1;

	return 0;
}

#define OCF_ERICSSON_STORE_IN_FLASH	0x0022
typedef struct {
	uint8_t		user_id;
	uint8_t		flash_length;
	uint8_t		flash_data[253];
} __attribute__ ((packed)) ericsson_store_in_flash_cp;

static int ericsson_store_in_flash(int dd, uint8_t user_id, uint8_t flash_length, uint8_t *flash_data)
{
	struct hci_request rq;
	ericsson_store_in_flash_cp cp;

	memset(&cp, 0, sizeof(cp));
	cp.user_id = user_id;
	cp.flash_length = flash_length;
	if (flash_length > 0)
		memcpy(cp.flash_data, flash_data, flash_length);

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = OCF_ERICSSON_STORE_IN_FLASH;
	rq.cparam = &cp;
	rq.clen   = sizeof(cp);
	rq.rparam = NULL;
	rq.rlen   = 0;

	if (hci_send_req(dd, &rq, 1000) < 0)
		return -1;

	return 0;
}

static int csr_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
	unsigned char cmd[] = { 0x02, 0x00, 0x0c, 0x00, 0x11, 0x47, 0x03, 0x70,
				0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	unsigned char cp[254], rp[254];
	struct hci_request rq;

	if (transient)
		cmd[14] = 0x08;

	cmd[16] = bdaddr->b[2];
	cmd[17] = 0x00;
	cmd[18] = bdaddr->b[0];
	cmd[19] = bdaddr->b[1];
	cmd[20] = bdaddr->b[3];
	cmd[21] = 0x00;
	cmd[22] = bdaddr->b[4];
	cmd[23] = bdaddr->b[5];

	memset(&cp, 0, sizeof(cp));
	cp[0] = 0xc2;
	memcpy(cp + 1, cmd, sizeof(cmd));

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = 0x00;
	rq.event  = EVT_VENDOR;
	rq.cparam = cp;
	rq.clen   = sizeof(cmd) + 1;
	rq.rparam = rp;
	rq.rlen   = sizeof(rp);

	if (hci_send_req(dd, &rq, 2000) < 0)
		return -1;

	if (rp[0] != 0xc2) {
		errno = EIO;
		return -1;
	}

	if ((rp[9] + (rp[10] << 8)) != 0) {
		errno = ENXIO;
		return -1;
	}

	return 0;
}

static int csr_reset_device(int dd)
{
	unsigned char cmd[] = { 0x02, 0x00, 0x09, 0x00,
				0x00, 0x00, 0x01, 0x40, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	unsigned char cp[254], rp[254];
	struct hci_request rq;

	if (transient)
		cmd[6] = 0x02;

	memset(&cp, 0, sizeof(cp));
	cp[0] = 0xc2;
	memcpy(cp + 1, cmd, sizeof(cmd));

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = 0x00;
	rq.event  = EVT_VENDOR;
	rq.cparam = cp;
	rq.clen   = sizeof(cmd) + 1;
	rq.rparam = rp;
	rq.rlen   = sizeof(rp);

	if (hci_send_req(dd, &rq, 2000) < 0)
		return -1;

	return 0;
}

#define OCF_TI_WRITE_BD_ADDR		0x0006
typedef struct {
	bdaddr_t	bdaddr;
} __attribute__ ((packed)) ti_write_bd_addr_cp;

static int ti_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
	struct hci_request rq;
	ti_write_bd_addr_cp cp;

	memset(&cp, 0, sizeof(cp));
	bacpy(&cp.bdaddr, bdaddr);

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = OCF_TI_WRITE_BD_ADDR;
	rq.cparam = &cp;
	rq.clen   = sizeof(cp);
	rq.rparam = NULL;
	rq.rlen   = 0;

	if (hci_send_req(dd, &rq, 1000) < 0)
		return -1;

	return 0;
}

#define OCF_BCM_WRITE_BD_ADDR		0x0001
typedef struct {
	bdaddr_t	bdaddr;
} __attribute__ ((packed)) bcm_write_bd_addr_cp;

static int bcm_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
	struct hci_request rq;
	bcm_write_bd_addr_cp cp;

	memset(&cp, 0, sizeof(cp));
	bacpy(&cp.bdaddr, bdaddr);

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = OCF_BCM_WRITE_BD_ADDR;
	rq.cparam = &cp;
	rq.clen   = sizeof(cp);
	rq.rparam = NULL;
	rq.rlen   = 0;

	if (hci_send_req(dd, &rq, 1000) < 0)
		return -1;

	return 0;
}

#define OCF_INTEL_WRITE_BD_ADDR           0x0031
typedef struct {
        bdaddr_t        bdaddr;
} __attribute__ ((packed)) intel_write_bd_addr_cp;
#define INTEL_WRITE_BD_ADDR_CP_SIZE 6

static int intel_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
        struct hci_request rq;
        intel_write_bd_addr_cp cp;

        memset(&cp, 0, sizeof(cp));
        bacpy(&cp.bdaddr, bdaddr);

        memset(&rq, 0, sizeof(rq));
        rq.ogf    = OGF_VENDOR_CMD;
        rq.ocf    = OCF_INTEL_WRITE_BD_ADDR;
        rq.cparam = &cp;
        rq.clen   = INTEL_WRITE_BD_ADDR_CP_SIZE;
        rq.rparam = NULL;
        rq.rlen   = 0;

        if (hci_send_req(dd, &rq, 1000) < 0)
                return -1;

        return 0;
}

static int intel_reset_device(int dd)
{
	return -1;	// request Manual reset
}

#define OCF_ZEEVO_WRITE_BD_ADDR		0x0001
typedef struct {
	bdaddr_t	bdaddr;
} __attribute__ ((packed)) zeevo_write_bd_addr_cp;

static int zeevo_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
	struct hci_request rq;
	zeevo_write_bd_addr_cp cp;

	memset(&cp, 0, sizeof(cp));
	bacpy(&cp.bdaddr, bdaddr);

	memset(&rq, 0, sizeof(rq));
	rq.ogf    = OGF_VENDOR_CMD;
	rq.ocf    = OCF_ZEEVO_WRITE_BD_ADDR;
	rq.cparam = &cp;
	rq.clen   = sizeof(cp);
	rq.rparam = NULL;
	rq.rlen   = 0;

	if (hci_send_req(dd, &rq, 1000) < 0)
		return -1;

	return 0;
}

#define OCF_MRVL_WRITE_BD_ADDR		0x0022
typedef struct {
	uint8_t		parameter_id;
	uint8_t		bdaddr_len;
	bdaddr_t	bdaddr;
} __attribute__ ((packed)) mrvl_write_bd_addr_cp;

static int mrvl_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
	mrvl_write_bd_addr_cp cp;

	memset(&cp, 0, sizeof(cp));
	cp.parameter_id = 0xFE;
	cp.bdaddr_len = 6;
	bacpy(&cp.bdaddr, bdaddr);

	if (hci_send_cmd(dd, OGF_VENDOR_CMD, OCF_MRVL_WRITE_BD_ADDR,
							sizeof(cp), &cp) < 0)
		return -1;

	sleep(1);
	return 0;
}

static int st_write_bd_addr(int dd, bdaddr_t *bdaddr)
{
	return ericsson_store_in_flash(dd, 0xfe, 6, (uint8_t *) bdaddr);
}

static struct {
	uint16_t compid;
	int (*write_bd_addr)(int dd, bdaddr_t *bdaddr);
	int (*reset_device)(int dd);
} vendor[] = {
	{ 0,		ericsson_write_bd_addr,	NULL			},
	{ 2,		intel_write_bd_addr,	intel_reset_device	},
	{ 10,		csr_write_bd_addr,	csr_reset_device	},
	{ 13,		ti_write_bd_addr,	NULL			},
	{ 15,		bcm_write_bd_addr,	generic_reset_device	},
	{ 18,		zeevo_write_bd_addr,	NULL			},
	{ 48,		st_write_bd_addr,	generic_reset_device	},
	{ 57,		ericsson_write_bd_addr,	generic_reset_device	},
	{ 72,		mrvl_write_bd_addr,	generic_reset_device	},
	{ 305,	bcm_write_bd_addr,	generic_reset_device	},
	{ 65535,	NULL,			NULL			},
};

int set_device_bdaddr(int dd, const struct hci_version * ver, const bdaddr_t * bdaddr)
{
	if (ver == NULL)
	{
		return -1;
	}

	if (!bacmp(bdaddr, BDADDR_ANY))
	{
		return -1;
	}

	for (int i = 0; vendor[i].compid != 65535; i++)
	{
		if (ver->manufacturer == vendor[i].compid)
		{
			if (!vendor[i].reset_device)
			{
				fprintf(stderr, "Can't automatically reset device\n");
				return -1;
			}

			if (vendor[i].write_bd_addr(dd, (bdaddr_t *)bdaddr) < 0) {
				fprintf(stderr, "Can't write new address\n");
				return -1;
			}

			return 0;
		}
	}

	fprintf(stderr, "Unsupported manufacturer\n");

	return -1;
}
