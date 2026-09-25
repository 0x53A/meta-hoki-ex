# Watch BlueZ corrections

`bluez5_5.84.bbappend` applies only to Hoki and the reviewed BlueZ 5.84 recipe.
It preserves the base recipe's patches. Rebase and repeat the tests when changing
BlueZ versions; do not rename this append to a wildcard without review.

The patch fixes external GATT registration/descriptor ownership, range overlap,
notification socket recovery for persistent bonded CCC state, and per-peer
asynchronous write/notification lifetimes. Actual ATT opcodes determine write
request completion and the D-Bus `type` option. Characteristic-aware allocation
rejects collisions instead of replacing existing attributes. The second patch
limits discovery cleanup to its requested handle range, preserving unrelated
services after Service Changed.

Source provenance: upstream BlueZ 5.84 release archive from
https://www.kernel.org/pub/linux/bluetooth/bluez-5.84.tar.xz, SHA-256
`5ba73d030f7b00087d67800b0e321601aec0f892827c72e5a2c8390d8c886b11`.
The target binary is built using the existing image recipe's compiler, sysroot,
configuration and prior patches, in a separate scratch build with the original
BitBake tree mounted read-only.

Validation and remaining review/deployment status are recorded in
[`_Tasks/0174_Bluetooth_Dependency_Fixes/bluez/summary.md`](../../../_Tasks/0174_Bluetooth_Dependency_Fixes/bluez/summary.md).
Radio patches have **not been submitted upstream**.
