/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2008 Kouji TAKAO & JMA (Japan Medical Association).
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gtkpanda/gtkpanda.h>

#include "gettext.h"
#include "glclient.h"
#include "const.h"
#include "bd_config.h"
#include "bd_component.h"
#include "logger.h"

#define PRINTER_CONFIG_SIZE (10)
#define MAX_COPIES (99)

void open_file_chooser(GtkWidget *w, gpointer entry) {
  GtkWidget *dialog;
  gchar *default_path;

  dialog = gtk_file_chooser_dialog_new(
      _("Open File"), NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
      GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
  default_path = (gchar *)gtk_entry_get_text((GtkEntry *)entry);
  if (default_path != NULL) {
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), default_path);
  }
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename;

    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_entry_set_text(GTK_ENTRY(entry), filename);
    g_free(filename);
  }

  gtk_widget_destroy(dialog);
}

void open_dir_chooser(GtkWidget *w, gpointer entry) {
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new(
      _("Open Directory"), NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
      GTK_RESPONSE_ACCEPT, NULL);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename;

    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    gtk_entry_set_text(GTK_ENTRY(entry), filename);
    g_free(filename);
  }

  gtk_widget_destroy(dialog);
}

static void on_pkcs11_toggle(GtkWidget *widget, BDComponent *self) {
  gboolean sensitive;

  sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->pkcs11));
  gtk_widget_set_sensitive(self->pkcs11_container, sensitive);
}

static void on_timer_toggle(GtkWidget *widget, BDComponent *self) {
  gboolean sensitive;

  sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->timer));
  gtk_widget_set_sensitive(self->timer_container, sensitive);
}

/*********************************************************************
 * boot dialog component
 ********************************************************************/
void bd_component_set_value(BDComponent *self, int n) {
  gchar buf[256];
  gboolean save;

  // basic
  gtk_entry_set_text(GTK_ENTRY(self->authuri),
                     gl_config_get_string(n, "authuri"));
  gtk_entry_set_text(GTK_ENTRY(self->user), gl_config_get_string(n, "user"));

  save = gl_config_get_boolean(n, "savepassword");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->savepassword), save);
  if (save) {
    gtk_entry_set_text(GTK_ENTRY(self->password),
                       gl_config_get_string(n, "password"));
  } else {
    gtk_entry_set_text(GTK_ENTRY(self->password), "");
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->sso),
                               gl_config_get_boolean(n, "sso"));

  // ssl
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->ssl),
                               gl_config_get_boolean(n, "ssl"));
  gtk_entry_set_text(GTK_ENTRY(self->cafile),
                     gl_config_get_string(n, "cafile"));
  gtk_entry_set_text(GTK_ENTRY(self->certfile),
                     gl_config_get_string(n, "certfile"));
  gtk_entry_set_text(GTK_ENTRY(self->certkeyfile),
                     gl_config_get_string(n, "certkeyfile"));
  gtk_entry_set_text(GTK_ENTRY(self->ciphers),
                     gl_config_get_string(n, "ciphers"));
  save = gl_config_get_boolean(n, "savecertpassword");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->savecertpass), save);
  if (save) {
    gtk_entry_set_text(GTK_ENTRY(self->certpass),
                       gl_config_get_string(n, "certpassword"));
  } else {
    gtk_entry_set_text(GTK_ENTRY(self->certpass), "");
  }
  // pkcs11
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->pkcs11),
                               gl_config_get_boolean(n, "pkcs11"));
  gtk_widget_set_sensitive(self->pkcs11_container,
                           gl_config_get_boolean(n, "pkcs11"));
  gtk_entry_set_text(GTK_ENTRY(self->pkcs11lib),
                     gl_config_get_string(n, "pkcs11lib"));
  // printer
  {
    int i, j;
    gchar **entries, **kv, *str;
    GtkEntry *entry;
    GtkComboBox *combo;
    GtkTreeModel *model;
    GtkTreeIter iter;

    for (i = 0; i < PRINTER_CONFIG_SIZE; i++) {
      entry = GTK_ENTRY(g_list_nth_data(self->printer_entry_list, i));
      gtk_entry_set_text(entry, "");
      combo = GTK_COMBO_BOX(g_list_nth_data(self->printer_combo_list, i));
      gtk_combo_box_set_active(combo, 0);
    }

    entries = g_strsplit(gl_config_get_string(n, "printer_config"), ",", -1);
    for (i = 0; entries[i] != NULL && i < PRINTER_CONFIG_SIZE; i++) {
      kv = g_strsplit(entries[i], ":=:", -1);
      if (kv[0] != NULL && kv[1] != NULL) {
        entry = GTK_ENTRY(g_list_nth_data(self->printer_entry_list, i));
        combo = GTK_COMBO_BOX(g_list_nth_data(self->printer_combo_list, i));
        model = gtk_combo_box_get_model(combo);
        gtk_combo_box_set_active(combo, 0);
        gtk_entry_set_text(entry, kv[0]);
        if (gtk_tree_model_get_iter_first(model, &iter)) {
          j = 0;
          do {
            gtk_tree_model_get(model, &iter, 0, &str, -1);
            if (!strcmp(str, kv[1])) {
              gtk_combo_box_set_active(combo, j);
            }
            g_free(str);
            j++;
          } while (gtk_tree_model_iter_next(model, &iter));
        }
      }
      g_strfreev(kv);
    }
    g_strfreev(entries);
  }

  // other
  gtk_entry_set_text(GTK_ENTRY(self->style), gl_config_get_string(n, "style"));
  gtk_entry_set_text(GTK_ENTRY(self->gtkrc), gl_config_get_string(n, "gtkrc"));
  gtk_font_button_set_font_name(GTK_FONT_BUTTON(self->fontbutton),
                                gl_config_get_string(n, "fontname"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->debug),
                               gl_config_get_boolean(n, "debug"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->keybuff),
                               gl_config_get_boolean(n, "keybuff"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->timer),
                               gl_config_get_boolean(n, "timer"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->imkanaoff),
                               gl_config_get_boolean(n, "im_kana_off"));

  memset(buf, 0, sizeof(buf));
  snprintf(buf, sizeof(buf) - 1, "%d", gl_config_get_int(n, "timerperiod"));
  gtk_entry_set_text(GTK_ENTRY(self->timerperiod), buf);

  gtk_widget_set_sensitive(self->timer_container,
                           gl_config_get_boolean(n, "timer"));

  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(self->startup_message),
      gl_config_get_boolean_val(n, "show_startup_message", TRUE));
}

void bd_component_value_to_config(BDComponent *self, int n) {
  const gchar *password, *certpassword, *uri;
  gchar *newuri;
  gboolean save;

  // basic
  uri = gtk_entry_get_text(GTK_ENTRY(self->authuri));
  if (g_regex_match_simple("/$", uri, G_REGEX_CASELESS, 0)) {
    newuri = g_strdup(uri);
  } else {
    newuri = g_strdup_printf("%s/", uri);
  }
  gl_config_set_string(n, "authuri", newuri);
  g_free(newuri);
  gl_config_set_boolean(
      n, "sso", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->sso)));

  gl_config_set_string(n, "user", gtk_entry_get_text(GTK_ENTRY(self->user)));
  password = gtk_entry_get_text(GTK_ENTRY(self->password));
  save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->savepassword));
  if (save) {
    gl_config_set_string(n, "password", password);
  } else {
    gl_config_set_string(n, "password", "");
    Pass = g_strdup(password);
  }
  gl_config_set_boolean(n, "savepassword", save);

  // ssl
  gl_config_set_boolean(
      n, "ssl", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->ssl)));
  gl_config_set_string(n, "cafile",
                       gtk_entry_get_text(GTK_ENTRY(self->cafile)));
  gl_config_set_string(n, "certfile",
                       gtk_entry_get_text(GTK_ENTRY(self->certfile)));
  gl_config_set_string(n, "certkeyfile",
                       gtk_entry_get_text(GTK_ENTRY(self->certkeyfile)));
  gl_config_set_string(n, "ciphers",
                       gtk_entry_get_text(GTK_ENTRY(self->ciphers)));
  certpassword = gtk_entry_get_text(GTK_ENTRY(self->certpass));
  save = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->savecertpass));
  if (save) {
    gl_config_set_string(n, "certpassword", certpassword);
  } else {
    gl_config_set_string(n, "certpassword", "");
    CertPass = g_strdup(certpassword);
  }
  gl_config_set_boolean(n, "savecertpassword", save);
  // pkcs11
  gl_config_set_boolean(
      n, "pkcs11",
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->pkcs11)));
  gl_config_set_string(n, "pkcs11lib",
                       gtk_entry_get_text(GTK_ENTRY(self->pkcs11lib)));

  // printer
  {
    GRegex *re;
    GMatchInfo *match;
    GtkEntry *entry;
    GtkComboBox *combo;
    gchar *k, *v, *pname, *strcp, *ret;
    GString *gstr;
    int i, cp;

    re = g_regex_new("^(.*)#(\\d+)$", 0, 0, NULL);
    gstr = g_string_new(NULL);
    for (i = 0; i < PRINTER_CONFIG_SIZE; i++) {
      entry = GTK_ENTRY(g_list_nth_data(self->printer_entry_list, i));
      k = (gchar *)gtk_entry_get_text(entry);
      if (k != NULL && strlen(k) > 0) {
        combo = GTK_COMBO_BOX(g_list_nth_data(self->printer_combo_list, i));
        v = gtk_combo_box_get_active_text(combo);
        if (i > 0) {
          g_string_append_printf(gstr, ",");
        }
        if (g_regex_match(re, k, 0, &match)) {
          pname = g_match_info_fetch(match, 1);
          strcp = g_match_info_fetch(match, 2);
          cp = atoi(strcp);
          if (cp > MAX_COPIES) {
            Warning("max copies limit; printer %s set #%d", pname, MAX_COPIES);
            cp = MAX_COPIES;
          }
          g_string_append_printf(gstr, "%s#%d:=:%s", pname, cp, v);
          g_free(pname);
          g_free(strcp);
          g_match_info_free(match);
        } else {
          g_string_append_printf(gstr, "%s:=:%s", k, v);
        }
        if (v != NULL) {
          g_free(v);
        }
      }
    }
    ret = g_string_free(gstr, FALSE);
    gl_config_set_string(n, "printer_config", ret);
    g_free(ret);
    g_regex_unref(re);
  }

  // other
  gl_config_set_string(n, "style", gtk_entry_get_text(GTK_ENTRY(self->style)));
  gl_config_set_string(n, "gtkrc", gtk_entry_get_text(GTK_ENTRY(self->gtkrc)));
  gl_config_set_string(
      n, "fontname",
      gtk_font_button_get_font_name(GTK_FONT_BUTTON(self->fontbutton)));
  gl_config_set_boolean(
      n, "debug", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->debug)));
  gl_config_set_boolean(
      n, "keybuff",
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->keybuff)));
  gl_config_set_boolean(
      n, "im_kana_off",
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->imkanaoff)));
  gl_config_set_boolean(
      n, "timer", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->timer)));
  gl_config_set_int(n, "timerperiod",
                    atoi(gtk_entry_get_text(GTK_ENTRY(self->timerperiod))));
  gl_config_set_boolean(
      n, "show_startup_message",
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->startup_message)));
}

BDComponent *bd_component_new() {
  BDComponent *self;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *alignment;
  GtkWidget *check;
  char buff[256];
  gint ypos;

  self = g_new0(BDComponent, 1);

  // basic
  table = gtk_table_new(2, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  self->basictable = table;

  ypos = 0;

  label = gtk_label_new(_("AuthURI"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->authuri = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new(_("User"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->user = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new(_("Password"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->password = entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Remember Password"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  self->savepassword = check;
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Use Single Sign On"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  self->sso = check;

  // ssl
  table = gtk_table_new(3, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  self->ssltable = table;

  ypos = 0;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Use SSL Client Verification"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  self->ssl = check;
  ypos++;

  table = gtk_table_new(3, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  self->ssl_container = table;
  gtk_table_attach(GTK_TABLE(self->ssltable), table, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos = 0;

  label = gtk_label_new(_("CA Certificate File"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->cafile = entry = gtk_entry_new();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(open_file_chooser),
                   (gpointer)entry);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new(_("Certificate(*.crt)"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->certfile = entry = gtk_entry_new();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(open_file_chooser),
                   (gpointer)entry);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new(_("CertificateKey(*.pem)"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->certkeyfile = entry = gtk_entry_new();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(open_file_chooser),
                   (gpointer)entry);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new(_("CertificateKey Passphrase"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->certpass = entry = gtk_entry_new();
  gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check =
      gtk_check_button_new_with_label(_("Remember CertificateKey Passphrase"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  self->savecertpass = check;
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new(_("Cipher Suite"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->ciphers = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  // pkcs11
  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Use Security Device"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  g_signal_connect(G_OBJECT(check), "clicked", G_CALLBACK(on_pkcs11_toggle),
                   self);
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  self->pkcs11 = check;
  ypos++;

  table = gtk_table_new(3, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 0);
  self->pkcs11_container = table;
  gtk_table_attach(GTK_TABLE(self->ssl_container), table, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  ypos = 0;

  label = gtk_label_new(_("PKCS#11 Library"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->pkcs11lib = entry = gtk_entry_new();
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(open_file_chooser),
                   (gpointer)entry);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  ypos++;

  // printer
  table = gtk_table_new(11, 2, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 0);
  self->printertable = table;

  ypos = 0;

  label = gtk_label_new(_("Printer Name"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  label = gtk_label_new(_("Actual Printer"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  {
    int i, j;
    GList *list;
    GtkWidget *combo;

    self->printer_entry_list = NULL;
    self->printer_combo_list = NULL;
    list = gtk_panda_pdf_get_printer_list();
    for (i = 0; i < PRINTER_CONFIG_SIZE; i++) {
      entry = gtk_entry_new();
      combo = gtk_combo_box_new_text();
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "");
      for (j = 0; j < g_list_length(list); j++) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                                  (char *)g_list_nth_data(list, j));
      }
      self->printer_entry_list = g_list_append(self->printer_entry_list, entry);
      self->printer_combo_list = g_list_append(self->printer_combo_list, combo);

      gtk_table_attach(GTK_TABLE(table), entry, 0, 1, ypos, ypos + 1,
                       GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
      gtk_table_attach(GTK_TABLE(table), combo, 1, 2, ypos, ypos + 1,
                       GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
      ypos++;
    }
  }

  // other
  table = gtk_table_new(3, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  self->othertable = table;

  ypos = 0;

  label = gtk_label_new(_("Style"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  entry = gtk_entry_new();
  self->style = entry;
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(open_file_chooser),
                   (gpointer)entry);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new(_("Gtkrc"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  entry = gtk_entry_new();
  self->gtkrc = entry;
  button = gtk_button_new_with_label(_("Open"));
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(open_file_chooser),
                   (gpointer)entry);
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  ypos++;

  label = gtk_label_new(_("FontName"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  button = gtk_font_button_new();
  self->fontbutton = button;
  gtk_hbox_new(FALSE, 5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), button, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Debug"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  self->debug = check;
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Enable Keybuffer"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  self->keybuff = check;
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check =
      gtk_check_button_new_with_label(_("Disable IM control and Kana input"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  self->imkanaoff = check;
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Enable Timer"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  self->timer = check;
  g_signal_connect(G_OBJECT(check), "clicked", G_CALLBACK(on_timer_toggle),
                   self);
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  /* timer container */
  table = gtk_table_new(3, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 0);
  gtk_table_set_row_spacings(GTK_TABLE(table), 0);
  self->timer_container = table;
  gtk_table_attach(GTK_TABLE(self->othertable), table, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos = 0;

  label = gtk_label_new(_("Timer Period(ms)"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  self->timerperiod = entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  /* show startup message  */
  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  check = gtk_check_button_new_with_label(_("Show Startup Message"));
  gtk_container_add(GTK_CONTAINER(alignment), check);
  self->startup_message = check;
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  // info
  table = gtk_table_new(2, 1, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table), 5);
  gtk_table_set_row_spacings(GTK_TABLE(table), 4);
  self->infotable = table;

  ypos = 0;

  sprintf(buff, "glclient2 ver %s %s", PACKAGE_VERSION, PACKAGE_DATE);
  label = gtk_label_new(buff);
  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  gtk_container_add(GTK_CONTAINER(alignment), label);
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  label = gtk_label_new("Copyright (C) 2017 ORCA Project");
  alignment = gtk_alignment_new(0.5, 0.5, 0, 1);
  gtk_container_add(GTK_CONTAINER(alignment), label);
  gtk_table_attach(GTK_TABLE(table), alignment, 0, 2, ypos, ypos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  ypos++;

  return self;
}

/*************************************************************
 * Local Variables:
 * mode: c
 * c-set-style: gnu
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 ************************************************************/
