from helpers import register_and_login


def test_returns_created_and_creates_parents(client, server):
    token = register_and_login(client)

    response = client.post("/folders", token=token, json={"path": "docs/2026"})

    assert response.status_code == 201
    assert "id" in response.json()
    folders = server.query("SELECT path FROM folders ORDER BY path;")
    assert [folder["path"] for folder in folders] == ["docs", "docs/2026"]


def test_rejects_invalid_path(client):
    token = register_and_login(client)

    response = client.post("/folders", token=token, json={"path": "../secret"})

    assert response.status_code == 400
    assert response.json()["error"]["code"] == "INVALID_PATH"


def test_rejects_missing_path_field(client):
    token = register_and_login(client)

    response = client.post("/folders", token=token, json={"name": "docs"})

    assert response.status_code == 400
    assert response.json()["error"]["code"] == "INVALID_REQUEST"


def test_rejects_missing_token(client):
    response = client.post("/folders", json={"path": "docs"})

    assert response.status_code == 401
