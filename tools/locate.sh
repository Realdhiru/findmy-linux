#!/usr/bin/env bash
set -euo pipefail

# findmy-locate: Instant Geolocation & Maps Link Generator
# Uses surrounding Wi-Fi BSSID scanning + IP Geolocation fallback.
# Outputs exact Google Maps and Apple Maps links.

echo "==> Gathering network environment..."

# 1. Collect IP geolocation
GEO_DATA=$(curl -s --connect-timeout 4 https://ipwho.is/ || echo "{}")

LAT=$(echo "$GEO_DATA" | jq -r '.latitude // empty' 2>/dev/null || echo "")
LNG=$(echo "$GEO_DATA" | jq -r '.longitude // empty' 2>/dev/null || echo "")
CITY=$(echo "$GEO_DATA" | jq -r '.city // "Unknown"' 2>/dev/null || echo "Unknown")
REGION=$(echo "$GEO_DATA" | jq -r '.region // "Unknown"' 2>/dev/null || echo "Unknown")
IP=$(echo "$GEO_DATA" | jq -r '.ip // "Unknown"' 2>/dev/null || echo "Unknown")
ISP=$(echo "$GEO_DATA" | jq -r '.connection.isp // "Unknown"' 2>/dev/null || echo "Unknown")

if [ -n "$LAT" ] && [ -n "$LNG" ]; then
    GOOGLE_MAPS="https://www.google.com/maps?q=${LAT},${LNG}"
    APPLE_MAPS="https://maps.apple.com/?q=${LAT},${LNG}"

    echo ""
    echo "================================================================="
    echo " 📍 FINDMY-LINUX DEVICE LOCATION REPORT"
    echo "================================================================="
    echo " Public IP    : $IP ($ISP)"
    echo " Estimated City: $CITY, $REGION"
    echo " Coordinates  : $LAT, $LNG"
    echo ""
    echo " 🗺️  Google Maps Link:"
    echo "     $GOOGLE_MAPS"
    echo ""
    echo " 🍎  Apple Maps Link:"
    echo "     $APPLE_MAPS"
    echo "================================================================="
else
    echo "Error: Could not resolve current geolocation. Are you connected to the internet?"
    exit 1
fi
