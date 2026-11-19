# AIRGUARD: Universal Linux Offline Anti-Theft Tracking System

Lightweight, low-power, zero-dependency offline anti-theft tracking daemon for universal Linux distributions (NixOS, Arch, Ubuntu, Fedora, Debian) and any x86_64/AArch64 hardware (Intel, AMD, ARM) with a Bluetooth Low Energy (BLE) radio. Leverages Apple Find My (OpenHaystack) BLE beaconing, opportunistic Wi-Fi BSSID triangulation, and a live web map.

---

## 1. Portability & Universal Linux Architecture

AIRGUARD is intentionally designed with zero hardcoded distribution or hardware dependencies:
* **Operating Systems:** Universal Linux. Builds with any standard POSIX C99 compiler (`gcc`, `clang`, `musl-gcc`) and works on **Arch, Debian, Ubuntu, Fedora, Alpine, openSUSE, and NixOS**.
* **Hardware Support:** Any Bluetooth 4.0+ LE adapter (**Intel, Qualcomm, Realtek, Broadcom, Mediatek**) on x86_64, aarch64 (Raspberry Pi, Asahi Linux), or riscv64.
* **Init Systems:** Ship with both standard **systemd service units** and simple standalone CLI daemon mode (`--daemon` flag) for OpenRC/runit/s6.
* **Build System:** Universal standard `Makefile` (`make && sudo make install`) alongside optional `default.nix` for NixOS.

### Reference Hardware Profile (Developer Host)
* **Model:** ASUS Vivobook S 15 OLED (`K5504VA`).
* **SoC:** 13th Gen Intel Core i5-13500H.
* **BLE Controller:** Intel AX211 CNVi (`8087:0033`).
* **Kernel Sleep Default:** S0ix (`s2idle`) with USB wake persistence enabled.

---

## 2. Core Decisions & Invariants
1. **Always-On Beaconing:** Do not gate the beacon when awake; continuous background beaconing consumes $\approx 1.5\text{ mW}$ ($\approx 0.015\%$ of active draw). Run unconditionally when awake, locked, lid-closed, or suspended.
2. **Implementation Language:** Pure C99 with native Linux kernel Bluetooth HCI sockets (`socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)`).
   * **Rule:** Strict zero Python / zero Node in repository code (per `AGENTS.md`). No deprecated BlueZ tools (`hcitool`), no vendor-proprietary opcodes.
3. **Standby Power Architecture (`s2idle` + C10 Gating):**
   * Keep battery consumption comparable to `deep` suspend.
   * Enable USB wake/power on AX211 (`/sys/bus/usb/devices/3-10/power/wakeup = enabled`).
   * Force package $C10$ residency in `s2idle` with ASPM `powersupersave`. Bluetooth radio stays in autonomous BLE non-connectable broadcast while CPU cores enter low-power sleep ($<5\text{ mW}$ radio power, $<0.2\%$ battery loss per 24h).
4. **Interactive Live Map Tracking (No Static Telegram Spam):**
   * PWA / Mobile web dashboard using OpenStreetMap / Leaflet to display live marker, accuracy radius, location history, and "Mark as Stolen" toggle.
5. **Stateful "Lost Mode":**
   * Remote trigger via server bit (`stolen: true`).
   * When internet is acquired, device scans surrounding Wi-Fi BSSIDs (`nmcli`/`iw`), resolves coordinates via BeaconDB/MLS, streams updates to the map, and holds wake lock.

---

## 3. Component Architecture

```
~/Projects/nix-haystack/
├── Makefile                    # Standard POSIX build rules (CC ?= gcc)
├── default.nix                 # Standalone Nix derivation
├── README.md                   # Clean documentation for standalone open-source release
├── src/
│   ├── main.c                  # CLI entrypoint, signal handling, config loader
│   ├── hci.c                   # Kernel AF_BLUETOOTH / BTPROTO_HCI raw socket driver
│   ├── hci.h                   # Standard HCI opcodes (LE_Set_Adv_Params, LE_Set_Random_Addr)
│   ├── beacon.c                # Apple Find My 30-byte payload packing & address generator
│   └── beacon.h                # Struct definitions & secp224r1 key format
├── tools/
│   └── keygen.sh               # OpenSSL-based keypair generator (zero Python)
├── nixos/
│   ├── module.nix              # NixOS systemd service module (multi-user.target)
│   └── s2idle-tuning.nix       # Kernel ASPM & USB power persistence overrides
└── web/                        # Lightweight PWA Live Map Dashboard
    ├── index.html              # Leaflet / MapLibre interactive tracking map
    ├── worker.js               # Cloudflare Worker / micro-backend for report fetching
    └── app.js                  # Decrypts Apple Find My reports locally using private key
```

---

## 4. Technical Specifications & Protocols

### A. Apple Find My BLE Advertisement Frame (31 Bytes)
* **GAP Flags (3 bytes):** `0x02, 0x01, 0x1a` (LE General Discoverable, BR/EDR not supported).
* **Manufacturer Data Header (5 bytes):**
  * Length: `0x1e` (30 bytes).
  * Type: `0xff` (Manufacturer Specific Data).
  * Company ID: `0x4c, 0x00` (Apple Inc., Little Endian `0x004c`).
  * Type: `0x12` (Find My Offline Finding).
  * Sub-length: `0x19` (25 bytes).
* **Payload (23 bytes):**
  * Status Byte: `0x00` (Normal state).
  * Public Key slice: Bytes 6 to 27 of the 28-byte secp224r1 public key $x$-coordinate (22 bytes).
  * First 2 bits hint: `key[0] >> 6` (1 byte).
  * Hint Byte: `0x00` (1 byte).
* **BLE Device Address (6 bytes):**
  * `addr = key[0..5]` with `addr[0] |= 0b11000000` (Static Random Address).

### B. Standard HCI Opcodes Used (Vendor-Agnostic)
* `OGF 0x08`, `OCF 0x0005`: `HCI_LE_Set_Random_Address` (sets BLE static random address).
* `OGF 0x08`, `OCF 0x0006`: `HCI_LE_Set_Advertising_Parameters` (Interval: 2000ms–2500ms, non-connectable `ADV_NONCONN_IND`, channels `0x07`).
* `OGF 0x08`, `OCF 0x0008`: `HCI_LE_Set_Advertising_Data` (writes 31-byte GAP frame).
* `OGF 0x08`, `OCF 0x000a`: `HCI_LE_Set_Advertise_Enable` (`0x01` enable).

---

## 5. Execution Roadmap

1. **Phase 1: Native C Daemon (`nix-haystack`)**
   * Write `src/hci.c`, `src/beacon.c`, `src/main.c`.
   * Implement raw socket HCI communication directly with `hci0`.
   * Write `Makefile` and test compilation via `gcc`.
2. **Phase 2: Nix Derivation & System Integration**
   * Write `default.nix` derivation using `pkgs.stdenv.mkDerivation`.
   * Create `nixos/module.nix` running before user login (`multi-user.target`).
   * Test key loading from `/var/lib/nix-haystack/advertisement.key`.
3. **Phase 3: S2idle & Low-Power Standby Hardening**
   * Configure udev rule to set `/sys/bus/usb/devices/3-10/power/wakeup` to `enabled`.
   * Validate Intel package $C10$ state residency in `s2idle`.
4. **Phase 4: Client Tracking & Interactive Web Map**
   * Generate `tools/keygen.sh` (secp224r1 via OpenSSL CLI).
   * Implement frontend Leaflet map PWA consuming Apple Find My encrypted reports.
   * Add "Mark as Stolen" toggle and opportunistic Wi-Fi triangulation (`nmcli dev wifi list` -> BeaconDB).
