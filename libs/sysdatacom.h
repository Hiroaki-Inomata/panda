/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Ogochan & JMA (Japan Medical Association).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _INC_BLOBCOM_H
#define _INC_BLOBCOM_H

#include "libmondai.h"
#include "net.h"

/*sysata*/
#define SYSDATA_SYSDB (PacketClass)0x02
#define SYSDATA_END (PacketClass)0xFF

/*sysdb*/
#define SYSDB_GET_DATA (PacketClass)0x01
#define SYSDB_GET_MESSAGE (PacketClass)0x02
#define SYSDB_RESET_MESSAGE (PacketClass)0x03
#define SYSDB_SET_MESSAGE (PacketClass)0x04
#define SYSDB_SET_MESSAGE_ALL (PacketClass)0x05
#define SYSDB_GET_DATA_ALL (PacketClass)0x06

#endif
