/*	PANDA -- a simple transaction monitor

Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).

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

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<string.h>
#include	<strings.h>
#include	<netinet/in.h>
#include	<glib.h>
#include	<math.h>

#include	"types.h"
#include	"libmondai.h"
#include	"glserver.h"
#include	"glcomm.h"
#include	"debug.h"

#include	<endian.h>

#define	RECV32(v)	ntohl(v)
#define	RECV16(v)	ntohs(v)
#define	SEND32(v)	htonl(v)
#define	SEND16(v)	htons(v)

#define	GL_OLDTYPE_INT			(PacketDataType)0x10
#define	GL_OLDTYPE_BOOL			(PacketDataType)0x11
#define	GL_OLDTYPE_FLOAT		(PacketDataType)0x20
#define	GL_OLDTYPE_CHAR			(PacketDataType)0x30
#define	GL_OLDTYPE_TEXT			(PacketDataType)0x31
#define	GL_OLDTYPE_VARCHAR		(PacketDataType)0x32
#define	GL_OLDTYPE_BYTE			(PacketDataType)0x40
#define	GL_OLDTYPE_NUMBER		(PacketDataType)0x50
#define	GL_OLDTYPE_DBCODE		(PacketDataType)0x60
#define	GL_OLDTYPE_ARRAY		(PacketDataType)0x90
#define	GL_OLDTYPE_RECORD		(PacketDataType)0xA0

static	LargeByteString	*Buff;

static	PacketDataType	ToOldType[256];
static	PacketDataType	ToNewType[256];

extern	void
GL_SendPacketClass(
	NETFILE	*fp,
	PacketClass	c,
	Bool	fNetwork)
{
	nputc(c,fp);
}

extern	PacketClass
GL_RecvPacketClass(
	NETFILE	*fp,
	Bool	fNetwork)
{
	PacketClass	c;

	c = ngetc(fp);
	return	(c);
}

#define	SendChar(fp,c)	nputc((c),(fp))
#define	RecvChar(fp)	ngetc(fp)

extern	void
GL_SendInt(
	NETFILE	*fp,
	int		data,
	Bool	fNetwork)
{
	byte	buff[sizeof(int)];

	if		(  fNetwork  ) {
		*(int *)buff = SEND32(data);
	} else {
		*(int *)buff = data;
	}
	Send(fp,buff,sizeof(int));
}

static	void
GL_SendUInt(
	NETFILE	*fp,
	unsigned	int		data,
	Bool	fNetwork)
{
	byte	buff[sizeof(int)];

	if		(  fNetwork  ) {
		*(unsigned int *)buff = SEND32(data);
	} else {
		*(unsigned int *)buff = data;
	}
	Send(fp,buff,sizeof(unsigned int));
}

extern	void
GL_SendLong(
	NETFILE	*fp,
	long	data,
	Bool	fNetwork)
{
	byte	buff[sizeof(long)];

	if		(  fNetwork  ) {
		*(long *)buff = SEND32(data);
	} else {
		*(long *)buff = data;
	}
	Send(fp,buff,sizeof(long));
}

static	void
GL_SendLength(
	NETFILE	*fp,
	size_t	data,
	Bool	fNetwork)
{
	byte	buff[sizeof(size_t)];

	if		(  fNetwork  ) {
		*(size_t *)buff = SEND32(data);
	} else {
		*(size_t *)buff = data;
	}
	Send(fp,buff,sizeof(size_t));
}

extern	int
GL_RecvInt(
	NETFILE	*fp,
	Bool	fNetwork)
{
	byte	buff[sizeof(int)];
	int		data;

	Recv(fp,buff,sizeof(int));
	if		(  fNetwork  ) {
		data = RECV32(*(int *)buff);
	} else {
		data = *(int *)buff;
	}
	return	(data);
}

static	size_t
GL_RecvLength(
	NETFILE	*fp,
	Bool	fNetwork)
{
	byte	buff[sizeof(size_t)];
	size_t	data;

	Recv(fp,buff,sizeof(size_t));
	if		(  fNetwork  ) {
		data = RECV32(*(size_t *)buff);
	} else {
		data = *(size_t *)buff;
	}
	return	(data);
}

static	void
GL_SendLBS(
	NETFILE	*fp,
	LargeByteString	*lbs,
	Bool	fNetwork)
{
	GL_SendLength(fp,LBS_Size(lbs),fNetwork);
	if		(  LBS_Size(lbs)  >  0  ) {
		Send(fp,LBS_Body(lbs),LBS_Size(lbs));
	}
}

extern	void
GL_RecvString(
	NETFILE	*fp,
	char	*str,
	Bool	fNetwork)
{
	size_t	size;

ENTER_FUNC;
	size = GL_RecvLength(fp,fNetwork);
	Recv(fp,str,size);
	str[size] = 0;
LEAVE_FUNC;
}

extern	void
GL_SendString(
	NETFILE	*fp,
	char	*str,
	Bool	fNetwork)
{
	size_t	size;

ENTER_FUNC;
	if		(   str  !=  NULL  ) { 
		size = strlen(str);
	} else {
		size = 0;
	}
	GL_SendLength(fp,size,fNetwork);
	if		(  size  >  0  ) {
		Send(fp,str,size);
	}
LEAVE_FUNC;
}

static	void
GL_SendObject(
	NETFILE	*fp,
	MonObjectType	*obj,
	Bool	fNetwork)
{
	int		i;

	GL_SendInt(fp,obj->source,fNetwork);
	for	( i = 0 ; i < SIZE_OID/sizeof(unsigned int) ; i ++ ) {
		GL_SendUInt(fp,obj->id.el[i],fNetwork);
	}
}

extern	Fixed	*
GL_RecvFixed(
	NETFILE	*fp,
	Bool	fNetwork)
{
	Fixed	*xval;

dbgmsg(">RecvFixed");
	xval = New(Fixed);
	xval->flen = GL_RecvLength(fp,fNetwork);
	xval->slen = GL_RecvLength(fp,fNetwork);
	xval->sval = (char *)xmalloc(xval->flen+1);
	GL_RecvString(fp,xval->sval,fNetwork);
dbgmsg("<RecvFixed");
	return	(xval); 
}

static	void
GL_SendFixed(
	NETFILE	*fp,
	Fixed	*xval,
	Bool	fNetwork)
{
	GL_SendLength(fp,xval->flen,fNetwork);
	GL_SendLength(fp,xval->slen,fNetwork);
	GL_SendString(fp,xval->sval,fNetwork);
}

extern	double
GL_RecvFloat(
	NETFILE	*fp,
	Bool	fNetwork)
{
	double	data;

	Recv(fp,&data,sizeof(data));
	return	(data);
}

static	void
GL_SendFloat(
	NETFILE	*fp,
	double	data,
	Bool	fNetwork)
{
	Send(fp,&data,sizeof(data));
}

extern	Bool
GL_RecvBool(
	NETFILE	*fp,
	Bool	fNetwork)
{
	char	buf[1];

	Recv(fp,buf,1);
	return	((buf[0] == 'T' ) ? TRUE : FALSE);
}

static	void
GL_SendBool(
	NETFILE	*fp,
	Bool	data,
	Bool	fNetwork)
{
	char	buf[1];

	buf[0] = data ? 'T' : 'F';
	Send(fp,buf,1);
}

static	void
GL_SendDataType(
	NETFILE	*fp,
	PacketClass	c,
	Bool	fNetwork)
{
#ifdef	DEBUG
	printf("SendDataType = %X\n",c);
#endif
	if		(  fFetureOld  ) {
		c = ToOldType[c];
	}
	nputc(c,fp);
}

extern	PacketDataType
GL_RecvDataType(
	NETFILE	*fp,
	Bool	fNetwork)
{
	PacketClass	c;

	c = ngetc(fp);
	if		(  fFetureOld  ) {
		c = ToNewType[c];
	}
	return	(c);
}

/*
 *	This function sends value with valiable name.
 */
extern	void
GL_SendValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*coding,
	Bool		fExpand,
	Bool		fNetwork)
{
	int		i;

	ValueIsNotUpdate(value);
	GL_SendDataType(fp,ValueType(value),fNetwork);
	switch	(ValueType(value)) {
	  case	GL_TYPE_INT:
		GL_SendInt(fp,ValueInteger(value),fNetwork);
		break;
	  case	GL_TYPE_BOOL:
		GL_SendBool(fp,ValueBool(value),fNetwork);
		break;
	  case	GL_TYPE_BINARY:
	  case	GL_TYPE_BYTE:
		LBS_ReserveSize(Buff,ValueByteLength(value),FALSE);
		memcpy(LBS_Body(Buff),ValueByte(value),ValueByteLength(value));
		GL_SendLBS(fp,Buff,fNetwork);
		break;
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_SendString(fp,ValueToString(value,coding),fNetwork);
		break;
	  case	GL_TYPE_FLOAT:
		GL_SendFloat(fp,ValueFloat(value),fNetwork);
		break;
	  case	GL_TYPE_NUMBER:
		GL_SendFixed(fp,&ValueFixed(value),fNetwork);
		break;
	  case	GL_TYPE_OBJECT:
		if		(  fExpand  ) {
		} else {
			GL_SendObject(fp,ValueObject(value),fNetwork);
		}
		break;
	  case	GL_TYPE_ARRAY:
		GL_SendInt(fp,ValueArraySize(value),fNetwork);
		for	( i = 0 ; i < ValueArraySize(value) ; i ++ ) {
			GL_SendValue(fp,ValueArrayItem(value,i),coding,fExpand,fNetwork);
		}
		break;
	  case	GL_TYPE_RECORD:
		GL_SendInt(fp,ValueRecordSize(value),fNetwork);
		for	( i = 0 ; i < ValueRecordSize(value) ; i ++ ) {
			if		(  fFetureOld  ) {
				if		(	(  stricmp(ValueRecordName(value,i),"row")      ==  0  )
						||	(  stricmp(ValueRecordName(value,i),"column")   ==  0  ) )	{
					GL_SendString(fp,ValueRecordName(value,i),fNetwork);
				} else
				if		(  stricmp(ValueRecordName(value,i),"rowattr")  ==  0  ) {
					GL_SendString(fp,"row",fNetwork);
				} else {
					GL_SendString(fp,ValueRecordName(value,i),fNetwork);
					GL_SendValue(fp,ValueRecordItem(value,i),coding,fExpand,fNetwork);
				}
			} else {
				GL_SendString(fp,ValueRecordName(value,i),fNetwork);
				GL_SendValue(fp,ValueRecordItem(value,i),coding,fExpand,fNetwork);
			}
		}
		break;
	  default:
		break;
	}
}

extern	void
GL_RecvValue(
	NETFILE		*fp,
	ValueStruct	*value,
	char		*coding,
	Bool		fExpand,
	Bool		fNetwork)
{
	PacketDataType	type;
	Fixed		*xval;
	int			ival;
	Bool		bval;
	double		fval;
	char		str[SIZE_BUFF];

ENTER_FUNC;
	ValueIsUpdate(value);
	type = GL_RecvDataType(fp,fNetwork);
	ON_IO_ERROR(fp,badio);
	switch	(type)	{
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
	  case	GL_TYPE_TEXT:
		GL_RecvString(fp,str,fNetwork);ON_IO_ERROR(fp,badio);
		SetValueString(value,str,coding);	ON_IO_ERROR(fp,badio);
		break;
	  case	GL_TYPE_NUMBER:
		xval = GL_RecvFixed(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueFixed(value,xval);
		xfree(xval->sval);
		xfree(xval);
		break;
	  case	GL_TYPE_INT:
		ival = GL_RecvInt(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueInteger(value,ival);
		break;
	  case	GL_TYPE_FLOAT:
		fval = GL_RecvFloat(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueFloat(value,fval);
		break;
	  case	GL_TYPE_BOOL:
		bval = GL_RecvBool(fp,fFetureNetwork);
		ON_IO_ERROR(fp,badio);
		SetValueBool(value,bval);
		break;
	  default:
		printf("type = [%d]\n",type);
		break;
	}
  badio:
LEAVE_FUNC;
}

extern	void
InitGL_Comm(void)
{
	int		i;

	Buff = NewLBS();
#define	TO_OLDTYPE(t)	ToOldType[GL_TYPE_##t] = GL_OLDTYPE_##t
#define	TO_NEWTYPE(t)	ToNewType[GL_OLDTYPE_##t] = GL_TYPE_##t

	for	( i = 0 ; i < 256 ; i ++ ) {
		ToOldType[i] = GL_TYPE_NULL;
		ToNewType[i] = GL_TYPE_NULL;
	}

	TO_OLDTYPE(INT);
	TO_OLDTYPE(BOOL);
	TO_OLDTYPE(FLOAT);
	TO_OLDTYPE(CHAR);
	TO_OLDTYPE(TEXT);
	TO_OLDTYPE(VARCHAR);
	TO_OLDTYPE(BYTE);
	TO_OLDTYPE(NUMBER);
	TO_OLDTYPE(DBCODE);
	TO_OLDTYPE(ARRAY);
	TO_OLDTYPE(RECORD);

	TO_NEWTYPE(INT);
	TO_NEWTYPE(BOOL);
	TO_NEWTYPE(FLOAT);
	TO_NEWTYPE(CHAR);
	TO_NEWTYPE(TEXT);
	TO_NEWTYPE(VARCHAR);
	TO_NEWTYPE(BYTE);
	TO_NEWTYPE(NUMBER);
	TO_NEWTYPE(DBCODE);
	TO_NEWTYPE(ARRAY);
	TO_NEWTYPE(RECORD);
}