/*
 * beleg-ablage-kvp.h -- KVP persistence for the Beleg-Ablage module
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
 * @file beleg-ablage-kvp.h
 * @brief Beleg assignment persisted in flat KVP slots on the Transaction.
 *
 * Storage contract (SQLite-safe, readable by external scripts):
 *  - "beleg-ablage::path"  relative archive path, '/' separators,
 *                          e.g. "belege/2026-11/B2611-007_rechnung.pdf"
 *  - "beleg-ablage::id"    B-number, e.g. "B2611-007"
 * Both are flat top-level KVP string slots. In the SQLite backend they
 * appear in the "slots" table as rows with slot_type=4 (STRING) and
 * string_val=<value>, obj_guid=<transaction guid>. No schema migration
 * is needed and piecash/external tools can query them trivially.
 *
 * In addition the native transaction doclink (TRANS_DOCLINK "doclink")
 * is kept in sync so the standard register shows the doclink indicator.
 *
 * All mutating functions must be wrapped by the caller in
 * xaccTransBeginEdit()/xaccTransCommitEdit().
 */

#ifndef BELEG_ABLAGE_KVP_H
#define BELEG_ABLAGE_KVP_H

#include <glib.h>
#include "qof.h"
#include "Transaction.h"

G_BEGIN_DECLS

#define BELEG_KVP_PATH_KEY  "beleg-ablage::path"
#define BELEG_KVP_ID_KEY    "beleg-ablage::id"

/** Relative archive path of the assigned Beleg, g_free() by caller. NULL if unset. */
gchar   *beleg_kvp_get_path (Transaction *trans);

/** B-number of the assigned Beleg, g_free() by caller. NULL if unset. */
gchar   *beleg_kvp_get_id   (Transaction *trans);

/** TRUE when the transaction has a Beleg assigned in its KVP slots. */
gboolean beleg_kvp_has      (Transaction *trans);

/**
 * Store path + id in the KVP slots and sync the native doclink.
 * Marks the instance dirty. Caller wraps in Begin/CommitEdit.
 */
void     beleg_kvp_store    (Transaction *trans, const char *rel_path, const char *beleg_id);

/** Remove both KVP slots and the native doclink. Marks dirty. */
void     beleg_kvp_clear    (Transaction *trans);

G_END_DECLS

#endif /* BELEG_ABLAGE_KVP_H */
