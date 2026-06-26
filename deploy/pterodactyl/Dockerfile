# =============================================================================
# AyuGram Desktop for Pterodactyl Panel
# =============================================================================
# Includes xvfb + x11vnc + noVNC so you can scan QR code from your browser.
# =============================================================================

FROM rockylinux:8

# Install runtime deps + VNC
RUN dnf install -y epel-release && \
    dnf install -y \
        xorg-x11-server-Xvfb \
        xorg-x11-xauth \
        x11vnc \
        dejavu-sans-fonts \
        dejavu-sans-mono-fonts \
        fontconfig \
        dbus-daemon \
        mesa-dri-drivers \
        which \
        git \
        nodejs \
        npm \
        && \
    dnf clean all && \
    rm -rf /var/cache/dnf

# Install noVNC (browser-based VNC client)
RUN git clone --depth 1 https://github.com/novnc/noVNC.git /opt/noVNC && \
    git clone --depth 1 https://github.com/novnc/websockify.git /opt/noVNC/utils/websockify

# Create user
RUN useradd -m -u 1000 -s /bin/bash user

# Copy pre-built AyuGram binary
COPY --chown=user:user SandyGram /home/user/SandyGram
RUN chmod +x /home/user/SandyGram

# Copy startup scripts
COPY --chown=user:user entrypoint.sh /home/user/entrypoint.sh
RUN chmod +x /home/user/entrypoint.sh

USER user
WORKDIR /home/user

# noVNC web port + VNC internal port
EXPOSE 8080 5900

ENTRYPOINT ["/home/user/entrypoint.sh"]
