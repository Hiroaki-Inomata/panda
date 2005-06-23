/*
PANDA -- a simple transaction monitor
Copyright (C) 2000-2003 Ogochan & JMA (Japan Medical Association).
Copyright (C) 2004-2005 Ogochan.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#define	MAIN
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"
#include	"const.h"
#include	"pgserver.h"
#include	"dirs.h"
#include	"RecParser.h"
#include	"option.h"
#include	"front.h"
#include	"message.h"
#include	"debug.h"

static	char		*AuthURL;
static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"�ݡ����ֹ�"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"��³�Ԥ����塼�ο�" 							},
	{	"screen",	STRING,		TRUE,	(void*)&ScreenDir,
		"���̳�Ǽ�ǥ��쥯�ȥ�"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"�ǡ��������Ǽ�ǥ��쥯�ȥ�"	 				},
	{	"cache",	STRING,		TRUE,	(void*)&CacheDir,
		"BLOB����å���ǥ��쥯�ȥ�̾"					},
	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		"ǧ�ڥ�����"			 						},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = PORT_PGSERV;
	Back = 5;
	ScreenDir = ".";
	RecordDir = ".";
	AuthURL = "glauth://localhost:8001";	/*	PORT_GLAUTH	*/
	CacheDir = "cache";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	SetDefault();
	(void)GetOption(option,argc,argv);
	InitMessage("pgserver",NULL);

	ParseURL(&Auth,AuthURL,"file");
	InitSystem(argc,argv);
	ExecuteServer();
	return	(0);
}
