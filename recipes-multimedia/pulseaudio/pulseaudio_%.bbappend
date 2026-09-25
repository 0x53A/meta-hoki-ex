FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append:hoki = " file://hoki-alsa.pa file://0001-alsa-honor-disabled-rewinds.patch file://0002-alsa-coalesce-copy-writes.patch"
RDEPENDS:pulseaudio-server:append:hoki = " pulseaudio-module-alsa-sink pulseaudio-module-alsa-source "
do_install:append:hoki() {
    install -m 0644 ${UNPACKDIR}/hoki-alsa.pa ${D}${sysconfdir}/pulse/hoki-alsa.pa
    echo '.include /etc/pulse/hoki-alsa.pa' >> ${D}${sysconfdir}/pulse/default.pa
}
FILES:${PN}-server:append:hoki = " ${sysconfdir}/pulse/hoki-alsa.pa"
