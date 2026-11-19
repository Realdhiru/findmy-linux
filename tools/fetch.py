#!/usr/bin/env python3
import asyncio
import sys
import getpass
from datetime import datetime, timezone
from findmy import AppleAccount, KeyPair, LocalAnisetteProvider

async def main():
    if len(sys.argv) < 2:
        print("Usage: fetch.py <path_to_private.key> [apple_id]")
        sys.exit(1)

    key_path = sys.argv[1]
    with open(key_path, "r") as f:
        priv_b64 = f.read().strip()

    kp = KeyPair.from_b64(priv_b64)
    print("=================================================================")
    print(" 📡 FINDMY-LINUX APPLE CLOUD LOCATOR")
    print("=================================================================")
    print(f" Loaded Tracking Key:")
    print(f"  Public Key (b64)       : {kp.adv_key_b64}")
    print(f"  Search Party Hash (b64): {kp.hashed_adv_key_b64}")
    print("-----------------------------------------------------------------")

    apple_id = sys.argv[2] if len(sys.argv) > 2 else input("Apple ID: ")
    password = getpass.getpass("Apple Password: ")

    print("\n[*] Initializing local Apple Anisette provider...")
    anisette = LocalAnisetteProvider()
    account = AppleAccount(anisette)

    print(f"[*] Authenticating with Apple ID ({apple_id})...")
    await account.login(apple_id, password)

    print("[*] Fetching crowd-sourced reports from Apple Find My network...")
    reports = await account.fetch_reports([kp])

    if not reports:
        print("\n[-] No location reports uploaded to Apple yet.")
        print("    (Ensure your laptop is broadcasting with 'sudo ./findmy-beacon ...' and an iPhone has passed nearby)")
        return

    print(f"\n[+] Found {len(reports)} crowd-sourced report(s) from Apple mesh!")
    reports.sort(key=lambda r: r.timestamp, reverse=True)
    latest = reports[0]

    lat = latest.latitude
    lng = latest.longitude
    acc = latest.confidence
    ts = datetime.fromtimestamp(latest.timestamp, tz=timezone.utc).astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")

    print("=================================================================")
    print(" 📍 DECRYPTED APPLE FIND MY LOCATION REPORT")
    print("=================================================================")
    print(f" Last Seen   : {ts}")
    print(f" Coordinates : {lat:.6f}, {lng:.6f}")
    print(f" Accuracy    : ±{acc} meters")
    print("")
    print(f" 🗺️  Google Maps Link:")
    print(f"     https://www.google.com/maps?q={lat:.6f},{lng:.6f}")
    print("")
    print(f" 🍎  Apple Maps Link:")
    print(f"     https://maps.apple.com/?q={lat:.6f},{lng:.6f}")
    print("=================================================================")

if __name__ == "__main__":
    asyncio.run(main())
