/*
 * beleg-ablage-prefs.h -- configurable Beleg archive root directory
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

/**
 * @file beleg-ablage-prefs.h
 * @brief Resolution of the Beleg archive root directory.
 *
 * Lookup order:
 *  1. GSettings schema "dev.roither.belegablage", key "belege-root"
 *     (path /dev/roither/belegablage/). The schema is looked up in the
 *     schema sources, so a missing schema degrades gracefully instead of
 *     aborting.
 *  2. Fallback: the native GnuCash document-link path head
 *     (gnc_doclink_get_path_head()).
 *  3. NULL: no root configured.
 */

#ifndef BELEG_ABLAGE_PREFS_H
#define BELEG_ABLAGE_PREFS_H

#include <glib.h>

G_BEGIN_DECLS

#define BELEG_GSETTINGS_SCHEMA  "dev.roither.belegablage"
#define BELEG_GSETTINGS_PATH    "/dev/roither/belegablage/"
#define BELEG_GSETTINGS_KEY_ROOT "belege-root"

/**
 * Returns the configured Beleg archive root directory (g_free() it),
 * or NULL when neither the GSettings key nor the doclink fallback
 * provides a usable value.
 */
gchar   *beleg_prefs_get_root (void);

/** TRUE when the GSettings schema is available in the schema sources. */
gboolean beleg_prefs_schema_available (void);

G_END_DECLS

#endif /* BELEG_ABLAGE_PREFS_H */
