FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:hoki = " file://0001-bgcom-recover-from-resume-timeout-without-BUG.patch"

SRC_URI:append:hoki = " file://0002-nfc-nci-complete-before-callback-and-free-failed-sends.patch"

SRC_URI:append:hoki = " file://0003-nfc-rawsock-deactivate-target-in-release-context.patch"

SRC_URI:append:hoki = " file://0004-bg-rsb-restore-wheel-enable-on-resume.patch"

# Correctness fixes with read-only occurrence counters (task 0513).
SRC_URI:append:hoki = " file://0005-fg-fix-shadow-retry-results-and-count-boundaries.patch"
SRC_URI:append:hoki = " file://0006-fg-release-cancelled-esr-wake-and-count-recovery.patch"
SRC_URI:append:hoki = " file://0007-lpm-report-idle-and-suspend-failures-with-counters.patch"
