from helpers import register_and_login, upload


def restore(client, file_id, version, token=None):
    return client.post(
        f"/files/{file_id}/versions/{version}/restore",
        token=token,
        data=b"",
        headers={"Content-Type": "text/plain"},
    )


def test_creates_new_version_with_previous_content(client):
    token = register_and_login(client)
    first_upload = upload(client, token, content="first version")
    second_upload = upload(client, token, content="second version")
    assert first_upload.status_code == 201
    assert second_upload.status_code == 201
    file_id = first_upload.json()["id"]
    assert second_upload.json()["id"] == file_id

    restored = restore(client, file_id, 1, token)
    downloaded = client.get(f"/files/{file_id}", token=token)

    assert restored.status_code == 201
    assert restored.json()["id"] == file_id
    assert restored.json()["version"] == 3
    assert downloaded.status_code == 200
    assert downloaded.text == "first version"


def test_rejects_missing_token(client):
    response = restore(client, 1, 1)

    assert response.status_code == 401


def test_returns_not_found_for_nonexistent_version(client):
    token = register_and_login(client)
    uploaded = upload(client, token)
    assert uploaded.status_code == 201

    response = restore(client, uploaded.json()["id"], 99, token)

    assert response.status_code == 404


def test_returns_not_found_for_nonexistent_file(client):
    token = register_and_login(client)

    response = restore(client, 9999, 1, token)

    assert response.status_code == 404


def test_returns_not_found_for_file_owned_by_another_user(client, server):
    owner_token = register_and_login(client, "owner@example.com")
    other_token = register_and_login(client, "other@example.com")
    uploaded = upload(
        client, owner_token, content="private content", filename="secret.txt"
    )
    assert uploaded.status_code == 201

    response = restore(client, uploaded.json()["id"], 1, other_token)

    assert response.status_code == 404

    # the owner's file was not touched
    versions = server.query("SELECT COUNT(*) AS count FROM file_versions;")
    assert versions[0]["count"] == 1
