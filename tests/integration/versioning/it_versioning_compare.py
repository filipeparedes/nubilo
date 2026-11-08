from helpers import register_and_login, upload


def test_shows_the_chunk_that_changed(client):
    token = register_and_login(client)
    first = upload(client, token, content="first")
    upload(client, token, content="second version")
    assert first.status_code == 201
    file_id = first.json()["id"]

    response = client.get(f"/files/{file_id}/versions/compare?from=1&to=2", token=token)

    assert response.status_code == 200
    body = response.json()
    assert body["fromVersion"] == 1
    assert body["toVersion"] == 2
    assert body["fromSize"] == 5
    assert body["toSize"] == 14
    assert body["unchangedChunks"] == 0
    assert len(body["changes"]) == 1
    assert body["changes"][0]["position"] == 0
    assert body["changes"][0]["from"] is not None
    assert body["changes"][0]["to"] is not None


def test_rejects_invalid_parameters(client):
    token = register_and_login(client)

    missing = client.get("/files/1/versions/compare?from=1", token=token)
    not_a_number = client.get("/files/1/versions/compare?from=a&to=2", token=token)
    zero = client.get("/files/1/versions/compare?from=0&to=2", token=token)

    assert missing.status_code == 400
    assert not_a_number.status_code == 400
    assert zero.status_code == 400
    assert missing.json()["error"]["code"] == "INVALID_REQUEST"


def test_rejects_missing_token(client):
    response = client.get("/files/1/versions/compare?from=1&to=2")

    assert response.status_code == 401


def test_returns_not_found_for_missing_or_foreign_file(client):
    other_token = register_and_login(client, "other@example.com")
    uploaded = upload(client, other_token, content="secret", filename="private.txt")
    assert uploaded.status_code == 201
    foreign_id = uploaded.json()["id"]

    token = register_and_login(client)

    foreign = client.get(
        f"/files/{foreign_id}/versions/compare?from=1&to=1", token=token
    )
    missing = client.get("/files/9999/versions/compare?from=1&to=1", token=token)

    assert foreign.status_code == 404
    assert missing.status_code == 404


def test_returns_not_found_for_missing_version(client):
    token = register_and_login(client)
    uploaded = upload(client, token, content="hello")
    assert uploaded.status_code == 201
    file_id = uploaded.json()["id"]

    response = client.get(f"/files/{file_id}/versions/compare?from=1&to=7", token=token)

    assert response.status_code == 404
