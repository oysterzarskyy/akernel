#!/usr/bin/env python3
"""Cookie-backed anonymous JSON API for the akernel community forum."""

from __future__ import annotations

import hashlib
import json
import mimetypes
import os
import re
import secrets
import sqlite3
from contextlib import contextmanager
from http import HTTPStatus
from http.cookies import CookieError, SimpleCookie
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Iterator
from urllib.parse import parse_qs, unquote, urlsplit


ROOT = Path(__file__).resolve().parent
WEB_ROOT = ROOT.parent / "web"
DATABASE_PATH = Path(os.environ.get("AKERNEL_DATABASE", ROOT / "forum.sqlite3"))
ALLOWED_ORIGIN = os.environ.get("AKERNEL_ALLOWED_ORIGIN", "").rstrip("/")
COOKIE_SAMESITE = os.environ.get("AKERNEL_COOKIE_SAMESITE", "Lax")
COOKIE_SECURE = os.environ.get("AKERNEL_COOKIE_SECURE", "0").lower() in {
    "1", "true", "yes"
}
COOKIE_NAME = "ak_session"
COOKIE_MAX_AGE = 60 * 60 * 24 * 365
MAX_REQUEST_BYTES = 16_384
TOPIC_CATEGORIES = {"kernel", "hardware", "help", "community"}
TOPIC_ID_PATTERN = re.compile(r"^[0-9a-f]{32}$")


class ApiError(Exception):
    def __init__(self, status: HTTPStatus, message: str) -> None:
        super().__init__(message)
        self.status = status


@contextmanager
def _database() -> Iterator[sqlite3.Connection]:
    connection = sqlite3.connect(DATABASE_PATH, timeout=10)
    connection.row_factory = sqlite3.Row
    connection.execute("PRAGMA foreign_keys = ON")
    try:
        yield connection
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()


def _initialize_database() -> None:
    DATABASE_PATH.parent.mkdir(parents=True, exist_ok=True)
    with _database() as connection:
        connection.executescript(
            """
            CREATE TABLE IF NOT EXISTS sessions (
                token_hash TEXT PRIMARY KEY,
                display_name TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT (
                    strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
                ),
                last_seen_at TEXT NOT NULL DEFAULT (
                    strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
                )
            );
            CREATE TABLE IF NOT EXISTS topics (
                id TEXT PRIMARY KEY,
                author_hash TEXT NOT NULL REFERENCES sessions(token_hash),
                title TEXT NOT NULL,
                category TEXT NOT NULL,
                body TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT (
                    strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
                ),
                last_activity TEXT NOT NULL DEFAULT (
                    strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
                )
            );
            CREATE TABLE IF NOT EXISTS replies (
                id TEXT PRIMARY KEY,
                topic_id TEXT NOT NULL REFERENCES topics(id) ON DELETE CASCADE,
                author_hash TEXT NOT NULL REFERENCES sessions(token_hash),
                body TEXT NOT NULL,
                created_at TEXT NOT NULL DEFAULT (
                    strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
                )
            );
            CREATE INDEX IF NOT EXISTS topics_activity_idx
                ON topics(last_activity DESC);
            CREATE INDEX IF NOT EXISTS replies_topic_idx
                ON replies(topic_id, created_at);
            """
        )


def _token_hash(token: str) -> str:
    return hashlib.sha256(token.encode("ascii")).hexdigest()


class ForumHandler(BaseHTTPRequestHandler):
    server_version = "akernel-forum-api/1"

    def log_message(self, format_string: str, *args: object) -> None:
        print("%s - %s" % (self.address_string(), format_string % args))

    def _headers(self, status: HTTPStatus, content_type: str = "application/json") -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Referrer-Policy", "no-referrer")
        origin = self.headers.get("Origin")
        if origin and ALLOWED_ORIGIN and origin.rstrip("/") == ALLOWED_ORIGIN:
            self.send_header("Access-Control-Allow-Origin", ALLOWED_ORIGIN)
            self.send_header("Access-Control-Allow-Credentials", "true")
            self.send_header("Vary", "Origin")

    def _send_static(self, path: str) -> None:
        relative_path = unquote(path).lstrip("/") or "index.html"
        target = (WEB_ROOT / relative_path).resolve()
        try:
            target.relative_to(WEB_ROOT.resolve())
        except ValueError:
            raise ApiError(HTTPStatus.NOT_FOUND, "File not found.") from None
        if target.is_dir():
            target = target / "index.html"
        if not target.is_file():
            raise ApiError(HTTPStatus.NOT_FOUND, "File not found.")
        body = target.read_bytes()
        content_type = mimetypes.guess_type(target.name)[0] or "application/octet-stream"
        self._headers(HTTPStatus.OK, content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_json(
        self,
        payload: dict[str, object],
        status: HTTPStatus = HTTPStatus.OK,
        cookie: str | None = None,
    ) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self._headers(status)
        if cookie:
            self.send_header("Set-Cookie", cookie)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _error(self, error: ApiError) -> None:
        self._send_json({"error": str(error)}, error.status)

    def _check_origin(self) -> None:
        origin = self.headers.get("Origin")
        if not origin:
            return
        if ALLOWED_ORIGIN:
            expected_origin = ALLOWED_ORIGIN
        else:
            scheme = self.headers.get("X-Forwarded-Proto", "http").split(",", 1)[0].strip()
            expected_origin = f"{scheme}://{self.headers.get('Host', '')}"
        if origin.rstrip("/") != expected_origin:
            raise ApiError(HTTPStatus.FORBIDDEN, "Origin is not allowed.")

    def _read_json(self) -> dict[str, object]:
        if self.headers.get_content_type() != "application/json":
            raise ApiError(HTTPStatus.UNSUPPORTED_MEDIA_TYPE, "Send application/json.")
        try:
            size = int(self.headers.get("Content-Length", "0"))
        except ValueError as error:
            raise ApiError(HTTPStatus.BAD_REQUEST, "Invalid content length.") from error
        if size < 0 or size > MAX_REQUEST_BYTES:
            raise ApiError(HTTPStatus.REQUEST_ENTITY_TOO_LARGE, "Request body is too large.")
        try:
            payload = json.loads(self.rfile.read(size))
        except (json.JSONDecodeError, UnicodeDecodeError) as error:
            raise ApiError(HTTPStatus.BAD_REQUEST, "Request body must be valid JSON.") from error
        if not isinstance(payload, dict):
            raise ApiError(HTTPStatus.BAD_REQUEST, "Request body must be a JSON object.")
        return payload

    def _cookie_token(self) -> str | None:
        cookies = SimpleCookie()
        try:
            cookies.load(self.headers.get("Cookie", ""))
        except CookieError:
            return None
        morsel = cookies.get(COOKIE_NAME)
        if morsel is None or not re.fullmatch(r"[0-9a-f]{64}", morsel.value):
            return None
        return morsel.value

    def _get_session(self, create: bool = False) -> tuple[sqlite3.Row, str | None]:
        token = self._cookie_token()
        if token:
            token_hash = _token_hash(token)
            with _database() as connection:
                session = connection.execute(
                    "SELECT token_hash, display_name FROM sessions WHERE token_hash = ?",
                    (token_hash,),
                ).fetchone()
                if session:
                    connection.execute(
                        "UPDATE sessions SET last_seen_at = "
                        "strftime('%Y-%m-%dT%H:%M:%fZ', 'now') WHERE token_hash = ?",
                        (token_hash,),
                    )
                    return session, None
        if not create:
            raise ApiError(HTTPStatus.UNAUTHORIZED, "Start an anonymous session first.")

        token = secrets.token_hex(32)
        token_hash = _token_hash(token)
        display_name = f"Guest-{token[:4].upper()}"
        with _database() as connection:
            connection.execute(
                "INSERT INTO sessions (token_hash, display_name) VALUES (?, ?)",
                (token_hash, display_name),
            )
            session = connection.execute(
                "SELECT token_hash, display_name FROM sessions WHERE token_hash = ?",
                (token_hash,),
            ).fetchone()
        cookie_parts = [
            f"{COOKIE_NAME}={token}",
            "Path=/",
            f"Max-Age={COOKIE_MAX_AGE}",
            "HttpOnly",
            f"SameSite={COOKIE_SAMESITE}",
        ]
        if COOKIE_SECURE:
            cookie_parts.append("Secure")
        return session, "; ".join(cookie_parts)

    def _author(self, token_hash: str) -> str:
        with _database() as connection:
            session = connection.execute(
                "SELECT display_name FROM sessions WHERE token_hash = ?",
                (token_hash,),
            ).fetchone()
        if not session:
            raise ApiError(HTTPStatus.UNAUTHORIZED, "Anonymous session has expired.")
        return str(session["display_name"])

    def _topic_row(self, topic_id: str) -> sqlite3.Row:
        with _database() as connection:
            topic = connection.execute(
                """
                SELECT t.id, t.author_hash, t.title, t.category, t.body,
                       t.created_at, t.last_activity,
                       s.display_name AS author,
                       (SELECT count(*) FROM replies r WHERE r.topic_id = t.id)
                           AS reply_count
                FROM topics t
                JOIN sessions s ON s.token_hash = t.author_hash
                WHERE t.id = ?
                """,
                (topic_id,),
            ).fetchone()
        if topic is None:
            raise ApiError(HTTPStatus.NOT_FOUND, "Discussion not found.")
        return topic

    def do_OPTIONS(self) -> None:
        try:
            self._check_origin()
        except ApiError as error:
            self._error(error)
            return
        self._headers(HTTPStatus.NO_CONTENT)
        if self.headers.get("Origin"):
            self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "Content-Type")
            self.send_header("Access-Control-Max-Age", "600")
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_GET(self) -> None:
        try:
            self._check_origin()
            parsed = urlsplit(self.path)
            path = parsed.path.rstrip("/")
            if not path.startswith("/api/"):
                self._send_static(path)
                return
            if path == "/api/health":
                self._send_json({"status": "ok", "service": "akernel-forum"})
                return
            if path == "/api/session":
                session, cookie = self._get_session(create=True)
                self._send_json(
                    {"user": {"display_name": session["display_name"], "anonymous": True}},
                    cookie=cookie,
                )
                return
            if path == "/api/topics":
                query = parse_qs(parsed.query)
                category = query.get("category", [""])[0]
                sort = query.get("sort", ["latest"])[0]
                if category and (
                    not isinstance(category, str) or category not in TOPIC_CATEGORIES
                ):
                    raise ApiError(HTTPStatus.BAD_REQUEST, "Unknown discussion category.")
                if sort not in {"latest", "active"}:
                    raise ApiError(HTTPStatus.BAD_REQUEST, "Sort must be latest or active.")
                order_by = "t.last_activity DESC" if sort == "active" else "t.created_at DESC"
                sql = f"""
                    SELECT t.id, t.title, t.category, t.body, t.created_at,
                           t.last_activity, s.display_name AS author,
                           (SELECT count(*) FROM replies r WHERE r.topic_id = t.id)
                               AS reply_count
                    FROM topics t
                    JOIN sessions s ON s.token_hash = t.author_hash
                    {"WHERE t.category = ?" if category else ""}
                    ORDER BY {order_by}
                    LIMIT 100
                """
                with _database() as connection:
                    rows = connection.execute(sql, (category,) if category else ()).fetchall()
                self._send_json({"topics": [dict(row) for row in rows]})
                return
            match = re.fullmatch(r"/api/topics/([0-9a-f]{32})", path)
            if match:
                topic = self._topic_row(match.group(1))
                with _database() as connection:
                    replies = connection.execute(
                        """
                        SELECT r.id, r.body, r.created_at, s.display_name AS author
                        FROM replies r
                        JOIN sessions s ON s.token_hash = r.author_hash
                        WHERE r.topic_id = ?
                        ORDER BY r.created_at ASC
                        """,
                        (match.group(1),),
                    ).fetchall()
                self._send_json({**dict(topic), "replies": [dict(row) for row in replies]})
                return
            raise ApiError(HTTPStatus.NOT_FOUND, "API route not found.")
        except ApiError as error:
            self._error(error)

    def do_POST(self) -> None:
        try:
            self._check_origin()
            parsed = urlsplit(self.path)
            path = parsed.path.rstrip("/")
            if path == "/api/session":
                session, cookie = self._get_session(create=True)
                self._send_json(
                    {"user": {"display_name": session["display_name"], "anonymous": True}},
                    cookie=cookie,
                )
                return

            payload = self._read_json()
            session, _ = self._get_session()
            session_hash = str(session["token_hash"])
            if path == "/api/topics":
                title = payload.get("title")
                category = payload.get("category")
                body = payload.get("body")
                if not isinstance(title, str) or not 5 <= len(title.strip()) <= 120:
                    raise ApiError(HTTPStatus.BAD_REQUEST, "Title must be 5–120 characters.")
                if not isinstance(category, str) or category not in TOPIC_CATEGORIES:
                    raise ApiError(HTTPStatus.BAD_REQUEST, "Choose a valid discussion category.")
                if not isinstance(body, str) or not 20 <= len(body.strip()) <= 10000:
                    raise ApiError(HTTPStatus.BAD_REQUEST, "Details must be 20–10,000 characters.")
                topic_id = secrets.token_hex(16)
                with _database() as connection:
                    connection.execute(
                        """
                        INSERT INTO topics (id, author_hash, title, category, body)
                        VALUES (?, ?, ?, ?, ?)
                        """,
                        (topic_id, session_hash, title.strip(), category, body.strip()),
                    )
                self._send_json({"id": topic_id}, HTTPStatus.CREATED)
                return

            match = re.fullmatch(r"/api/topics/([0-9a-f]{32})/replies", path)
            if match:
                body = payload.get("body")
                if not isinstance(body, str) or not 1 <= len(body.strip()) <= 5000:
                    raise ApiError(HTTPStatus.BAD_REQUEST, "Reply must be 1–5,000 characters.")
                topic_id = match.group(1)
                with _database() as connection:
                    exists = connection.execute(
                        "SELECT 1 FROM topics WHERE id = ?", (topic_id,)
                    ).fetchone()
                    if not exists:
                        raise ApiError(HTTPStatus.NOT_FOUND, "Discussion not found.")
                    reply_id = secrets.token_hex(16)
                    connection.execute(
                        "INSERT INTO replies (id, topic_id, author_hash, body) "
                        "VALUES (?, ?, ?, ?)",
                        (reply_id, topic_id, session_hash, body.strip()),
                    )
                    connection.execute(
                        "UPDATE topics SET last_activity = "
                        "strftime('%Y-%m-%dT%H:%M:%fZ', 'now') WHERE id = ?",
                        (topic_id,),
                    )
                self._send_json({"id": reply_id}, HTTPStatus.CREATED)
                return

            raise ApiError(HTTPStatus.NOT_FOUND, "API route not found.")
        except ApiError as error:
            self._error(error)


def main() -> None:
    if COOKIE_SAMESITE not in {"Lax", "Strict", "None"}:
        raise SystemExit("AKERNEL_COOKIE_SAMESITE must be Lax, Strict, or None.")
    if COOKIE_SAMESITE == "None" and not COOKIE_SECURE:
        raise SystemExit("SameSite=None requires AKERNEL_COOKIE_SECURE=1.")
    _initialize_database()
    host = os.environ.get("AKERNEL_API_HOST", "127.0.0.1")
    port = int(os.environ.get("AKERNEL_API_PORT", "8787"))
    server = ThreadingHTTPServer((host, port), ForumHandler)
    print(f"akernel forum API listening on http://{host}:{port}")
    if ALLOWED_ORIGIN:
        print(f"Allowed browser origin: {ALLOWED_ORIGIN}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping akernel forum API.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
