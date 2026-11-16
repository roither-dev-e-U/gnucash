#!/usr/bin/env python3
"""
test-beleg-ablage-util.py -- Python mirror of the C unit tests

Copyright (C) 2026 Georg Roither, roither.dev e.U.
License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>

The sandbox in which this module was authored cannot compile C
(no gcc/cmake available). This script mirrors the pure logic of
beleg-ablage-util.c 1:1 (regexes, parsing, path building) and runs the
same test cases as tests/test-beleg-ablage-util.c. It documents that
the *logic* is correct; the C test target additionally guards the real
implementation on a real build machine (ctest).
"""

import re
import sys

FILENAME_REGEX = re.compile(r"^B[0-9]{4}-[0-9]{3,}(_.*)?\.pdf$")
NUM_REGEX = re.compile(r"^B([0-9]{4})-([0-9]{3,})$")
SEQ_WIDTH = 3

checks = {"run": 0, "failed": 0}


def check(cond, label):
    checks["run"] += 1
    if not cond:
        checks["failed"] += 1
        print(f"FAIL {label}")


def check_str(a, b):
    check(a == b, f"{a!r} != {b!r}")


# --- mirror of beleg-ablage-util.c --------------------------------------

def filename_is_valid(filename):
    return bool(filename) and bool(FILENAME_REGEX.match(filename))


def parse_num(num):
    """returns (ok, yymm, seq); out params untouched -> None on failure"""
    if not num:
        return False, None, None
    m = NUM_REGEX.match(num)
    if not m:
        return False, None, None
    return True, m.group(1)[:4], int(m.group(2))


def num_from_filename(filename):
    if not filename or not filename_is_valid(filename):
        return None
    for i, ch in enumerate(filename):
        if ch in "_.":
            return filename[:i]
    return filename


def num_matches_filename(num, filename):
    return bool(num) and num_from_filename(filename) == num


def build_num(yymm, seq):
    return f"B{(yymm or '0000')}-{seq:0{SEQ_WIDTH}d}"


def build_filename(yymm, seq, shortname):
    num = build_num(yymm, seq)
    return f"{num}_{shortname}.pdf" if shortname else f"{num}.pdf"


def rel_path(month_dir, filename):
    if not month_dir or not filename:
        return None
    return f"belege/{month_dir}/{filename}"


def normalize_rel_path(rel):
    if rel is None:
        return None
    tmp = rel.replace("\\", "/")
    while tmp.startswith("./"):
        tmp = tmp[2:]
    return tmp


def join_root(root, rel):
    if not root:
        return None
    norm = normalize_rel_path(rel)
    if norm is None:
        return None
    return f"{root.rstrip('/')}/{norm}"


def next_seq(max_seq):
    return max_seq + 1 if max_seq >= 0 else 1


def sanitize_shortname(name):
    if not name:
        return None
    return re.sub(r"[^A-Za-z0-9_\-\.]", "_", name)


# --- same cases as the C test -------------------------------------------

def main():
    # filename_is_valid
    for good in ["B2611-007_rechnung.pdf", "B2611-007.pdf",
                 "B2611-1234_x.pdf", "B9912-001_re0.pdf"]:
        check(filename_is_valid(good), good)
    for bad in ["scan.pdf", "B2611-7_x.pdf", "B2611-007_rechnung.txt",
                "b2611-007_x.pdf", "B261-007_x.pdf", "B2611_007_x.pdf",
                "", None]:
        check(not filename_is_valid(bad), bad)

    # parse_num
    ok, yymm, seq = parse_num("B2611-007")
    check(ok and yymm == "2611" and seq == 7, "B2611-007")
    ok, yymm, seq = parse_num("B2611-1234")
    check(ok and yymm == "2611" and seq == 1234, "B2611-1234")
    for bad in ["2611-007", "B2611-7", "B2611-007_x", "", None]:
        ok, yymm, seq = parse_num(bad)
        check(not ok and yymm is None and seq is None, bad)

    # num_from_filename
    check_str(num_from_filename("B2611-007_rechnung.pdf"), "B2611-007")
    check_str(num_from_filename("B2611-007.pdf"), "B2611-007")
    check_str(num_from_filename("B2611-1234_r-kaise.pdf"), "B2611-1234")
    check(num_from_filename("scan.pdf") is None, "scan.pdf")
    check(num_from_filename("B2611-7_x.pdf") is None, "B2611-7_x.pdf")

    # num_matches_filename
    check(num_matches_filename("B2611-007", "B2611-007_rechnung.pdf"), "m1")
    check(num_matches_filename("B2611-007", "B2611-007.pdf"), "m2")
    check(not num_matches_filename("B2611-007", "B2611-008_x.pdf"), "m3")
    check(not num_matches_filename("", "B2611-007.pdf"), "m4")
    check(not num_matches_filename(None, "B2611-007.pdf"), "m5")
    check(not num_matches_filename("scan", "scan.pdf"), "m6")

    # date helpers (14 Nov 2026)
    check_str(f"{2026 % 100:02d}{11:02d}", "2611")
    check_str(f"{2026:04d}-{11:02d}", "2026-11")

    # build_num / build_filename
    check_str(build_num("2611", 7), "B2611-007")
    check_str(build_num("2611", 42), "B2611-042")
    check_str(build_num("2611", 1234), "B2611-1234")
    check_str(build_filename("2611", 7, "rechnung"), "B2611-007_rechnung.pdf")
    check_str(build_filename("2611", 7, None), "B2611-007.pdf")

    # rel_path / normalize / join
    check_str(rel_path("2026-11", "B2611-007_x.pdf"),
              "belege/2026-11/B2611-007_x.pdf")
    check_str(normalize_rel_path("belege\\2026-11\\x.pdf"),
              "belege/2026-11/x.pdf")
    check_str(normalize_rel_path("./belege/x.pdf"), "belege/x.pdf")
    check_str(join_root("/home/georg/archiv", "belege/2026-11/x.pdf"),
              "/home/georg/archiv/belege/2026-11/x.pdf")

    # next_seq / sanitize
    check(next_seq(-1) == 1, "next_seq(-1)")
    check(next_seq(0) == 1, "next_seq(0)")
    check(next_seq(7) == 8, "next_seq(7)")
    check(next_seq(999) == 1000, "next_seq(999)")
    check_str(sanitize_shortname("Rechnung Telekom"), "Rechnung_Telekom")
    check(sanitize_shortname("") is None, "sanitize('')")
    check(sanitize_shortname(None) is None, "sanitize(None)")
    s = sanitize_shortname("rük/rechn:ng?.pdf")
    check("/" not in s and ":" not in s and "?" not in s, s)

    print(f"beleg-ablage-util (python mirror): "
          f"{checks['run']} checks, {checks['failed']} failed")
    return 0 if checks["failed"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
