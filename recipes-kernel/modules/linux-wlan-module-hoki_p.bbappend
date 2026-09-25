# The dual-BSD/GPL Prima driver must use its Linux workqueue implementation.
# Without this define it falls back to flush_scheduled_work(), including in the
# SME session-close callback while another thread waits for that callback.
CFLAGS_MODULE:append:hoki = " -DWLAN_OPEN_SOURCE"
EXTRA_OEMAKE:append:hoki = " 'CFLAGS_MODULE=${CFLAGS_MODULE}'"
PR:append:hoki = ".hoki2"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append:hoki = " file://0001-prima-initialize-station-remain-on-channel-work.patch"
