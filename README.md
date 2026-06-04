# Touch Up D — v3.0

Tap-to-click for touchscreen monitors on macOS, done at the layer below WindowServer.

macOS has no native touchscreen support: it treats USB touch digitizers as absolute
pointing devices and moves the cursor on whatever display it's on — there is no
built-in way to bind touch to the touchscreen itself. User-space drivers can't fix
this on modern macOS because WindowServer keeps its own event connection to the
device even when an app "seizes" it.

**Touch Up D** is a root launchd daemon that:

1. **Seizes the touch device exclusively** (root + Input Monitoring) — macOS stops
   receiving its events entirely; the cursor no longer jumps when you touch.
2. **Parses the raw HID reports** (absolute X/Y + tip switch).
3. **Maps touches to the display you bind** and posts clicks there.
4. **Restores the cursor** to its pre-touch position on finger lift — your pointer
   never visibly leaves the screen you're working on.

## Install

```bash
sudo bash install.sh
```

Then grant `/usr/local/bin/touchupd` in **System Settings → Privacy & Security**:

| Permission | Why |
|---|---|
| Input Monitoring | exclusive (seized) access to the touch device |
| Accessibility | posting synthetic clicks |

Use **+** in each pane, press `Cmd+Shift+G`, type `/usr/local/bin/touchupd`.
The daemon retries every 10 s, so it starts working the moment both are granted.

## Configure

Flags (add to `ProgramArguments` in `/Library/LaunchDaemons/de.modularequity.touchupd.plist`):

| Flag | Purpose |
|---|---|
| `--vid 0x222a` / `--pid 0x335` | your touchscreen's USB vendor/product ID (`hidutil list` to find) |
| `--display 1280x480` | logical resolution of the display to bind (System Settings → Displays) |
| `--swap-xy`, `--invert-x`, `--invert-y` | axis calibration |
| `--no-restore-cursor` | leave the cursor where you tapped instead of snapping back |
| `-v` | verbose logging to `/var/log/touchupd.log` |

## Uninstall

```bash
sudo bash uninstall.sh
```

## Tested on

- Prechen 12.3" touchscreen (Techwin/ILITEK controller, VID `0x222a` PID `0x335`)
- macOS 26, Apple Silicon

## Credits

Inspired by and developed alongside [Touch-Up](https://github.com/shueber/Touch-Up)
by Sebastian Hueber (MIT). This project takes the seize-based approach his
TouchUpCore framework sketched and moves it into a root daemon where it actually
severs WindowServer's event delivery.

MIT License — see LICENSE.
