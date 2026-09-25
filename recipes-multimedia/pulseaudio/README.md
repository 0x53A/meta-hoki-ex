# Hoki PulseAudio hardware support

The mono microphone and stereo speaker use the DSP copy PCM path with mmap
and timer scheduling disabled for the sink. The rewind safeguard exceeds the
maximum DSP ring size. The patches propagate zero rewind capacity and coalesce
client writes to avoid exhausting DSP packet slots. The corresponding vendor
module fixes are in recipes-kernel/modules.

The speaker-routing service comes from meta-smartwatch/meta-hoki. AirPlay and
PulseAudio session-lifetime settings live in meta-nereid. See task 0491 in the
parent repository for validation evidence and deployment limits.
