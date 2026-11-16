/*
 * dialog-beleg-ablage.h -- dialogs of the Beleg-Ablage module
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
 * @file dialog-beleg-ablage.h
 * @brief Assign / open / inspect the Beleg of a transaction.
 *
 * NOTE Stufe 2 (in-tree): these German UI strings will become English
 * source strings with gettext translation (de) as part of the
 * upstream contribution.
 */

#ifndef DIALOG_BELEG_ABLAGE_H
#define DIALOG_BELEG_ABLAGE_H

#include <gtk/gtk.h>
#include "Transaction.h"

G_BEGIN_DECLS

/** "Beleg zuordnen" flow: choose a PDF from belege/<YYYY-MM>/, validate
 *  the file name convention, optionally rename the file, optionally
 *  write back a suggested number into the transaction's "Nr." field, and
 *  store the assignment in the KVP slots + doclink. */
void beleg_dialog_assign (GtkWindow *parent, Transaction *trans);

/** Opens the assigned Beleg PDF with the system default application. */
void beleg_dialog_open (GtkWindow *parent, Transaction *trans);

/** Shows num, Beleg id, relative path, root and validity status. */
void beleg_dialog_info (GtkWindow *parent, Transaction *trans);

/** Hint dialog when no (usable) transaction is selected. */
void beleg_dialog_no_transaction (GtkWindow *parent);

G_END_DECLS

#endif /* DIALOG_BELEG_ABLAGE_H */
