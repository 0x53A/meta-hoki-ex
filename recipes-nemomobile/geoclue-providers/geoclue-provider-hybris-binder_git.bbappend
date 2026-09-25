# Keep upstream's package/D-Bus identity, replace its backend only on Hoki.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/../../../hoki-location/backend:"
SRC_URI:append:hoki = " file://0001-hoki-location-api-backend.patch file://locationapibackend.h file://locationapibackend.cpp file://locationprotocol.h file://locationapibackend.pro file://locationbackendfactory.cpp"
PR:append:hoki = ".hoki4"
SUMMARY:hoki = "GeoClue provider with isolated Hoki LocationAPI backend"
QMAKE_PROFILES:hoki = "${S}/hoki/locationapibackend.pro"
DEPENDS:remove:hoki = "libhybris libgbinder libglibutil glib-2.0"
RDEPENDS:${PN}:append:hoki = " hoki-location"
do_configure:prepend:hoki() {
    install -d ${S}/hoki
    for source in locationprotocol.h locationapibackend.h locationapibackend.cpp locationapibackend.pro locationbackendfactory.cpp; do
        install -m 0644 ${UNPACKDIR}/$source ${S}/hoki/
    done
}
