# Keep upstream's package/D-Bus identity, replace its backend only on Hoki.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/../../recipes-hoki/hoki-location/files/hoki-location/backend:"
SRC_URI:append:hoki = " file://0001-hoki-location-api-backend.patch file://locationapibackend.h file://locationapibackend.cpp file://locationprotocol.h file://locationapibackend.pro file://locationbackendfactory.cpp"
PR:append:hoki = ".hoki6"
SUMMARY:hoki = "GeoClue provider with isolated Hoki LocationAPI backend"
QMAKE_PROFILES:hoki = "${S}/hoki/locationapibackend.pro"
DEPENDS:remove:hoki = "libhybris libgbinder libglibutil glib-2.0"
RDEPENDS:${PN}:append:hoki = " hoki-location"

# The lower Hoki layer installs this for its Binder HAL. LocationAPI cannot
# consume the injected NTP time, so remove it after all do_install appends.
python remove_hoki_legacy_gps_xtra() {
    import os
    if d.getVar('MACHINE') == 'hoki':
        path = d.expand('${D}${sysconfdir}/gps_xtra.ini')
        if os.path.lexists(path):
            os.unlink(path)
}
do_install[postfuncs] += " remove_hoki_legacy_gps_xtra"
FILES:${PN}:remove:hoki = "${sysconfdir}/gps_xtra.ini"
CONFFILES:${PN}:remove:hoki = "${sysconfdir}/gps_xtra.ini"

do_configure:prepend:hoki() {
    install -d ${S}/hoki
    for source in locationprotocol.h locationapibackend.h locationapibackend.cpp locationapibackend.pro locationbackendfactory.cpp; do
        install -m 0644 ${UNPACKDIR}/$source ${S}/hoki/
    done
}
