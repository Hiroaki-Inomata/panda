/*	PANDA -- a simple transaction monitor

Copyright (C) 1998-1999 Ogochan.
              2000-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<signal.h>

#include	"types.h"
#include	"const.h"
#include	"glserver.h"
#include	"dirs.h"
#include	"DDparser.h"
#include	"front.h"
#include	"option.h"
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
	{	"auth",		STRING,		TRUE,	(void*)&AuthURL,
		"ǧ�ڥ�����"			 						},
#ifdef	USE_SSL
	{	"ssl",		BOOLEAN,	TRUE,	(void*)&fSsl,
		"SSL��Ȥ�"				 						},
	{	"key",		STRING,		TRUE,	(void*)&KeyFile,
		"���ե�����̾(pem)"		 						},
	{	"cert",		STRING,		TRUE,	(void*)&CertFile,
		"������ե�����̾(pem)"	 						},
	{	"verifypeer",BOOLEAN,	TRUE,	(void*)&fVerify,
		"���饤����Ⱦ�����θ��ڤ�Ԥ�"				},
	{	"CApath",	STRING,		TRUE,	(void*)&CA_Path,
		"CA������ؤΥѥ�"								},
	{	"CAfile",	STRING,		TRUE,	(void*)&CA_File,
		"CA������ե�����"								},
#endif

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	PortNumber = PORT_GLTERM;
	Back = 5;
	ScreenDir = ".";
	RecordDir = ".";
	AuthURL = "glauth://localhost:8001";	/*	PORT_GLAUTH	*/
#ifdef	USE_SSL
	fSsl = FALSE;
	KeyFile = NULL;
	CertFile = NULL;
	fVerify = FALSE;
	CA_Path = NULL;
	CA_File = NULL;
#endif	
}

static	void
StopProcess(
	int		ec)
{
dbgmsg(">StopProcess");
dbgmsg("<StopProcess");
	exit(ec);
}
extern	int
main(
	int		argc,
	char	**argv)
{
	(void)signal(SIGPIPE,(void *)StopProcess);
	SetDefault();
	(void)GetOption(option,argc,argv);
	InitMessage("glserver",NULL);

	ParseURL(&Auth,AuthURL,"file");
	InitSystem(argc,argv);
	ExecuteServer();
	return	(0);
}
