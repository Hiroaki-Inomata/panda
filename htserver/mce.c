/*	PANDA -- a simple transaction monitor

Copyright (C) 2004 Ogochan

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
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<glib.h>
#include	<time.h>
#include	<libxml/parser.h>
#include	<sys/mman.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<libxslt/xslt.h>
#include	<libxslt/xsltInternals.h>
#include	<libxslt/transform.h>
#include	<libxslt/xsltutils.h>
#include 	<libexslt/exslt.h>
#include	<libexslt/exsltconfig.h>

#include	"const.h"
#include	"libmondai.h"
#include	"types.h"
#include	"debug.h"

static	xsltStylesheetPtr	style = NULL;

extern	char	*
MceHTML2XML(
	char	*html)
{
	FILE	*fp;
	char	name[SIZE_LONGNAME+1];
	xmlDocPtr	doc
		,		res;
	char		*str
		,		*p;
	int		fd;
	struct	stat	sb;

ENTER_FUNC;
	sprintf(name,"/tmp/mcd2xml%d.xhtml",(int)getpid());
	if		(  ( fp = fopen(name,"w") )  ==  NULL  ) {
		fprintf(stderr,"tempfile can not make.: %s\n", name);
		exit(1);
	}
	fprintf(fp,
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
			"\"DTD/xhtml1-strict.dtd\">\n"
			"<html xmlns=\"http://www.w3.org/1999/xhtml\" "
			"xml:lang=\"ja\" lang=\"ja\">\n"
			"<body>%s\n</body>"
			"</html>\n",html);
	fclose(fp);
	doc = xmlParseFile(name);
	remove(name);
	//	xmlSaveFormatFileEnc("-",doc,"utf-8",0);

	if		(  style  ==  NULL  ) {
		xmlSubstituteEntitiesDefault(1);
		xmlLoadExtDtdDefaultValue = 1;
		exsltRegisterAll();
		style = xsltParseStylesheetFile("xhtml2sdoc.xsl");
	}
	res = xsltApplyStylesheet(style, doc, NULL);
	xmlFreeDoc(doc);

	sprintf(name,"/tmp/mcd2xml%d.sdoc",(int)getpid());
	xmlSaveFormatFileEnc(name,res,"utf-8",0);

	if		(  ( fd = open(name,O_RDONLY ) )  >=  0  ) {
		fstat(fd,&sb);
		if		(  S_ISREG(sb.st_mode)  ) {
			if		(  ( p = mmap(NULL,sb.st_size,PROT_READ,MAP_PRIVATE,fd,0) )  !=  NULL  ) {
				str = (char *)xmalloc(sb.st_size+1);
				memcpy(str,p,sb.st_size);
				str[sb.st_size] = 0;
				munmap(p,sb.st_size);
			}
		}
	} else {
		fprintf(stderr,"tempfile can not open.: %s\n", name);
		exit(1);
	}
	remove(name);

LEAVE_FUNC;
	return	(str);
}


#ifdef	MAIN
#include	<iconv.h>
static	char	*
ConvUTF8(
	unsigned char	*str,
	char	*code)
{
	char	*istr;
	iconv_t	cd;
	size_t	sib
	,		sob;
	char	*ostr;
	static	char	cbuff[SIZE_BUFF];

	cd = iconv_open("utf8",code);
	istr = str;
	if		(  ( sib = strlen(str)  )  >  0  ) {
		ostr = cbuff;
		sob = SIZE_BUFF;
		if		(  iconv(cd,&istr,&sib,&ostr,&sob)  !=  0  ) {
			dbgprintf("error = %d\n",errno);
		}
		*ostr = 0;
		iconv_close(cd);
	} else {
		*cbuff = 0;
	}

	return	(cbuff);
}

extern	int
main(
	int		argc,
	char	**argv)
{
	char	*html1
		,	*html2;

	html1 =
		"�ƥ��ȤǤ���<br />\n"
		"<strong>�����Υƥ��ȤǤ���<em>������å��ΤΥƥ��ȤǤ���"
		"<u>�����Ҥ��Ǥ���<br />\n"
		"<strike>�������Ǥ���<br />\n"
		"<br />\n"
		"</strike>����������</u>��������</em>��������</strong>��������<br />\n"
		"<br />\n"
		"<ul>\n"
		"<li>�վ��Ǥ���</li>\n"
		"<li>�վ��Ǥ���</li>\n"
		"</ul>\n"
		"<ol>\n"
		"<li>�ֹ��դ��Ǥ���</li>\n"
		"<li>�ֹ��դ��Ǥ���</li>\n"
		"</ol>\n"
		"<br />\n"
		"����Ϻ��ͤ�<br />\n"
		"<div align=\"center\">��������·��<br />\n"
		"<div align=\"right\">����ϱ��ͤ�<br />\n"
		"<br />\n"
		"<div align=\"left\">�����<a href=\"http://www.nurs.or.jp/~ogochan/\" "
		"target=\"_blank\">���</a>�Ǥ���<br />\n"
		"<br />\n"
		"����������Ƥߤޤ���<br />\n"
		"<img vspace=\"0\" hspace=\"0\" border=\"0\" "
		"src=\"http://www.nurs.or.jp/~ogochan/tatsujin_photo_2000_06.png\" "
		"alt=\"���������β���\" /><br />\n"
		"<br />\n"
		"<table width=\"273\" height=\"53\" border=\"1\"><tr><td>&#160;̾��</td>\n"
		"<td align=\"center\">ǯ��</td></tr><tr><td>&#160;���������</td>\n"
		"<td align=\"right\">41</td></tr></table><br />\n"
		"</div>\n"
		"</div>\n"
		"</div>\n";

	html2 =
		"�ƥ��ȤǤ���\n"
		"<strong>�����Υƥ��ȤǤ���<em>������å��ΤΥƥ��ȤǤ���<u>�����Ҥ��Ǥ���\n"
		"<strike>�������Ǥ���\n"
		"</strike>����������</u>��������</em>��������</strong>��������\n"
		"<ul>\n"
		"<li>�վ��Ǥ���</li>\n"
		"<li>�վ��Ǥ���</li>\n"
		"</ul>\n"
		"<ol>\n"
		"<li>�ֹ��դ��Ǥ���</li>\n"
		"<li>�ֹ��դ��Ǥ���</li>\n"
		"</ol>\n"
		"����Ϻ��ͤ�\n"
		"<div align=\"center\">��������·��\n"
		"<div align=\"right\">����ϱ��ͤ�\n"
		"<div align=\"left\">�����<a href=\"http://www.nurs.or.jp/~ogochan/\" "
		"target=\"_blank\">���</a>�Ǥ���\n"
		"����������Ƥߤޤ���\n"
		"<img vspace=\"0\" hspace=\"0\" border=\"0\" "
		"src=\"http://www.nurs.or.jp/~ogochan/tatsujin_photo_2000_06.png\" "
		"alt=\"���������β���\" />\n"
		"<table width=\"273\" height=\"53\" border=\"1\"><tr><td>&#160;̾��</td>"
		"<td align=\"center\">ǯ��</td></tr><tr><td>&#160;���������</td>"
		"<td align=\"right\">41</td></tr></table>\n"
		"</div>\n"
		"</div>\n"
		"</div>\n";

	printf("%s\n",MceHTML2XML(ConvUTF8(html1,"euc-jp")));
	return	(0);
}
#endif
