#!/bin/bash

killall cm_runner 2>/dev/null

if [ -f /etc/systemd/system/cm.service ]; then
    systemctl stop cm 2>/dev/null
    systemctl disable cm 2>/dev/null
    rm -f /etc/systemd/system/cm.service
    systemctl daemon-reload
fi

rm -f /usr/local/bin/cm
rm -f /usr/local/bin/cm_runner
rm -f /run/cm.pid

echo "Uninstallation complete."