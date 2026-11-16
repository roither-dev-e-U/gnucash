/*
 * gnc-plugin-beleg-ablage.c -- GncPlugin subclass of the Beleg-Ablage module
 *
 * Registers the "Tools" menu entries for assigning, opening and
 * inspecting receipt documents (Belege) of the current register
 * transaction.
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

#include <config.h>

#include <gtk/gtk.h>
#include <string.h>

#include "gnc-plugin-beleg-ablage.h"
#include "gnc-plugin-manager.h"
#include "gnc-main-window.h"
#include "gnc-plugin-page.h"
#include "gnc-plugin-page-register.h"
#include "gnc-engine.h"
#include "dialog-beleg-ablage.h"

/* This static indicates the debugging module that this .o belongs to.  */
static QofLogModule log_module = G_LOG_DOMAIN;

#define PLUGIN_ACTIONS_NAME  "gnc-plugin-beleg-ablage-actions"
#define PLUGIN_UI_FILENAME   "gnc-plugin-beleg-ablage.ui"

static void gnc_plugin_beleg_ablage_finalize (GObject *object);

/* Command callbacks */
static void gnc_plugin_beleg_ablage_cmd_assign (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void gnc_plugin_beleg_ablage_cmd_open   (GSimpleAction *simple, GVariant *parameter, gpointer user_data);
static void gnc_plugin_beleg_ablage_cmd_info   (GSimpleAction *simple, GVariant *parameter, gpointer user_data);

static GActionEntry gnc_plugin_actions [] =
{
    { "BelegAssignAction", gnc_plugin_beleg_ablage_cmd_assign, NULL, NULL, NULL },
    { "BelegOpenAction",   gnc_plugin_beleg_ablage_cmd_open,   NULL, NULL, NULL },
    { "BelegInfoAction",   gnc_plugin_beleg_ablage_cmd_info,   NULL, NULL, NULL },
};
static guint gnc_plugin_n_actions = G_N_ELEMENTS (gnc_plugin_actions);

/* update_menu_model() in gnc-main-window.cpp requires ui_updates != NULL:
 * each entry names a <menu id="..."> section of our ui file that is
 * merged into the main window's menu model. */
static const gchar *gnc_plugin_load_ui_items [] =
{
    "ToolsPlaceholder0",
    NULL,
};

/************************************************************
 *              Object Implementation                       *
 ************************************************************/

struct _GncPluginBelegAblage
{
    GncPlugin gnc_plugin;
};

G_DEFINE_TYPE (GncPluginBelegAblage, gnc_plugin_beleg_ablage, GNC_TYPE_PLUGIN)

GncPlugin *
gnc_plugin_beleg_ablage_new (void)
{
    return GNC_PLUGIN (g_object_new (GNC_TYPE_PLUGIN_BELEG_ABLAGE, (gchar *)NULL));
}

static void
gnc_plugin_beleg_ablage_class_init (GncPluginBelegAblageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GncPluginClass *plugin_class = GNC_PLUGIN_CLASS (klass);

    object_class->finalize = gnc_plugin_beleg_ablage_finalize;

    /* plugin info */
    plugin_class->plugin_name  = GNC_PLUGIN_BELEG_ABLAGE_NAME;

    /* widget addition/removal */
    plugin_class->actions_name    = PLUGIN_ACTIONS_NAME;
    plugin_class->actions         = gnc_plugin_actions;
    plugin_class->n_actions       = gnc_plugin_n_actions;
    plugin_class->ui_filename     = PLUGIN_UI_FILENAME;
    plugin_class->ui_updates      = gnc_plugin_load_ui_items;
}

static void
gnc_plugin_beleg_ablage_init (GncPluginBelegAblage *plugin)
{
}

static void
gnc_plugin_beleg_ablage_finalize (GObject *object)
{
    G_OBJECT_CLASS (gnc_plugin_beleg_ablage_parent_class)->finalize (object);
}

/************************************************************
 *              Utility functions                           *
 ************************************************************/

/** Transaction of the cursor line in the register page currently
 *  active in @a window, or NULL when no register page / no usable
 *  transaction is selected. */
static Transaction *
beleg_ablage_get_current_txn (GncMainWindow *window)
{
    GncPluginPage *page;
    Transaction *trans = NULL;

    g_return_val_if_fail (window != NULL, NULL);

    page = gnc_main_window_get_current_page (window);
    if (!page || !GNC_IS_PLUGIN_PAGE_REGISTER (page))
        return NULL;

    trans = gnc_plugin_page_register_get_current_txn (
        GNC_PLUGIN_PAGE_REGISTER (page));
    return trans;
}

/* Idempotent self registration; called from every load path. */
void
beleg_ablage_self_register (void)
{
    static gsize registered = 0;

    if (g_once_init_enter (&registered))
    {
        GncPluginManager *manager = gnc_plugin_manager_get ();

        gnc_plugin_manager_add_plugin (
            manager, gnc_plugin_beleg_ablage_new ());

        g_once_init_leave (&registered, 1);
    }
}

/************************************************************
 *                    Command Callbacks                     *
 ************************************************************/

static void
gnc_plugin_beleg_ablage_cmd_assign (GSimpleAction *simple,
                                    GVariant *parameter,
                                    gpointer user_data)
{
    GncMainWindowActionData *mw = user_data;
    Transaction *trans;

    g_return_if_fail (mw != NULL);

    trans = beleg_ablage_get_current_txn (mw->window);
    if (trans)
        beleg_dialog_assign (mw->window, trans);
    else
        beleg_dialog_no_transaction (mw->window);
}

static void
gnc_plugin_beleg_ablage_cmd_open (GSimpleAction *simple,
                                  GVariant *parameter,
                                  gpointer user_data)
{
    GncMainWindowActionData *mw = user_data;
    Transaction *trans;

    g_return_if_fail (mw != NULL);

    trans = beleg_ablage_get_current_txn (mw->window);
    if (trans)
        beleg_dialog_open (mw->window, trans);
    else
        beleg_dialog_no_transaction (mw->window);
}

static void
gnc_plugin_beleg_ablage_cmd_info (GSimpleAction *simple,
                                  GVariant *parameter,
                                  gpointer user_data)
{
    GncMainWindowActionData *mw = user_data;
    Transaction *trans;

    g_return_if_fail (mw != NULL);

    trans = beleg_ablage_get_current_txn (mw->window);
    if (trans)
        beleg_dialog_info (mw->window, trans);
    else
        beleg_dialog_no_transaction (mw->window);
}

/* ================= END OF FILE ============================== */
