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
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#include	<sys/socket.h>
#include	<sys/select.h>
#include	<time.h>
#include	<unistd.h>
#include	<pthread.h>
#include	<glib.h>

#include	"types.h"
#include	"libmondai.h"
#include	"RecParser.h"
#include	"comm.h"
#include	"dirs.h"
#include	"redirect.h"
#include	"dblib.h"
#include	"directory.h"
#include	"queue.h"
#include	"socket.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

#define		CONNECT_INTERVAL		60

static	char	*PortNumber;
static	int		Back;
static	char	*Directory;

static	sigset_t	hupset;
static	DBG_Struct	*ThisDBG;
static	pthread_t	_FileThread;
static	Queue		*FileQueue;
static	Queue		*CheckQueue;
static	Port		*RedirectPort;

static	void
LogThread(
	void	*para)
{
	int		fhLog = (int)para;
	LargeByteString	*data, *cdata;
	NETFILE	*fpLog;
	PacketClass	c;
	Bool	fSuc = TRUE;

ENTER_FUNC;
	dbgmsg("log thread!\n");
	fpLog = SocketToNet(fhLog);
	do {
		switch	( c = RecvPacketClass(fpLog) ) {
		  case	RED_DATA:
			data = NewLBS();
			LBS_EmitStart(data);
			RecvLBS(fpLog,data);
			LBS_EmitEnd(data);
			SendPacketClass(fpLog,RED_OK);
			EnQueue(FileQueue,data);
			fSuc = TRUE;
			break;
		  case	RED_CHECK:
			cdata = NewLBS();
			LBS_EmitStart(cdata);
			RecvLBS(fpLog,cdata);
			LBS_EmitEnd(cdata);
			SendPacketClass(fpLog,RED_OK);
			EnQueue(CheckQueue,cdata);
			fSuc = TRUE;
			break;
		  case	RED_PING:
			SendPacketClass(fpLog,RED_PONG);
			fSuc = TRUE;
			break;
		  case	RED_STATUS:
			SendChar(fpLog, ThisDBG->fConnect);
			fSuc = TRUE;
			break;
		  case	RED_END:
			fSuc = FALSE;
			break;
		  default:
			SendPacketClass(fpLog,RED_NOT);
			fSuc = FALSE;
			break;
		}
	}	while	(  fSuc  );
	CloseNet(fpLog);
	dbgmsg("log thread close!\n");
LEAVE_FUNC;
}

extern	pthread_t
ConnectLog(
	int		_fhLog)
{
	int		fhLog;
	pthread_t	thr;

ENTER_FUNC;
	if		(  ( fhLog = accept(_fhLog,0,0) )  <  0  )	{
		printf("_fhLog = %d\n",_fhLog);
		Error("INET Domain Accept");
	}
	pthread_create(&thr,NULL,(void *(*)(void *))LogThread,(void *)fhLog);
	pthread_detach(thr);
LEAVE_FUNC;
	return	(thr); 
}

static  FILE	*
OpenLogFile(
	char	*file)
{
	FILE	*fp = NULL;

	if		(  ThisDBG->file  !=  NULL  ) {
		umask((mode_t) 0077);
		if		(  ( fp = fopen(ThisDBG->file,"a+") )  ==  NULL  ) {
			Error("can not open log file :%s", ThisDBG->file);
		}
	}
	return fp;
}

static  void
WriteLog(
	FILE	*fp,
	char	*state)
{
	time_t	nowtime;
	struct	tm	*Now;
ENTER_FUNC;
	if		(  fp  !=  NULL  ) {
		time(&nowtime);
		Now = localtime(&nowtime);
		fprintf(fp, "%s %04d/%02d/%02d/%02d:%02d:%02d/ ========== %s ========== %s\n"
				,ThisDBG->func->commentStart
				, Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
				, Now->tm_hour,Now->tm_min,Now->tm_sec,state
				,ThisDBG->func->commentEnd);
		fflush(fp);
	}
LEAVE_FUNC;
}

static  void
WriteLogQuery(
	FILE	*fp,
	char	*query)
{
	static  int count = 0;
	time_t	nowtime;
	struct	tm	*Now;

ENTER_FUNC;
	if		(  fp  !=  NULL  ) {
		time(&nowtime);
		Now = localtime(&nowtime);
		fprintf(fp,"%s %04d/%02d/%02d/%02d:%02d:%02d/%08d %s"
				,ThisDBG->func->commentStart
				, Now->tm_year+1900,Now->tm_mon+1,Now->tm_mday
				, Now->tm_hour,Now->tm_min,Now->tm_sec,count
				,ThisDBG->func->commentEnd);
		fprintf(fp,"%s\n", query);
		fflush(fp);
		count ++;
	}
LEAVE_FUNC;
}

static  Bool
ConnectDB(void)
{
	Bool rc = TRUE;
ENTER_FUNC;
	if ( OpenRedirectDB(ThisDBG) == MCP_OK ) {
		Message("connect to database successed");
		ThisDBG->checkData = NewLBS();
	} else {
		Message("connect to database failed");
		rc = FALSE;
	}
LEAVE_FUNC;
	return rc;
}

void
ReConnectDB(void)
{
	int	retry = 0;
ENTER_FUNC;
	while ( !ConnectDB() ){
		retry ++;
		if ( retry > MaxRetry ){
			break;
		}
		sleep (CONNECT_INTERVAL);
	}
	if ( ThisDBG->fConnect == UNCONNECT ){
		ThisDBG->fConnect = FAILURE;
	}
LEAVE_FUNC;
}

static int
CheckRedirectData(
	LargeByteString	*src,
	LargeByteString	*dsc)
{
	int rc = MCP_OK;
ENTER_FUNC;
	if ( strcmp(LBS_Body(src), LBS_Body(dsc)) == 0){
		rc = MCP_OK;
	} else {
		Warning("DB synchronous failure");
		rc = MCP_BAD_OTHER;
	}
LEAVE_FUNC;
	return	rc;
}

static	int
WriteDB(
	char	*query,
	LargeByteString	*orgcheck)
{
	int rc;
	LargeByteString	*redcheck;
	
ENTER_FUNC;
	rc = TransactionRedirectStart(ThisDBG);
	if ( rc == MCP_OK ) {
		rc = ExecRedirectDBOP(ThisDBG, query);
		redcheck = ThisDBG->checkData;
	}
	if ( rc == MCP_OK ){
		rc = CheckRedirectData(orgcheck, redcheck);
	}
	if ( rc == MCP_OK ) {
		rc = TransactionRedirectEnd(ThisDBG);
	}
LEAVE_FUNC;
	return rc;
}


static	void
ExecDB(
	char	*query,
	LargeByteString	*orgcheck)
{
	int rc;
	
ENTER_FUNC;
	rc = WriteDB(query, orgcheck);
	if ( rc == MCP_BAD_CONN ) {
		CloseRedirectDB(ThisDBG);
		ThisDBG->fConnect = UNCONNECT;
		ReConnectDB();
		if ( ThisDBG->fConnect == CONNECT ){
			rc = WriteDB(query, orgcheck);
		}
	} else
	if ( rc != MCP_OK ) {
		ThisDBG->fConnect = FAILURE;
		CloseRedirectDB(ThisDBG);
	}
LEAVE_FUNC;
}

static  void
ReRedirect(
	char	*query)
{
ENTER_FUNC;
	BeginDB_Redirect(ThisDBG);
	PutDB_Redirect(ThisDBG, query);
	CommitDB_Redirect(ThisDBG);
LEAVE_FUNC;
}

static	void
FileThread(
	void	*dummy)
{
	LargeByteString	*data, *orgcheck;
	char	*query;
	FILE	*fp;

ENTER_FUNC;
	fp = OpenLogFile(ThisDBG->file);
	if		(  ThisDBG->dbname  !=  NULL  ) {
		WriteLog(fp, "dbredirector start");
		ConnectDB();
	} else {
		WriteLog(fp, "dbredirector start(No database)");
		OpenDB_RedirectPort(ThisDBG);
		ThisDBG->fConnect = NOCONNECT;
	}
	while	( TRUE )	{
		orgcheck= (LargeByteString *)DeQueue(CheckQueue);
		data = (LargeByteString *)DeQueue(FileQueue);
		query = LBS_Body(data);
		if		(  *query  !=  0  ) {
			if ( ThisDBG->fConnect == UNCONNECT ) {
				ReConnectDB();
			}
			if ( ThisDBG->fConnect == CONNECT ){
				ExecDB(query, orgcheck);
			}
			if ( ThisDBG->fConnect == FAILURE ){
				WriteLog(fp, "DB synchronous failure");
				ThisDBG->fConnect = DISCONNECT;
			}
			ReRedirect(query);
			WriteLogQuery(fp, query);
		}
		FreeLBS(data);
		FreeLBS(orgcheck);
	}
	if		(  ThisDBG->dbname  ==  NULL  ) {
		CloseDB_RedirectPort(ThisDBG);
	}
	WriteLog(fp, "dbredirector stop");
	exit(0);
LEAVE_FUNC;
}


extern	void
ExecuteServer(void)
{
	int		_fhLog;
	fd_set	ready;
	int		maxfd;

ENTER_FUNC;
	pthread_create(&_FileThread,NULL,(void *(*)(void *))FileThread,NULL); 
	_fhLog = InitServerPort(RedirectPort,Back);
	maxfd = _fhLog;

	while	(TRUE)	{
		FD_ZERO(&ready);
		FD_SET(_fhLog,&ready);
		select(maxfd+1,&ready,NULL,NULL,NULL);
		if		(  FD_ISSET(_fhLog,&ready)  ) {
			ConnectLog(_fhLog);
		}
	}
LEAVE_FUNC;
}

#ifdef	DEBUG
static	void
DumpDBG(
	char		*name,
	DBG_Struct	*dbg,
	void		*dummy)
{
	printf("name     = [%s]\n",dbg->name);
	printf("\ttype     = [%s]\n",dbg->type);
	printf("\tDB name  = [%s]\n",dbg->dbname);
	printf("\tDB user  = [%s]\n",dbg->user);
	printf("\tDB pass  = [%s]\n",dbg->pass);
	if		(  dbg->file  !=  NULL  ) {
		printf("\tlog file = [%s]\n",dbg->file);
	}
	if		(  dbg->redirect  !=  NULL  ) {
		dbg = dbg->redirect;
		printf("\tredirect = [%s]\n",dbg->name);
	}
}
#endif

static	void
_CheckDBG(
	char		*name,
	DBG_Struct	*dbg,
	char		*red_name)
{
	DBG_Struct	*red_dbg;
	char *src_port, *dsc_port;
	char *dbg_dbname = "", *red_dbg_dbname = "";
ENTER_FUNC;		
	if		(  dbg->redirect  !=  NULL  ) {
		red_dbg = dbg->redirect;
		if ( strcmp(red_dbg->name, red_name ) == 0 ){
			src_port = StrDup(StringPort(dbg->port));
			dsc_port = StrDup(StringPort(red_dbg->port));
			if ( dbg->dbname != NULL){
				dbg_dbname = dbg->dbname;
			}
			if ( red_dbg->dbname != NULL){
				red_dbg_dbname = red_dbg->dbname;
			}
			if ( strcmp(dbg->type, red_dbg->type ) == 0
			  && strcmp(dbg_dbname, red_dbg_dbname ) == 0 
			  && strcmp(src_port, dsc_port) == 0 ) {
				Error("The connection destination is same DB");
			}
			xfree(src_port);
			xfree(dsc_port);
		}
	}
LEAVE_FUNC;
}

static	void
CheckDBG(
	char		*name)
{
	g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)_CheckDBG,name);
}


extern	void
InitSystem(
	char	*name)
{

ENTER_FUNC;
	InitNET();
	sigemptyset(&hupset); 
	sigaddset(&hupset,SIGHUP);
	InitDirectory();
	SetUpDirectory(Directory,NULL,NULL,NULL,FALSE);
	if		( ThisEnv == NULL ) {
		Error("DI file parse error.");
	}
	InitDB_Process(NULL);
#ifdef	DEBUG
	g_hash_table_foreach(ThisEnv->DBG_Table,(GHFunc)DumpDBG,NULL);
#endif
	if		(  ( ThisDBG = (DBG_Struct *)g_hash_table_lookup(ThisEnv->DBG_Table,name) )
			   ==  NULL  ) {
		Error("DB group not found");
	}

	CheckDBG(name);

	if		(  PortNumber  ==  NULL  ) {
		if		(  ( ThisDBG != NULL) 
				   && (ThisDBG->redirectPort  !=  NULL )) {
			RedirectPort = ThisDBG->redirectPort;
		} else {
			RedirectPort = ParPortName(PORT_REDIRECT);
		}
	} else {
		RedirectPort = ParPortName(PortNumber);
	}
	FileQueue = NewQueue();
	CheckQueue = NewQueue();
LEAVE_FUNC;
}

static	ARG_TABLE	option[] = {
	{	"port",		STRING,		TRUE,	(void*)&PortNumber,
		"�ݡ����ֹ�"	 								},
	{	"back",		INTEGER,	TRUE,	(void*)&Back,
		"��³�Ԥ����塼�ο�" 							},

	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"�ǥ��쥯�ȥ�ե�����"	 						},
	{	"base",		STRING,		TRUE,	(void*)&BaseDir,
		"�Ķ��Υ١����ǥ��쥯�ȥ�"		 				},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"�ǡ��������Ǽ�ǥ��쥯�ȥ�"	 				},
	{	"ddir",	STRING,			TRUE,	(void*)&D_Dir,
		"�����Ǽ�ǥ��쥯�ȥ�"			 				},

	{	"host",		STRING,		TRUE,	(void*)&DB_Host,
		"PostgreSQL��Ư�ۥ���̾"						},
	{	"port",		STRING,		TRUE,	(void*)&DB_Port,
		"PostgreSQL�ݡ����ֹ�"							},
	{	"db",		STRING,		TRUE,	(void*)&DB_Name,
		"�ǡ����١���̾"								},
	{	"user",		STRING,		TRUE,	(void*)&DB_User,
		"�桼��̾"										},
	{	"pass",		STRING,		TRUE,	(void*)&DB_Pass,
		"�ѥ����"									},

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
	PortNumber = NULL;
	Back = 5;
	BaseDir = NULL;
	RecordDir = NULL;
	D_Dir = NULL;
	Directory = "./directory";

	DB_User = NULL;
	DB_Pass = NULL;
	DB_Host = NULL;
	DB_Port = NULL;
	DB_Name = DB_User;

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
	char		*name;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage("dbredirector",NULL);

	if		(	fl	&&	fl->name  ) {
		name = fl->name;
	} else {
		name = "";
	}
	InitSystem(name);
	Message("dbredirector start");
	ExecuteServer();
	Message("dbredirector end");
	return	(0);
}
