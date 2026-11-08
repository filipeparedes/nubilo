"""Fixtures for the integration tests.

The tests start the real server, the binary built by CMake, and talk to it
over HTTP, as any client would. Each test gets its own server, with its own
port, database and blob directory, so tests never share state.

The binary is build/nubilo by default. Another one can be given with the
NUBILO_BIN environment variable.
"""

import json
import os
import socket
import sqlite3
import subprocess
import time
from contextlib import closing
from pathlib import Path

import pytest
import requests

from helpers import CORS_ORIGIN, Client

REPO_ROOT = Path(__file__).resolve().parents[2]
START_TIMEOUT_SECONDS = 10


def free_port():
    """A port that no one is listening on."""
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


class Server:
    """A running server on a temporary database and blob directory. The test
    can also read and change the database directly, with query and execute."""

    def __init__(self, binary, workdir):
        self.port = free_port()
        self.url = f"http://127.0.0.1:{self.port}"
        self.db_path = workdir / "nubilo.db"

        config = {
            "config": {
                "server": {
                    "host": "127.0.0.1",
                    "port": self.port,
                    "corsOrigin": CORS_ORIGIN,
                },
                "storage": {
                    "dbPath": str(self.db_path),
                    "blobDir": str(workdir / "blobs"),
                    "threads": 2,
                },
                "auth": {"sessionDays": 7},
            }
        }
        config_path = workdir / "config.json"
        config_path.write_text(json.dumps(config))

        self.log_path = workdir / "server.log"
        self.log = open(self.log_path, "w")
        self.process = subprocess.Popen(
            [str(binary), str(config_path)],
            cwd=workdir,
            stdout=self.log,
            stderr=subprocess.STDOUT,
        )
        self._wait_until_ready()

    def _wait_until_ready(self):
        deadline = time.monotonic() + START_TIMEOUT_SECONDS
        while time.monotonic() < deadline:
            if self.process.poll() is not None:
                self.log.flush()
                raise RuntimeError(
                    "The server stopped while starting:\n" + self.log_path.read_text()
                )
            try:
                if requests.get(self.url + "/health", timeout=0.5).status_code == 200:
                    return
            except requests.ConnectionError:
                time.sleep(0.05)
        self.stop()
        raise RuntimeError("The server did not answer /health in time.")

    def stop(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        self.log.close()

    def query(self, sql, params=()):
        """Runs a SELECT on the database of the server. Gives a list of rows,
        each one a dict from column name to value."""
        with closing(sqlite3.connect(self.db_path, timeout=5)) as connection:
            connection.row_factory = sqlite3.Row
            return [dict(row) for row in connection.execute(sql, params)]

    def execute(self, sql, params=()):
        """Runs a statement that changes the database of the server."""
        with closing(sqlite3.connect(self.db_path, timeout=5)) as connection:
            connection.execute(sql, params)
            connection.commit()


@pytest.fixture(scope="session")
def server_binary():
    binary = Path(os.environ.get("NUBILO_BIN", REPO_ROOT / "build" / "nubilo"))
    if not binary.is_file():
        pytest.exit(
            f"The server was not found at {binary}. Build it with scripts/build.sh "
            "or set NUBILO_BIN.",
            returncode=2,
        )
    return binary


@pytest.fixture
def server(server_binary, tmp_path):
    running = Server(server_binary, tmp_path)
    yield running
    running.stop()


@pytest.fixture
def client(server):
    return Client(server.url)
