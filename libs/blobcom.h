/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan & JMA (Japan Medical Association).

This module is part of PANDA.

	PANDA is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves
any particular purpose or works at all, unless he says so in writing.
Refer to the GNU General Public License for full details. 

	Everyone is granted permission to copy, modify and redistribute
PANDA, but only under the conditions described in the GNU General
Public License.  A copy of this license is supposed to have been given
to you along with PANDA so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies. 
*/

#ifndef	_INC_BLOBCOM_H
#define	_INC_BLOBCOM_H

#include	"libmondai.h"
#include	"net.h"
#include	"blob.h"

extern	void	PassiveBLOB(NETFILE *fp, BLOB_Space *Blob);

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif

#define	BLOB_CREATE			(PacketClass)0x01
#define	BLOB_OPEN			(PacketClass)0x02
#define	BLOB_IMPORT			(PacketClass)0x03
#define	BLOB_EXPORT			(PacketClass)0x04
#define	BLOB_READ			(PacketClass)0x05
#define	BLOB_WRITE			(PacketClass)0x06
#define	BLOB_CLOSE			(PacketClass)0x07
#define	BLOB_SEEK			(PacketClass)0x08
#define	BLOB_SAVE			(PacketClass)0x09
#define	BLOB_CHECK			(PacketClass)0x0A

#define	BLOB_OK				(PacketClass)0xFE
#define	BLOB_NOT			(PacketClass)0xF0
#define	BLOB_END			(PacketClass)0xFF

#endif