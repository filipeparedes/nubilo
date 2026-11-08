from helpers import register_and_login, upload


def test_returns_uploaded_files(client):
    token = register_and_login(client)
    uploaded = upload(client, token)
    assert uploaded.status_code == 201
    file_id = uploaded.json()["id"]

    response = client.get("/files", token=token)

    assert response.status_code == 200
    body = response.json()
    assert isinstance(body, list)
    assert len(body) == 1
    assert body[0]["id"] == file_id
    assert body[0]["path"] == "notes.txt"
    assert body[0]["size"] == 13
    assert body[0]["contentType"] == "text/plain"
