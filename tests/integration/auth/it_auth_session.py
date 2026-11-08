from helpers import register_and_login


def test_expired_session_is_rejected_and_removed(client, server):
    token = register_and_login(client)

    server.execute(
        "UPDATE sessions SET expires_at = datetime('now', '-1 day') WHERE token = ?;",
        (token,),
    )

    response = client.post("/auth/logout", token=token)

    assert response.status_code == 401
    assert server.query("SELECT 1 FROM sessions WHERE token = ?;", (token,)) == []
