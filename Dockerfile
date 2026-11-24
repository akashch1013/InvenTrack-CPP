# =============================================================================
# StoreForge - Production Multi-Stage Dockerfile
# Publishes to GitHub Packages (ghcr.io)
# =============================================================================

# Stage 1: Build Environment
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    libpq-dev \
    libpqxx-dev \
    ca-certificates \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source tree
COPY . .

# Build StoreForge Release binary
RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DSTOREFORGE_BUILD_TESTS=OFF \
    -DSTOREFORGE_ENABLE_POSTGRES=ON \
    && cmake --build build --config Release -j$(nproc)

# Stage 2: Minimal Runtime Image
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install only shared runtime libraries for PostgreSQL and libpqxx
RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    libpqxx-6.4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN groupadd -r storeforge && useradd -r -g storeforge storeforge

WORKDIR /home/storeforge

# Copy binary from builder
COPY --from=builder /app/build/storeforge_cli /usr/local/bin/storeforge_cli
COPY --from=builder /app/sql /home/storeforge/sql

# Set ownership
RUN chown -R storeforge:storeforge /home/storeforge

USER storeforge

# Metadata labels for GitHub Packages
LABEL org.opencontainers.image.title="StoreForge POS & Inventory"
LABEL org.opencontainers.image.description="Enterprise Retail Inventory and POS Billing System in modern C++17"
LABEL org.opencontainers.image.source="https://github.com/akashch1013/InvenTrack-CPP"
LABEL org.opencontainers.image.licenses="MIT"

ENTRYPOINT ["/usr/local/bin/storeforge_cli"]
