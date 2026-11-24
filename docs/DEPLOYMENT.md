# StoreForge Deployment & Operations Manual

## 1. Native Build Deployment

### 1.1 Host Dependencies (Ubuntu/Debian)
```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libpq-dev \
    libpqxx-dev \
    postgresql-client
```

### 1.2 Compiling from Source
```bash
# Clone repository
git clone https://github.com/akashch1013/InvenTrack-CPP.git
cd InvenTrack-CPP

# Configure CMake with Release optimization
cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DSTOREFORGE_BUILD_TESTS=ON \
    -DSTOREFORGE_ENABLE_POSTGRES=ON

# Compile targets
cmake --build build --config Release -j$(nproc)

# Run test suite
ctest --test-dir build --output-on-failure
```

---

## 2. PostgreSQL Database Setup

### 2.1 Database Provisioning
```bash
# Login to postgres and create database
psql -U postgres -c "CREATE DATABASE storeforge_db;"

# Apply DDL schema
psql -U postgres -d storeforge_db -f sql/schema.sql

# Seed initial retail catalog
psql -U postgres -d storeforge_db -f sql/seed.sql
```

### 2.2 Environment Configuration
StoreForge supports configuration via environment variables:

| Variable | Default Value | Description |
|:---|:---|:---|
| `STOREFORGE_DB_HOST` | `localhost` | PostgreSQL host |
| `STOREFORGE_DB_PORT` | `5432` | PostgreSQL port |
| `STOREFORGE_DB_NAME` | `storeforge_db` | Database catalog name |
| `STOREFORGE_DB_USER` | `postgres` | Database username |
| `STOREFORGE_DB_PASS` | `postgres` | Database password |
| `STOREFORGE_POOL_MIN` | `2` | Minimum pooled connections |
| `STOREFORGE_POOL_MAX` | `10` | Maximum pooled connections |

---

## 3. Container Deployment (Docker & GitHub Packages)

### 3.1 Pulling from GitHub Container Registry (GHCR)
```bash
docker pull ghcr.io/akashch1013/inventrack-cpp:latest
```

### 3.2 Running the Container
```bash
docker run -it --rm \
    --name storeforge-pos \
    --network host \
    ghcr.io/akashch1013/inventrack-cpp:latest
```

### 3.3 Building the Docker Image Locally
```bash
docker build -t storeforge:local .
docker run -it --rm storeforge:local
```

---

## 4. Systemd Service Configuration

For dedicated POS counter kiosks, configure StoreForge as a managed service:

Create `/etc/systemd/system/storeforge.service`:
```ini
[Unit]
Description=StoreForge Retail Inventory and POS Billing System
After=network.target postgresql.service
Requires=postgresql.service

[Service]
Type=simple
User=storeforge
WorkingDirectory=/opt/storeforge
ExecStart=/opt/storeforge/bin/storeforge_cli
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

Enable and start the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable storeforge
sudo systemctl start storeforge
```
