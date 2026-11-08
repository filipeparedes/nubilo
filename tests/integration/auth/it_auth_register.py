def test_returns_created_on_success(client):
    response = client.post(
        "/auth/register", json={"email": "user@example.com", "password": "password123"}
    )

    assert response.status_code == 201


def test_rejects_duplicate_email(client):
    client.post(
        "/auth/register", json={"email": "user@example.com", "password": "password123"}
    )

    second = client.post(
        "/auth/register", json={"email": "user@example.com", "password": "other"}
    )

    assert second.status_code == 409


def test_rejects_missing_fields(client):
    response = client.post("/auth/register", json={"email": "user@example.com"})

    assert response.status_code == 400


def test_rejects_invalid_email_format(client):
    response = client.post(
        "/auth/register", json={"email": "not-an-email", "password": "password123"}
    )

    assert response.status_code == 400
