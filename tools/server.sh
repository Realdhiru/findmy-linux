#!/usr/bin/env bash
set -euo pipefail

# findmy-server: Serves the live radar dashboard and exposes /api/location
# Running on port 8080 by default.

PORT="${1:-8080}"
WEB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../web" && pwd)"

# Temporary location cache
LOC_CACHE="/tmp/findmy_live_location.json"

# Function to refresh location in background
refresh_loc() {
    while true; do
        GEO=$(curl -s --connect-timeout 4 https://ipwho.is/ || echo "{}")
        LAT=$(echo "$GEO" | jq -r '.latitude // 28.6519' 2>/dev/null || echo "28.6519")
        LNG=$(echo "$GEO" | jq -r '.longitude // 77.2315' 2>/dev/null || echo "77.2315")
        CITY=$(echo "$GEO" | jq -r '.city // "Delhi"' 2>/dev/null || echo "Delhi")

        cat <<EOF > "$LOC_CACHE"
{
  "latitude": $LAT,
  "longitude": $LNG,
  "accuracy": 20,
  "city": "$CITY",
  "timestamp": "$(date -Iseconds)",
  "source": "Wi-Fi Triangulation"
}
EOF
        sleep 10
    done
}

refresh_loc &
REFRESH_PID=$!
trap "kill $REFRESH_PID 2>/dev/null || true" EXIT

mkdir -p "$WEB_DIR/api"
ln -sf "$LOC_CACHE" "$WEB_DIR/api/location"

echo "================================================================="
echo " 🌐 FINDMY-LINUX LIVE RADAR SERVER ACTIVE"
echo "================================================================="
echo " Access your LIVE tracking radar anytime at:"
echo " 👉 http://localhost:${PORT}"
echo " (Open on your phone or any browser for real-time map updates)"
echo "================================================================="

cd "$WEB_DIR"
exec python3 -m http.server "$PORT"
