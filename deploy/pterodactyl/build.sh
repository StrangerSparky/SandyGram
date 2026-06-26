#!/bin/bash
# =============================================================================
# Build and Push AyuGram Docker Image for Pterodactyl
# =============================================================================
# Run this script after you have:
#   1. Built AyuGram binary (SandyGram) using the official build process
#   2. Copied SandyGram into this directory
#
# Usage:
#   ./build.sh                          # Build only (local)
#   ./build.sh push myusername          # Build and push to Docker Hub
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
IMAGE_NAME="ayugram-pterodactyl"
IMAGE_TAG="latest"

if [ ! -f "$SCRIPT_DIR/SandyGram" ]; then
    echo "ERROR: SandyGram binary not found in $SCRIPT_DIR"
    echo ""
    echo "Build AyuGram first:"
    echo "  docker run --rm -it \\"
    echo "      -v \"\$PWD/..:/usr/src/tdesktop\" \\"
    echo "      ghcr.io/telegramdesktop/tdesktop/centos_env:latest \\"
    echo "      /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \\"
    echo "      -D TDESKTOP_API_ID=YOUR_API_ID \\"
    echo "      -D TDESKTOP_API_HASH=YOUR_API_HASH"
    echo ""
    echo "Then copy the binary:"
    echo "  cp ../out/Release/SandyGram $SCRIPT_DIR/"
    exit 1
fi

echo "[build] Building $IMAGE_NAME:$IMAGE_TAG..."
docker build -t "$IMAGE_NAME:$IMAGE_TAG" "$SCRIPT_DIR"

echo "[build] Done. Image: $IMAGE_NAME:$IMAGE_TAG"

if [ "$1" = "push" ] && [ -n "$2" ]; then
    DOCKERHUB_USER="$2"
    REMOTE_TAG="$DOCKERHUB_USER/$IMAGE_NAME:$IMAGE_TAG"

    echo "[build] Tagging for Docker Hub: $REMOTE_TAG..."
    docker tag "$IMAGE_NAME:$IMAGE_TAG" "$REMOTE_TAG"

    echo "[build] Pushing to Docker Hub..."
    docker push "$REMOTE_TAG"

    echo ""
    echo "============================================="
    echo "  Pushed: $REMOTE_TAG"
    echo ""
    echo "  Update your egg.json with:"
    echo "  \"image\": \"$REMOTE_TAG\""
    echo "============================================="
else
    echo ""
    echo "To push to Docker Hub:"
    echo "  $0 push YOUR_DOCKERHUB_USERNAME"
fi
