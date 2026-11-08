from helpers import register_and_login, upload


def test_removes_empty_folder(client, server):
    token = register_and_login(client)
    created = client.post("/folders", token=token, json={"path": "docs"})
    assert created.status_code == 201
    folder_id = created.json()["id"]

    response = client.delete(f"/folders/{folder_id}", token=token)

    assert response.status_code == 204
    assert server.query("SELECT id FROM folders;") == []


def test_removes_folder_and_everything_inside_it(client, server):
    token = register_and_login(client)
    created = client.post("/folders", token=token, json={"path": "docs"})
    assert created.status_code == 201
    folder_id = created.json()["id"]
    upload(client, token, path="docs/sub/a.txt")

    response = client.delete(f"/folders/{folder_id}", token=token)

    assert response.status_code == 204
    assert server.query("SELECT id FROM files;") == []
    assert server.query("SELECT id FROM folders;") == []


def test_rejects_missing_token(client):
    response = client.delete("/folders/1")

    assert response.status_code == 401


def test_returns_not_found_for_missing_or_foreign_folder(client):
    other_token = register_and_login(client, "other@example.com")
    created = client.post("/folders", token=other_token, json={"path": "private"})
    assert created.status_code == 201
    foreign_id = created.json()["id"]

    token = register_and_login(client)

    foreign = client.delete(f"/folders/{foreign_id}", token=token)
    missing = client.delete("/folders/9999", token=token)

    assert foreign.status_code == 404
    assert missing.status_code == 404
