from helpers import register_and_login, upload


def test_removes_file(client):
    token = register_and_login(client)
    uploaded = upload(client, token)
    assert uploaded.status_code == 201
    path = f"/files/{uploaded.json()['id']}"

    deleted = client.delete(path, token=token)
    downloaded = client.get(path, token=token)

    assert deleted.status_code == 204
    assert downloaded.status_code == 404
