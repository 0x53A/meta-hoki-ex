<p align="center">
  <img src="assets/certified-slop.svg" alt="100% Certified Slop" width="640">
</p>

> [!NOTE]
> This project was largely LLM generated.

# Hoki hardware extensions

Additional Fossil Gen 6 (Hoki) hardware support on top of AsteroidOS
`meta-smartwatch/meta-hoki`. Includes kernel and vendor audio-module fixes,
ALSA/tinyalsa compatibility, PulseAudio hardware routing and DSP workarounds,
Bluetooth/WLAN integration, and the Hoki LocationAPI GPS backend.

Requires the `core`, `asteroid-layer`, and `hoki-hybris-layer` collections,
Whinlatter, and the upstream layers required by those recipes. It does not
depend on meta-nereid or select its custom UI.

## GPS builds

The GPS helpers, GeoClue backend, vendor headers and tests are source files in
[recipes-hoki/hoki-location/files/hoki-location](recipes-hoki/hoki-location/files/hoki-location/README.md).
BitBake builds the helpers with a pinned Android NDK native toolchain and fetched
Android 9 libc++ headers. android-system-data stages checksummed link libraries
from its existing upstream Hoki archive. No sibling project, local SDK or
precompiled helper is required. The NDK toolchain currently requires an x86-64
Linux build host.

```sh
bitbake hoki-location geoclue-provider-hybris-binder
```

PulseAudio hardware configuration is here; AirPlay and session-lifetime policy
are in meta-nereid. Source patches and existing authorship are preserved.
These are fork-local integrations, not a claim of upstream acceptance.
