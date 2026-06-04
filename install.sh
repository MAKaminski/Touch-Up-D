#!/bin/bash
# Touch Up D v3.1 — guided installer (run with sudo)
# Set NO_GUIDE=1 to skip the interactive permission walkthrough.
set -euo pipefail
[ "$EUID" -eq 0 ] || { echo "Run with: sudo bash install.sh"; exit 1; }
DIR="$(cd "$(dirname "$0")" && pwd)"
CONSOLE_USER="${SUDO_USER:-$(stat -f%Su /dev/console)}"
uopen() { sudo -u "$CONSOLE_USER" open "$@"; }

echo "[1/3] Compiling daemon..."
mkdir -p /usr/local/bin
clang -O2 -framework IOKit -framework ApplicationServices \
      -o /usr/local/bin/touchupd "$DIR/touchupd.c"
chown root:wheel /usr/local/bin/touchupd
chmod 755 /usr/local/bin/touchupd

echo "[2/3] Installing LaunchDaemon..."
cp "$DIR/de.modularequity.touchupd.plist" /Library/LaunchDaemons/
chown root:wheel /Library/LaunchDaemons/de.modularequity.touchupd.plist
chmod 644 /Library/LaunchDaemons/de.modularequity.touchupd.plist

echo "[3/3] Starting daemon..."
: > /var/log/touchupd.log
launchctl bootout system/de.modularequity.touchupd 2>/dev/null || true
launchctl bootstrap system /Library/LaunchDaemons/de.modularequity.touchupd.plist

if [ "${NO_GUIDE:-0}" = "1" ]; then
    echo "Installed. Grant /usr/local/bin/touchupd Input Monitoring + Accessibility"
    echo "in System Settings -> Privacy & Security. See SETUP.md."
    exit 0
fi

echo ""
echo "============================================================"
echo " macOS requires YOU to grant two permissions."
echo " Opening the setup guide and each System Settings window."
echo "============================================================"
uopen "$DIR/SETUP.md"
sleep 1

echo ""
echo ">>> WINDOW 1 of 2: \"Input Monitoring\""
echo "    (System Settings -> Privacy & Security -> Input Monitoring)"
echo "    Toggle 'touchupd' ON. If not listed: + -> Cmd+Shift+G -> /usr/local/bin/touchupd"
uopen "x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent"
echo -n "    Waiting for grant (up to 3 min, Ctrl+C to abort) "
GRANTED=0
for i in $(seq 1 60); do
    if grep -q "SUCCESS" /var/log/touchupd.log 2>/dev/null; then GRANTED=1; break; fi
    echo -n "."; sleep 3
done
echo ""
if [ "$GRANTED" = "1" ]; then
    echo "    ✓ Input Monitoring granted — device seized."
else
    echo "    ! Not detected yet — finish Window 1 using SETUP.md; the daemon retries every 10s."
fi

echo ""
echo ">>> WINDOW 2 of 2: \"Accessibility\""
echo "    (System Settings -> Privacy & Security -> Accessibility)"
echo "    Toggle 'touchupd' ON. If not listed: + -> Cmd+Shift+G -> /usr/local/bin/touchupd"
uopen "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"
echo -n "    Waiting for grant (up to 3 min, Ctrl+C to abort) "
GRANTED=0
for i in $(seq 1 60); do
    if grep -q "accessibility granted" /var/log/touchupd.log 2>/dev/null; then GRANTED=1; break; fi
    echo -n "."; sleep 3
done
echo ""
if [ "$GRANTED" = "1" ]; then
    echo "    ✓ Accessibility granted — clicks enabled."
    echo ""
    echo "============================================================"
    echo " Done. Touch your screen — taps land on the touchscreen and"
    echo " your cursor snaps back where it was. Survives reboots."
    echo "============================================================"
else
    echo "    ! Not detected yet — finish Window 2 using SETUP.md."
    echo "    The daemon picks it up automatically once granted. Log: /var/log/touchupd.log"
fi
