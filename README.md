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

## Current integration contract

This layer is extracted from 0x53A/asteroid-watch's meta-hoki-local. It preserves
the existing integration rather than converting external inputs to source recipes.
Check it out beside `hoki-location/`; its recipes consume the backend source and
locally built helper binaries there. The root repository's `tools/build-hoki.sh`
builds and synchronizes those inputs. A standalone layer clone is not enough
to build GPS. Hardware fixes remain scoped as in the original layer.

PulseAudio hardware configuration is here; AirPlay and session-lifetime policy
are in meta-nereid. Source patches and existing authorship are preserved.
These are fork-local integrations, not a claim of upstream acceptance.
