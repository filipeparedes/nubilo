from helpers import register_and_login, upload


def test_root_shows_folders_and_files(client):
    token = register_and_login(client)
    client.post("/folders", token=token, json={"path": "docs"})
    upload(client, token)

    response = client.get("/root-contents", token=token)

    assert response.status_code == 200
    body = response.json()
    assert body["path"] == ""
    assert len(body["folders"]) == 1
    assert body["folders"][0]["path"] == "docs"
    assert len(body["files"]) == 1
    assert body["files"][0]["path"] == "notes.txt"


def test_folder_by_id_shows_its_direct_children(client):
    token = register_and_login(client)
    created = client.post("/folders", token=token, json={"path": "docs"})
    assert created.status_code == 201
    docs_id = created.json()["id"]

    client.post("/folders", token=token, json={"path": "docs/sub"})
    upload(client, token, path="docs/a.txt")

    response = client.get(f"/folders/{docs_id}", token=token)

    assert response.status_code == 200
    body = response.json()
    assert body["path"] == "docs"
    assert len(body["folders"]) == 1
    assert body["folders"][0]["path"] == "docs/sub"
    assert len(body["files"]) == 1
    assert body["files"][0]["path"] == "docs/a.txt"


def test_returns_not_found_for_missing_or_foreign_folder(client):
    other_token = register_and_login(client, "other@example.com")
    created = client.post("/folders", token=other_token, json={"path": "private"})
    assert created.status_code == 201
    foreign_id = created.json()["id"]

    token = register_and_login(client)

    foreign = client.get(f"/folders/{foreign_id}", token=token)
    missing = client.get("/folders/9999", token=token)

    assert foreign.status_code == 404
    assert missing.status_code == 404


def test_rejects_missing_token(client):
    root = client.get("/root-contents")
    folders = client.get("/folders")
    by_id = client.get("/folders/1")

    assert root.status_code == 401
    assert folders.status_code == 401
    assert by_id.status_code == 401


def test_folder_list_shows_every_folder_and_no_files(client):
    token = register_and_login(client)
    client.post("/folders", token=token, json={"path": "docs/sub"})
    upload(client, token)

    response = client.get("/folders", token=token)

    assert response.status_code == 200
    body = response.json()
    assert isinstance(body, list)
    assert len(body) == 2
    assert body[0]["path"] == "docs"
    assert body[1]["path"] == "docs/sub"
    assert "id" in body[0]
    assert "createdAt" in body[0]
