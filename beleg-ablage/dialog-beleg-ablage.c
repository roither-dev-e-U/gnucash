/*
 * dialog-beleg-ablage.c -- dialogs of the Beleg-Ablage module
 *
 * Copyright (C) 2026 Georg Roither, roither.dev e.U.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652
 * Boston, MA  02110-1301,  USA       gnu@gnu.org
 */

#include "dialog-beleg-ablage.h"

#include <glib/gstdio.h>
#include <string.h>

#include "gnc-engine.h"
#include "gnc-gnome-utils.h"          /* gnc_launch_doclink */
#include "gnc-main-window.h"
#include "qof.h"
#include "Transaction.h"

#include "beleg-ablage-kvp.h"
#include "beleg-ablage-prefs.h"
#include "beleg-ablage-util.h"

/* Custom dialog responses for the number-conflict dialog */
enum
{
    BELEG_RESPONSE_ASSIGN_ANYWAY = 1,
    BELEG_RESPONSE_UPDATE_NUM    = 2,
};

/* ------------------------------------------------------------ */
/* Small helpers                                                */
/* ------------------------------------------------------------ */

static void
beleg_error (GtkWindow *parent, const char *format, ...)
{
    va_list args;
    char *msg;
    GtkWidget *dialog;

    va_start (args, format);
    msg = g_strdup_vprintf (format, args);
    va_end (args);

    dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     "%s", msg);
    gtk_window_set_title (GTK_WINDOW (dialog), "Beleg-Ablage");
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_free (msg);
}

static gboolean
beleg_question (GtkWindow *parent, GtkMessageType type, const char *msg,
                const char *yes_label, const char *no_label)
{
    GtkWidget *dialog;
    gint response;
    gboolean yes;

    dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL, type,
                                     GTK_BUTTONS_NONE, "%s", msg);
    gtk_window_set_title (GTK_WINDOW (dialog), "Beleg-Ablage");
    gtk_dialog_add_button (GTK_DIALOG (dialog), yes_label, GTK_RESPONSE_YES);
    gtk_dialog_add_button (GTK_DIALOG (dialog), no_label, GTK_RESPONSE_NO);
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    yes = (response == GTK_RESPONSE_YES);
    return yes;
}

/* Open a PDF with the platform default application. */
static void
beleg_launch_file (GtkWindow *parent, const char *abs_path)
{
    gchar *uri = g_filename_to_uri (abs_path, NULL, NULL);

    if (!uri)
    {
        beleg_error (parent, "Ungültiger Dateipfad:\n%s", abs_path);
        return;
    }
    gnc_launch_doclink (parent, uri);
    g_free (uri);
}

/* ------------------------------------------------------------ */
/* Number scanning                                              */
/* ------------------------------------------------------------ */

typedef struct
{
    char yymm[5];
    int  max_seq;
} BelegSeqScan;

static void
beleg_scan_num_cb (gpointer obj, gpointer user_data)
{
    Transaction *trans = (Transaction *)obj;
    BelegSeqScan *scan = (BelegSeqScan *)user_data;
    const char *num = xaccTransGetNum (trans);
    char yymm[5];
    int seq;

    if (num && beleg_parse_num (num, yymm, &seq)
            && strcmp (yymm, scan->yymm) == 0
            && seq > scan->max_seq)
        scan->max_seq = seq;
}

/* Highest used running number for the "B<yymm>-" prefix in the book. */
static int
beleg_max_seq_in_book (QofBook *book, const char *yymm)
{
    BelegSeqScan scan = {{0, 0, 0, 0, 0}, 0};

    strncpy (scan.yymm, yymm, sizeof (scan.yymm) - 1);
    scan.yymm[sizeof (scan.yymm) - 1] = '\0';
    scan.max_seq = 0;

    qof_collection_foreach (qof_book_get_collection (book, GNC_ID_TRANS),
                            beleg_scan_num_cb, &scan);
    return scan.max_seq;
}

/* ------------------------------------------------------------ */
/* File lookup / renaming helpers                               */
/* ------------------------------------------------------------ */

/* Find an existing archive file that matches the given "Nr." value. */
static gchar *
beleg_find_file_for_num (const char *dir, const char *num)
{
    GDir *d;
    const char *name;
    gchar *result = NULL;

    if (!dir || !num || !*num)
        return NULL;

    d = g_dir_open (dir, 0, NULL);
    if (!d)
        return NULL;

    while ((name = g_dir_read_name (d)) != NULL)
    {
        if (beleg_filename_is_valid (name) && beleg_num_matches_filename (num, name))
        {
            result = g_build_filename (dir, name, (char *)NULL);
            break;
        }
    }
    g_dir_close (d);
    return result;
}

/* ------------------------------------------------------------ */
/* Assign                                                      */
/* ------------------------------------------------------------ */

void
beleg_dialog_assign (GtkWindow *parent, Transaction *trans)
{
    QofBook *book;
    GDate date;
    gchar *root = NULL;
    gchar *month_dir = NULL;
    gchar *yymm = NULL;
    gchar *abs_month_dir = NULL;
    const char *num = NULL;
    gchar *writeback_num = NULL;
    GtkFileChooserNative *chooser = NULL;
    GtkFileFilter *filter = NULL;
    gint response;
    gchar *sel_path = NULL;
    gchar *filename = NULL;
    gchar *file_num = NULL;
    gchar *rel = NULL;

    g_return_if_fail (parent != NULL);
    g_return_if_fail (trans != NULL);

    /* --- read-only checks ---------------------------------- */
    book = xaccTransGetBook (trans);
    if (qof_book_is_readonly (book))
    {
        beleg_error (parent, "Das Buch ist schreibgeschützt.\n"
                      "Belege können nicht zugeordnet werden.");
        return;
    }
    if (xaccTransGetReadOnly (trans))
    {
        beleg_error (parent, "Die Buchung ist schreibgeschützt:\n%s",
                      xaccTransGetReadOnly (trans));
        return;
    }
    if (xaccTransIsReadonlyByPostedDate (trans))
    {
        beleg_error (parent, "Das Buchungsdatum liegt vor der "
                      "automatischen Schreibschutz-Schwelle des Buchs.\n"
                      "Belege können nicht zugeordnet werden.");
        return;
    }

    /* --- archive root and month folder --------------------- */
    root = beleg_prefs_get_root ();
    if (!root)
    {
        beleg_error (parent,
                      "Kein Belegarchiv-Stammverzeichnis konfiguriert.\n\n"
                      "Bitte setzen Sie es einmalig mit:\n"
                      "  gsettings set dev.roither.belegablage belege-root "
                      "'/pfad/zum/belegarchiv'\n\n"
                      "Ohne eigenes Schema dient der GnuCash-Standardpfad für "
                      "Dokumentverknüpfungen als Fallback "
                      "(Bearbeiten → Einstellungen → Dokumente).");
        return;
    }

    date = xaccTransGetDatePostedGDate (trans);
    month_dir = beleg_month_dir_from_gdate (date);
    yymm = beleg_yymm_from_gdate (date);
    abs_month_dir = g_build_filename (root, "belege", month_dir, (char *)NULL);

    if (!g_file_test (abs_month_dir, G_FILE_TEST_IS_DIR))
    {
        char *msg = g_strdup_printf (
            "Der Monatsordner existiert noch nicht:\n%s\n\nAnlegen?",
            abs_month_dir);
        if (beleg_question (parent, GTK_MESSAGE_QUESTION, msg,
                            "_Anlegen", "_Abbrechen"))
        {
            if (g_mkdir_with_parents (abs_month_dir, 0755) != 0)
            {
                beleg_error (parent, "Ordner konnte nicht angelegt werden:\n%s",
                              abs_month_dir);
                g_free (msg);
                goto cleanup;
            }
        }
        else
        {
            g_free (msg);
            goto cleanup;
        }
        g_free (msg);
    }

    /* --- file chooser -------------------------------------- */
    num = xaccTransGetNum (trans);

    chooser = gtk_file_chooser_native_new ("Beleg zuordnen",
                                          parent,
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          "_Zuordnen", "_Abbrechen");
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "PDF-Belege");
    gtk_file_filter_add_pattern (filter, "*.pdf");
    gtk_file_filter_add_pattern (filter, "*.PDF");
    gtk_file_filter_add_mime_type (filter, "application/pdf");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
                                         abs_month_dir);

    /* auto-match: preselect an existing file for the current "Nr." */
    if (num && *num)
    {
        gchar *pre = beleg_find_file_for_num (abs_month_dir, num);
        if (pre)
        {
            gtk_file_chooser_select_filename (GTK_FILE_CHOOSER (chooser), pre);
            g_free (pre);
        }
    }

    response = gtk_native_dialog_run (GTK_NATIVE_DIALOG (chooser));
    if (response != GTK_RESPONSE_ACCEPT)
        goto cleanup;

    sel_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    if (!sel_path)
        goto cleanup;

    filename = g_path_get_basename (sel_path);

    /* --- file name convention ------------------------------ */
    if (!beleg_filename_is_valid (filename))
    {
        int next_seq = beleg_next_seq (beleg_max_seq_in_book (book, yymm));
        char *base = g_strdup (filename);
        char *ext = NULL;
        char *shortname = NULL;
        gchar *proposal = NULL;
        gchar *msg;
        gboolean do_rename;

        /* strip a trailing ".pdf" (case-insensitive) for the shortname */
        if (g_str_has_suffix (base, ".pdf") || g_str_has_suffix (base, ".PDF"))
        {
            ext = g_strrstr (base, ".");
            if (ext)
                *ext = '\0';
        }
        shortname = beleg_sanitize_shortname (base);
        proposal = beleg_build_filename (yymm, next_seq, shortname);
        g_free (base);

        msg = g_strdup_printf (
            "Der Dateiname entspricht nicht der Konvention\n"
            "B<YYMM>-<Nr>_<Kurzname>.pdf:\n\n%s\n\n"
            "Datei umbenennen zu:\n%s ?", filename, proposal);
        do_rename = beleg_question (parent, GTK_MESSAGE_QUESTION, msg,
                                    "_Umbenennen", "_Abbrechen");
        g_free (msg);

        if (!do_rename)
        {
            g_free (proposal);
            g_free (shortname);
            goto cleanup;
        }

        {
            gchar *dir = g_path_get_dirname (sel_path);
            gchar *new_path = g_build_filename (dir, proposal, (char *)NULL);
            g_free (dir);
            if (g_rename (sel_path, new_path) != 0)
            {
                beleg_error (parent, "Umbenennen fehlgeschlagen:\n%s → %s",
                              sel_path, new_path);
                g_free (new_path);
                g_free (proposal);
                g_free (shortname);
                goto cleanup;
            }
            g_free (sel_path);
            sel_path = new_path;
        }
        g_free (proposal);
        g_free (shortname);
        g_free (filename);
        filename = g_path_get_basename (sel_path);
    }

    /* --- number handling ----------------------------------- */
    file_num = beleg_num_from_filename (filename);

    if (!num || !*num)
    {
        /* Nr. is empty: suggest the number encoded in the file name */
        gchar *msg = g_strdup_printf (
            "Die Buchung hat keine Nummer im Feld „Nr.“.\n"
            "Belegnummer %s übernehmen?", file_num);
        gboolean take = beleg_question (parent, GTK_MESSAGE_QUESTION, msg,
                                        "_Übernehmen", "_Abbrechen");
        g_free (msg);
        if (!take)
            goto cleanup;
        writeback_num = g_strdup (file_num);
    }
    else if (g_strcmp0 (file_num, num) != 0)
    {
        GtkWidget *dialog = gtk_message_dialog_new (
            parent, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
            "Die Belegnummer der Datei (%s) weicht vom Feld „Nr.“ (%s) ab.\n\n"
            "Wie soll verfahren werden?",
            file_num ? file_num : "–", num);
        gtk_window_set_title (GTK_WINDOW (dialog), "Beleg-Ablage");
        gtk_dialog_add_button (GTK_DIALOG (dialog), "_Abbrechen",
                               GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (dialog), "Nummer _übernehmen",
                               BELEG_RESPONSE_UPDATE_NUM);
        gtk_dialog_add_button (GTK_DIALOG (dialog), "_Trotzdem zuordnen",
                               BELEG_RESPONSE_ASSIGN_ANYWAY);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        if (response == GTK_RESPONSE_CANCEL)
            goto cleanup;
        if (response == BELEG_RESPONSE_UPDATE_NUM)
            writeback_num = g_strdup (file_num);
    }

    /* --- store ---------------------------------------------- */
    rel = beleg_rel_path (month_dir, filename);
    if (!rel)
    {
        beleg_error (parent, "Relativer Pfad konnte nicht gebildet werden.");
        goto cleanup;
    }

    xaccTransBeginEdit (trans);
    if (writeback_num)
        xaccTransSetNum (trans, writeback_num);
    beleg_kvp_store (trans, rel, file_num);
    xaccTransCommitEdit (trans);

    /* --- summary + open ------------------------------------ */
    {
        gchar *abs = beleg_join_root (root, rel);
        gchar *msg = g_strdup_printf (
            "Beleg zugeordnet:\n\n%s\n\nBeleg jetzt öffnen?", rel);
        gboolean open_now = beleg_question (parent, GTK_MESSAGE_INFO, msg,
                                            "_Öffnen", "_Schließen");
        g_free (msg);
        if (open_now && abs)
            beleg_launch_file (parent, abs);
        g_free (abs);
    }

cleanup:
    g_free (rel);
    g_free (file_num);
    g_free (filename);
    g_free (sel_path);
    g_free (writeback_num);
    if (chooser)
        g_object_unref (chooser);
    g_free (abs_month_dir);
    g_free (yymm);
    g_free (month_dir);
    g_free (root);
}

/* ------------------------------------------------------------ */
/* Open                                                        */
/* ------------------------------------------------------------ */

void
beleg_dialog_open (GtkWindow *parent, Transaction *trans)
{
    gchar *root = NULL;
    gchar *rel = NULL;
    gchar *abs = NULL;

    g_return_if_fail (parent != NULL);
    g_return_if_fail (trans != NULL);

    rel = beleg_kvp_get_path (trans);
    if (!rel || !*rel)
    {
        const char *doclink = xaccTransGetDocLink (trans);
        if (doclink && *doclink)
            rel = beleg_normalize_rel_path (doclink);
        else
        {
            beleg_error (parent, "Dieser Buchung ist noch kein Beleg "
                          "zugeordnet.");
            g_free (rel);
            return;
        }
    }

    /* absolute URI (doclink) passes through unchanged */
    if (strstr (rel, "://"))
    {
        gnc_launch_doclink (parent, rel);
        g_free (rel);
        return;
    }

    root = beleg_prefs_get_root ();
    if (!root)
    {
        beleg_error (parent, "Kein Belegarchiv-Stammverzeichnis konfiguriert "
                      "(gsettings set dev.roither.belegablage belege-root …).");
        g_free (rel);
        return;
    }

    abs = beleg_join_root (root, rel);
    g_free (root);

    if (!abs || !g_file_test (abs, G_FILE_TEST_EXISTS))
    {
        beleg_error (parent, "Belegdatei nicht gefunden:\n%s",
                      abs ? abs : rel);
        g_free (abs);
        g_free (rel);
        return;
    }

    beleg_launch_file (parent, abs);
    g_free (abs);
    g_free (rel);
}

/* ------------------------------------------------------------ */
/* Info                                                        */
/* ------------------------------------------------------------ */

void
beleg_dialog_info (GtkWindow *parent, Transaction *trans)
{
    gchar *rel = NULL;
    gchar *id = NULL;
    gchar *root = NULL;
    gchar *abs = NULL;
    GtkWidget *dialog;
    GDate date;
    char date_buf[128] = "";

    g_return_if_fail (parent != NULL);
    g_return_if_fail (trans != NULL);

    rel = beleg_kvp_get_path (trans);
    id = beleg_kvp_get_id (trans);
    root = beleg_prefs_get_root ();
    if (rel)
        abs = beleg_join_root (root ? root : "", rel);

    date = xaccTransGetDatePostedGDate (trans);
    g_date_strftime (date_buf, sizeof (date_buf), "%d.%m.%Y", &date);

    {
        const char *num = xaccTransGetNum (trans);
        const char *doclink = xaccTransGetDocLink (trans);
        const char *base = rel ? (strrchr (rel, '/') ? strrchr (rel, '/') + 1
                                                     : rel) : "";
        gboolean file_ok = (abs && g_file_test (abs, G_FILE_TEST_EXISTS));
        gboolean name_ok = beleg_filename_is_valid (base);
        GString *text = g_string_new ("");

        g_string_append_printf (text, "Datum:  %s\n", date_buf);
        g_string_append_printf (text, "Nr.:  %s\n", (num && *num) ? num : "(leer)");
        g_string_append_printf (text, "Beleg-Nummer:  %s\n",
                                (id && *id) ? id : "(nicht gespeichert)");
        g_string_append_printf (text, "Relativer Pfad:  %s\n",
                                (rel && *rel) ? rel : "(kein Beleg zugeordnet)");
        g_string_append_printf (text, "Archiv-Stammverzeichnis:  %s\n",
                                (root && *root) ? root : "(nicht konfiguriert)");
        g_string_append_printf (text, "Datei vorhanden:  %s\n",
                                rel ? (file_ok ? "✓ ja" : "✗ nein")
                                    : "–");
        g_string_append_printf (text, "Namenskonvention:  %s\n",
                                rel ? (name_ok ? "✓ erfüllt" : "✗ verletzt")
                                    : "–");
        g_string_append_printf (text, "GnuCash-Doclink gesetzt:  %s\n",
                                (doclink && *doclink) ? "✓ ja" : "✗ nein");

        dialog = gtk_message_dialog_new (parent, GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
                                         "%s", text->str);
        gtk_window_set_title (GTK_WINDOW (dialog), "Beleg-Info");
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_string_free (text, TRUE);
    }

    g_free (abs);
    g_free (root);
    g_free (id);
    g_free (rel);
}

/* ------------------------------------------------------------ */
/* No transaction selected                                      */
/* ------------------------------------------------------------ */

void
beleg_dialog_no_transaction (GtkWindow *parent)
{
    beleg_error (parent, "Bitte zuerst eine bestehende Buchung im Register "
                  "auswählen.");
}

/* ================= END OF FILE ============================== */
