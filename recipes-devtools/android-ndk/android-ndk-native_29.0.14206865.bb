SUMMARY = "Pinned Android NDK for Hoki's Android/bionic helper builds"
HOMEPAGE = "https://developer.android.com/ndk"
# Precompiled upstream toolchain distribution; retain its bundled notices.
LICENSE = "CLOSED"
SRC_URI = "https://dl.google.com/android/repository/android-ndk-r29-linux.zip"
SRC_URI[sha256sum] = "4abbbcdc842f3d4879206e9695d52709603e52dd68d3c1fff04b3b5e7a308ecf"
S = "${UNPACKDIR}/android-ndk-r29"
inherit native
COMPATIBLE_HOST = "x86_64.*-linux"
INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_STRIP = "1"
do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    install -d ${D}${libdir}/android-ndk
    cp -R --no-preserve=ownership ${S}/toolchains ${D}${libdir}/android-ndk/
    install -m 0644 ${S}/NOTICE ${S}/NOTICE.toolchain ${S}/source.properties ${D}${libdir}/android-ndk/
}
