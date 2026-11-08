"""Helpers shared by the integration tests: an HTTP client for the server and
the calls that almost every test needs (register, log in, upload)."""

import requests

# The origin the test servers allow, set in the config of conftest.py.
CORS_ORIGIN = "http://localhost:5173"

PASSWORD = "password123"


class Client:
    """An HTTP client for one running server. Every method takes the path of
    the endpoint and, optionally, the session token, which is sent as the
    Authorization header. The other arguments are those of requests."""

    def __init__(self, base_url):
        self.base_url = base_url
        self.session = requests.Session()

    def request(self, method, path, token=None, **kwargs):
        headers = dict(kwargs.pop("headers", None) or {})
        if token is not None:
            headers["Authorization"] = f"Bearer {token}"
        return self.session.request(
            method, self.base_url + path, headers=headers, timeout=10, **kwargs
        )

    def get(self, path, token=None, **kwargs):
        return self.request("GET", path, token, **kwargs)

    def post(self, path, token=None, **kwargs):
        return self.request("POST", path, token, **kwargs)

    def delete(self, path, token=None, **kwargs):
        return self.request("DELETE", path, token, **kwargs)

    def options(self, path, token=None, **kwargs):
        return self.request("OPTIONS", path, token, **kwargs)


def register_and_login(client, email="user@example.com"):
    """Registers a user and logs in, returning a valid session token."""
    credentials = {"email": email, "password": PASSWORD}
    client.post("/auth/register", json=credentials)
    response = client.post("/auth/login", json=credentials)
    return response.json()["token"]


def upload(
    client,
    token,
    content="hello, nubilo",
    filename="notes.txt",
    content_type="text/plain",
    path=None,
):
    """Uploads a file. Without path, the file is stored under its filename."""
    url = "/files" if path is None else f"/files?path={path}"
    return client.post(
        url, token=token, files={"file": (filename, content, content_type)}
    )
