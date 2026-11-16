;; config-user-snippet.scm
;;
;; Copyright (C) 2026 Georg Roither, roither.dev e.U.
;; License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl-2.0.html>
;;
;; Loading hook for the out-of-tree Beleg-Ablage module.
;;
;; Stock GnuCash 5.x does NOT load third-party gnc modules automatically
;; (its startup only loads the fixed set of bundled modules). The supported
;; user-level extension point is the per-user Guile config file, which is
;; loaded by Gnucash::gnc_load_scm_config during startup:
;;
;;   Linux:   ~/.config/gnucash/config-user.scm
;;            ($XDG_CONFIG_HOME/gnucash/config-user.scm)
;;   Windows: %APPDATA%\GnuCash\config-user.scm
;;
;; Append the line below to that file (create the file if it does not
;; exist). "load-extension" resolves the library via ltdl:
;;
;;   - the directory containing this module must be findable, e.g. put
;;     libgncmod-beleg-ablage.so/.dll on your LTDL_LIBRARY_PATH
;;     (Linux) or PATH (Windows), or in GnuCash's own lib/gnucash
;;     module directory;
;;   - it calls the exported symbol scm_beleg_ablage_scm_init, which
;;     registers the plugin idempotently (safe to be loaded twice).
;;
;; The module additionally carries the classic gnc-module glue
;; (libgncmod_beleg_ablage_gnc_module_path/init etc.) so it can also be
;; loaded via gnc_module_load or catalogued by GNC_MODULE_PATH tools.

(load-extension "libgncmod-beleg-ablage" "beleg_ablage_scm_init")
