def test_succeeds_with_correct_credentials(client):
    client.post(
        "/auth/register", json={"email": "user@example.com", "password": "password123"}
    )

    response = client.post(
        "/auth/login", json={"email": "user@example.com", "password": "password123"}
    )

    assert response.status_code == 200


def test_rejects_wrong_password(client):
    client.post(
        "/auth/register", json={"email": "user@example.com", "password": "password123"}
    )

    response = client.post(
        "/auth/login", json={"email": "user@example.com", "password": "wrongPassword"}
    )

    assert response.status_code == 401


def test_rejects_nonexistent_email(client):
    response = client.post(
        "/auth/login", json={"email": "nobody@example.com", "password": "password123"}
    )

    assert response.status_code == 401


def test_rejects_missing_fields(client):
    response = client.post("/auth/login", json={"email": "user@example.com"})

    assert response.status_code == 400
