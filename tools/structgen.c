/*	PANDA -- a simple transaction monitor

Copyright (C) 2001-2003 Ogochan & JMA (Japan Medical Association).

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
#include	<string.h>
#include	<ctype.h>
#include	<unistd.h>
#include	<glib.h>
#include	"types.h"
#include	"libmondai.h"
#include	"const.h"
#include	"enum.h"
#include	"wfc.h"
#include	"apsio.h"
#include	"directory.h"
#include	"driver.h"
#include	"dirs.h"
#include	"option.h"
#include	"message.h"
#include	"debug.h"

static	Bool	fFiller;
static	Bool	fSPA;
static	Bool	fCTRL;
static	Bool	fLinkage;
static	Bool	fScreen;
static	Bool	fWindowPrefix;
static	Bool	fLDW;
static	Bool	fLDR;
static	Bool	fDB;
static	Bool	fDBREC;
static	Bool	fDBINFO;
static	Bool	fDBCOMM;
static	Bool	fDBPATH;
static	int		TextSize;
static	int		ArraySize;
static	char	*Prefix;
static	char	*RecName;
static	char	*LD_Name;
static	char	*BD_Name;

static	int		level;

static	void	COBOL(ValueStruct *val, size_t arraysize, size_t textsize);

static	int		Col;

static	void
PutLevel(
	int		level)
{
	int		n;

	n = level - 1;
	for	( ; n > 0 ; n -- ) {
		fputc('\t',stdout);
	}
	Col = 0;
}

static	void
PutTab(void)
{
	fputc('\t',stdout);
}

static	void
PutChar(
	int		c)
{
	fputc(c,stdout);
	Col ++;
}

static	void
PutString(
	char	*str)
{
	int		c;

	while	(  *str  !=  0  ) {
		PutChar(*str);
		str ++;
	}
}

static	void
PutName(
	char	*name)
{
	char	buff[SIZE_NAME+1];

	if		(  !stricmp(name,"filler")  ) {
		strcpy(buff,"filler");
	} else {
		sprintf(buff,"%s%s",Prefix,name);
	}
	PutString(buff);
}

static	void
C(
	ValueStruct	*val,
	size_t		arraysize,
	size_t		textsize)
{
	int		i
	,		n;
	ValueStruct	*tmp;
	char	buff[SIZE_BUFF+1];

	if		(  val  ==  NULL  )	return;

	switch	(val->type) {
	  case	GL_TYPE_INT:
		PutString("int");
		break;
	  case	GL_TYPE_BOOL:
		PutString("Bool");
		break;
	  case	GL_TYPE_BYTE:
	  case	GL_TYPE_CHAR:
	  case	GL_TYPE_VARCHAR:
	  case	GL_TYPE_DBCODE:
		sprintf(buff,"PIC X(%d)",val->body.CharData.len);
		PutString(buff);
		break;
	  case	GL_TYPE_NUMBER:
		if		(  val->body.FixedData.slen  ==  0  ) {
			sprintf(buff,"PIC S9(%d)",val->body.FixedData.flen);
		} else {
			sprintf(buff,"PIC S9(%d)V9(%d)",
					(val->body.FixedData.flen - val->body.FixedData.slen),
					val->body.FixedData.slen);
		}
		PutString(buff);
		break;
	  case	GL_TYPE_TEXT:
		sprintf(buff,"PIC X(%d)",textsize);
		PutString(buff);
		break;
	  case	GL_TYPE_ARRAY:
		tmp = val->body.ArrayData.item[0];
		n = val->body.ArrayData.count;
		if		(  n  ==  0  ) {
			n = arraysize;
		}
		if		(  tmp->type  ==  GL_TYPE_RECORD  ) {
			sprintf(buff,"OCCURS  %d TIMES",n);
			PutTab(8);
			PutString(buff);
			COBOL(tmp,arraysize,textsize);
		} else {
			COBOL(tmp,arraysize,textsize);
			sprintf(buff,"OCCURS  %d TIMES",n);
			PutTab(8);
			PutString(buff);
		}
		break;
	  case	GL_TYPE_RECORD:
		level ++;
		for	( i = 0 ; i < val->body.RecordData.count ; i ++ ) {
			PutLevel(level);
			tmp = val->body.RecordData.item[i];
			PutName(val->body.RecordData.names[i]);
			if		(  tmp->type  !=  GL_TYPE_RECORD  ) {
				PutTab(4);
			}
			C(tmp,arraysize,textsize);
		}
		level --;
		break;
	  default:
		break;
	}
}

static	void
SIZE(
	ValueStruct	*val,
	size_t		arraysize,
	size_t		textsize)
{
	char	buff[SIZE_BUFF+1];

	if		(  val  ==  NULL  )	return;
	level ++;
	PutLevel(level);
	PutName("filler");
	PutTab(8);
	sprintf(buff,"PIC X(%d)",SizeValue(val,arraysize,textsize));
	PutString(buff);
	level --;
}

static	void
MakeFromRecord(
	char	*name)
{
	RecordStruct	*rec;

	if		(  fScreen  ) {
		level = 3;
	} else {
		level = 1;
	}
	RecParserInit();
	if		(  ( rec = DD_ParserDataDefines(name) )  !=  NULL  ) {
		PutLevel(level);
		if		(  *RecName  ==  0  ) {
			PutString(rec->name);
		} else {
			PutString(RecName);
		}
		if		(  fFiller  ) {
			printf(".\n");
			SIZE(rec->rec,ArraySize,TextSize);
		} else {
			COBOL(rec->rec,ArraySize,TextSize);
		}
		printf(".\n");
	}
}

static	void
MakeLD(void)
{
	LD_Struct	*ld;
	int		i;
	char	buff[SIZE_BUFF+1];
	char	*_prefix;
	size_t	size
	,		num
	,		spasize;
	int		base;

dbgmsg(">MakeLD");
	InitDirectory(TRUE);
	SetUpDirectory(Directory,LD_Name,"",TRUE);
	if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
		Error("LD not found.\n");
	}

	PutLevel(1);
	if		(  *RecName  ==  0  ) {
		sprintf(buff,"%s",ld->name);
	} else {
		sprintf(buff,"%s",RecName);
	}
	PutString(buff);
	printf(".\n");

	base = 2;
	if		(	(  fLDR     )
			||	(  fLDW     ) ) {
		size =	sizeof(ControlData)
			+	ThisEnv->linksize
			+	SizeValue(ld->sparec,ld->arraysize,ld->textsize);
		for	( i = 0 ; i < ld->cWindow ; i ++ ) {
			size += SizeValue(ld->window[i]->value,ld->arraysize,ld->textsize);
		}
		num = ( size / SIZE_BLOCK ) + 1;

		PutLevel(base);
		PutName("dataarea");
		printf(".\n");

		PutLevel(base+1);
		PutName("data");
		PutTab(12);
		printf("OCCURS  %d.\n",num);

		PutLevel(base+2);
		PutName("filler");
		PutTab(12);
		printf("PIC X(%d).\n",SIZE_BLOCK);

		PutLevel(base);
		PutName("inputarea-red");
		PutTab(4);
		printf("REDEFINES   ");
		PutName("dataarea.\n");
		base ++;
	} else {
		num = 0;
	}
	if		(  fCTRL  ) {
		PutLevel(base);
		PutName("ctrldata");
		printf(".\n");

		PutLevel(base+1);
		PutName("ctl-term");
		PutTab(4);
		printf("PIC X(%d).\n",SIZE_TERM);

		PutLevel(base+1);
		PutName("filler");
		PutTab(12);
		printf("PIC X(1).\n");

		PutLevel(base+1);
		PutName("ctl-user");
		PutTab(4);
		printf("PIC X(%d).\n",SIZE_USER);

		PutLevel(base+1);
		PutName("filler");
		PutTab(12);
		printf("PIC X(1).\n");

		PutLevel(base+1);
		PutName("filler");
		PutTab(12);
		printf("PIC X(%d).\n",(64-(SIZE_TERM+SIZE_USER+5)));

		PutLevel(base+1);
		PutName("ctl-status");
		PutTab(4);
		printf("PIC 9(1).\n");

		PutLevel(base+1);
		PutName("ctl-puttype");
		PutTab(4);
		printf("PIC 9(1).\n");

		PutLevel(base+1);
		PutName("ctl-rc");
		PutTab(12);
		printf("PIC 9(1).\n");

		PutLevel(base+1);
		PutName("ctl-module");
		PutTab(4);
		printf("PIC X(%d).\n",SIZE_NAME);

		PutLevel(base+1);
		PutName("ctl-window");
		PutTab(4);
		printf("PIC X(%d).\n",SIZE_NAME);

		PutLevel(base+1);
		PutName("ctl-widget");
		PutTab(4);
		printf("PIC X(%d).\n",SIZE_NAME);

		PutLevel(base+1);
		PutName("ctl-event");
		PutTab(4);
		printf("PIC X(%d).\n",SIZE_EVENT);

	}
	if		(  fSPA  ) {
		if		(  ( spasize = SizeValue(ld->sparec,ld->arraysize,ld->textsize) )
				   >  0  ) {
			PutLevel(base);
			PutName("spadata");
			if		(  ld->sparec  ==  NULL  ) {
				printf(".\n");
				PutLevel(base+1);
				PutName("filler");
				printf("      PIC X(%d).\n",spasize);
			} else {
				level = base;
				if		(	(  fFiller  )
						||	(  fLDW     ) ) {
					printf(".\n");
					SIZE(ld->sparec,ld->arraysize,ld->textsize);
				} else {
					_prefix = Prefix;
					Prefix = "spa-";
					COBOL(ld->sparec,ld->arraysize,ld->textsize);
					Prefix = _prefix;
				}
				printf(".\n");
			}
		}
	}
	if		(  fLinkage  ) {
		if		(  SizeValue(ld->linkrec,ld->arraysize,ld->textsize)  >  0  ) {
			PutLevel(base);
			PutName("linkdata");
			level = base;
			if		(	(  fFiller  )
					||	(  fLDR     )
					||	(  fLDW     ) ) {
				printf(".\n");
				PutLevel(level+1);
				PutName("filler");
				printf("      PIC X(%d)",ThisEnv->linksize);
			} else {
				_prefix = Prefix;
				Prefix = "lnk-";
				COBOL(ld->linkrec,ld->arraysize,ld->textsize);
				Prefix = _prefix;
			}
			printf(".\n");
		}
	}
	if		(  fScreen  ) {
		PutLevel(base);
		sprintf(buff,"screendata");
		PutName(buff);
		printf(".\n");
		_prefix = Prefix;
		for	( i = 0 ; i < ld->cWindow ; i ++ ) {
			if		(  SizeValue(ld->window[i]->value,ld->arraysize,ld->textsize)  >  0  ) {
				Prefix = _prefix;
				PutLevel(base+1);
				sprintf(buff,"%s",ld->window[i]->name);
				PutName(buff);
				if		(  fWindowPrefix  ) {
					sprintf(buff,"%s-",ld->window[i]->name);
					Prefix = StrDup(buff);
				}
				level = base+1;
				if		(	(  fFiller  )
						||	(  fLDR     )
						||	(  fLDW     ) ) {
					printf(".\n");
					SIZE(ld->window[i]->value,ld->arraysize,ld->textsize);
				} else {
					COBOL(ld->window[i]->value,ld->arraysize,ld->textsize);
				}
				printf(".\n");
				if		(  fWindowPrefix  ) {
					xfree(Prefix);
				}
			}
		}
	}
	if		(	(  fLDR     )
			||	(  fLDW     ) ) {
		PutLevel(1);
		PutName("blocks");
		PutTab(8);
		printf("PIC S9(9)   BINARY  VALUE   %d.\n",num);
	}
dbgmsg("<MakeLD");
}

static	void
MakeLinkage(void)
{
	LD_Struct	*ld;
	char	*_prefix;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,LD_Name,"",TRUE);
	if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
		Error("LD not found.\n");
	}

	_prefix = Prefix;
	Prefix = "";
	PutLevel(1);
	PutName("linkarea.\n");
	PutLevel(2);
	PutName("linkdata-redefine.\n");
	PutLevel(3);
	PutName("filler");
	printf("      PIC X(%d).\n",ThisEnv->linksize);

	PutLevel(2);
	PutName(ld->name);
	PutTab(4);
	printf("REDEFINES   ");
	PutName("linkdata-redefine");
	Prefix = _prefix;
	level = 3;
	COBOL(ld->linkrec,
		  ld->arraysize,
		  ld->textsize);
	printf(".\n");
}

static	void
MakeDB(void)
{
	size_t	msize
	,		size;
	int		i;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,TRUE);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
	}

	PutLevel(1);
	PutName("dbarea");
	printf(".\n");
	msize = 0;
	for	( i = 0 ; i < cDB ; i ++ ) {
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		msize = ( msize > size ) ? msize : size;
	}

	PutLevel(2);
	PutName("dbdata");
	PutTab(12);
	printf("PIC X(%d).\n",msize);
}

static	void
MakeDBREC(
	char	*name)
{
	RecordStruct	*rec;
	size_t	msize
	,		size;
	int		i;
	char	*_prefix;
	char	*rname;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,TRUE);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
	}

	msize = 0;
	for	( i = 0 ; i < cDB ; i ++ ) {
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		msize = ( msize > size ) ? msize : size;
	}
	if		(  ( rec = DD_ParserDataDefines(name) )  !=  NULL  ) {
		level = 1;
		PutLevel(level);
		if		(  *RecName  ==  0  ) {
			rname = rec->name;
		} else {
			rname = RecName;
		}
		_prefix = Prefix;
		Prefix = "";
		PutName(rname);
		Prefix = _prefix;
		COBOL(rec->rec,arraysize,textsize);
		printf(".\n");

		size = SizeValue(rec->rec,arraysize,textsize);
		if		(  msize  !=  size  ) {
			PutLevel(2);
			PutName("filler");
			PutTab(12);
			printf("PIC X(%d).\n",msize - size);
		}
	}
}

static	void
MakeDBINFO(void)
{
	int		i;
	char	buff[SIZE_NAME+1];
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,TRUE);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
	}

	PutLevel(1);
	PutName("dbinfo");
	printf(".\n");
	PutLevel(2);
	PutName("dbinfo-num");
	PutTab(12);
	printf("PIC S9(9)   BINARY  VALUE   %d.\n",cDB);
	PutLevel(2);
	PutName("dbinfo-set.\n");
	for	( i = 0 ; i < cDB ; i ++ ) {
		PutLevel(3);
		sprintf(buff,"dbinfo-%d",i+1);
		PutName(buff);
		printf(".\n");

		PutLevel(4);
		PutName("filler");
		PutTab(12);
		printf("PIC X(64)\n");
		PutB(4);
		printf("VALUE '%s'.\n",dbrec[i]->name);

		PutLevel(4);
		PutName("filler");
		PutTab(12);
		printf("PIC S9(9)   BINARY\n");
		PutB(4);
		printf("VALUE %d.\n",SizeValue(dbrec[i]->rec,arraysize,textsize));

		PutLevel(4);
		PutName("filler");
		PutTab(12);
		printf("PIC S9(9)   BINARY\n");
		PutB(4);
		printf("VALUE %d.\n",( SizeValue(dbrec[i]->rec,arraysize,textsize) / SIZE_BLOCK + 1));
	}
	PutLevel(2);
	PutName("dbinfo-red");
	PutTab(4);
	printf("REDEFINES   ");
	PutName("dbinfo-set");
	printf(".\n");
	PutLevel(3);
	PutName("dbinfo-item");
	PutTab(4);
	printf("OCCURS  %d.\n",cDB);
	
	PutLevel(4);
	PutName("dbinfo-name");
	PutTab(16);
	printf("PIC X(64).\n");

	PutLevel(4);
	PutName("dbinfo-size");
	PutTab(16);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(4);
	PutName("dbinfo-blocks");
	PutTab(16);
	printf("PIC S9(9)   BINARY.\n");
}

static	void
MakeDBCOMM(void)
{
	size_t	msize
	,		size
	,		num;
	int		i;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,TRUE);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		}
		dbrec = ld->db;
		arraysize = ld->arraysize;
		textsize = ld->textsize;
		cDB = ld->cDB;
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		}
		dbrec = bd->db;
		arraysize = bd->arraysize;
		textsize = bd->textsize;
		cDB = bd->cDB;
	} else {
		Error("LD or BD not specified");
	}

	msize = 0;
	for	( i = 0 ; i < cDB ; i ++ ) {
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		msize = ( msize > size ) ? msize : size;
	}

	PutLevel(1);
	PutName("dbcomm");
	printf(".\n");

	num = ( ( msize + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;

	PutLevel(2);
	PutName("dbcomm-blocks");
	printf(".\n");

	PutLevel(3);
	PutName("dbcomm-block");
	PutTab(12);
	printf("OCCURS  %d.\n",num);

	PutLevel(4);
	PutName("filler");
	PutTab(12);
	printf("PIC X(%d).\n",SIZE_BLOCK);

	PutLevel(2);
	PutName("dbcomm-data");
	PutTab(4);
	printf("REDEFINES   ");
	PutName("dbcomm-blocks");
	printf(".\n");

	PutLevel(3);
	PutName("dbcomm-ctrl");
	printf(".\n");

	PutLevel(4);
	PutName("dbcomm-func");
	PutTab(12);
	printf("PIC X(16).\n");

	PutLevel(4);
	PutName("dbcomm-rc");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(4);
	PutName("dbcomm-path");
	printf(".\n");

	PutLevel(5);
	PutName("dbcomm-path-blocks");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(5);
	PutName("dbcomm-path-rname");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(5);
	PutName("dbcomm-path-pname");
	PutTab(12);
	printf("PIC S9(9)   BINARY.\n");

	PutLevel(3);
	PutName("dbcomm-record");
	printf(".\n");

	PutLevel(4);
	PutName("filler");
	PutTab(12);
	printf("PIC X(%d).\n",msize);
}

static	void
MakeDBPATH(void)
{
	int		i
	,		j
	,		blocks;
	size_t	size;
	char	buff[SIZE_NAME*2+1];
	DB_Struct	*db;
	LD_Struct	*ld;
	BD_Struct	*bd;
	RecordStruct	**dbrec;
	size_t		arraysize
	,			textsize;
	size_t	cDB;

	InitDirectory(TRUE);
	SetUpDirectory(Directory,NULL,NULL,TRUE);
	if		(  LD_Name  !=  NULL  ) {
		if		(  ( ld = GetLD(LD_Name) )  ==  NULL  ) {
			Error("LD not found.\n");
		} else {
			dbrec = ld->db;
			arraysize = ld->arraysize;
			textsize = ld->textsize;
			cDB = ld->cDB;
		}
	} else
	if		(  BD_Name  !=  NULL  ) {
		if		(  ( bd = GetBD(BD_Name) )  ==  NULL  ) {
			Error("BD not found.\n");
		} else {
			dbrec = bd->db;
			arraysize = bd->arraysize;
			textsize = bd->textsize;
			cDB = bd->cDB;
		}
	} else {
		Error("LD or BD not specified");
	}


	PutLevel(1);
	PutName("dbpath");
	printf(".\n");
	for	( i = 0 ; i < cDB ; i ++ ) {
		db = dbrec[i]->opt.db;
		size = SizeValue(dbrec[i]->rec,arraysize,textsize);
		blocks = ( ( size + sizeof(DBCOMM_CTRL) ) / SIZE_BLOCK ) + 1;
		
		for	( j = 0 ; j < db->pcount ; j ++ ) {
			PutLevel(2);
			sprintf(buff,"path-%s-%s",dbrec[i]->name,db->path[j]->name);
			PutName(buff);
			printf(".\n");

			PutLevel(3);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",blocks);

			PutLevel(3);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",i);

			PutLevel(3);
			PutName("filler");
			PutTab(12);
			printf("PIC S9(9)   BINARY  VALUE %d.\n",j);
		}
	}
}

static	ARG_TABLE	option[] = {
	{	"ldw",	BOOLEAN,	TRUE,		(void*)&fLDW,
		"$B@)8f%U%#!<%k%I0J30$r(BFILLER$B$K$9$k(B"				},
	{	"ldr",	BOOLEAN,	TRUE,		(void*)&fLDR,
		"$B@)8f%U%#!<%k%I$H(BSPA$B0J30$r(BFILLER$B$K$9$k(B"			},
	{	"filler",	BOOLEAN,	TRUE,	(void*)&fFiller,
		"$B%l%3!<%I$NCf?H$r(BFILLER$B$K$9$k(B"					},
	{	"spa",		BOOLEAN,	TRUE,	(void*)&fSPA,
		"SPA$BNN0h$r=PNO$9$k(B"								},
	{	"ctrl",		BOOLEAN,	TRUE,	(void*)&fCTRL,
		"$B@)8fNN0h$r=PNO$9$k(B"							},
	{	"linkage",	BOOLEAN,	TRUE,	(void*)&fLinkage,
		"$BO"MmNN0h$r=PNO$9$k(B"							},
	{	"screen",	BOOLEAN,	TRUE,	(void*)&fScreen,
		"$B2hLL%l%3!<%INN0h$r=PNO$9$k(B"					},
	{	"db",		BOOLEAN,	TRUE,	(void*)&fDB,
		"MCPDBSUB$BMQNN0h$r=PNO$9$k(B"						},
	{	"dbinfo",	BOOLEAN,	TRUE,	(void*)&fDBINFO,
		"MCPDBSUB$BMQ4IM}NN0h$r=PNO$9$k(B"					},
	{	"dbrec",	BOOLEAN,	TRUE,	(void*)&fDBREC,
		"MCPDBSUB$B$N0z?t$K;H$&%l%3!<%INN0h$r=PNO$9$k(B"	},
	{	"dbcomm",	BOOLEAN,	TRUE,	(void*)&fDBCOMM,
		"MCPDBSUB$B$H(BDB$B%5!<%P$H$NDL?.NN0h$r=PNO$9$k(B"		},
	{	"dbpath",	BOOLEAN,	TRUE,	(void*)&fDBPATH,
		"DB$B$N%Q%9L>%F!<%V%k$r=PNO$9$k(B"					},
	{	"textsize",	INTEGER,	TRUE,	(void*)&TextSize,
		"text$B$N:GBgD9(B"									},
	{	"arraysize",INTEGER,	TRUE,	(void*)&ArraySize,
		"$B2DJQMWAGG[Ns$N:GBg7+$jJV$7?t(B"					},
	{	"prefix",	STRING,		TRUE,	(void*)&Prefix,
		"$B9`L\L>$NA0$KIU2C$9$kJ8;zNs(B"					},
	{	"wprefix",	BOOLEAN,	TRUE,	(void*)&fWindowPrefix,
		"$B2hLL%l%3!<%I$N9`L\$NA0$K%&%#%s%I%&L>$rIU2C$9$k(B"},
	{	"name",		STRING,		TRUE,	(void*)&RecName,
		"$B%l%3!<%I$NL>A0(B"								},
	{	"dir",		STRING,		TRUE,	(void*)&Directory,
		"$B%G%#%l%/%H%j%U%!%$%k(B"	 						},
	{	"record",	STRING,		TRUE,	(void*)&RecordDir,
		"$B%l%3!<%I$N$"$k%G%#%l%/%H%j(B"					},
	{	"ld",		STRING,		TRUE,	(void*)&LD_Name,
		"LD$BL>(B"						 					},
	{	"bd",		STRING,		TRUE,	(void*)&BD_Name,
		"BD$BL>(B"						 					},
	{	"lddir",	STRING,		TRUE,	(void*)&LD_Dir,
		"LD$BDj5A3JG<%G%#%l%/%H%j(B"	 					},
	{	"bddir",	STRING,		TRUE,	(void*)&BD_Dir,
		"BD$BDj5A3JG<%G%#%l%/%H%j(B"	 					},
	{	NULL,		0,			FALSE,	NULL,	NULL 	}
};

static	void
SetDefault(void)
{
	fNoConv = FALSE;
	fFiller = FALSE;
	fSPA = FALSE;
	fCTRL = FALSE;
	fLinkage = FALSE;
	fScreen = FALSE;
	fDB = FALSE;
	fDBREC = FALSE;
	fDBINFO = FALSE;
	fDBCOMM = FALSE;
	fDBPATH = FALSE;
	ArraySize = -1;
	TextSize = -1;
	Prefix = "";
	RecName = "";
	LD_Name = NULL;
	BD_Name = NULL;
	fLDW = FALSE;
	fLDR = FALSE;
	fWindowPrefix = FALSE;
	RecordDir = ".";
	LD_Dir = NULL;
	BD_Dir = NULL;
	Directory = "./directory";
}

extern	int
main(
	int		argc,
	char	**argv)
{
	FILE_LIST	*fl;
	char		*name
	,			*ext;

	SetDefault();
	fl = GetOption(option,argc,argv);
	InitMessage();

	if		(  fl  !=  NULL  ) {
		name = fl->name;
		ext = GetExt(name);
		if		(  fDBREC  ) {
			MakeDBREC(name);
		} else
		if		(	(  !stricmp(ext,".rec")  )
				||	(  !stricmp(ext,".db")  ) ) {
			MakeFromRecord(name);
		}
	} else {
		if		(  fScreen  ) {
			if		(  LD_Name  ==  NULL  ) {
				PutLevel(1);
				PutString("scrarea");
				printf(".\n");
				PutLevel(2);
				PutString("screendata");
				printf(".\n");
			} else {
				MakeLD();
			}
		} else
		if		(  fLinkage  ) {
			MakeLinkage();
		} else
		if		(  fDB  ) {
			MakeDB();
		} else
		if		(  fDBINFO  ) {
			MakeDBINFO();
		} else
		if		(  fDBCOMM  ) {
			MakeDBCOMM();
		} else
		if		(  fDBPATH  ) {
			MakeDBPATH();
		} else
		if		(  fLDW  ) {
			fCTRL = TRUE;
			fSPA = TRUE;
			fLinkage = TRUE;
			fScreen = TRUE;
			RecName = "ldw";
			Prefix = "ldw-";
			MakeLD();
		} else
		if		(  fLDR  ) {
			fCTRL = TRUE;
			fSPA = TRUE;
			fLinkage = TRUE;
			fScreen = TRUE;
			RecName = "ldr";
			Prefix = "ldr-";
			MakeLD();
		}
	}
	return	(0);
}
