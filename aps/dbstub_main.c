/*
PANDA -- a simple transaction monitor
Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).
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
/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<glib.h>

#include	"types.h"

#include	"dirs.h"
#include	"const.h"
#include	"enum.h"
#include	"dblib.h"
#include	"dbgroup.h"
#include	"RecParser.h"
#include	"directory.h"
#include	"handler.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	char	*CommandParameter;
static	char	*BD_Name;
static	BatchBind	*Bind;

static	void
InitData(
	char	*name)
{
dbgmsg(">InitData");
	InitDirectory();
	SetUpDirectory(Directory,"",name,"",TRUE);
dbgmsg("<InitData");
}

extern	void
InitSystem(
	char	*name)
{
dbgmsg(">InitSystem");
	InitData(BD_Name);
	if		(  ( ThisBD = GetBD(BD_Name) )  ==  NULL  ) {
		Error("BD file not found.");
	}
	if		(  ThisBD->home  !=  NULL  ) {
		chdir(ThisBD->home);
	}
	ThisLD = NULL;
	ThisDBD = NULL;
	InitiateBatchHandler();
	ThisDB = ThisBD->db;
	DB_Table = ThisBD->DB_Table;
	TextSize = ThisBD->textsize;
	if		(  ( Bind = g_hash_table_lookup(ThisBD->BatchTable,name) )  ==  NULL  ) {
		Error("%s application is not in BD.",name);
	}
	if		(  ThisBD->cDB  >  0  ) {
		InitDB_Process(NULL);
		ReadyHandlerDB(Bind->handler);
	}
dbgmsg("<InitSystem");
}

static	int
ExecuteSubProcess(
	char	*name)
{
	int		rc;
dbgmsg(">ExecuteSubProcess");
	printf("[%s][%s]\n",name,CommandParameter);
	rc = StartBatch(name,CommandParameter);
dbgmsg("<ExecuteSubProcess");
	return	(rc); 
}

static	void
StopProcess(
	int		ec)
{
dbgmsg(">StopProcess");
	if		(  ThisBD->cDB  >  0  ) {
		StopHandlerDB(Bind->handler);
		CleanUpHandlerDB(Bind->handler);
	}
dbgmsg("<StopProcess");
	exit(ec);
}
static	ARG_TABLE	option[] = {
	{	"host",		STRING,		TRUE,	(void*)&DB_Host,
		"PostgreSQL��Ư�ۥ���̾"						},
	{	"port",		STRING,		TRUE,	(void*)&DB_Port,
		"PostgreSQL�ݡ����ֹ�"							},

	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"�Ķ��Υ١����ǥ��쥯�ȥ�"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"�ǡ��������Ǽ�ǥ��쥯�ȥ�"	 				},
	{	"bddir",	STRING,		TRUE,	(void*)&D_Dir,
		"�Хå������Ǽ�ǥ��쥯�ȥ�"	 				},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"�ǥ��쥯�ȥ�ե�����"	 						},
	{	"path",		STRING,		TRUE,	(void*)&LibPath,
		"�⥸�塼��Υ��ɥѥ�"						},
	{	"parameter",STRING,		TRUE,	(void*)&CommandParameter,
		"���ޥ�ɥ饤��"								},

	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"�ǡ����١���̾"								},
	{	"user",		STRING,		TRUE,	(void*)&DB_User,
		"�桼��̾"										},
	{	"pass",		STRING,		TRUE,	(void*)&DB_Pass,
		"�ѥ����"									},
	{	"bd",		STRING,		TRUE,	(void*)&BD_Name,
		"BD���̾"										},

	{	"nocheck",	BOOLEAN,	TRUE,	(void*)&fNoCheck,
		"dbredirector�ε�ư������å����ʤ�"			},
	{	"noredirect",BOOLEAN,	TRUE,	(void*)&fNoRedirect,
		"dbredirector��Ȥ�ʤ�"						},
	{	"maxretry",	INTEGER,	TRUE,	(void*)&MaxRetry,
		"dbredirector�����κƻ�Կ�����ꤹ��"			},
	{	"retryint",	INTEGER,	TRUE,	(void*)&RetryInterval,
		"dbredirector�����κƻ�Ԥδֳ֤���ꤹ��(��)"	},

	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";
	LibPath = NULL;

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;
	BD_Name = NULL;
	CommandParameter = "";

	fNoCheck = FALSE;
	fNoRedirect = FALSE;
	MaxRetry = 3;
	RetryInterval = 5;
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	int		rc;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("dbstub",NULL);
	InitNET();

	(void)signal(SIGHUP,(void *)StopProcess);
	if		(  BD_Name  ==  NULL  ) {
		Error("BD name is not specified.");
	}
	if		( fl == NULL ) {
		Error("module name is not specified.");
	}
	InitSystem(fl->name);
	rc = ExecuteSubProcess(fl->name);
	StopProcess(rc);
	return	(rc);
}
