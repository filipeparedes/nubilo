from helpers import register_and_login, upload


def test_returns_the_content_of_that_version(client):
    token = register_and_login(client)
    first_upload = upload(client, token, content="first version")
    upload(
        client,
        token,
        content='{"v": 2}',
        content_type="application/json",
    )
    assert first_upload.status_code == 201
    base = f"/files/{first_upload.json()['id']}"

    first = client.get(f"{base}/versions/1", token=token)
    second = client.get(f"{base}/versions/2", token=token)

    assert first.status_code == 200
    assert first.text == "first version"
    assert first.headers["Content-Type"] == "text/plain"
    assert "notes.txt" in first.headers["Content-Disposition"]
    assert second.status_code == 200
    assert second.text == '{"v": 2}'
    assert second.headers["Content-Type"] == "application/json"


def test_rejects_missing_token(client):
    response = client.get("/files/1/versions/1")

    assert response.status_code == 401


def test_returns_not_found_for_missing_version(client):
    token = register_and_login(client)
    uploaded = upload(client, token)
    assert uploaded.status_code == 201

    response = client.get(f"/files/{uploaded.json()['id']}/versions/99", token=token)

    assert response.status_code == 404


def test_returns_not_found_for_missing_file(client):
    token = register_and_login(client)

    response = client.get("/files/9999/versions/1", token=token)

    assert response.status_code == 404


def test_returns_not_found_for_foreign_file(client):
    other_token = register_and_login(client, "other@example.com")
    uploaded = upload(client, other_token, content="secret", filename="private.txt")
    assert uploaded.status_code == 201
    foreign_id = uploaded.json()["id"]

    token = register_and_login(client)

    response = client.get(f"/files/{foreign_id}/versions/1", token=token)

    assert response.status_code == 404
