#!/bin/bash
# =============================================================================
# Docker-based Build Script for Vitals Monitor
# =============================================================================
#
# CDSCO Class B Medical Device — Vitals Monitor
# Target: STM32MP157F-DK2 (Cortex-A7 + Cortex-M4)
#
# PURPOSE:
#   Provides a reproducible build environment using Docker.  The container
#   includes a complete Buildroot toolchain cross-compiling for the
#   STM32MP157F-DK2 (ARM Cortex-A7, hard-float).  This ensures:
#     - Identical builds across developer machines and CI/CD
#     - No host toolchain contamination
#     - Auditable build environment (pinned container image)
#
# REGULATORY CONTEXT:
#   IEC 62304 Section 5.8.5  — Build environment reproducibility
#   IEC 62304 Section 8.1.2  — Configuration management of tools
#   CDSCO Class B            — Documented, reproducible build process
#
# USAGE:
#   ./scripts/docker-build.sh [command]
#
#   Commands:
#     all         Build everything (default)
#     clean       Remove all build artifacts
#     rebuild     Clean + build
#     menuconfig  Open Buildroot menuconfig (interactive)
#     sdk         Build and export the SDK tarball
#     shell       Drop into an interactive shell in the container
#
# PREREQUISITES:
#   - Docker (or Podman with docker alias)
#   - At least 15 GB free disk space (Buildroot downloads + build artifacts)
#   - Internet access (first build downloads toolchain and packages)
#
# =============================================================================
set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Docker image name and tag — pin to a specific version for reproducibility.
# The Dockerfile is in buildroot-external/docker/ and builds on top of a
# minimal Ubuntu base with the Buildroot prerequisites installed.
DOCKER_IMAGE="vitals-monitor/buildroot"
DOCKER_TAG="2024.02"
DOCKER_FULL="${DOCKER_IMAGE}:${DOCKER_TAG}"

# Paths (relative to project root)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILDROOT_EXT="$PROJECT_ROOT/buildroot-external"
DOCKERFILE="$BUILDROOT_EXT/docker/Dockerfile"
OUTPUT_DIR="$PROJECT_ROOT/output"

# Buildroot defconfig for STM32MP157F-DK2
DEFCONFIG="stm32mp157f_dk2_vitals_defconfig"

# Number of parallel make jobs (use all cores in container)
NPROC="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

# ---------------------------------------------------------------------------
# Color output helpers
# ---------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${GREEN}[BUILD]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }
step()  { echo -e "${CYAN}[STEP]${NC}  $*"; }
die()   { error "$*"; exit 1; }

# ---------------------------------------------------------------------------
# Docker image management
# ---------------------------------------------------------------------------

# Build or pull the Docker image containing the Buildroot toolchain.
# If a Dockerfile exists in the external tree, build from that;
# otherwise, create a minimal one on the fly.
ensure_docker_image() {
    if docker image inspect "$DOCKER_FULL" >/dev/null 2>&1; then
        info "Docker image $DOCKER_FULL found locally"
        return 0
    fi

    info "Docker image $DOCKER_FULL not found. Building..."

    if [ -f "$DOCKERFILE" ]; then
        docker build \
            -t "$DOCKER_FULL" \
            -f "$DOCKERFILE" \
            "$BUILDROOT_EXT/docker/"
    else
        # Generate a minimal Dockerfile for Buildroot builds.
        # This installs all Buildroot mandatory and optional dependencies
        # per https://buildroot.org/downloads/manual/manual.html#requirement
        info "No Dockerfile found at $DOCKERFILE, generating one..."
        mkdir -p "$(dirname "$DOCKERFILE")"
        cat > "$DOCKERFILE" << 'DOCKER_EOF'
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Buildroot mandatory dependencies + useful extras
# Reference: https://buildroot.org/downloads/manual/manual.html#requirement
RUN apt-get update && apt-get install -y \
    bc \
    binutils \
    build-essential \
    bzip2 \
    cpio \
    curl \
    file \
    g++ \
    gcc \
    git \
    graphviz \
    gzip \
    libncurses5-dev \
    libssl-dev \
    locales \
    make \
    patch \
    perl \
    python3 \
    python3-pip \
    rsync \
    sed \
    tar \
    unzip \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

# Set locale (required by some Buildroot packages)
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8

# Create non-root build user (Buildroot warns about running as root)
RUN useradd -m -s /bin/bash builder
USER builder
WORKDIR /home/builder

# Download and extract Buildroot (pinned version for reproducibility)
ARG BUILDROOT_VERSION=2024.02
RUN wget -q "https://buildroot.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.xz" \
    && tar xf "buildroot-${BUILDROOT_VERSION}.tar.xz" \
    && rm "buildroot-${BUILDROOT_VERSION}.tar.xz" \
    && mv "buildroot-${BUILDROOT_VERSION}" buildroot

WORKDIR /home/builder/buildroot
DOCKER_EOF
        docker build \
            -t "$DOCKER_FULL" \
            -f "$DOCKERFILE" \
            "$(dirname "$DOCKERFILE")"
    fi

    info "Docker image built: $DOCKER_FULL"
}

# ---------------------------------------------------------------------------
# Run a command inside the Docker container
# ---------------------------------------------------------------------------
# The container mounts:
#   - Project source tree (read-only for safety, except output/)
#   - Buildroot external tree (contains defconfigs, packages, board files)
#   - Output directory (writable, persisted on host)
#   - Buildroot download cache (persisted as named volume to avoid re-downloads)
#
docker_run() {
    local interactive_flags=""
    if [ -t 0 ]; then
        interactive_flags="-it"
    fi

    docker run --rm $interactive_flags \
        --name "vitals-build-$$" \
        -v "$PROJECT_ROOT:/src:ro" \
        -v "$OUTPUT_DIR:/output" \
        -v "vitals-buildroot-dl:/home/builder/buildroot/dl" \
        -e "BR2_EXTERNAL=/src/buildroot-external" \
        -e "BR2_DEFCONFIG=$DEFCONFIG" \
        -e "NPROC=$NPROC" \
        -w "/home/builder/buildroot" \
        "$DOCKER_FULL" \
        "$@"
}

# ---------------------------------------------------------------------------
# Build commands
# ---------------------------------------------------------------------------

cmd_all() {
    step "Building Vitals Monitor firmware (all)"
    info "  Project root: $PROJECT_ROOT"
    info "  External tree: $BUILDROOT_EXT"
    info "  Defconfig: $DEFCONFIG"
    info "  Parallel jobs: $NPROC"
    info "  Output: $OUTPUT_DIR"
    echo ""

    ensure_docker_image
    mkdir -p "$OUTPUT_DIR"

    # Step 1: Load the defconfig (idempotent — skips if .config exists)
    step "Loading defconfig..."
    docker_run bash -c '
        if [ ! -f /output/.config ]; then
            make O=/output BR2_EXTERNAL=/src/buildroot-external ${BR2_DEFCONFIG}
        else
            echo "Configuration already exists, skipping defconfig load."
            echo "Run \"rebuild\" or \"clean\" to start fresh."
        fi
    '

    # Step 2: Run the full build
    step "Running Buildroot build (this may take 30-60 minutes on first run)..."
    docker_run bash -c "make O=/output -j${NPROC}"

    # Step 3: Copy output images to a well-known location
    step "Copying output images..."
    mkdir -p "$OUTPUT_DIR/images"
    if [ -d "$OUTPUT_DIR/images" ]; then
        info "Build artifacts available in: $OUTPUT_DIR/images/"
        ls -lh "$OUTPUT_DIR/images/"*.img 2>/dev/null || true
        ls -lh "$OUTPUT_DIR/images/"*.stm32 2>/dev/null || true
    fi

    info "Build complete!"
}

cmd_clean() {
    step "Cleaning build artifacts..."
    ensure_docker_image

    if [ -d "$OUTPUT_DIR" ]; then
        docker_run bash -c "make O=/output clean"
        info "Clean complete."
    else
        info "No output directory to clean."
    fi
}

cmd_rebuild() {
    step "Full rebuild (clean + all)..."
    cmd_clean
    # Remove .config to force defconfig reload
    rm -f "$OUTPUT_DIR/.config"
    cmd_all
}

cmd_menuconfig() {
    step "Opening Buildroot menuconfig..."
    ensure_docker_image
    mkdir -p "$OUTPUT_DIR"

    # menuconfig requires an interactive terminal
    if [ ! -t 0 ]; then
        die "menuconfig requires an interactive terminal. Run from a TTY."
    fi

    # Ensure config exists
    docker_run bash -c '
        if [ ! -f /output/.config ]; then
            make O=/output BR2_EXTERNAL=/src/buildroot-external ${BR2_DEFCONFIG}
        fi
    '

    docker_run bash -c "make O=/output menuconfig"
    info "menuconfig closed. Run 'all' to build with new configuration."
}

cmd_sdk() {
    step "Building SDK tarball..."
    ensure_docker_image
    mkdir -p "$OUTPUT_DIR"

    docker_run bash -c '
        if [ ! -f /output/.config ]; then
            make O=/output BR2_EXTERNAL=/src/buildroot-external ${BR2_DEFCONFIG}
        fi
    '

    docker_run bash -c "make O=/output sdk"
    info "SDK tarball built. Check $OUTPUT_DIR/images/"
}

cmd_shell() {
    step "Opening interactive shell in build container..."
    ensure_docker_image
    mkdir -p "$OUTPUT_DIR"

    if [ ! -t 0 ]; then
        die "Shell requires an interactive terminal."
    fi

    docker_run bash
}

# ---------------------------------------------------------------------------
# Main entry point
# ---------------------------------------------------------------------------
COMMAND="${1:-all}"

case "$COMMAND" in
    all)
        cmd_all
        ;;
    clean)
        cmd_clean
        ;;
    rebuild)
        cmd_rebuild
        ;;
    menuconfig)
        cmd_menuconfig
        ;;
    sdk)
        cmd_sdk
        ;;
    shell)
        cmd_shell
        ;;
    -h|--help|help)
        echo "Usage: $0 [command]"
        echo ""
        echo "Commands:"
        echo "  all         Build everything (default)"
        echo "  clean       Remove build artifacts"
        echo "  rebuild     Clean + full build"
        echo "  menuconfig  Open Buildroot menuconfig (interactive)"
        echo "  sdk         Build SDK tarball"
        echo "  shell       Interactive shell in build container"
        echo "  help        Show this help message"
        ;;
    *)
        die "Unknown command: $COMMAND (run '$0 help' for usage)"
        ;;
esac
