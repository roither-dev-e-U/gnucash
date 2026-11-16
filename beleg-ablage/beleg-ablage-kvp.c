/*
 * beleg-ablage-kvp.c -- KVP persistence for the Beleg-Ablage module
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

#include "beleg-ablage-kvp.h"

/* qof_instance_get_kvp / qof_instance_set_kvp / qof_instance_slot_delete
 * are exported C symbols of libgnc-engine (declared in qofinstance-p.h).
 * They are re-declared here so the out-of-tree module does not have to
 * include a private GnuCash header. */
void qof_instance_get_kvp  (QofInstance *inst, GValue *value, unsigned count, ...);
void qof_instance_set_kvp  (QofInstance *inst, GValue const *value, unsigned count, ...);
void qof_instance_slot_delete (const QofInstance *inst, const char *path);

/* ------------------------------------------------------------ */
/* Helpers                                                     */
/* ------------------------------------------------------------ */

static void
beleg_kvp_set_string (Transaction *trans, const char *key, const char *value)
{
    GValue v = G_VALUE_INIT;

    g_value_init (&v, G_TYPE_STRING);
    g_value_set_string (&v, value);
    qof_instance_set_kvp (QOF_INSTANCE (trans), &v, 1, key);
    g_value_unset (&v);
}

static gchar *
beleg_kvp_get_string (Transaction *trans, const char *key)
{
    GValue v = G_VALUE_INIT;
    gchar *result = NULL;

    qof_instance_get_kvp (QOF_INSTANCE (trans), &v, 1, key);
    if (G_VALUE_HOLDS_STRING (&v))
    {
        const char *s = g_value_get_string (&v);
        if (s && *s)
            result = g_strdup (s);
    }
    g_value_unset (&v);
    return result;
}

/* Mark the instance (and its collection) dirty; qof_instance_set_kvp
 * itself does not do this. The GObject "dirty" property setter calls
 * qof_instance_set_dirty() internally. */
static void
beleg_kvp_mark_dirty (Transaction *trans)
{
    qof_instance_set (QOF_INSTANCE (trans), "dirty", TRUE, NULL);
}

/* ------------------------------------------------------------ */
/* Public API                                                  */
/* ------------------------------------------------------------ */

gchar *
beleg_kvp_get_path (Transaction *trans)
{
    g_return_val_if_fail (trans != NULL, NULL);
    return beleg_kvp_get_string (trans, BELEG_KVP_PATH_KEY);
}

gchar *
beleg_kvp_get_id (Transaction *trans)
{
    g_return_val_if_fail (trans != NULL, NULL);
    return beleg_kvp_get_string (trans, BELEG_KVP_ID_KEY);
}

gboolean
beleg_kvp_has (Transaction *trans)
{
    g_return_val_if_fail (trans != NULL, FALSE);
    return beleg_kvp_get_string (trans, BELEG_KVP_PATH_KEY) != NULL;
}

void
beleg_kvp_store (Transaction *trans, const char *rel_path, const char *beleg_id)
{
    g_return_if_fail (trans != NULL);
    g_return_if_fail (rel_path != NULL && *rel_path != '\0');

    beleg_kvp_set_string (trans, BELEG_KVP_PATH_KEY, rel_path);
    if (beleg_id && *beleg_id)
        beleg_kvp_set_string (trans, BELEG_KVP_ID_KEY, beleg_id);
    else
        qof_instance_slot_delete (QOF_INSTANCE (trans), BELEG_KVP_ID_KEY);

    /* keep the native doclink in sync so the register shows the
       standard doclink indicator for this transaction */
    xaccTransSetDocLink (trans, rel_path);

    beleg_kvp_mark_dirty (trans);
}

void
beleg_kvp_clear (Transaction *trans)
{
    g_return_if_fail (trans != NULL);

    qof_instance_slot_delete (QOF_INSTANCE (trans), BELEG_KVP_PATH_KEY);
    qof_instance_slot_delete (QOF_INSTANCE (trans), BELEG_KVP_ID_KEY);
    xaccTransSetDocLink (trans, "");
    beleg_kvp_mark_dirty (trans);
}

/* ================= END OF FILE ============================== */
