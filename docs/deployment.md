# Deployment guide

How to run Nubilo on a Linux server: the server (`nubilo`) behind a reverse proxy with HTTPS, and the web frontend (`nubilo-web`) as static files. The steps were written for Ubuntu 24.04 and work, with the package names changed, on any Linux distribution with systemd.

## What you end up with

```
browser ──HTTPS──> Caddy ──> /var/www/nubilo-web        https://nubilo.example.com      (the frontend)
                      └───> nubilo on 127.0.0.1:8080      https://api.nubilo.example.com  (the API)
```

- `nubilo` listens only on the local machine. The reverse proxy is the only thing exposed to the network, and it handles HTTPS.
- The frontend and the API have different addresses, so the browser sends cross-origin requests. The server allows the address of the frontend (`server.corsOrigin`), and no one else.
- The state of the server is two things: the SQLite database and the directory of blobs. Both live in `/var/lib/nubilo`.

Replace `nubilo.example.com` and `api.nubilo.example.com` with your own names, which must already point to the server.

## Requirements

- A Linux server with a public address, and two names for it, one for the frontend and one for the API.
- Ports 80 and 443 open. Nothing else needs to be reachable from outside, including 8080.
- To build the server: CMake 3.28 or newer, a C++23 compiler (GCC 14 or Clang) and the libraries OpenSSL, zlib and Brotli.
- To build the frontend: Node.js 22 LTS or newer. It can be built on another machine, since only the `dist/` folder goes to the server.

```bash
sudo apt update
sudo apt install git cmake g++-14 libssl-dev zlib1g-dev libbrotli-dev
```

## 1. Build and install the server

```bash
git clone https://github.com/filipeparedes/nubilo.git
cd nubilo
CC=gcc-14 CXX=g++-14 scripts/build.sh Release
```

The first build downloads the dependencies (cpp-httplib, nlohmann/json, SQLite, Argon2, PicoSHA2) from the internet and takes a few minutes. The result is a single executable, `build/nubilo`.

Create a user for the service and install the executable and the configuration:

```bash
sudo useradd --system --no-create-home --shell /usr/sbin/nologin nubilo
sudo install -D build/nubilo /opt/nubilo/nubilo
sudo install -d /etc/nubilo
```

## 2. Configure the server

Create `/etc/nubilo/config.json`:

```json
{
  "app": { "version": "1.0" },
  "config": {
    "server": {
      "host": "127.0.0.1",
      "port": 8080,
      "corsOrigin": "https://nubilo.example.com"
    },
    "storage": {
      "dbPath": "/var/lib/nubilo/nubilo.db",
      "blobDir": "/var/lib/nubilo/blobs",
      "threads": 0
    },
    "auth": {
      "sessionDays": 7
    }
  }
}
```

The keys are described in the [README](../README.md#configuration). The ones that matter here:

- `server.host` is `127.0.0.1` so that only the reverse proxy on the same machine can reach the server.
- `server.corsOrigin` must be the address of the frontend exactly as the browser shows it: with `https://`, without a trailing slash and without a path. If it does not match, the browser blocks every request of the frontend.
- `storage.dbPath` and `storage.blobDir` are absolute paths, so it does not matter where the service is started. The parent directory `/var/lib/nubilo` is created by systemd in the next step; the server creates the database and the blob directory inside it.
- If a value is missing or invalid, the server does not start and says which one.

The file has no secrets. The service runs as the user `nubilo`, which must be able to read it, and the default permissions of a file created with `sudo` allow that.

## 3. Run the server as a service

Create `/etc/systemd/system/nubilo.service`:

```ini
[Unit]
Description=Nubilo server
After=network.target

[Service]
User=nubilo
ExecStart=/opt/nubilo/nubilo /etc/nubilo/config.json
Restart=on-failure
RestartSec=2

# /var/lib/nubilo, owned by the user of the service
StateDirectory=nubilo

# The service can only write in its state directory
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
```

Start it and make it start with the machine:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now nubilo
```

Check that it is running and answering:

```bash
systemctl status nubilo
curl http://127.0.0.1:8080/health          # {"status":"ok"}
journalctl -u nubilo -f                    # the output of the server
```

On the first start the server creates the database and applies the migrations. On later starts it applies only the ones that are new.

## 4. Build and install the frontend

On the server or on your computer:

```bash
git clone https://github.com/filipeparedes/nubilo-web.git
cd nubilo-web
npm ci
npm run build
```

The result is the `dist/` folder. Copy it to the server:

```bash
sudo mkdir -p /var/www/nubilo-web
sudo cp -r dist/. /var/www/nubilo-web/
```

The frontend reads its settings from `config.json` when the page loads, so the address of the API is changed in the built files, with no rebuild. Edit `/var/www/nubilo-web/config.json`:

```json
{
  "config": {
    "api": { "url": "https://api.nubilo.example.com" },
    "devServer": { "port": 5173 }
  }
}
```

`devServer` is only used by the development server and can stay as it is.

## 5. Put the reverse proxy in front

This guide uses [Caddy](https://caddyserver.com/docs/install), which gets and renews the HTTPS certificates by itself. Install it from its documentation, then write `/etc/caddy/Caddyfile`:

```
nubilo.example.com {
    root * /var/www/nubilo-web
    encode gzip
    # the frontend routes in the browser, so any path that is not a file gives the app
    try_files {path} /index.html
    file_server
}

api.nubilo.example.com {
    reverse_proxy 127.0.0.1:8080
}
```

```bash
sudo systemctl reload caddy
```

Some details:

- Do not add CORS headers in the proxy. The server answers the preflight requests and adds its own headers, and a second set makes the browser reject the response.
- Caddy puts no limit on the size of a request. With nginx instead, raise `client_max_body_size` (its default is 1 MB, which blocks almost every upload) and use `try_files $uri /index.html;` for the frontend.
- A file is held in memory in full while it is uploaded and downloaded, so the memory of the server limits the size of the files and how many can be transferred at the same time.

If a firewall is active, open only what is needed:

```bash
sudo ufw allow 80,443/tcp
```

## 6. Check it

1. `curl https://api.nubilo.example.com/health` answers `{"status":"ok"}`.
2. Open `https://nubilo.example.com`, create an account and log in. The top bar shows `API: ONLINE`.
3. Upload a file, download it, upload it again with other content and look at its history.

If something fails:

| Symptom | Cause |
|---|---|
| 502 from the proxy | The service is not running: `systemctl status nubilo` and `journalctl -u nubilo`. |
| The browser blocks the requests (CORS error in the console) | `server.corsOrigin` is not exactly the address of the frontend. Fix it, then `sudo systemctl restart nubilo`. |
| The page says it failed to load `config.json` | The file is missing from `/var/www/nubilo-web`, or the proxy does not serve it. |
| The service stops right after starting | The configuration is not valid or a path is not writable. The journal says which. |
| Uploads fail with a size error | With nginx, `client_max_body_size` is too low. |

## Updating

```bash
cd nubilo
git pull
CC=gcc-14 CXX=g++-14 scripts/build.sh Release
sudo install build/nubilo /opt/nubilo/nubilo
sudo systemctl restart nubilo
```

The migrations of the new version run when the server starts. Back up before updating (see below). For the frontend, build again and copy `dist/` over `/var/www/nubilo-web`, then put the `config.json` with the right API address back, because the copy replaces it.

## Backup and restore

All the state is in `/var/lib/nubilo`. The chunks in `blobs/` are never changed after they are written, so they can be copied while the server runs. Copy them **first** and the database **after**, so that every chunk the database points to is in the backup.

```bash
sudo install -d -o nubilo /backup/nubilo        # once: the destination, writable by the service user
sudo -u nubilo rsync -a /var/lib/nubilo/blobs/ /backup/nubilo/blobs/
sudo -u nubilo sqlite3 /var/lib/nubilo/nubilo.db ".backup '/backup/nubilo/nubilo.db'"
```

`sqlite3` is in the package of the same name (`sudo apt install sqlite3`). Its `.backup` command gives a consistent copy of a database that is in use. Run both as the user `nubilo`, so the files that SQLite creates next to the database do not end up owned by root.

To restore, stop the service, put the files back in `/var/lib/nubilo` (owned by the user `nubilo`) and start it:

```bash
sudo systemctl stop nubilo
sudo rsync -a /backup/nubilo/blobs/ /var/lib/nubilo/blobs/
sudo cp /backup/nubilo/nubilo.db /var/lib/nubilo/nubilo.db
sudo chown -R nubilo:nubilo /var/lib/nubilo
sudo systemctl start nubilo
```

## Known limits

These are the limits of version 1.0 to keep in mind when running it:

- **Anyone who can reach the API can create an account.** There is no invitation and no limit of requests. If the server is not meant to be public, restrict who can reach `api.nubilo.example.com` in the proxy or the firewall.
- **No password policy.** Any password is accepted at registration.
- **Chunks are not removed.** Deleting a file or a version removes the references to its chunks, but the chunks stay on disk, so the disk usage only grows. There is no garbage collection yet.
- **One process, one machine.** There is no replication and no high availability.
- **HTTPS is not done by Nubilo.** Without the proxy, passwords and tokens would travel in clear text. Do not expose port 8080 directly.
