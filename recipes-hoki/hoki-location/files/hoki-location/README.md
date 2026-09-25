# Hoki LocationAPI backend for GeoClue 0

Existing QtPositioning applications keep the same GeoClue master/provider interfaces. The Hoki-selected provider backend runs an isolated Android-native helper as `ceres`, using the installed Qualcomm libraries. No vendor library is replaced. The Hoki overlay builds only the LocationAPI backend; Binder AIDL/HIDL backends are excluded. The upstream package and D-Bus names remain unchanged for client compatibility.

## Components

- `helper.cpp`: control enable, tracking, interval updates, positions/satellites, stop/disable/destruction. Private versioned pipe protocol; no network listener. A lock under the ceres runtime directory prevents overlapping copies. EOF, parent death and termination initiate cleanup, with a hard watchdog if vendor calls hang. Standard output contains coordinates and must remain a private pipe, not a journal sink.
- `backend/`: Qt backend for geoclue-providers-hybris. Existing provider owns D-Bus clients; backend owns one helper while references exist. It reports acquisition/errors and retries helper failures up to three attempts per client session. Coordinates/vendor diagnostics are not forwarded to the journal.
- `assistance.c`: short-lived Android-native QCCI client. Requires transport/LOC success, matching chunk acknowledgements, final nonzero constellation mask and current modem-reported validity. Callback data is copied under its mutex. No engine control or modem NV/firmware changes.
- Assistance runs after receiver startup, checks `NTPSynchronized` through timedate1 before UTC injection, and downloads orbit data over verified HTTPS. Cache `/var/cache/hoki-location/xtra.bin` is private to ceres. Reuse is limited to24hours and file size1MiB. Network/injection failures leave standalone acquisition running. Download retries are bounded and canceled on last-client stop. No SUPL implementation or forced global power mode is added.

Backend conversion preserves GeoClue0 speed in knots, timestamps in seconds on D-Bus, and constellation PRN conventions. Zero-signal satellite inventory is reported honestly as zero SNR, not evidence of reception. Hardware coordinates are range/validity checked; exact positioning still depends on receiver reception.

## Build and package

From an initialized AsteroidOS BitBake build with meta-hoki-ex enabled:

```sh
bitbake hoki-location geoclue-provider-hybris-binder
```

All project source is in this layer. The helper recipe fetches the pinned
Android 9 libc++ headers at `bc21e7a0d91e21c042d9e303571a2c5da5dad613` and depends
on the checksummed Android NDK r29 native recipe. Both helpers compile in
`do_compile`; no local SDK, source checkout beside the layer, prebuilt helper,
or access to a watch is needed.

The existing android-system-data recipe supplies the four vendor/platform
link inputs through its sysroot. Each is checked against the verified Hoki
archive manifest before staging. The library bytes are the same as the former
local link-libs inputs; no proprietary library is rebuilt or replaced.

The binaries retain Android/bionic API 28 and `/system/bin/linker`, with the
existing enum sizes and Android 9 C++ ABI. Do not apply the Rust app patchelf
procedure or substitute Yocto's glibc compiler. The GeoClue backend continues
to build with the regular Qt/qmake recipe and invokes the isolated helpers.

Source tests are retained in tests/. Vendor header provenance and licenses
are in NOTICE; the libc++ license is fetched with its headers and installed
with the helper notices. Historical test records refer to the former root
hoki-location/ directory.

## Operational scope

Run only one backend controlling GNSS. Do not run standalone diagnostic probes or an independent HAL tracking session while this backend owns GNSS. The default provider stays a ceres user service; helpers need no root service. No continuous wake lock is taken. Suspend/resume, low-power modes and outdoor acquisition timing require separate validation. This implementation does not implement `NoCachedAidingData` deletion, SUPL, coarse-location injection or NI responses; it logs unsupported deletion rather than pretending success.

Tests and deployment evidence: `asteroid-watch/_Tasks/0151_GeoClue_LocationAPI/`. No full image flash is required for installation. Rollback now requires rebuilding/reinstalling the original provider and Android runtime/vendor packages: there is no environment-switch fallback. The overlay removes GNSS HAL executables, implementations, launchers, VINTF declarations and garden diagnostics. It retains GNSS HIDL interface libraries referenced by shipped Android libraries, the Qualcomm LocationAPI/engine/QMI stack, GNSS configuration and firmware, and shared Android infrastructure. Cleanup build evidence is in `asteroid-watch/_Tasks/0153_GNSS_Image_Cleanup/`.

Whole-stack iterative review and regression evidence: [`asteroid-watch/_Tasks/0160_GPS_Stack_Review`](https://github.com/0x53A/asteroid-watch/blob/main/_Tasks/0160_GPS_Stack_Review/summary.md). Backend tests exercise late interval acknowledgements, shutdown deadlines, helper-generation invalidation and large assistance diagnostics; native mock-QMI tests separate UTC injection success from orbit-query failure.
