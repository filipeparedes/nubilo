from helpers import register_and_login


def test_invalidates_session_with_valid_token(client):
    token = register_and_login(client)
    logout = client.post("/auth/logout", token=token)

    # using the same token again should now be rejected
    second_logout = client.post("/auth/logout", token=token)

    assert logout.status_code == 200
    assert second_logout.status_code == 401


def test_rejects_missing_token(client):
    response = client.post("/auth/logout")

    assert response.status_code == 401


def test_rejects_invalid_token(client):
    response = client.post("/auth/logout", token="not-a-real-token")

    assert response.status_code == 401
