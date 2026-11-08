from helpers import register_and_login, upload


def test_create_folder_returns_conflict_when_file_has_that_path(client):
    token = register_and_login(client)
    upload(client, token, filename="docs")

    response = client.post("/folders", token=token, json={"path": "docs"})

    assert response.status_code == 409
    assert response.json()["error"]["code"] == "PATH_CONFLICT"


def test_upload_returns_conflict_when_folder_has_that_path(client):
    token = register_and_login(client)
    client.post("/folders", token=token, json={"path": "docs"})

    response = upload(client, token, filename="docs")

    assert response.status_code == 409
    assert response.json()["error"]["code"] == "PATH_CONFLICT"
