#!/bin/bash
# =============================================================================
# AyuGram Pterodactyl Deployment - Full Setup Script
# =============================================================================
# This script builds the Docker image and optionally pushes it to Docker Hub.
#
# Prerequisites:
#   1. Docker installed and running
#   2. AyuGram binary (SandyGram) in the current directory
#   3. Docker Hub account (if pushing to registry)
#
# Usage:
#   ./deploy-pterodactyl.sh                    # Build locally
#   ./deploy-pterodactyl.sh push username      # Build and push to Docker Hub
#   ./deploy-pterodactyl.sh test               # Build and test locally
# =============================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
IMAGE_NAME="ayugram-pterodactyl"
IMAGE_TAG="latest"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() {
    echo -e "${GREEN}[$(date +%H:%M:%S)] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[$(date +%H:%M:%S)] WARNING: $1${NC}"
}

print_error() {
    echo -e "${RED}[$(date +%H:%M:%S)] ERROR: $1${NC}"
}

# Check prerequisites
check_prerequisites() {
    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed. Please install Docker first."
        exit 1
    fi

    if ! docker info &> /dev/null; then
        print_error "Docker is not running. Please start Docker."
        exit 1
    fi

    if [ ! -f "$SCRIPT_DIR/SandyGram" ]; then
        print_error "SandyGram binary not found in $SCRIPT_DIR"
        echo ""
        echo "Please build AyuGram first:"
        echo "  1. Clone the repository"
        echo "  2. Run the official build process"
        echo "  3. Copy the resulting SandyGram binary to this directory"
        exit 1
    fi
}

# Build the Docker image
build_image() {
    print_status "Building Docker image $IMAGE_NAME:$IMAGE_TAG..."
    
    docker build -t "$IMAGE_NAME:$IMAGE_TAG" "$SCRIPT_DIR"
    
    print_status "Image built successfully: $IMAGE_NAME:$IMAGE_TAG"
}

# Push to Docker Hub
push_image() {
    local username="$1"
    
    if [ -z "$username" ]; then
        print_error "Please provide your Docker Hub username"
        echo "Usage: $0 push YOUR_DOCKERHUB_USERNAME"
        exit 1
    fi
    
    local remote_tag="$username/$IMAGE_NAME:$IMAGE_TAG"
    
    print_status "Tagging image for Docker Hub: $remote_tag..."
    docker tag "$IMAGE_NAME:$IMAGE_TAG" "$remote_tag"
    
    print_status "Pushing to Docker Hub..."
    docker push "$remote_tag"
    
    print_status "Image pushed successfully!"
    echo ""
    echo "============================================="
    echo "  Docker Hub: $remote_tag"
    echo "  Update your Pterodactyl egg.json with:"
    echo "  \"image\": \"$remote_tag\""
    echo "============================================="
}

# Test locally
test_image() {
    print_status "Testing image locally..."
    
    docker run -d \
        --name ayugram-test \
        -p 8080:8080 \
        -p 5900:5900 \
        -e VNC_PASSWORD=test123 \
        "$IMAGE_NAME:$IMAGE_TAG"
    
    print_status "Container started! Testing..."
    
    # Wait for services to start
    sleep 5
    
    # Check if noVNC is running
    if curl -s http://localhost:8080 > /dev/null; then
        print_status "noVNC is accessible at http://localhost:8080/vnc.html"
    else
        print_warning "noVNC might not be ready yet. Wait a few seconds and try:"
        echo "  curl http://localhost:8080"
    fi
    
    echo ""
    echo "============================================="
    echo "  AyuGram is running!"
    echo "  noVNC: http://localhost:8080/vnc.html"
    echo "  VNC: localhost:5900"
    echo "  Password: test123"
    echo ""
    echo "  To stop: docker stop ayugram-test"
    echo "  To view logs: docker logs -f ayugram-test"
    echo "============================================="
}

# Main
case "${1:-build}" in
    build)
        check_prerequisites
        build_image
        echo ""
        echo "Image built successfully. To push to Docker Hub:"
        echo "  $0 push YOUR_DOCKERHUB_USERNAME"
        ;;
    push)
        check_prerequisites
        build_image
        push_image "$2"
        ;;
    test)
        check_prerequisites
        build_image
        test_image
        ;;
    *)
        echo "Usage: $0 {build|push|test}"
        echo ""
        echo "Commands:"
        echo "  build           Build Docker image locally"
        echo "  push <username> Build and push to Docker Hub"
        echo "  test            Build and test locally"
        exit 1
        ;;
esac
