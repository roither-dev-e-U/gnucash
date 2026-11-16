/*
 * beleg-ablage-util.h -- pure helper logic for the Beleg-Ablage module
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
 * @file beleg-ablage-util.h
 * @brief Pure, engine-independent helpers for file names and dates.
 *
 * Archive contract (roither.dev e.U.):
 *  - Archive layout:  <root>/belege/<YYYY-MM>/<file>.pdf
 *  - File convention: ^B\d{4}-\d{3,}(_.*)?\.pdf$   (B<YYMM>-<Nr>_<Kurzname>.pdf)
 *  - "Nr." (Transaction.num) is bijective with the B-number of the file.
 */

#ifndef BELEG_ABLAGE_UTIL_H
#define BELEG_ABLAGE_UTIL_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * File name convention: ^B[0-9]{4}-[0-9]{3,}(_.*)?\.pdf$
 *
 * NOTE: the original spec draft stated "^B\d{6}-\d{3,}(_.*)?\.pdf$",
 * which contradicts its own convention B<YYMM>-<Nr>_<Kurzname>.pdf (YYMM
 * is a four-digit year+month, e.g. "B2611-007_rechnung.pdf") and the
 * bijection to the "Nr." field (B<YYMM>-<Nr>, see BELEG_NUM_REGEX). With
 * six digits every convention-conforming file would be rejected. The
 * four-digit form is the only self-consistent reading; keep in sync
 * with beleg-check.py!
 */
#define BELEG_FILENAME_REGEX  "^B[0-9]{4}-[0-9]{3,}(_.*)?\\.pdf$"
/** "Nr." field convention: B<YYMM>-<Nr> with at least 3 digits. */
#define BELEG_NUM_REGEX       "^B([0-9]{4})-([0-9]{3,})$"

/** Minimum zero-padded width of the running number in a B-number. */
#define BELEG_SEQ_WIDTH 3

/** TRUE if @a filename matches the Beleg file name convention. */
gboolean     beleg_filename_is_valid     (const char *filename);

/**
 * Parse a "Nr." value of the form B<YYMM>-<Nr>.
 *
 * @param num  the string to parse (may be NULL)
 * @param yymm out: 5 bytes buffer ("YYMM"), untouched when FALSE is returned
 * @param seq  out: running number, untouched when FALSE is returned
 * @return TRUE when @a num matches the B-number convention.
 */
gboolean     beleg_parse_num            (const char *num, char yymm[5], int *seq);

/** Extract the B-number ("B2611-007") from a valid file name, NULL otherwise. */
gchar       *beleg_num_from_filename    (const char *filename);

/** TRUE when the B-number encoded in @a filename equals @a num. */
gboolean     beleg_num_matches_filename (const char *num, const char *filename);

/** "YYMM" for the given date, e.g. "2611" for November 2026. */
gchar       *beleg_yymm_from_gdate      (GDate date);

/** "YYYY-MM" for the given date, e.g. "2026-11". */
gchar       *beleg_month_dir_from_gdate (GDate date);

/** Build a B-number: "B<yymm>-<seq>" with at least 3 digits for seq. */
gchar       *beleg_build_num            (const char *yymm, int seq);

/** Build a file name: "B<yymm>-<seq>[_shortname].pdf". */
gchar       *beleg_build_filename       (const char *yymm, int seq,
                                         const char *shortname);

/** Build the portable relative archive path "belege/<month>/<filename>". */
gchar       *beleg_rel_path             (const char *month_dir, const char *filename);

/** Normalize a stored relative path: '\' -> '/', strip leading "./". */
gchar       *beleg_normalize_rel_path   (const char *rel);

/** Join root and relative path into a native absolute path. */
gchar       *beleg_join_root            (const char *root, const char *rel);

/** Next free running number: max+1, at least 1. */
int          beleg_next_seq             (int max_seq);

/** Replace characters that are inconvenient in file names with '_'. */
gchar       *beleg_sanitize_shortname   (const char *name);

G_END_DECLS

#endif /* BELEG_ABLAGE_UTIL_H */
