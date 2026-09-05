from __future__ import annotations

import json
import mimetypes
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlparse

from .registry import (
    ControlNotFoundError,
    DeviceNotFoundError,
    DeviceRegistry,
    InvalidControlValueError,
)


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
        if not path.startswith(prefix):
            self.send_error(HTTPStatus.NOT_FOUND)
            return

        control_marker = "/controls/"
        remainder = path[len(prefix) :]
        if control_marker in remainder:
            encoded_device_id, encoded_control_id = remainder.split(control_marker, 1)
            self._handle_control(
                unquote(encoded_device_id),
                unquote(encoded_control_id),
            )
            return

        suffix = "/alias"
        if not remainder.endswith(suffix):
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        device_id = unquote(remainder[: -len(suffix)])
        try:
            payload = self._read_json_body()
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

    def _read_json_body(self) -> dict:
        length = int(self.headers.get("Content-Length", "0"))
        if length <= 0 or length > 4096:
            raise ValueError("Invalid request body size")
        payload = json.loads(self.rfile.read(length).decode("utf-8"))
        if not isinstance(payload, dict):
            raise ValueError
        return payload

    def _handle_control(self, device_id: str, control_id: str) -> None:
        try:
            payload = self._read_json_body()
            if "value" not in payload:
                raise InvalidControlValueError("Missing control value")
            result = self.server.registry.apply_control(
                device_id,
                control_id,
                payload["value"],
            )
        except ValueError as error:
            self._send_json({"error": str(error) or "Invalid request"}, HTTPStatus.BAD_REQUEST)
            return
        except (DeviceNotFoundError, ControlNotFoundError) as error:
            self._send_json({"error": str(error)}, HTTPStatus.NOT_FOUND)
            return
        except OSError as error:
            self._send_json(
                {"error": f"Device request failed: {error}"},
                HTTPStatus.BAD_GATEWAY,
            )
            return
        self._send_json({"ok": True, **result})

    def log_message(self, format: str, *args: object) -> None:
        if self.path != "/api/devices":
            super().log_message(format, *args)
