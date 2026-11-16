/*
 * beleg-ablage-prefs.c -- configurable Beleg archive root directory
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

#include "beleg-ablage-prefs.h"

#include <gio/gio.h>
#include "dialog-doclink-utils.h"

gboolean
beleg_prefs_schema_available (void)
{
    GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
    GSettingsSchema *schema = NULL;
    gboolean available = FALSE;

    if (source)
    {
        /* recursive=TRUE so schemas in parent/user schema dirs are found */
        schema = g_settings_schema_source_lookup (source, BELEG_GSETTINGS_SCHEMA, TRUE);
        if (schema)
        {
            available = TRUE;
            g_settings_schema_unref (schema);
        }
    }
    return available;
}

gchar *
beleg_prefs_get_root (void)
{
    gchar *result = NULL;

    if (beleg_prefs_schema_available ())
    {
        GSettings *settings = g_settings_new (BELEG_GSETTINGS_SCHEMA);
        gchar *value = g_settings_get_string (settings, BELEG_GSETTINGS_KEY_ROOT);
        g_object_unref (settings);

        if (value && *value)
            result = value;             /* transfer ownership */
        else
            g_free (value);
    }

    if (!result)
    {
        /* fallback: native GnuCash document-link path head */
        result = gnc_doclink_get_path_head ();
        if (result && !*result)
        {
            g_free (result);
            result = NULL;
        }
    }

    return result;
}

/* ================= END OF FILE ============================== */
