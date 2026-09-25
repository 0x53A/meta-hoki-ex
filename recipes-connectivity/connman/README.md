# ConnMan 2.0 and Hoki Wi-Fi fixes

The version override and generic ConnMan patches now live in
`meta-asteroid/recipes-connectivity/connman/connman_%.bbappend`, reusing
OE-Core packaging with pinned ConnMan 2.0 sources. The candidate is `2.0-r7`;
Hoki rfkill ownership and package architecture live in the
`meta-smartwatch/meta-hoki` ConnMan append and remain machine-specific.
The generic interface-readiness fix moved there from meta-smartwatch too.
The version/patch relocation preserves patch contents; it is not new hardware
validation. The historical validation record below describes earlier revisions.
The last hardware-validated package remains `2.0-r4`. R6 removes the unused
PMF-disable option, fixes aggregate offline/hard-block handling, and fails closed
on unreadable Bluetooth ownership names. Native tests and ARM build/package QA
pass; r6 has not been deployed.
Hoki's binary package uses MACHINE_ARCH because the physical rfkill policy is
device-specific.

- Authentication is filtered against supplicant capabilities and respects PMF.
- The obsolete `WiFiDisableProtectedManagementFrames` escape hatch is removed.
  Optional PMF remains the normal policy; the retired patch is archived in task0146.
- Device power requests survive an asynchronous opposite transition; the latest
  request is reconciled after completion, with owned work cancelled on removal.

Pair this with the local `wpa-supplicant` append: it backports upstream SAE
reporting, resolves optional PMF by pinning the selected AP for affected
associations, and handles the vendor kernel's partial SAE backport without changing
the negotiated authentication algorithm. Removing the old PMF workaround alone
is insufficient. Existing installations must also remove the old true setting
from `/etc/connman/main.conf` after upgrading both packages.

No credentials or SSID-specific settings are included. The native tests and ARM
package QA pass. **Deployed r2 passes FRoST SAE/PMF, 20 rapid Wi-Fi sequences, reconnects
and reboot persistence. Pixel mixed-mode SAE and WPA2 regression checks pass.** See [task 0127](../../../_Tasks/0127_WiFi_Production_Fixes/summary.md)
and its [upstreaming notes](../../../_Tasks/0127_WiFi_Production_Fixes/UPSTREAMING.md).

Hoki r3 also excludes the physical `bt_power` rfkill from ConnMan ownership and
uses indexed Bluetooth requests for virtual controllers. Android's HAL owns
physical sequencing through `/dev/btpower`; a broadcast rfkill change cuts rails
before its pre-shutdown command, causing wake failure and HAL self-SIGKILL.
The patch is Hoki-only; other rfkill types keep their original behavior. Pair it
with bluebinder r1.hoki7's asynchronous lifecycle fixes. This is device integration;
a generic upstream version would need a configurable ownership policy. See
[task0134](../../../_Tasks/0134_Bluetooth_Root_Cause/summary.md).

Hoki's excluded device is the software-only `bt_power` platform rfkill; the kernel
source does not attach a hardware switch to it. The patch does not exclude other
Bluetooth rfkill devices or hard-block flags on the managed virtual controller.
Use Settings/ConnMan for policy: unrelated tools broadcasting directly to all
kernel Bluetooth rfkill devices can still bypass this ownership arrangement.

R4 also blocks newly-added unblocked rfkill controllers while OfflineMode is
active, before announcing them powered. This preserves airplane mode when
bluebinder recreates hci0 without relying on the kernel broadcast default.

R6 preserves hard-block metadata for mixed controllers and suppresses transient
enabling when a hardware block is released offline, while retaining explicit
soft-block overrides. Unreadable Bluetooth names return an error without writing
to a possibly HAL-owned device; DEL still works after sysfs removal.

See the [completed iterative review](../../../_Tasks/0150_Radio_Review_Iterations/summary.md)
for fixes, submission drafts, validation and remaining live-test requirements.
