# Avahi publishes the Hoki hostname; resolved only performs mDNS lookups.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append:hoki = " file://60-hoki-mdns.conf"

do_install:append:hoki() {
    install -d ${D}${sysconfdir}/systemd/resolved.conf.d
    install -m 0644 ${UNPACKDIR}/60-hoki-mdns.conf ${D}${sysconfdir}/systemd/resolved.conf.d/60-hoki-mdns.conf
}

# OE packages resolved and ${sysconfdir}/systemd in the main systemd package.
