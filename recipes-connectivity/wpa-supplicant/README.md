# Hoki supplicant integration

The Hoki-only append builds candidate **2.11-r0.hoki2**, paired with ConnMan
**2.0-r6**. Native tests and ARM build/package QA pass; it has not been deployed.
The last hardware-validated supplicant remains 2.11-r0.hoki1.
These Hoki-only changes are packaged with MACHINE_ARCH, not generic ARM metadata.

1. Backport upstream 26c7f1bc107843f0ae57d35f3c9363367a0d11e7: report external-auth
   SAE over D-Bus, preserving original authorship.
2. Pin the selected AP for an optional-PMF association when the driver cannot
   represent optional MFP, then resolve PMF from that AP. Interface-wide BSS
   selection/roaming flags remain intact; explicit ap_scan=2 is unchanged.
3. Retry a rejected host-SAE request using legacy RSN encoding for partially
   backported kernels. The bounded retry preserves SAE, PMF and key parameters,
   excludes offload and caches only an accepted request.

The retired ConnMan PMF-disable setting is unnecessary. Remove any old true
override from existing installations. Earlier FRoST/Pixel/WPA2 hardware results
apply to the older production series, not the new association policy. Repeat
those checks, including traffic and roaming, before shipping this revision.

See the [iteration report](../../../_Tasks/0150_Radio_Review_Iterations/summary.md)
and [upstream drafts](../../../_Tasks/0150_Radio_Review_Iterations/upstream/README.md).
The two new hostap changes remain RFCs pending broader validation and maintainer
agreement; no patches were submitted.
