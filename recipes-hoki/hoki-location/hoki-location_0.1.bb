SUMMARY = "Hoki isolated Android LocationAPI helpers"
LICENSE = "MIT & BSD-3-Clause"
LIC_FILES_CHKSUM = "file://NOTICE;md5=78d5332256e96cbed0af6081ded4117b file://${UNPACKDIR}/libcxx/LICENSE.TXT;md5=7b3a0e1b99822669d630011defe9bfd9"
SRC_URI = "file://hoki-location file://location-api.conf file://hoki-location.conf \
    git://android.googlesource.com/platform/external/libcxx;protocol=https;nobranch=1;destsuffix=libcxx"
SRCREV = "bc21e7a0d91e21c042d9e303571a2c5da5dad613"
S = "${UNPACKDIR}/hoki-location"
PR = "r3"
COMPATIBLE_MACHINE = "^hoki$"
PACKAGE_ARCH = "${MACHINE_ARCH}"
DEPENDS = "android-ndk-native android-system-data"
# Android/bionic helpers resolve against the device's vendor image, not glibc.
INSANE_SKIP:${PN} += "file-rdeps"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
NDK_TOOLCHAIN = "${STAGING_LIBDIR_NATIVE}/android-ndk/toolchains/llvm/prebuilt/linux-x86_64"
# Keep NDK-generated debug information independent of the build directory.
HOKI_DEBUG_PREFIX_MAP = "-ffile-prefix-map=${WORKDIR}=/usr/src/debug/${PN}/${PV}"
HOKI_VENDOR_LIBDIR = "${RECIPE_SYSROOT}${datadir}/hoki-location-link-libs"
do_configure[noexec] = "1"

do_compile() {
    ${NDK_TOOLCHAIN}/bin/clang++ --target=armv7a-linux-androideabi28 \
        -std=c++14 -O2 -g ${HOKI_DEBUG_PREFIX_MAP} -fno-short-enums -nostdinc++ \
        -isystem ${UNPACKDIR}/libcxx/include -I ${S}/vendor-headers \
        -nostdlib++ -L ${HOKI_VENDOR_LIBDIR} -Wl,--allow-shlib-undefined \
        ${S}/helper.cpp -o ${B}/hoki-location-helper -llocation_api -lc++ -ldl -llog
    ${NDK_TOOLCHAIN}/bin/clang --target=armv7a-linux-androideabi28 \
        -O2 -g ${HOKI_DEBUG_PREFIX_MAP} -Wall -Wextra ${S}/assistance.c -o ${B}/hoki-location-assist -ldl
}

do_install() {
    install -d ${D}${datadir}/licenses/hoki-location
    install -m 0644 ${S}/NOTICE ${D}${datadir}/licenses/hoki-location/
    install -m 0644 ${UNPACKDIR}/libcxx/LICENSE.TXT ${D}${datadir}/licenses/hoki-location/libcxx-LICENSE.TXT
    install -d ${D}${libexecdir} ${D}${systemd_user_unitdir}/geoclue-providers-hybris.service.d ${D}${libdir}/tmpfiles.d
    install -m 0755 ${B}/hoki-location-helper ${B}/hoki-location-assist ${D}${libexecdir}/
    install -m 0644 ${UNPACKDIR}/location-api.conf ${D}${systemd_user_unitdir}/geoclue-providers-hybris.service.d/
    install -m 0644 ${UNPACKDIR}/hoki-location.conf ${D}${libdir}/tmpfiles.d/
}
FILES:${PN} += "${systemd_user_unitdir} ${libdir}/tmpfiles.d ${datadir}/licenses/hoki-location"
