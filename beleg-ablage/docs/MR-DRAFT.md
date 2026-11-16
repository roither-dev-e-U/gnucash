# MR-Entwurf (Stufe 2): „Beleg-Ablage" in-tree

> **Status: ENTWURF.** Zielsystem: code.gnucash.org (GitLab),
> Ziel-Branch: `master`, Feature-Branch: `feature/beleg-ablage`
> (eine Feature = ein Branch = ein MR).
> **Reihenfolge laut Plan:** erst Ankündigung/Spec-Post auf gnucash-devel
> (`SPEC-gnucash-devel.md`), Diskussion abwarten, dann diesen MR als Draft
> öffnen. Bugfixes gehen separat nach `maint`.

## Titel (Vorschlag)

Add "document archive assignment" to transactions (Beleg-Ablage)

## Beschreibung (Vorschlag, englisch)

This merge request adds per-transaction receipt-document assignment
("Beleg-Ablage"), grown from a production out-of-tree module used for
single-person small-business bookkeeping (Austrian EÜR).

### What it does

- Assign a receipt PDF to a transaction from the register (context menu
  and a dedicated action). The archive root and layout
  (`belege/<YYYY-MM>/`), the file naming convention and its validation,
  auto-match against the transaction's "Nr." field, and a
  next-running-number proposal with write-back are handled by a small,
  engine-independent helper layer.
- Storage uses two namespaced flat KVP slots on the Transaction
  (`beleg-ablage::path`, `beleg-ablage::id`); the native doclink field is
  kept in sync so existing register features (doclink cell,
  double-click-to-open) keep working. No schema migration.
- A new register column shows receipt status; opening uses the existing
  doclink launcher (xdg-open / ShellExecute via `gnc_launch_doclink`).

### In-tree work items (deltas to the out-of-tree v1)

- [ ] Move sources to `gnucash/gnome/` (or a proposed plugin subdir per
      maintainer feedback), wire into the toplevel CMake and the module
      catalog; drop the custom `config-user.scm` load path (module loads
      via the standard mechanism).
- [ ] Merge the GResource bundle into the main GnuCash resource bundle
      (single ui file), removing the separate `glib-compile-resources` step.
- [ ] Replace German literals with gettext `_()` / `Q_()` strings; add
      `beleg-ablage` domain entries to `po/POTFILES.in` and German
      translations in `po/de.po`.
- [ ] Register context-menu entries on the register page and add the
      status column (register model, follow-up discussion if the
      maintainers prefer the generic doclink column).
- [ ] Documentation: new chapter in GnuCash Help (assign, validate,
      open; archive layout recommendation), `doc/help/*`.
- [ ] Generalize the naming pattern / archive layout per maintainer
      feedback from the gnucash-devel spec thread (make it configurable,
      or adopt as a generic "document archive" feature).
- [ ] Add tests: unit tests for the helper layer (already exist, to be
      ported to the in-tree test harness) plus KVP round-trip tests.

### License / attribution

GPLv2+, all new files carry the standard GnuCash header; author
contribution: Georg Roither, roither.dev e.U.

### Open questions (to be resolved on gnucash-devel before this MR)

1. KVP policy: namespaced flat keys vs. doclink reuse (see
   `docs/SPEC-gnucash-devel.md`, question 1).
2. GSettings schema vs. generalized doclink preference (question 2).
3. Whether the register column should be doclink-generic instead of
   feature-specific.

### Stage 3 (explicitly NOT this MR)

Period lock for closed years — separate MR once this one lands.

---

*Referenz: out-of-tree v1 liegt im Fork `roither-dev-e-U/gnucash`,
Branch `feature/beleg-ablage`, Verzeichnis `beleg-ablage/`.*
