#!/bin/bash
# Touch Up D v3.0 — uninstaller (run with sudo)
set -euo pipefail
[ "$EUID" -eq 0 ] || { echo "Run with: sudo bash uninstall.sh"; exit 1; }
launchctl bootout system/de.modularequity.touchupd 2>/dev/null || true
rm -f /Library/LaunchDaemons/de.modularequity.touchupd.plist
rm -f /usr/local/bin/touchupd
rm -f /var/log/touchupd.log
echo "Uninstalled. You may also remove touchupd from Input Monitoring and Accessibility in System Settings."
