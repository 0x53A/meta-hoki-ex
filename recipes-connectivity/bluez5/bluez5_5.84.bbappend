# Pin these source patches to the reviewed watch BlueZ version.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append:hoki = " file://0001-gatt-lifecycle-and-handle-allocation.patch file://0002-bound-discovery-cleanup-range.patch"
