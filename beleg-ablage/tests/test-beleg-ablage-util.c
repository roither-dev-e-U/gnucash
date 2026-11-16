/*
 * test-beleg-ablage-util.c -- unit tests for the pure logic helpers
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
 *
 * Plain main() with asserts — runs without a GnuCash installation,
 * only GLib is required. Build & run via the CMake test target:
 *   ctest --test-dir build
 */

#include "beleg-ablage-util.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int checks_run = 0;
static int checks_failed = 0;

#define CHECK(cond)                                                      \
    do                                                                   \
    {                                                                    \
        checks_run++;                                                    \
        if (!(cond))                                                     \
        {                                                                \
            checks_failed++;                                              \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                \
    } while (0)

#define CHECK_STR(a, b)                                                  \
    do                                                                   \
    {                                                                    \
        checks_run++;                                                    \
        if (!((a) && (b) && strcmp((a), (b)) == 0))                      \
        {                                                                \
            checks_failed++;                                             \
            fprintf(stderr, "FAIL %s:%d: \"%s\" != \"%s\"\n",            \
                    __FILE__, __LINE__, (a) ? (a) : "(null)",            \
                    (b) ? (b) : "(null)");                               \
        }                                                                \
    } while (0)

static void
test_filename_valid(void)
{
    /* positives */
    CHECK(beleg_filename_is_valid ("B2611-007_rechnung.pdf"));
    CHECK(beleg_filename_is_valid ("B2611-007.pdf"));
    CHECK(beleg_filename_is_valid ("B2611-1234_x.pdf"));   /* 4-digit seq */
    CHECK(beleg_filename_is_valid ("B9912-001_re0.pdf"));  /* YY 99 */

    /* negatives */
    CHECK(!beleg_filename_is_valid ("scan.pdf"));
    CHECK(!beleg_filename_is_valid ("B2611-7_x.pdf"));        /* < 3 digits */
    CHECK(!beleg_filename_is_valid ("B2611-007_rechnung.txt")); /* no .pdf */
    CHECK(!beleg_filename_is_valid ("b2611-007_x.pdf"));      /* lowercase */
    CHECK(!beleg_filename_is_valid ("B261-007_x.pdf"));       /* 5 digits  */
    CHECK(!beleg_filename_is_valid ("B2611_007_x.pdf"));      /* no dash   */
    CHECK(!beleg_filename_is_valid (""));
    CHECK(!beleg_filename_is_valid (NULL));
}

static void
test_parse_num(void)
{
    char yymm[5] = "";
    int seq = 0;

    CHECK(beleg_parse_num ("B2611-007", yymm, &seq));
    CHECK_STR(yymm, "2611");
    CHECK(seq == 7);

    CHECK(beleg_parse_num ("B2611-1234", yymm, &seq));
    CHECK_STR(yymm, "2611");
    CHECK(seq == 1234);

    yymm[0] = '\0'; seq = 0;
    CHECK(!beleg_parse_num ("2611-007", yymm, &seq));   /* missing B */
    CHECK(yymm[0] == '\0' && seq == 0);                /* out params untouched */
    CHECK(!beleg_parse_num ("B2611-7", yymm, &seq));   /* < 3 digits */
    CHECK(!beleg_parse_num ("B2611-007_x", yymm, &seq));
    CHECK(!beleg_parse_num ("", yymm, &seq));
    CHECK(!beleg_parse_num (NULL, yymm, &seq));
}

static void
test_num_from_filename(void)
{
    gchar *num;

    num = beleg_num_from_filename ("B2611-007_rechnung.pdf");
    CHECK_STR(num, "B2611-007");
    g_free(num);

    num = beleg_num_from_filename ("B2611-007.pdf");
    CHECK_STR(num, "B2611-007");
    g_free(num);

    num = beleg_num_from_filename ("B2611-1234_r-kaise.pdf");
    CHECK_STR(num, "B2611-1234");
    g_free(num);

    CHECK(beleg_num_from_filename ("scan.pdf") == NULL);
    CHECK(beleg_num_from_filename ("B2611-7_x.pdf") == NULL);
    CHECK(beleg_num_from_filename (NULL) == NULL);
}

static void
test_num_matches_filename(void)
{
    CHECK(beleg_num_matches_filename ("B2611-007", "B2611-007_rechnung.pdf"));
    CHECK(beleg_num_matches_filename ("B2611-007", "B2611-007.pdf"));
    CHECK(!beleg_num_matches_filename ("B2611-007", "B2611-008_x.pdf"));
    CHECK(!beleg_num_matches_filename ("", "B2611-007.pdf"));
    CHECK(!beleg_num_matches_filename (NULL, "B2611-007.pdf"));
    CHECK(!beleg_num_matches_filename ("B2611-007", NULL));
    CHECK(!beleg_num_matches_filename ("scan", "scan.pdf"));
}

static void
test_gdate_helpers(void)
{
    GDate d = g_date_new_dmy(14, G_DATE_NOVEMBER, 2026);
    gchar *s;

    s = beleg_yymm_from_gdate(d);
    CHECK_STR(s, "2611");
    g_free(s);

    s = beleg_month_dir_from_gdate(d);
    CHECK_STR(s, "2026-11");
    g_free(s);

    g_date_free(d);
}

static void
test_build_num_and_filename(void)
{
    gchar *s;

    s = beleg_build_num("2611", 7);
    CHECK_STR(s, "B2611-007");          /* zero-padded to 3 */
    g_free(s);

    s = beleg_build_num("2611", 42);
    CHECK_STR(s, "B2611-042");
    g_free(s);

    s = beleg_build_num("2611", 1234);
    CHECK_STR(s, "B2611-1234");         /* > 3 digits kept */
    g_free(s);

    s = beleg_build_filename("2611", 7, "rechnung");
    CHECK_STR(s, "B2611-007_rechnung.pdf");
    g_free(s);

    s = beleg_build_filename("2611", 7, NULL);
    CHECK_STR(s, "B2611-007.pdf");
    g_free(s);
}

static void
test_rel_path_and_normalize(void)
{
    gchar *s;

    s = beleg_rel_path("2026-11", "B2611-007_x.pdf");
    CHECK_STR(s, "belege/2026-11/B2611-007_x.pdf");
    g_free(s);

    /* backslashes -> forward slashes (Windows-tolerant storage) */
    s = beleg_normalize_rel_path("belege\\2026-11\\x.pdf");
    CHECK_STR(s, "belege/2026-11/x.pdf");
    g_free(s);

    s = beleg_normalize_rel_path("./belege/x.pdf");
    CHECK_STR(s, "belege/x.pdf");
    g_free(s);

    s = beleg_normalize_rel_path("belege/2026-11/B2611-007_x.pdf");
    CHECK_STR(s, "belege/2026-11/B2611-007_x.pdf");  /* idempotent */
    g_free(s);

    /* join: forward-slash rel path works on both platforms via
     * g_build_filename */
    s = beleg_join_root("/home/georg/archiv", "belege/2026-11/x.pdf");
    CHECK_STR(s, "/home/georg/archiv/belege/2026-11/x.pdf");
    g_free(s);
}

static void
test_next_seq_and_sanitize(void)
{
    gchar *s;

    CHECK(beleg_next_seq(-1) == 1);
    CHECK(beleg_next_seq(0) == 1);
    CHECK(beleg_next_seq(7) == 8);
    CHECK(beleg_next_seq(999) == 1000);

    s = beleg_sanitize_shortname("Rechnung Telekom");
    CHECK_STR(s, "Rechnung_Telekom");    /* space -> '_' */
    g_free(s);

    s = beleg_sanitize_shortname("rük/rechn:ng?.pdf");
    CHECK(strchr(s, '/') == NULL);       /* separators removed */
    CHECK(strchr(s, ':') == NULL);
    CHECK(strchr(s, '?') == NULL);
    g_free(s);

    s = beleg_sanitize_shortname("");
    CHECK(s == NULL);                   /* empty shortname stays unused */
    g_free(s);

    s = beleg_sanitize_shortname(NULL);
    CHECK(s == NULL);
}

int
main(void)
{
    test_filename_valid();
    test_parse_num();
    test_num_from_filename();
    test_num_matches_filename();
    test_gdate_helpers();
    test_build_num_and_filename();
    test_rel_path_and_normalize();
    test_next_seq_and_sanitize();

    printf("beleg-ablage-util: %d checks, %d failed\n",
           checks_run, checks_failed);
    return checks_failed == 0 ? 0 : 1;
}
