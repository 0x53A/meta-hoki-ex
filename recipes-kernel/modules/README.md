# Hoki Prima workqueue configuration

`linux-wlan-module-hoki_p.bbappend` adds `-DWLAN_OPEN_SOURCE` through the kernel's
module-only compiler flags. The driver already declares Dual BSD/GPL; this selects
its intended Linux workqueue and timer implementation rather than legacy fallbacks.
No kernel image or driver source patch is required for this correction.

Without this define, `hdd_smeCloseSessionCallback()` calls
`flush_scheduled_work()` before completing `session_close_comp_var`. The driver
comment explicitly warns that a global flush is unsafe; the open-source branch
omits this global flush. VOSS work helpers use individual cancellation in both
modes on this kernel (directly or through wcnss wrappers). Global work can itself wait for the interface
teardown whose caller is awaiting the session-close completion, creating a wait
cycle until `WLAN_WAIT_TIME_SESSIONOPENCLOSE` expires (15000ms).

Task0134 found the define absent from the actual compiled object command line,
captured the driver control thread in `flush_workqueue`, and recorded repeated
15-second airplane transitions with ConnMan in `hdd_stop_adapter`. The rebuilt
module has the define and imports `cancel_work_sync`/`cancel_delayed_work_sync`
instead of `flush_workqueue`. These are source and live evidence for correcting
the build mode; the detailed full wait graph was not recovered from kernel stacks.

Build integration belongs in meta-smartwatch's module recipe. Keep the append
until that is upstreamed. Package revision suffix `.hoki1` distinguishes this
build; the inherited base revision depends on the meta-smartwatch checkout.
Task0134 records tested artifacts, module backup, on-watch validation and limits.

## Qualcomm PCM partial writes

`linux-audio-modules-hoki_p.bbappend` patches the vendor PCM/Q6 modules built
against the existing 4.14.206 kernel. PulseAudio sends sub-period writes: the
vendor completion handler counted a whole period, queued pre-RUN writes lost
their lengths, and pointer wrapping discarded any remainder. The patch retains
actual lengths, snapshots completion size before recycling a DSP slot, and
keeps the hardware pointer bounded without losing bytes.

The patch changes shared internal audio structures. Build and deploy the entire
`linux-audio-modules-hoki` module set together, run `depmod`, and reboot. Do not
live-unload the DSP module stack. Preserve a copy of the installed modules and
watch logs before deployment. Task 0491 records live validation and rollback
artifacts; repeated playback and simultaneous recording must be checked.

The capture follow-up retains each DSP buffer until all samples are consumed,
loops over multiple buffers for larger ALSA reads, and returns real errors for
timeout/unavailable data. A stopped stream is flushed before its ring ownership
is reset. Task 0491 includes a deterministic copy-function test and an ARM ALSA
probe covering mixed read sizes, partial-buffer drop/prepare and reopen.
