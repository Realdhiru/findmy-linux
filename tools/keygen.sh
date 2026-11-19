#!/usr/bin/env bash
set -euo pipefail

# FindMy Linux - Apple Find My Keypair Generator
# Generates a standard NIST P-224 (secp224r1) keypair using native OpenSSL.
# Zero Python or external dependencies required.

KEY_DIR="${1:-.}"
mkdir -p "$KEY_DIR"

PRIV_PEM="$KEY_DIR/findmy_private.pem"
PUB_PEM="$KEY_DIR/findmy_public.pem"
PUB_RAW="$KEY_DIR/findmy_public.raw"

echo "==> Generating secp224r1 ECC private key..."
openssl ecparam -name secp224r1 -genkey -noout -out "$PRIV_PEM" 2>/dev/null
chmod 600 "$PRIV_PEM"

echo "==> Extracting public key coordinates..."
openssl ec -in "$PRIV_PEM" -pubout -out "$PUB_PEM" 2>/dev/null

# Extract raw uncompressed public point (57 bytes: 0x04 || X (28 bytes) || Y (28 bytes))
# In DER format, skipping headers yields raw public point bytes
openssl ec -in "$PRIV_PEM" -pubout -outform DER 2>/dev/null | tail -c 57 > "$PUB_RAW"

# Extract X-coordinate (bytes 1 to 28)
ADV_KEY_B64=$(dd if="$PUB_RAW" bs=1 skip=1 count=28 2>/dev/null | base64)

# Private key raw scalar (28 bytes)
PRIV_KEY_B64=$(openssl ec -in "$PRIV_PEM" -outform DER 2>/dev/null | tail -c 28 | base64)

echo "$ADV_KEY_B64" > "$KEY_DIR/advertisement.key"
chmod 644 "$KEY_DIR/advertisement.key"

echo "$PRIV_KEY_B64" > "$KEY_DIR/private.key"
chmod 600 "$KEY_DIR/private.key"

rm -f "$PUB_RAW" "$PUB_PEM"

echo ""
echo "================================================================="
echo " findmy-linux Keypair Generated Successfully!"
echo "================================================================="
echo ""
echo "1. Advertisement Key (Saved to: $KEY_DIR/advertisement.key):"
echo "   $ADV_KEY_B64"
echo "   (This is public. Your laptop broadcasts this key)."
echo ""
echo "2. Private Key (Saved to: $KEY_DIR/private.key):"
echo "   $PRIV_KEY_B64"
echo "   (KEEP THIS SECRET! Use this on your phone/tracker to decrypt location)."
echo "================================================================="
