#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <json.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>
#include <libmondai.h>

#include "gettext.h"
#include "bd_config.h"
#include "desktop.h"
#include "notify.h"
#include "protocol.h"
#include "download.h"
#include "print.h"
#include "tempdir.h"
#include "logger.h"
#include "utils.h"

static void
_Print()
{
	char *ConfigNum,*Title,*Printer,*ShowDialog,*oid;
	int n;
	gboolean show;
	LargeByteString *lbs;

	ConfigNum = getenv("GLPUSH_CONFIG_NUMBER");
	if (ConfigNum == NULL) {
		n = 0;
	} else {
		n = atoi(ConfigNum);
	}

	Title = getenv("GLPUSH_TITLE");
	if (Title == NULL) {
		Title = "";
	}

	Printer    = getenv("GLPUSH_PRINTER");
	ShowDialog = getenv("GLPUSH_SHOW_DIALOG");
	if (ShowDialog != NULL && !strcmp("T",ShowDialog)) {
		show = TRUE;
	} else {
		show = FALSE;
	}
	if (Printer == NULL || strlen(Printer) <= 0 ) {
		show = TRUE;
	}
	oid = getenv("GLPUSH_OID");

	lbs = REST_GetBLOB_via_ENV();

	if (show) {
		ShowPrintDialog(Title,lbs);
	} else {
		Print(oid,Title,Printer,lbs);
	}
	FreeLBS(lbs);
}

static void
_Download()
{
	char *Filename,*Desc;
	LargeByteString *lbs;

	Filename = getenv("GLPUSH_FILENAME");
	if (Filename == NULL) {
		Filename = "";
	}
	Desc = getenv("GLPUSH_DESCRIPTION");
	if (Desc == NULL) {
		Desc = "";
	}
	lbs = REST_GetBLOB_via_ENV();
	ShowDownloadDialog(Filename,Desc,lbs);
	FreeLBS(lbs);
}

int
main(
	int argc,
	char **argv)
{
	char *LogFile,*TempDir,*Action;

	gtk_init(&argc,&argv);
	gtk_panda_init(&argc,&argv);
	gtk_set_locale();

	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	LogFile = getenv("GLPUSH_LOGFILE");
	if (LogFile == NULL) {
		InitLogger("glprint");
	} else {
		InitLogger_via_FileName(LogFile);
	}
	TempDir = getenv("GLPUSH_TEMPDIR");
	if (TempDir == NULL) {
		InitTempDir("glprint");
	} else {
		InitTempDir_via_Dir(TempDir);
	}

	Action = getenv("GLPUSH_ACTION");
	if (Action == NULL) {
		_Error("need GLPUSH_ACTION");
	}

	gl_config_init();
	InitDesktop();

	if (!strcmp(Action,"print")) {
		_Print();
	} else if (!strcmp(Action,"download")) {
		_Download();
	}

	return 0;
}