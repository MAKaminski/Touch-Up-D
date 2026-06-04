# Touch Up D — Setup Guide

The installer just placed the daemon at `/usr/local/bin/touchupd` and started it.
macOS now requires **you** to grant it two permissions — Apple does not allow any
installer to grant these for you. The installer opens each window for you and
detects automatically when you've finished each step.

---

## WINDOW 1 — "Input Monitoring"

> System Settings → Privacy & Security → **Input Monitoring**
> (the installer has already opened this window)

This permission lets the daemon take *exclusive* control of your touchscreen so
macOS stops bouncing the cursor around when you touch.

**In this window:**

1. Look for **touchupd** in the list. If it's already there, just switch its toggle **ON** and you're done with this window.
2. If it's not listed: click the **+** button below the list.
3. In the file picker, press **Cmd + Shift + G** (Go to Folder).
4. Type exactly: `/usr/local/bin/touchupd` and press **Return**, then click **Open**.
5. Switch the **touchupd** toggle **ON**.

The installer's Terminal output will print `✓ Input Monitoring granted — device seized`
within a few seconds of the toggle. Then move to Window 2.

---

## WINDOW 2 — "Accessibility"

> System Settings → Privacy & Security → **Accessibility**
> (the installer opens this window after Window 1 is detected)

This permission lets the daemon post the actual clicks where your finger lands.

**In this window:**

1. Look for **touchupd** in the list — toggle it **ON** if present.
2. If it's not listed: click **+** → press **Cmd + Shift + G** → type
   `/usr/local/bin/touchupd` → **Return** → **Open** → toggle **ON**.

The installer prints `✓ Accessibility granted — clicks enabled` when this lands.

---

## Test

Touch your screen. The tap should land **on the touchscreen** — and your mouse
cursor should snap back to wherever it was the moment you lift your finger.

## Troubleshooting

| Symptom | Fix |
|---|---|
| Log shows `open(seize) -> 0xe00002e2 FAIL` | Window 1 (Input Monitoring) not granted yet — the daemon retries every 10 s |
| Cursor stays still but taps don't click | Window 2 (Accessibility) not granted yet |
| Taps land mirrored / rotated | Add `--invert-x`, `--invert-y`, or `--swap-xy` to the plist's `ProgramArguments` (see README) |
| Wrong display | Add `--display WxH` matching your touchscreen's logical resolution |

Log file: `/var/log/touchupd.log`
Daemon status: `sudo launchctl print system/de.modularequity.touchupd`
