# AyuGram Headless Deployment Guide

## Prerequisites

- Linux VPS (tested on Rocky Linux 8, Ubuntu 22.04+, Debian 12)
- Docker + docker compose plugin
- Your own `api_id` and `api_hash` from [my.telegram.org](https://my.telegram.org/apps)

---

## Step 1: Build the binary

On your PC (or CI), run the official AyuGram Docker build:

```bash
# Clone the repo
git clone --recursive https://github.com/AyuGram/AyuGramDesktop.git
cd AyuGramDesktop

# Build inside the official centos_env container
docker run --rm -it \
    -u $(id -u) \
    -v "$PWD:/usr/src/tdesktop" \
    ghcr.io/telegramdesktop/tdesktop/centos_env:latest \
    /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
    -D TDESKTOP_API_ID=YOUR_API_ID \
    -D TDESKTOP_API_HASH=YOUR_API_HASH
```

The binary will be at `out/Release/SandyGram`.

---

## Step 2: Copy binary to deploy folder

```bash
cp out/Release/SandyGram deploy/
```

---

## Step 3: Build and deploy the runtime container

```bash
cd deploy
docker compose build
```

Copy the deploy folder to your VPS (or build directly there):

```bash
# On your VPS
docker compose up -d
```

---

## Step 4: Initial authentication (QR scan)

You need to log in once so the session token is saved. The container has no VNC by default, so the easiest method is to share the data directory with a local install.

**Option A — Login locally, then copy tdata:**

1. Run AyuGram normally on your PC
2. Log in, configure auto-reply rules
3. Copy `~/.local/share/SandyGram/tdata` to the VPS volume mount
4. Restart the container

**Option B — VNC into the headless desktop:**

Edit `docker-compose.yml` to add x11vnc:

```yaml
environment:
  - DISPLAY=:99
  - X11VNC_PASSWORD=yourpass
command: >
  sh -c "x11vnc -forever -display :99 -passwd $X11VNC_PASSWORD &
         xvfb-run -a -s '-screen 0 1024x768x24' /home/user/SandyGram --"
```

Install `x11vnc` in the Dockerfile:
```dockerfile
RUN dnf install -y x11vnc
```

Then VNC to `your-vps-ip:5900` to see the AyuGram window and scan QR.

---

## Step 5: Verify it's running

```bash
docker logs ayugram
docker ps
```

The container will restart automatically if the app crashes.

---

## How the auto-reply works

- The **real AyuGram Desktop binary** runs under xvfb (virtual display)
- All your auto-reply settings persist in the `ayugram-data` volume
- Telegram sees a normal desktop client — same network fingerprint, same MTProto layer
- At 1000+ messages/day, the ban risk is identical to running the app on your PC (effectively zero)
