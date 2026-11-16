/*
 * gnc-plugin-beleg-ablage.h -- GncPlugin subclass of the Beleg-Ablage module
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

#ifndef GNC_PLUGIN_BELEG_ABLAGE_H
#define GNC_PLUGIN_BELEG_ABLAGE_H

#include "gnc-plugin.h"

G_BEGIN_DECLS

/* type macros */
#define GNC_TYPE_PLUGIN_BELEG_ABLAGE            (gnc_plugin_beleg_ablage_get_type ())
G_DECLARE_FINAL_TYPE (GncPluginBelegAblage, gnc_plugin_beleg_ablage,
                      GNC, PLUGIN_BELEG_ABLAGE, GncPlugin)

/** The textual name of this plugin (also used for plugin-manager lookup). */
#define GNC_PLUGIN_BELEG_ABLAGE_NAME  "gnc-plugin-beleg-ablage"

/** Create a new Beleg-Ablage plugin instance. */
GncPlugin *gnc_plugin_beleg_ablage_new (void);

/**
 * Idempotently create the plugin singleton and add it to the plugin
 * manager. Safe to call from every load path (gnc_module_init and
 * scm_beleg_ablage_scm_init); internally guarded by g_once_init.
 *
 * NOTE Stufe 2 (in-tree): UI strings of this plugin become gettext
 * source strings (domain "gnucash") as part of the upstream MR.
 */
void beleg_ablage_self_register (void);

G_END_DECLS

#endif /* GNC_PLUGIN_BELEG_ABLAGE_H */
