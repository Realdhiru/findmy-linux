# findmy-linux

[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPL%203.0-blue.svg)](LICENSE)
[![Platforms: Linux](https://img.shields.io/badge/Platforms-Arch%20%7C%20Fedora%20%7C%20Ubuntu%20%7C%20Debian%20%7C%20NixOS-brightgreen.svg)]()

> **Universal, low-power, zero-dependency offline anti-theft tracking daemon for Linux.**  
> Turns any Linux laptop into an Apple AirTag using the 1.5+ billion iPhone crowd-sourced Find My network. Tracks your laptop even when it is **completely offline with zero Wi-Fi connection**.

---

## 🌟 Why findmy-linux?

Traditional anti-theft software (like Prey) requires an active internet connection to "phone home". When a thief steals your laptop, they don't have your Wi-Fi password, rendering traditional software completely useless.

`findmy-linux` solves this at the radio level:
* **Zero Internet Required:** Broadcasts raw Bluetooth Low Energy (BLE) packets containing cryptographic public keys.
* **Crowd-Sourced Mesh:** Any passing iPhone silently picks up the beacon, bundles its own GPS location, and uploads it to Apple's encrypted cloud.
* **End-to-End Encrypted:** Apple cannot read your location. Only you hold the private key to decrypt the coordinates.
* **100% Native C99:** Standalone 18 KB binary. Zero Python runtimes, zero pip dependencies, zero background bloat.
* **Ultra-Low Power:** Consumes $\approx 1.5\text{ mW}$ during 2-second pulses (< 0.2% battery drain over 24 hours).

---

## 📦 Universal Installation by Distribution

### 1. Arch Linux / Manjaro / EndeavourOS
```bash
# Install dependencies
sudo pacman -S --needed base-devel openssl

# Clone and install
git clone https://github.com/realdhiru/findmy-linux.git
cd findmy-linux
make
sudo make install
```

### 2. Ubuntu / Debian / Linux Mint / Pop!_OS
```bash
# Install dependencies
sudo apt update
sudo apt install -y build-essential libssl-dev

# Clone and install
git clone https://github.com/realdhiru/findmy-linux.git
cd findmy-linux
make
sudo make install
```

### 3. Fedora / RHEL / CentOS / AlmaLinux
```bash
# Install dependencies
sudo dnf groupinstall "Development Tools"
sudo dnf install -y openssl-devel

# Clone and install
git clone https://github.com/realdhiru/findmy-linux.git
cd findmy-linux
make
sudo make install
```

### 4. NixOS (Flake Input)
Add `findmy-linux` directly to your `flake.nix`:
```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    findmy-linux = {
      url = "github:realdhiru/findmy-linux";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, findmy-linux, ... }: {
    nixosConfigurations.yourhost = nixpkgs.lib.nixosSystem {
      modules = [
        findmy-linux.nixosModules.default
        {
          services.findmy-beacon.enable = true;
          services.findmy-beacon.interval = 2000;
        }
      ];
    };
  };
}
```

---

## 🚀 Quick Setup & Usage

### Step 1: Generate Your Keypair
Run the built-in native key generator:
```bash
# Generates keys in /etc/findmy (or any directory)
sudo mkdir -p /etc/findmy
sudo findmy-keygen /etc/findmy
```
* **`advertisement.key`**: Public 28-byte key broadcasted by your laptop.
* **`private.key`**: **Keep this secret!** Save this on your phone or backup drive to decrypt your laptop's location.

### Step 2: Enable the Service (systemd)
On standard distributions (Arch, Ubuntu, Fedora, Debian):
```bash
sudo cp systemd/findmy-beacon.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now findmy-beacon.service
```

### Step 3: Verify Status
```bash
systemctl status findmy-beacon.service
```
Your laptop is now actively broadcasting Apple Find My packets. Any passing iPhone will act as an anonymous location relay.

---

## 🗺️ How to Track Your Device

When your laptop is lost or stolen:
1. Use your `private.key` with any Apple Search Party query tool (like `tools/fetch.py` or the provided PWA radar).
2. It queries Apple's servers, decrypts the crowd-sourced reports, and provides an exact **Google Maps** and **Apple Maps** navigation pin.

---

## 🛠️ Project Architecture

```
findmy-linux/
├── src/
│   ├── main.c           # CLI entrypoint, argument parsing, signal handling
│   ├── hci.c            # Linux AF_BLUETOOTH / BTPROTO_HCI raw socket layer
│   ├── hci.h            # Vendor-neutral standard HCI opcodes
│   ├── beacon.c         # 31-byte Apple Find My GAP payload builder
│   ├── beacon.h         # Protocol definitions & Base64 routines
│   └── keygen.c         # Zero-dependency ECC secp224r1 key generator
├── systemd/
│   └── findmy-beacon.service # Hardened systemd unit file
├── tools/
│   ├── fetch.py         # Apple Cloud location report decrypter
│   ├── locate.sh        # Quick network triangulation test
│   └── server.sh        # Telemetry radar server
├── web/
│   └── index.html       # Clean OpenStreetMap Leaflet live radar
├── Makefile             # Universal POSIX C99 build system
├── flake.nix            # Flake definition for NixOS & Nix packages
├── default.nix          # Legacy Nix expression
└── README.md            # Documentation
```

---

## 📄 License

GPL-3.0-or-later. Open source and copyleft.
