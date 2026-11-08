from helpers import register_and_login, upload


def test_returns_created_on_success(client):
    token = register_and_login(client)

    response = upload(client, token)

    assert response.status_code == 201


def test_rejects_missing_file(client):
    token = register_and_login(client)

    # a multipart body with a "path" field and no "file" field
    response = client.post("/files", token=token, files={"path": (None, "notes.txt")})

    assert response.status_code == 400


def test_rejects_missing_token(client):
    response = client.post(
        "/files", files={"file": ("notes.txt", "hello, nubilo", "text/plain")}
    )

    assert response.status_code == 401


def test_rejects_invalid_token(client):
    response = upload(client, "not-a-real-token")

    assert response.status_code == 401


def test_uses_filename_when_path_not_provided(client, server):
    token = register_and_login(client)

    response = upload(client, token)

    assert response.status_code == 201
    rows = server.query("SELECT path FROM files;")
    assert len(rows) == 1
    assert rows[0]["path"] == "notes.txt"
