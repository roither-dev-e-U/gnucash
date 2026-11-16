/*
 * beleg-ablage-util.c -- pure helper logic for the Beleg-Ablage module
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

#include "beleg-ablage-util.h"

#include <string.h>

/* ------------------------------------------------------------ */
/* RegEx helpers (compiled once, cached)                        */
/* ------------------------------------------------------------ */

static GRegex *
beleg_filename_regex (void)
{
    static GRegex *regex = NULL;
    if (g_once_init_enter (&regex))
    {
        GRegex *r = g_regex_new (BELEG_FILENAME_REGEX, (GRegexCompileFlags)0, (GRegexMatchFlags)0, NULL);
        g_once_init_leave (&regex, r);
    }
    return regex;
}

static GRegex *
beleg_num_regex (void)
{
    static GRegex *regex = NULL;
    if (g_once_init_enter (&regex))
    {
        GRegex *r = g_regex_new (BELEG_NUM_REGEX, (GRegexCompileFlags)0, (GRegexMatchFlags)0, NULL);
        g_once_init_leave (&regex, r);
    }
    return regex;
}

/* ------------------------------------------------------------ */
/* Validation                                                   */
/* ------------------------------------------------------------ */

gboolean
beleg_filename_is_valid (const char *filename)
{
    if (!filename || !*filename)
        return FALSE;
    return g_regex_match (beleg_filename_regex (), filename, (GRegexMatchFlags)0, NULL);
}

gboolean
beleg_parse_num (const char *num, char yymm[5], int *seq)
{
    gboolean ok = FALSE;
    GMatchInfo *info = NULL;

    if (yymm)
        yymm[0] = '\0';
    if (seq)
        *seq = 0;

    if (!num || !*num)
        return FALSE;

    if (g_regex_match (beleg_num_regex (), num, (GRegexMatchFlags)0, &info))
    {
        gchar *grp_yymm = g_match_info_fetch (info, 1);
        gchar *grp_seq  = g_match_info_fetch (info, 2);
        if (grp_yymm && grp_seq)
        {
            if (yymm)
            {
                strncpy (yymm, grp_yymm, 4);
                yymm[4] = '\0';
            }
            if (seq)
                *seq = atoi (grp_seq);
            ok = TRUE;
        }
        g_free (grp_yymm);
        g_free (grp_seq);
    }
    g_match_info_free (info);
    return ok;
}

gchar *
beleg_num_from_filename (const char *filename)
{
    const char *p;

    if (!filename || !beleg_filename_is_valid (filename))
        return NULL;

    /* The B-number ends at the '_' shortname separator or at the
       extension: "B2611-007_kurzname.pdf" -> "B2611-007". */
    p = filename;
    while (*p && *p != '_' && *p != '.')
        p++;

    return g_strndup (filename, p - filename);
}

gboolean
beleg_num_matches_filename (const char *num, const char *filename)
{
    gchar *file_num;
    gboolean match;

    if (!num || !filename)
        return FALSE;

    file_num = beleg_num_from_filename (filename);
    if (!file_num)
        return FALSE;

    match = (g_strcmp0 (num, file_num) == 0);
    g_free (file_num);
    return match;
}

/* ------------------------------------------------------------ */
/* Dates                                                        */
/* ------------------------------------------------------------ */

gchar *
beleg_yymm_from_gdate (GDate date)
{
    return g_strdup_printf ("%02d%02d",
                            g_date_get_year (date) % 100,
                            g_date_get_month (date));
}

gchar *
beleg_month_dir_from_gdate (GDate date)
{
    return g_strdup_printf ("%04d-%02d",
                            g_date_get_year (date),
                            g_date_get_month (date));
}

/* ------------------------------------------------------------ */
/* Builders                                                     */
/* ------------------------------------------------------------ */

gchar *
beleg_build_num (const char *yymm, int seq)
{
    return g_strdup_printf ("B%s-%0*d", (yymm && *yymm) ? yymm : "0000",
                            BELEG_SEQ_WIDTH, seq);
}

gchar *
beleg_build_filename (const char *yymm, int seq, const char *shortname)
{
    gchar *num = beleg_build_num (yymm, seq);
    gchar *result;

    if (shortname && *shortname)
        result = g_strdup_printf ("%s_%s.pdf", num, shortname);
    else
        result = g_strdup_printf ("%s.pdf", num);
    g_free (num);
    return result;
}

gchar *
beleg_rel_path (const char *month_dir, const char *filename)
{
    if (!month_dir || !*month_dir || !filename || !*filename)
        return NULL;

    return g_strconcat ("belege/", month_dir, "/", filename, (char *)NULL);
}

gchar *
beleg_normalize_rel_path (const char *rel)
{
    gchar *tmp;
    gchar *result;

    if (!rel)
        return NULL;

    tmp = g_strdup (rel);
    /* always portable '/' separators in the stored value */
    for (gchar *p = tmp; *p; p++)
        if (*p == G_DIR_SEPARATOR)
            *p = '/';

    /* strip any number of leading "./" */
    while (g_str_has_prefix (tmp, "./"))
    {
        result = g_strdup (tmp + 2);
        g_free (tmp);
        tmp = result;
    }

    result = g_strdup (tmp);
    g_free (tmp);
    return result;
}

gchar *
beleg_join_root (const char *root, const char *rel)
{
    gchar *norm;
    gchar *result;

    if (!root || !*root)
        return NULL;

    norm = beleg_normalize_rel_path (rel);
    if (!norm)
        return NULL;

    /* native separators for filesystem access */
    result = g_build_filename (root, norm, (char *)NULL);
    g_free (norm);
    return result;
}

int
beleg_next_seq (int max_seq)
{
    return (max_seq >= 0) ? (max_seq + 1) : 1;
}

gchar *
beleg_sanitize_shortname (const char *name)
{
    if (!name || !*name)
        return NULL;

    gchar *result = g_strdup (name);
    for (gchar *p = result; *p; p++)
    {
        gunichar c = (gunichar)*p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.')
            continue;
        *p = '_';
    }
    if (!*result)
    {
        g_free (result);
        return NULL;
    }
    return result;
}

/* ================= END OF FILE ============================== */
