from helpers import register_and_login, upload


def test_returns_versions_newest_first(client):
    token = register_and_login(client)
    first = upload(client, token, content="first")
    upload(client, token, content="second version")
    assert first.status_code == 201
    file_id = first.json()["id"]

    response = client.get(f"/files/{file_id}/versions", token=token)

    assert response.status_code == 200
    body = response.json()
    assert body["path"] == "notes.txt"
    versions = body["versions"]
    assert len(versions) == 2
    assert versions[0]["version"] == 2
    assert versions[0]["size"] == 14
    assert versions[0]["contentType"] == "text/plain"
    assert "createdAt" in versions[0]
    assert versions[1]["version"] == 1
    assert versions[1]["size"] == 5


def test_shows_version_created_by_restore(client):
    token = register_and_login(client)
    first = upload(client, token, content="first")
    upload(client, token, content="second")
    assert first.status_code == 201
    base = f"/files/{first.json()['id']}"
    client.post(
        f"{base}/versions/1/restore",
        token=token,
        data=b"",
        headers={"Content-Type": "text/plain"},
    )

    response = client.get(f"{base}/versions", token=token)

    assert response.status_code == 200
    versions = response.json()["versions"]
    assert len(versions) == 3
    assert versions[0]["version"] == 3


def test_rejects_missing_token(client):
    response = client.get("/files/1/versions")

    assert response.status_code == 401


def test_returns_not_found_for_missing_or_foreign_file(client):
    other_token = register_and_login(client, "other@example.com")
    uploaded = upload(client, other_token, content="secret", filename="private.txt")
    assert uploaded.status_code == 201
    foreign_id = uploaded.json()["id"]

    token = register_and_login(client)

    foreign = client.get(f"/files/{foreign_id}/versions", token=token)
    missing = client.get("/files/9999/versions", token=token)

    assert foreign.status_code == 404
    assert missing.status_code == 404


def test_returns_the_full_path_of_a_nested_file(client):
    token = register_and_login(client)
    uploaded = upload(client, token, content="hello", path="docs/2026/notes.txt")
    assert uploaded.status_code == 201
    file_id = uploaded.json()["id"]

    response = client.get(f"/files/{file_id}/versions", token=token)

    assert response.status_code == 200
    body = response.json()
    assert body["path"] == "docs/2026/notes.txt"
    assert len(body["versions"]) == 1
