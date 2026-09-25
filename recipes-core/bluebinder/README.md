# Hoki Bluetooth HAL lifecycle

The local candidate is **r1.hoki7**, paired with ConnMan **2.0-r6**. Native
regressions and ARM build/package QA pass. It has not been deployed: the last
hardware-validated bluebinder remains r1.hoki6. See the
[iteration report](../../../_Tasks/0150_Radio_Review_Iterations/summary.md).

Two patches remain exact upstream backports: initialization-state checking
(5166af8) and rfkill callback dispatch (1e8f5de). The lifecycle patch now:

- Sends INITIALIZE and CLOSE asynchronously, servicing synchronous HAL callbacks.
- Waits for both the initialize method reply and completion callback, in either
  order, before reconciling the latest power request.
- Preserves initial rfkill policy while completing mandatory kernel HCI setup.
  An early stop waits for setup; closed-session traffic is discarded.
- Cancels owned lifecycle transactions and exits nonzero on transport failure.
  The supplied service recreates the proxy/vhci session after HAL death; it does
  not reconnect a replacement HAL to stale HCI state or issue synchronous CLOSE
  after stopping the main loop.

A scheduled eight-second deadline covers initialization plus initial HCI setup;
CLOSE has an operation deadline, and an independent scheduled eight-second bound
covers the whole signal-driven shutdown. Hoki retains TimeoutStopSec=10. These
are main-loop deadlines, not guarantees against kernel stalls or scheduling delay.

The ExecStartPre helper starts a stopped Android oneshot HAL once and leaves a
running HAL alone. It remains recovery infrastructure. ConnMan owns virtual HCI
policy; Android owns physical bt_power sequencing. These device choices remain
separate from the generic lifecycle patch.

The full-source native suite covers pinned HIDL and a current-upstream HIDL/AIDL
port. It mocks Binder transport; AIDL and the new Hoki revision have no new live
hardware validation. Recheck toggles, initial-off boot and service/death recovery
before deployment/submission. Upstream draft files are in task0150; no submission
or author sign-off has been made. Older task0134 tests describe earlier fixtures.
