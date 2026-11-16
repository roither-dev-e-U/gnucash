/*
 * gncmod-beleg-ablage.c -- module glue of the Beleg-Ablage module
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

/** @file gncmod-beleg-ablage.c
    @brief module definition/initialization for the Beleg-Ablage plugin
    @author Copyright (c) 2026 Georg Roither, roither.dev e.U.

    The file name libgncmod-beleg-ablage matches the requirement of the
    GnuCash module loader (prefix "libgncmod" + G_MODULE_SUFFIX), and
    the exported symbols follow the loader's convention of deriving
    symbol names from the file's base name.

    Stock GnuCash 5.x does not load third-party modules automatically.
    The supported user-level load path is the per-user Guile config
    (config-user.scm) with

        (load-extension "libgncmod-beleg-ablage" "beleg_ablage_scm_init")

    which calls the second entry point below. Both entry points run the
    same idempotent self registration.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gmodule.h>

#include "gnc-module.h"
#include "gnc-module-api.h"

#include "gnc-plugin-beleg-ablage.h"

GNC_MODULE_API_DECL(libgncmod_beleg_ablage)

/* version of the gnc module system interface we require */
int libgncmod_beleg_ablage_gnc_module_system_interface = 0;

/* module versioning uses libtool semantics. */
int libgncmod_beleg_ablage_gnc_module_current  = 0;
int libgncmod_beleg_ablage_gnc_module_revision = 0;
int libgncmod_beleg_ablage_gnc_module_age      = 0;

char *
libgncmod_beleg_ablage_gnc_module_path (void)
{
    return g_strdup ("gnucash/plugins/beleg-ablage");
}

char *
libgncmod_beleg_ablage_gnc_module_description (void)
{
    return g_strdup ("Beleg-Ablage: assigns receipt documents (PDF) from a "
                     "structured archive to register transactions");
}

int
libgncmod_beleg_ablage_gnc_module_init (int refcount)
{
    /* Create the plugin (idempotent, guarded by g_once_init internally).
     * No SCM dependency here, so no gnc_module_load calls are needed. */
    beleg_ablage_self_register ();

    return TRUE;
}

int
libgncmod_beleg_ablage_gnc_module_end (int refcount)
{
    return TRUE;
}

/* ------------------------------------------------------------ */
/* Entry point for (load-extension "libgncmod-beleg-ablage"     */
/*                 "beleg_ablage_scm_init") in config-user.scm  */
/* ------------------------------------------------------------ */

G_MODULE_EXPORT void
scm_beleg_ablage_scm_init (void)
{
    beleg_ablage_self_register ();
}

/* ================= END OF FILE ============================== */
