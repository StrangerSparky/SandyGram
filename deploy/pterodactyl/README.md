# AyuGram Pterodactyl Deployment Guide

## Overview

Deploy AyuGram Desktop as an auto-reply bot on your Pterodactyl panel. This setup runs the **real desktop client** with xvfb + noVNC, so you can scan QR codes and configure settings from your browser.

**Why this is safe:**
- Uses the actual AyuGram Desktop binary (same MTProto layer as the real app)
- Identical network fingerprint to a normal user running Telegram Desktop
- No custom API libraries, no scripts, no suspicious behavior
- Telegram sees a desktop client that never logs out

---

## Step 1: Build the AyuGram Binary

On your PC (Windows/Linux/Mac):

```bash
git clone --recursive https://github.com/AyuGram/AyuGramDesktop.git
cd AyuGramDesktop

# Build using the official Docker environment
docker run --rm -it \
    -v "$PWD:/usr/src/tdesktop" \
    ghcr.io/telegramdesktop/tdesktop/centos_env:latest \
    /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
    -D TDESKTOP_API_ID=YOUR_API_ID \
    -D TDESKTOP_API_HASH=YOUR_API_HASH
```

The binary will be at `out/Release/SandyGram` (Linux) or `out/Release/SandyGram.exe` (Windows).

---

## Step 2: Prepare the Docker Image

```bash
cd deploy/pterodactyl

# Copy the binary
cp /path/to/SandyGram .

# Make the build script executable
chmod +x *.sh

# Build the Docker image
./deploy-pterodactyl.sh build

# Push to Docker Hub (required for Pterodactyl)
./deploy-pterodactyl.sh push YOUR_DOCKERHUB_USERNAME
```

After pushing, you'll see:
```
Image pushed successfully!
Docker Hub: yourusername/ayugram-pterodactyl:latest
```

**Note this URL** — you'll need it for the egg configuration.

---

## Step 3: Import the Egg into Pterodactyl

1. Log in to your Pterodactyl admin panel
2. Go to **Admin** → **Nests** → **Create New**
   - Name: `AyuGram`
   - Description: `AyuGram Auto-Reply Bot`
3. Click the nest → **Create New Egg**
4. Click **Import Egg** (or manually edit)
5. Upload `egg.json` from `deploy/pterodactyl/`
6. **Important:** Edit the egg and update the `image` field with your Docker Hub image:
   ```json
   "image": "yourusername/ayugram-pterodactyl:latest"
   ```

---

## Step 4: Create a Server

1. In Pterodactyl admin, go to **Servers** → **Create New**
2. Select your AyuGram nest
3. Select `AyuGram Auto-Reply` egg
4. Fill in:
   - **Server Name:** `ayugram-bot`
   - **User:** Select or create a user
   - **Memory:** 512MB minimum
   - **Disk:** 1GB minimum
   - **Allocation:** Leave default (port 8080 will be assigned)
5. Click **Create Server**

---

## Step 5: Initial Setup (QR Code Login)

1. Start the server in Pterodactyl
2. Wait ~30 seconds for all services to start
3. In the server console, you'll see:
   ```
   noVNC: http://localhost:8080/vnc.html
   VNC: localhost:5900
   Password: (your VNC_PASSWORD)
   ```
4. Open the noVNC URL in your browser
5. You'll see the AyuGram desktop
6. Log in with your phone number or QR code
7. Go to **Settings** → **Auto-Reply** and configure your trigger rules

---

## Step 6: Configure Auto-Reply

In the AyuGram settings (via noVNC):

1. Go to **Settings** → **AyuGram** → **Auto-Reply**
2. Enable auto-reply
3. Set trigger rules (format: `word=shortcut_name`):
   ```
   hello=greeting
   help=support
   price=pricing
   ```
4. Configure typing simulation and delays
5. Save settings

---

## Step 7: Lock Down (Optional)

Once configured, you can:
- Disable noVNC by editing the egg's startup command
- Set a VNC password for security
- Monitor via the Pterodactyl console

---

## Troubleshooting

### Server shows "Offline" immediately
- The server might need more memory (try 1024MB)
- Check if noVNC is starting properly in the console

### Can't connect to noVNC
- Make sure port 8080 is allocated in Pterodactyl
- Try accessing directly: `http://YOUR_SERVER_IP:8080/vnc.html`

### AyuGram crashes on startup
- Check logs: `docker logs ayugram-server-name`
- Make sure the binary is compatible with Rocky Linux 8

### Auto-reply not working
- Verify trigger rules are configured correctly
- Check that shortcuts (quick replies) are set up in Telegram
- Look at the AyuGram logs in the console

---

## Files Included

| File | Purpose |
|------|---------|
| `Dockerfile` | Docker image with xvfb, x11vnc, noVNC |
| `entrypoint.sh` | Starts all services in order |
| `build.sh` | Builds and pushes Docker image |
| `deploy-pterodactyl.sh` | Full deployment script |
| `egg.json` | Pterodactyl egg configuration |

---

## Security Notes

- The VNC connection is unencrypted by default
- Set a strong VNC_PASSWORD for remote access
- Consider using a firewall to restrict access to port 5900
- The auto-reply runs as the real Telegram Desktop client — same risk as running it on your PC

---

## Resource Usage

| Resource | Minimum | Recommended |
|----------|---------|-------------|
| Memory | 512MB | 1024MB |
| Disk | 1GB | 2GB |
| CPU | 1 core | 2 cores |

The AyuGram binary itself uses ~100-200MB RAM. xvfb and VNC add ~50-100MB overhead.
