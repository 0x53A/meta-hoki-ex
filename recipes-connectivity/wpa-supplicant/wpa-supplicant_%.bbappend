# Separate patches for upstream review; no credentials or SSID-specific behavior.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
PACKAGE_ARCH:hoki = "${MACHINE_ARCH}"
SRC_URI:append:hoki = " \
    file://0001-dbus-fix-reporting-of-SAE-KeyMgmt.patch \
    file://0002-wpas-pin-selected-BSS-for-optional-PMF.patch \
    file://0003-nl80211-retry-host-SAE-for-partial-kernel-backports.patch \
"
PR:append:hoki = ".hoki2"
