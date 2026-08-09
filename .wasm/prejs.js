// LibreCAD wasm pre-js: mount an IndexedDB-backed filesystem (IDBFS) at the
// QSettings directory so user settings survive page reloads.
//
// The matching C++ side (RS_Settings::init, Q_OS_WASM branch) writes settings
// to an INI file under this mount point. Everything here is wrapped so that a
// missing IDBFS / a mount failure can never abort application startup: in the
// worst case settings stay ephemeral (same as before this change).
(function () {
    var settingsDir = '/home/web_user/.local/share/LibreCAD';

    if (typeof Module === 'undefined') Module = {};
    Module.preRun = Module.preRun || [];
    Module.preRun.push(function () {
        try {
            if (typeof FS === 'undefined' || typeof IDBFS === 'undefined') {
                console.warn('[LibreCAD] IDBFS unavailable; settings will not persist.');
                return;
            }
            try { FS.mkdirTree(settingsDir); } catch (e) { /* directory may already exist */ }
            try {
                FS.mount(IDBFS, {}, settingsDir);
            } catch (e) {
                // Already mounted / mount point in use — non-fatal.
            }
            // Restore previously persisted state into MEMFS before QSettings reads.
            FS.syncfs(true, function (err) {
                if (err) console.warn('[LibreCAD] IDBFS load (syncfs) failed:', err);
            });
        } catch (e) {
            console.warn('[LibreCAD] IDBFS setup failed:', e);
        }
    });

    // Best-effort flush when the tab is closed or reloaded. (Browsers may not
    // wait for async work here, so this is a best-effort, not a guarantee.)
    if (typeof window !== 'undefined') {
        window.addEventListener('beforeunload', function () {
            try {
                if (typeof FS !== 'undefined') {
                    FS.syncfs(false, function (err) {
                        if (err) console.warn('[LibreCAD] IDBFS flush failed:', err);
                    });
                }
            } catch (e) { /* ignore */ }
        });
    }
})();
