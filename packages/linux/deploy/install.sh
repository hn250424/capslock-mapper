#!/bin/bash

killall cm_runner 2>/dev/null
sleep 1
rm -f /usr/local/bin/cm /usr/local/bin/cm_runner
cp cm cm_runner /usr/local/bin/
chmod +x /usr/local/bin/cm /usr/local/bin/cm_runner
echo "Installation complete. Run 'cm --help'."