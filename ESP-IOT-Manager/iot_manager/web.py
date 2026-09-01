from __future__ import annotations

import json
import mimetypes
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlparse

from .registry import DeviceRegistry


class DashboardServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, address: tuple[str, int], registry: DeviceRegistry, static_dir: Path):
        super().__init__(address, DashboardHandler)
        self.registry = registry
        self.static_dir = static_dir


class DashboardHandler(BaseHTTPRequestHandler):
    server: DashboardServer

    def _send_json(self, payload: object, status: HTTPStatus = HTTPStatus.OK) -> None:
        body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _send_static(self, filename: str) -> None:
        path = self.server.static_dir / filename
        try:
            body = path.read_bytes()
        except OSError:
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        content_type = mimetypes.guess_type(path.name)[0] or "application/octet-stream"
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:  # noqa: N802 - required by BaseHTTPRequestHandler
        path = urlparse(self.path).path
        if path == "/api/devices":
            self._send_json({"devices": self.server.registry.snapshot()})
            return
        static_files = {"/": "index.html", "/app.js": "app.js", "/styles.css": "styles.css"}
        if path in static_files:
            self._send_static(static_files[path])
            return
        self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self) -> None:  # noqa: N802 - required by BaseHTTPRequestHandler
        path = urlparse(self.path).path
        prefix = "/api/devices/"
        suffix = "/alias"
        if not (path.startswith(prefix) and path.endswith(suffix)):
            self.send_error(HTTPStatus.NOT_FOUND)
            return

        device_id = unquote(path[len(prefix) : -len(suffix)])
        try:
            length = int(self.headers.get("Content-Length", "0"))
            payload = json.loads(self.rfile.read(length).decode("utf-8"))
            alias = payload.get("alias", "")
            if not isinstance(alias, str):
                raise ValueError
        except (ValueError, json.JSONDecodeError):
            self._send_json({"error": "Expected a JSON string field named alias"}, HTTPStatus.BAD_REQUEST)
            return

        if not self.server.registry.set_alias(device_id, alias):
            self._send_json({"error": "Unknown device"}, HTTPStatus.NOT_FOUND)
            return
        self._send_json({"ok": True})

    def log_message(self, format: str, *args: object) -> None:
        if self.path != "/api/devices":
            super().log_message(format, *args)
