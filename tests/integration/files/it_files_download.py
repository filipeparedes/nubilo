from helpers import register_and_login, upload


def test_returns_uploaded_content(client):
    token = register_and_login(client)
    uploaded = upload(client, token)
    assert uploaded.status_code == 201
    file_id = uploaded.json()["id"]

    response = client.get(f"/files/{file_id}", token=token)

    assert response.status_code == 200
    assert response.text == "hello, nubilo"
    assert response.headers["Content-Type"] == "text/plain"
    assert "notes.txt" in response.headers["Content-Disposition"]


def test_rejects_missing_token(client):
    response = client.get("/files/1")

    assert response.status_code == 401


def test_rejects_invalid_token(client):
    response = client.get("/files/1", token="not-a-real-token")

    assert response.status_code == 401


def test_returns_not_found_for_nonexistent_file(client):
    token = register_and_login(client)

    response = client.get("/files/9999", token=token)

    assert response.status_code == 404


def test_returns_not_found_for_file_owned_by_another_user(client):
    owner_token = register_and_login(client, "owner@example.com")
    other_token = register_and_login(client, "other@example.com")
    uploaded = upload(
        client, owner_token, content="private content", filename="secret.txt"
    )
    assert uploaded.status_code == 201
    file_id = uploaded.json()["id"]

    response = client.get(f"/files/{file_id}", token=other_token)

    assert response.status_code == 404
