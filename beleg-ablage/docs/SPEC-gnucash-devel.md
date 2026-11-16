# Spec-Post-Entwurf: gnucash-devel@gnucash.org

> **Status: ENTWURF — nicht versendet.** Versand durch Georg Roither
> (Absender muss Mitglied der Liste sein). Vor Versand: gegen den finalen
> Code lesen, Punkt „KVP policy" ggf. nach kvp_doc.txt-Review anpassen.

---

**Subject:** Proposed feature: per-transaction receipt/document archive assignment (out-of-tree module, interested in feedback before in-tree MR)

**To:** gnucash-devel@gnucash.org
**From:** Georg Roither, roither.dev e.U. <georg@roither.dev>

Hello everyone,

I'd like to propose a feature I've built first as an out-of-tree module,
and I'm looking for API/design feedback before working it into an in-tree
merge request. A working prototype exists (GPLv2+, out-of-tree module
`libgncmod-beleg-ablage`); I'll push the branch and link it once the
design discussion here settles the open questions below.

## Problem

Small-business users (I run a single-person e.U. in Austria on the EÜR
Kleinunternehmer scheme) typically keep receipt PDFs in a structured
archive — e.g. `belege/<YYYY-MM>/B<YYMM>-<Nr>_<shortname>.pdf`, where the
B-number is bijective with the transaction's "Nr." field. What's missing
in GnuCash is the glue inside the register:

1. Assign the matching PDF to a transaction (month folder derived from
   the posted date, auto-match on "Nr.", enforceable naming convention).
2. See at a glance which transactions have (or lack) a receipt.
3. Open the PDF with one interaction.
4. Suggest the next running number when "Nr." is empty and write it back.

The existing per-transaction *doclink* covers storage, but not
archive-layout conventions, number auto-match, or validation. My external
tooling (consistency checker, month/year manifests, yearly QES) already
enforces the bijection outside GnuCash; the module makes GnuCash a
first-class participant instead of fighting it.

## Design as implemented (v1)

- **Storage**: two flat top-level KVP slots on the Transaction —
  `beleg-ablage::path` (portable, forward-slash relative path below the
  archive root) and `beleg-ablage::id` (the B-number). No schema
  migration; in SQLite these land as one STRING row each in the
  `slots` table, readable by external tooling. I additionally sync the
  native *doclink* field so the existing register doclink cell and
  double-click-to-open keep working for free.
- **Root preference**: GSettings schema of my own
  (`dev.roither.belegablage`), falling back to the doclink path head —
  is a project-local GSettings schema acceptable for an in-tree feature,
  or should the doclink preference be generalized instead (e.g. a second
  registered "path head" category)?
- **Validation**: file names must match `^B\d{4}-\d{3,}(_.*)?\.pdf$`;
  invalid names get a rename proposal (next free number, sanitized
  shortname) before assignment. The pattern is obviously user-specific —
  in-tree I'd expect to generalize or make it configurable; feedback
  welcome.
- **UI (out-of-tree)**: a `GncPlugin` with three entries under
  *Tools* operating on the current register transaction
  (`gnc_plugin_page_register_get_current_txn`). Context menu and an
  extra register column feel in-tree-only (the register menu model lives
  in the register's own resources) — both are on my Stage-2 list.

## Questions for the list

1. **KVP policy**: are two namespaced flat keys per transaction
   (`beleg-ablage::*`) acceptable, or should this reuse/extend the
   doclink KVP plumbing? I've read `kvp_doc.txt` in the sources; opinions
   on the "one canonical location vs. namespaced addition" trade-off are
   appreciated, since my external checker reads these slots directly.
2. **Generalization**: is there interest in a generic
   "document archive layout" concept (template root + subfolder + naming
   pattern per book), of which my Beleg scheme would be one instance?
   Or should this stay a niche/plugin feature?
3. **Module loading**: stock GnuCash 5 loads third-party modules only
   via `config-user.scm`/`load-extension` (works, verified). Is there any
   appetite for a supported plugin directory scanned at startup, or is
   out-of-tree effectively discouraged in favor of in-tree?
4. If in-tree: feature branch off master, gettext, register context menu
   + column, Help chapter. Anything else the maintainers would want to
   see in the MR?

Happy to go into implementation details (GResource handling for plugin
UI, idempotent self-registration, readonly guards via
`qof_book_is_readonly`/`xaccTransIsReadonlyByPostedDate`) — I kept v1
deliberately minimal and C-only.

Thanks,
Georg Roither — roither.dev e.U., Austria
