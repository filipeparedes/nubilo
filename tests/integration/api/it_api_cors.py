from helpers import CORS_ORIGIN


def test_preflight_from_allowed_origin_is_answered(client):
    headers = {
        "Origin": CORS_ORIGIN,
        "Access-Control-Request-Method": "POST",
        "Access-Control-Request-Headers": "authorization",
    }

    response = client.options("/files", headers=headers)

    assert response.status_code == 204
    assert response.headers["Access-Control-Allow-Origin"] == CORS_ORIGIN
    assert "POST" in response.headers["Access-Control-Allow-Methods"]
    assert "Authorization" in response.headers["Access-Control-Allow-Headers"]


def test_responses_to_allowed_origin_have_the_cors_headers(client):
    response = client.get("/health", headers={"Origin": CORS_ORIGIN})

    assert response.status_code == 200
    assert response.headers["Access-Control-Allow-Origin"] == CORS_ORIGIN
    assert response.headers["Access-Control-Expose-Headers"] == "Content-Disposition"


def test_error_responses_to_allowed_origin_have_the_cors_headers(client):
    # no token: the browser must still be able to read the 401
    response = client.get("/files", headers={"Origin": CORS_ORIGIN})

    assert response.status_code == 401
    assert response.headers["Access-Control-Allow-Origin"] == CORS_ORIGIN


def test_other_origins_get_no_cors_headers(client):
    response = client.get("/health", headers={"Origin": "http://evil.example.com"})

    assert response.status_code == 200
    assert "Access-Control-Allow-Origin" not in response.headers


def test_requests_without_origin_get_no_cors_headers(client):
    response = client.get("/health")

    assert response.status_code == 200
    assert "Access-Control-Allow-Origin" not in response.headers
