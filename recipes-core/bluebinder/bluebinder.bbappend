FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append:hoki = " file://hoki-start-bluetooth file://hoki-start-bluetooth.conf"
PR:append:hoki = ".hoki7"
RDEPENDS:${PN}:append:hoki = " libhybris"

do_install:append:hoki() {
    install -Dm0755 ${UNPACKDIR}/hoki-start-bluetooth ${D}${sbindir}/hoki-start-bluetooth
    install -Dm0644 ${UNPACKDIR}/hoki-start-bluetooth.conf ${D}${systemd_system_unitdir}/bluebinder.service.d/hoki-start-bluetooth.conf
}

FILES:${PN}:append:hoki = " ${sbindir}/hoki-start-bluetooth ${systemd_system_unitdir}/bluebinder.service.d"

SRC_URI:append:hoki = " file://0001-fix-initialization-state-check.patch file://0002-fix-rfkill-callback-dispatch.patch file://0003-serialize-hal-initialization-and-close.patch"
