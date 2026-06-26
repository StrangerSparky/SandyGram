# =============================================================================
# AyuGram Desktop - Headless Deployment (Runtime Only)
# =============================================================================
# Usage:
#   1. Build the binary first (see BUILD.md in this directory):
#      docker run --rm -it \
#          -v "$PWD/..:/usr/src/tdesktop" \
#          ghcr.io/telegramdesktop/tdesktop/centos_env:latest \
#          /usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh \
#          -D TDESKTOP_API_ID=YOUR_API_ID \
#          -D TDESKTOP_API_HASH=YOUR_API_HASH
#
#   2. Build this runtime image:
#      docker build -t ayugram-headless .
#
#   3. Run:
#      docker run -d --name ayugram \
#          -v ayugram-data:/home/user/.local/share/SandyGram \
#          ayugram-headless
# =============================================================================

FROM rockylinux:8 AS runtime

# Install only what's needed to run AyuGram
RUN dnf install -y \
        epel-release && \
    dnf install -y \
        xorg-x11-server-Xvfb \
        xorg-x11-xauth \
        dejavu-sans-fonts \
        dejavu-serif-fonts \
        dejavu-sans-mono-fonts \
        fontconfig \
        dbus-daemon \
        mesa-dri-drivers \
        which \
        && \
    dnf clean all && \
    rm -rf /var/cache/dnf

# Create non-root user to match the binary's expected home path
RUN useradd -m -u 1000 user

# Copy the pre-built AyuGram binary from the build output
# Build it first using the instructions in BUILD.md, then copy:
#   cp ../out/Release/SandyGram .
COPY --chown=user:user SandyGram /home/user/SandyGram

# Required so the binary is actually executable
RUN chmod +x /home/user/SandyGram

# Copy the entrypoint
COPY --chown=user:user entrypoint.sh /home/user/entrypoint.sh
RUN chmod +x /home/user/entrypoint.sh

USER user
WORKDIR /home/user

ENTRYPOINT ["/home/user/entrypoint.sh"]
