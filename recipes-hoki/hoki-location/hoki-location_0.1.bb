SUMMARY = "Hoki isolated Android LocationAPI helpers"
LICENSE = "MIT & BSD-3-Clause"
LIC_FILES_CHKSUM = "file://${UNPACKDIR}/NOTICE;md5=78d5332256e96cbed0af6081ded4117b file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../hoki-location:${THISDIR}/../../../hoki-location/build:${THISDIR}/files:"
SRC_URI = "file://hoki-location-helper file://hoki-location-assist file://location-api.conf file://hoki-location.conf file://NOTICE"
S = "${UNPACKDIR}"
PR = "r2"
COMPATIBLE_MACHINE = "hoki"
PACKAGE_ARCH = "${MACHINE_ARCH}"
# Android/bionic ABI binaries deliberately resolve against the device's vendor image,
# not the glibc package namespace. Build provenance is in hoki-location/README.md.
INSANE_SKIP:${PN} += "file-rdeps"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
do_configure[noexec] = "1"
do_compile[noexec] = "1"
do_install() {
    install -d ${D}${datadir}/licenses/hoki-location
    install -m 0644 ${UNPACKDIR}/NOTICE ${D}${datadir}/licenses/hoki-location/
    install -d ${D}${libexecdir} ${D}${systemd_user_unitdir}/geoclue-providers-hybris.service.d ${D}${libdir}/tmpfiles.d
    install -m 0755 ${UNPACKDIR}/hoki-location-helper ${UNPACKDIR}/hoki-location-assist ${D}${libexecdir}/
    install -m 0644 ${UNPACKDIR}/location-api.conf ${D}${systemd_user_unitdir}/geoclue-providers-hybris.service.d/
    install -m 0644 ${UNPACKDIR}/hoki-location.conf ${D}${libdir}/tmpfiles.d/
}
FILES:${PN} += "${systemd_user_unitdir} ${libdir}/tmpfiles.d ${datadir}/licenses/hoki-location"
