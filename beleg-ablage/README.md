# Beleg-Ablage — GnuCash-Modul (out-of-tree)

*Copyright (C) 2026 Georg Roither, roither.dev e.U. — Lizenz GPLv2+.*

Out-of-tree-Modul `libgncmod-beleg-ablage` für GnuCash 5.x: ordnet jeder
Buchung im Register den Beleg (PDF) aus dem Archiv
`belege/<JJJJ-MM>/B<YYMM>-<Nr>_<Kurzname>.pdf` zu — bijektiv zum Feld
„Nr." der Buchung — und öffnet ihn auf Klick.

```
beleg-ablage/
├── CMakeLists.txt                        Build gegen GnuCash-Source/-Build-Tree
├── gncmod-beleg-ablage.c                 gnc-module-Glue + scm_init-Einstieg
├── gnc-plugin-beleg-ablage.c/.h          GncPlugin: Menü-Einträge, Actions
├── dialog-beleg-ablage.c/.h              Zuordnen-/Öffnen-/Info-Dialoge
├── beleg-ablage-kvp.c/.h                 KVP-Slots (path/id) auf der Transaction
├── beleg-ablage-prefs.c/.h               GSettings-Root (dev.roither.belegablage)
├── beleg-ablage-util.c/.h                Reine Logik: Regex, Nummern, Pfade
├── resources/
│   ├── gnc-plugin-beleg-ablage.ui        Menü-Einträge (Tools)
│   └── beleg-ablage.gresource.xml        GResource-Bundle (Prefix /org/gnucash/GnuCash)
├── data/
│   ├── dev.roither.belegablage.gschema.xml
│   └── config-user-snippet.scm           Lade-Hook für GnuCash-Start
├── tests/
│   ├── test-beleg-ablage-util.c          C-Unit-Tests (ctest)
│   └── test-beleg-ablage-util.py         Python-Spiegel derselben Logik
└── docs/
    ├── SPEC-gnucash-devel.md             Spec-Post-Entwurf (gnucash-devel)
    └── MR-DRAFT.md                       Stufe-2-MR-Beschreibung (Entwurf)
```

## Funktionsumfang (v1, Punkte 1–7 der Spez)

| # | Spez | Umsetzung |
|---|------|-----------|
| 1 | „Beleg zuordnen" für selektierte Buchung | Menü **Werkzeuge → Beleg zuordnen…** bei aktiver Register-Seite (Kontextmenü: Stufe 2, s. u.) |
| 2 | Monatsordner + PDF-Filter + Auto-Match | Dialog öffnet `<root>/belege/<JJJJ-MM>/` (JJJJ-MM aus Buchungsdatum), `GtkFileChooserNative` mit `*.pdf`-Filter, Preselect der Datei passend zur „Nr." |
| 3 | Namenskonvention prüfen | `^B\d{4}-\d{3,}(_.*)?\.pdf$` — Abweichung → Umbenenn-Vorschlag (nächste freie Nr.) |
| 4 | Speicherung ohne Schema-Migration | KVP-Slots `beleg-ablage::path` + `beleg-ablage::id` (flat, relativ, `/`-Separatoren) auf der Transaction; zusätzlich Sync des nativen Doclink-Felds |
| 5 | Status ✓/✗ + Doppelklick öffnet | Natives Register-Doclink-Icon zeigt Beleg-Status; **Werkzeuge → Beleg öffnen** (und Doppelklick auf das Doclink-Icon) öffnet das PDF via `xdg-open` (Linux) / `ShellExecute` (Windows) |
| 6 | Leere „Nr." | Vorschlag `B<YYMM>-<nächste laufende Nr.>`, Rück schreibbar ins Num-Feld |
| 7 | Root konfigurierbar | GSettings `dev.roither.belegablage` / `belege-root`; Fallback: GnuCash-Doclink-Pfad (Bearbeiten → Einstellungen → Dokumente) |

**Hinweis zur Namenskonvention:** Die ursprüngliche Spez nannte
`^B\d{6}-\d{3,}…` — das widerspricht der eigenen Konvention
`B<YYMM>-<Nr>_<Kurzname>.pdf` (YYMM = 4 Stellen) und der Bijektion zum
Num-Feld; damit würde jede konforme Datei abgelehnt. Implementiert ist das
einzige selbstkonsistente `^B\d{4}-\d{3,}(_.*)?\.pdf$` — **bitte gegen
`beleg-check.py` verifizieren** (dort muss dieselbe Gruppe gelten).

## Warum Menü statt Kontextmenü (v1)

Das Kontextmenü des Registers wird in-tree aus dessen Ressourcen
gebaut; out-of-tree lässt es sich nicht sauber erweitern, ohne Core-Patches
zu riskieren. Die drei Werkzeuge-Einträge nutzen denselben offiziellen
`GncPlugin`-Mechanismus wie die Business-Features. Kontextmenü +
Zusatzspalte sind explizit Stufe-2-In-tree-Themen (s. `docs/MR-DRAFT.md`).

## Build

Voraussetzungen: GnuCash-**Source-Tree** (Git-Checkout, z. B. dieser Fork)
und ein **fertig gebauter** GnuCash-**Build-Tree** (5.x, GTK3) sowie
CMake ≥ 3.16, C-Compiler (GCC/MinGW; **kein MSVC**), `pkg-config`,
`glib-compile-resources` (glib-dev). Getestet gegen GnuCash 5.17.

WSL/Debian bzw. MSYS2 (MINGW64):

```sh
cmake -B build \
  -DGNCCASH_SOURCE_DIR=~/src/gnucash \
  -DGNCCASH_BUILD_DIR=~/build/gnucash \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build            # Unit-Tests der reinen Logik
```

Ergebnis: `build/libgncmod-beleg-ablage.so` (Linux) bzw.
`build/libgncmod-beleg-ablage.dll` (Windows).

## Runtime-Setup

### 1. Modul-Lade-Hook

GnuCash 5.x lädt **keine** Fremdmodule automatisch. Der offiziell
unterstützte Weg ist die User-Config (`Gnucash::gnc_load_scm_config`):

- Linux: `~/.config/gnucash/config-user.scm`
- Windows: `%APPDATA%\GnuCash\config-user.scm`

Dort die Zeile aus `data/config-user-snippet.scm` ergänzen:

```scheme
(load-extension "libgncmod-beleg-ablage" "beleg_ablage_scm_init")
```

Das Modul muss auffindbar sein: `LTDL_LIBRARY_PATH` (Linux) bzw. `PATH`
(Windows) auf das Build-Verzeichnis setzen, oder die Datei nach
`lib/gnucash/` der GnuCash-Installation legen. Die Registrierung ist
idempotent — auch ein zusätzlicher `gnc_module_load`-Pfad ist unbedenklich.

### 2. GSettings-Schema

```sh
# Linux: Schema kompilieren und installieren (User)
cp data/dev.roither.belegablage.gschema.xml ~/.local/share/glib-2.0/schemas/
glib-compile-schemas ~/.local/share/glib-2.0/schemas/

# Archiv-Root setzen
gsettings set dev.roither.belegablage belege-root '/home/georg/Belegarchiv'
```

Windows (MSYS2): Schema-Verzeichnis per `GSETTINGS_SCHEMA_DIR` angeben.
Fehlt das Schema, startet das Modul trotzdem; der Fallback ist der
GnuCash-Doclink-Pfad. Tipp: GnuCash-Doclink-Head
(Bearbeiten → Einstellungen → Dokumente) auf denselben Root setzen — dann
funktioniert auch das native Register-Doclink-Icon (Punkt 5) ohne Extra-Setup.

### 3. Externe Tools

`beleg-check.py`, `Monatsabschluss.ps1`, `jahresmanifest.py` bleiben
unberührt: das Modul schreibt **nur** KVP-Slots (`slots`-Tabelle, je eine
STRING-Row `beleg-ablage::path` / `beleg-ablage::id`) und das native
Doclink-Feld — keine Schema-Migration, kein Verlust bei
`piecash`/SQLite-Exporten. Abnahme: `beleg-check.py` muss nach Zuordnungen
weiterhin Exit 0 liefern.

## Abnahme-Test (Kopie der echten Datei!)

1. Kopie von `roither-dev.gnucash` öffnen (niemals am Original testen).
2. Buchung im Register selektieren → Werkzeuge → Beleg zuordnen…
3. Monat `belege/<JJJJ-MM>/` prüfen, Datei wählen; bei falschem Namen
   Umbenenn-Vorschlag bestätigen; leere „Nr." mit Vorschlag befüllen lassen.
4. Doclink-Icon erscheint; Beleg öffnen per Werkzeuge → Beleg öffnen.
5. `beleg-check.py` gegen die Kopie: Exit 0 erforderlich.

## i18n / Stufe 2 / Stufe 3

- v1 bewusst mit deutschen Literalen: GnuCash lädt Menü-GtkBuilder mit der
  Textdomäne `gnucash`; eigene Kataloge griffen dort nicht (Stufe-2-TODO im
  Code markiert: gettext + In-tree-Ressource).
- Stufe 2: In-tree-MR `feature/beleg-ablage` ( gettext, Kontextmenü,
  Zusatzspalte, GnuCash-Help-Doku) — **vor** dem MR Ankündigung auf
  gnucash-devel (Entwurf: `docs/SPEC-gnucash-devel.md`), Upstream-Ziel
  bleibt code.gnucash.org (GitLab), Features → master.
- Stufe 3 (separater MR): Perioden-Lock für abgeschlossene Jahre.
