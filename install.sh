#!/bin/bash
# Touch Up D v3.0 — installer (run with sudo)
set -euo pipefail
[ "$EUID" -eq 0 ] || { echo "Run with: sudo bash install.sh"; exit 1; }
DIR="$(cd "$(dirname "$0")" && pwd)"

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
launchctl bootout system/de.modularequity.touchupd 2>/dev/null || true
launchctl bootstrap system /Library/LaunchDaemons/de.modularequity.touchupd.plist

echo ""
echo "Installed. REQUIRED: grant /usr/local/bin/touchupd these permissions in"
echo "System Settings -> Privacy & Security (use '+' and press Cmd+Shift+G to type the path):"
echo "  1. Input Monitoring   (lets it seize the touchscreen)"
echo "  2. Accessibility      (lets it post clicks)"
echo "The daemon retries every 10s, so it starts working the moment both are granted."
echo "Log: /var/log/touchupd.log"
