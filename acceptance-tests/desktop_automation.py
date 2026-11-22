from __future__ import annotations

import ctypes
import os
import re
import subprocess
import time
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
from typing import Callable, Optional, Tuple

from PIL import Image, ImageChops


LOGICAL_WIDTH = 1120
LOGICAL_HEIGHT = 800
WINDOW_TITLE = "Xadrez - ES2"
ROOT = Path(__file__).resolve().parents[1]
INTERFACE_DIR = ROOT / "interface"


class DesktopAutomationUnavailable(RuntimeError):
    pass


class DesktopAutomationTimeout(TimeoutError):
    pass


class BoardVisionUncertain(AssertionError):
    pass


def enable_dpi_awareness() -> None:
    if os.name != "nt":
        return
    user32 = ctypes.windll.user32
    try:
        if user32.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4)):
            return
    except AttributeError:
        pass
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(2)
    except (AttributeError, OSError):
        user32.SetProcessDPIAware()


enable_dpi_awareness()

try:
    import pyautogui
    import pygetwindow

    pyautogui.FAILSAFE = True
    pyautogui.PAUSE = 0
    _DESKTOP_IMPORT_ERROR: Optional[BaseException] = None
except Exception as exc:
    pyautogui = None
    pygetwindow = None
    _DESKTOP_IMPORT_ERROR = exc


if os.name == "nt":
    from ctypes import wintypes

    _USER32 = ctypes.windll.user32

    class _Rect(ctypes.Structure):
        _fields_ = [
            ("left", wintypes.LONG),
            ("top", wintypes.LONG),
            ("right", wintypes.LONG),
            ("bottom", wintypes.LONG),
        ]

    class _Point(ctypes.Structure):
        _fields_ = [("x", wintypes.LONG), ("y", wintypes.LONG)]
else:
    _USER32 = None


@dataclass(frozen=True)
class ClientRect:
    left: int
    top: int
    width: int
    height: int


@dataclass(frozen=True)
class ScreenCapture:
    image: Image.Image
    client_width: int
    client_height: int

    @property
    def scale(self) -> float:
        return min(
            self.client_width / LOGICAL_WIDTH,
            self.client_height / LOGICAL_HEIGHT,
        )

    @property
    def viewport_offset(self) -> Tuple[float, float]:
        scale = self.scale
        return (
            (self.client_width - LOGICAL_WIDTH * scale) / 2,
            (self.client_height - LOGICAL_HEIGHT * scale) / 2,
        )

    def logical_to_image(self, x: float, y: float) -> Tuple[int, int]:
        scale = self.scale
        offset_x, offset_y = self.viewport_offset
        pixel_x = round(offset_x + x * scale)
        pixel_y = round(offset_y + y * scale)
        if not (0 <= pixel_x < self.image.width and 0 <= pixel_y < self.image.height):
            raise ValueError(f"Coordenada lógica fora da área visível: ({x}, {y})")
        return pixel_x, pixel_y

    def rgb_at(self, x: float, y: float) -> Tuple[int, int, int]:
        pixel_x, pixel_y = self.logical_to_image(x, y)
        pixel = self.image.convert("RGB").getpixel((pixel_x, pixel_y))
        return pixel

    def crop_logical(self, box: Tuple[float, float, float, float]) -> Image.Image:
        left, top, right, bottom = box
        scale = self.scale
        offset_x, offset_y = self.viewport_offset
        image_box = (
            max(0, round(offset_x + left * scale)),
            max(0, round(offset_y + top * scale)),
            min(self.image.width, round(offset_x + right * scale)),
            min(self.image.height, round(offset_y + bottom * scale)),
        )
        return self.image.crop(image_box)


def color_matches(
    capture: ScreenCapture,
    point: Tuple[float, float],
    expected: Tuple[int, int, int],
    tolerance: int = 24,
) -> bool:
    actual = capture.rgb_at(*point)
    return max(abs(a - b) for a, b in zip(actual, expected)) <= tolerance


def is_main_menu(capture: ScreenCapture) -> bool:
    return color_matches(capture, (250, 150), (22, 26, 34), 3)


def is_setup_screen(capture: ScreenCapture) -> bool:
    return color_matches(capture, (250, 150), (28, 33, 44), 3)


def is_game_board(capture: ScreenCapture) -> bool:
    return color_matches(capture, (350, 450), (181, 136, 99), 24)


def changed_pixels(
    before: ScreenCapture,
    after: ScreenCapture,
    box: Tuple[float, float, float, float],
    threshold: int = 20,
) -> int:
    if before.image.size != after.image.size:
        raise ValueError("As capturas têm dimensões diferentes")
    first = before.crop_logical(box).convert("RGB")
    second = after.crop_logical(box).convert("RGB")
    if first.size != second.size:
        raise ValueError("A região comparada ficou fora da área capturada")
    difference = ImageChops.difference(first, second)
    maximum_channel = ImageChops.lighter(
        ImageChops.lighter(difference.getchannel("R"), difference.getchannel("G")),
        difference.getchannel("B"),
    )
    return sum(maximum_channel.histogram()[threshold + 1 :])


def count_dark_pixels(
    capture: ScreenCapture,
    box: Tuple[float, float, float, float],
    maximum_luminance: int = 70,
) -> int:
    histogram = capture.crop_logical(box).convert("L").histogram()
    return sum(histogram[: maximum_luminance + 1])


@lru_cache(maxsize=32)
def _piece_templates(width: int, height: int):
    templates = []
    pieces_dir = INTERFACE_DIR / "assets" / "pieces"
    for path in sorted(pieces_dir.glob("*.png")):
        image = Image.open(path).convert("RGBA").resize(
            (width, height), Image.Resampling.BILINEAR
        )
        alpha_mask = image.getchannel("A").point(
            lambda alpha: 255 if alpha >= 240 else 0
        )
        opaque_pixels = sum(alpha_mask.histogram()[1:])
        if opaque_pixels:
            templates.append(
                (path.stem, image.convert("RGB"), alpha_mask, opaque_pixels)
            )
    if len(templates) != 12:
        raise FileNotFoundError(
            f"Esperadas 12 texturas de peças em {pieces_dir}; "
            f"encontradas {len(templates)}."
        )
    return tuple(templates)


def piece_at(capture: ScreenCapture, column: int, row: int) -> Optional[str]:
    if not (0 <= column < 8 and 0 <= row < 8):
        raise ValueError(f"Casa fora do tabuleiro: coluna={column}, linha={row}")

    square = capture.crop_logical(
        (column * 100, row * 100, (column + 1) * 100, (row + 1) * 100)
    )
    if square.width == 0 or square.height == 0:
        raise ValueError("A casa do tabuleiro não está visível na captura")

    ranked = []
    for name, template, alpha_mask, opaque_pixels in _piece_templates(*square.size):
        difference = ImageChops.difference(square.convert("RGB"), template)
        maximum_channel = ImageChops.lighter(
            ImageChops.lighter(
                difference.getchannel("R"), difference.getchannel("G")
            ),
            difference.getchannel("B"),
        )
        histogram = ImageChops.multiply(maximum_channel, alpha_mask).histogram()
        error = sum(value * count for value, count in enumerate(histogram))
        ranked.append((error / opaque_pixels, name))

    ranked.sort()
    best_error, best_name = ranked[0]
    second_error = ranked[1][0]
    if best_error <= 35 and second_error - best_error >= 8:
        color, piece_type = best_name.split("-", 1)
        fen_piece = {
            "pawn": "p",
            "knight": "n",
            "bishop": "b",
            "rook": "r",
            "queen": "q",
            "king": "k",
        }[piece_type]
        return fen_piece.upper() if color == "w" else fen_piece
    if best_error >= 65:
        return None
    raise BoardVisionUncertain(
        f"Não foi possível classificar a casa {column},{row}: "
        f"melhor textura={best_name}, erro={best_error:.1f}, "
        f"segunda={second_error:.1f}."
    )


class GameSession:
    def __init__(self, run_dir: Path, test_name: str):
        self.run_dir = run_dir
        self.test_name = test_name
        self.evidence_dir = run_dir / "evidence" / self._slug(test_name)
        self.evidence_dir.mkdir(parents=True, exist_ok=True)
        self.evidence: list[Path] = []
        self.log_path = run_dir / "logs" / f"{self._slug(test_name)}.log"
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        self.process: Optional[subprocess.Popen[bytes]] = None
        self.window = None
        self.hwnd: Optional[int] = None
        self._log_handle = None
        self.last_capture: Optional[ScreenCapture] = None
        self.observed_behaviors: list[str] = []
        self.protocol_evidence: list[str] = []
        self.verification_limitations: list[str] = []

    @staticmethod
    def _slug(value: str) -> str:
        slug = re.sub(r"[^A-Za-z0-9_.-]+", "-", value).strip("-.")
        return slug or "scenario"

    @staticmethod
    def _game_executable() -> Path:
        configured = os.environ.get("CHESS_GAME_EXE")
        if configured:
            return Path(configured).expanduser().resolve()
        return INTERFACE_DIR / "build" / "xadrez.exe"

    @staticmethod
    def _engine_executable() -> Path:
        return ROOT / "Backend" / "build" / "main.exe"

    def launch(self, timeout: float = 25.0) -> None:
        if os.name != "nt":
            raise DesktopAutomationUnavailable(
                "Esta suíte usa o executável Windows e as APIs Win32 de janela; "
                "execute-a em uma sessão gráfica interativa do Windows."
            )
        if _DESKTOP_IMPORT_ERROR is not None:
            raise DesktopAutomationUnavailable(
                f"Falha ao carregar PyAutoGUI/PyGetWindow: {_DESKTOP_IMPORT_ERROR}"
            ) from _DESKTOP_IMPORT_ERROR

        executable = self._game_executable()
        engine = self._engine_executable()
        if not executable.is_file():
            raise FileNotFoundError(
                f"Jogo não encontrado: {executable}. Compile a interface ou defina CHESS_GAME_EXE."
            )
        if not engine.is_file():
            raise FileNotFoundError(
                f"Motor não encontrado: {engine}. Compile-o com o Makefile do Backend."
            )

        self._log_handle = self.log_path.open("wb")
        try:
            self.process = subprocess.Popen(
                [str(executable)],
                cwd=str(INTERFACE_DIR),
                stdin=subprocess.DEVNULL,
                stdout=self._log_handle,
                stderr=subprocess.STDOUT,
                creationflags=0x08000000,
            )
        except OSError:
            self._close_log()
            raise

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.process.poll() is not None:
                raise RuntimeError(
                    f"O jogo encerrou durante a inicialização "
                    f"(código {self.process.returncode}). Log: {self._log_tail()}"
                )
            window = self._find_window_for_process()
            if window is not None:
                self.window = window
                self.hwnd = int(window._hWnd)
                break
            time.sleep(0.1)
        if self.hwnd is None:
            raise DesktopAutomationTimeout(
                f"A janela '{WINDOW_TITLE}' não apareceu em {timeout:.1f}s. "
                f"Log: {self._log_tail()}"
            )

        self._fit_window_to_work_area()
        self.focus()
        self.move_to_safe_area()
        initial = self.wait_for(
            "menu inicial",
            lambda image: is_main_menu(image),
            timeout=8.0,
        )
        self.save_capture("00-menu-inicial", initial)

    def _find_window_for_process(self):
        assert self.process is not None
        for candidate in pygetwindow.getWindowsWithTitle(WINDOW_TITLE):
            handle = int(candidate._hWnd)
            process_id = wintypes.DWORD()
            _USER32.GetWindowThreadProcessId(
                wintypes.HWND(handle), ctypes.byref(process_id)
            )
            if process_id.value == self.process.pid and _USER32.IsWindowVisible(
                wintypes.HWND(handle)
            ):
                return candidate
        return None

    def _client_rect(self) -> ClientRect:
        if self.hwnd is None:
            raise RuntimeError("A janela do jogo ainda não foi localizada")
        rect = _Rect()
        point = _Point(0, 0)
        handle = wintypes.HWND(self.hwnd)
        if not _USER32.GetClientRect(handle, ctypes.byref(rect)):
            raise ctypes.WinError()
        if not _USER32.ClientToScreen(handle, ctypes.byref(point)):
            raise ctypes.WinError()
        return ClientRect(
            left=point.x,
            top=point.y,
            width=rect.right - rect.left,
            height=rect.bottom - rect.top,
        )

    def _work_area(self) -> _Rect:
        rect = _Rect()
        if not _USER32.SystemParametersInfoW(0x0030, 0, ctypes.byref(rect), 0):
            raise ctypes.WinError()
        return rect

    def _outer_size_for_client(self, width: int, height: int) -> Tuple[int, int]:
        assert self.hwnd is not None
        get_long = getattr(_USER32, "GetWindowLongPtrW")
        get_long.restype = ctypes.c_ssize_t
        get_long.argtypes = [wintypes.HWND, ctypes.c_int]
        handle = wintypes.HWND(self.hwnd)
        style = get_long(handle, -16)
        ex_style = get_long(handle, -20)
        rect = _Rect(0, 0, width, height)
        if not _USER32.AdjustWindowRectEx(
            ctypes.byref(rect), style, False, ex_style
        ):
            raise ctypes.WinError()
        return rect.right - rect.left, rect.bottom - rect.top

    def _set_client_size(self, width: int, height: int) -> None:
        assert self.hwnd is not None
        work = self._work_area()
        outer_width, outer_height = self._outer_size_for_client(width, height)
        x = work.left + (work.right - work.left - outer_width) // 2
        y = work.top + (work.bottom - work.top - outer_height) // 2
        flags = 0x0004 | 0x0010
        if not _USER32.SetWindowPos(
            wintypes.HWND(self.hwnd),
            None,
            x,
            y,
            outer_width,
            outer_height,
            flags,
        ):
            raise ctypes.WinError()

    def _fit_window_to_work_area(self) -> None:
        work = self._work_area()
        work_width = work.right - work.left
        work_height = work.bottom - work.top
        scale = min(1.0, (work_width - 40) / LOGICAL_WIDTH, (work_height - 80) / LOGICAL_HEIGHT)
        if scale < 0.6:
            raise DesktopAutomationUnavailable(
                f"Área de trabalho pequena demais para a interface: "
                f"{work_width}x{work_height}px."
            )
        width = round(LOGICAL_WIDTH * scale)
        height = round(LOGICAL_HEIGHT * scale)
        self._set_client_size(width, height)
        self._wait_for_client_size(width, height)

    def resize_client(self, width: int, height: int, timeout: float = 3.0) -> None:
        if width <= 0 or height <= 0:
            raise ValueError("As dimensões do cliente precisam ser positivas")
        self._set_client_size(width, height)
        self._wait_for_client_size(width, height, timeout)
        self.focus()

    def _wait_for_client_size(
        self, width: int, height: int, timeout: float = 3.0
    ) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            rect = self._client_rect()
            if rect.width == width and rect.height == height:
                return
            time.sleep(0.05)
        rect = self._client_rect()
        raise DesktopAutomationTimeout(
            f"Redimensionamento não aplicado: esperado {width}x{height}, "
            f"obtido {rect.width}x{rect.height}."
        )

    def focus(self, timeout: float = 2.0) -> None:
        if self.hwnd is None:
            raise RuntimeError("A janela do jogo ainda não foi localizada")
        handle = wintypes.HWND(self.hwnd)
        _USER32.ShowWindow(handle, 9)
        _USER32.BringWindowToTop(handle)
        _USER32.SetForegroundWindow(handle)
        if self.window is not None:
            try:
                self.window.activate()
            except Exception:
                pass

        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if int(_USER32.GetForegroundWindow()) == self.hwnd:
                return
            _USER32.SetForegroundWindow(handle)
            time.sleep(0.05)
        raise DesktopAutomationTimeout(
            f"Não foi possível ativar a janela '{WINDOW_TITLE}'."
        )

    def _screen_point(self, x: float, y: float) -> Tuple[int, int]:
        rect = self._client_rect()
        scale = min(rect.width / LOGICAL_WIDTH, rect.height / LOGICAL_HEIGHT)
        offset_x = (rect.width - LOGICAL_WIDTH * scale) / 2
        offset_y = (rect.height - LOGICAL_HEIGHT * scale) / 2
        pixel_x = round(rect.left + offset_x + x * scale)
        pixel_y = round(rect.top + offset_y + y * scale)
        if not (
            rect.left <= pixel_x < rect.left + rect.width
            and rect.top <= pixel_y < rect.top + rect.height
        ):
            raise ValueError(f"Coordenada lógica fora da janela: ({x}, {y})")
        return pixel_x, pixel_y

    def click(self, x: float, y: float) -> None:
        self.focus()
        pixel_x, pixel_y = self._screen_point(x, y)
        pyautogui.moveTo(pixel_x, pixel_y, duration=0.08)
        pyautogui.click()

    def press(self, key: str) -> None:
        self.focus()
        pyautogui.press(key)

    def move_to_safe_area(self) -> None:
        self.focus()
        pixel_x, pixel_y = self._screen_point(20, 20)
        pyautogui.moveTo(pixel_x, pixel_y, duration=0.05)

    def capture(self) -> ScreenCapture:
        rect = self._client_rect()
        if rect.width <= 0 or rect.height <= 0:
            raise RuntimeError("A janela do jogo não tem área de cliente visível")
        try:
            image = pyautogui.screenshot(
                region=(rect.left, rect.top, rect.width, rect.height)
            ).convert("RGB")
        except Exception as exc:
            raise DesktopAutomationUnavailable(
                f"Não foi possível capturar a janela do jogo: {exc}"
            ) from exc
        capture = ScreenCapture(image, rect.width, rect.height)
        self.last_capture = capture
        return capture

    def save_capture(
        self, label: str, capture: Optional[ScreenCapture] = None
    ) -> Path:
        snapshot = capture or self.capture()
        path = self.evidence_dir / f"{self._slug(label)}.png"
        snapshot.image.save(path)
        self.evidence.append(path)
        return path

    def wait_for(
        self,
        description: str,
        predicate: Callable[[ScreenCapture], bool],
        timeout: float = 5.0,
        interval: float = 0.1,
    ) -> ScreenCapture:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.process is not None and self.process.poll() is not None:
                raise RuntimeError(
                    f"O jogo encerrou ao aguardar {description} "
                    f"(código {self.process.returncode}). Log: {self._log_tail()}"
                )
            capture = self.capture()
            if predicate(capture):
                return capture
            time.sleep(interval)
        raise DesktopAutomationTimeout(
            f"Tempo esgotado aguardando {description} ({timeout:.1f}s)."
        )

    def wait_for_exit(self, timeout: float = 5.0) -> int:
        if self.process is None:
            raise RuntimeError("O processo do jogo não foi iniciado")
        try:
            return self.process.wait(timeout=timeout)
        except subprocess.TimeoutExpired as exc:
            raise DesktopAutomationTimeout(
                f"O jogo não encerrou em {timeout:.1f}s após o comando de saída."
            ) from exc

    def _log_tail(self, limit: int = 2000) -> str:
        self._flush_log()
        try:
            return self.log_path.read_text(encoding="utf-8", errors="replace")[-limit:]
        except OSError as exc:
            return f"log indisponível: {exc}"

    def logged_uci_responses(self) -> list[str]:
        try:
            lines = self.log_path.read_text(
                encoding="utf-8", errors="replace"
            ).splitlines()
        except OSError as exc:
            raise RuntimeError(f"Não foi possível ler o log do jogo: {exc}") from exc

        prefix = "[UCI] Comando recebido: "
        return [
            line.split(prefix, 1)[1]
            for line in lines
            if prefix in line
            and line.split(prefix, 1)[1].startswith(
                ("state ", "bestmove ", "info ")
            )
        ]

    def record_observation(self, text: str) -> None:
        if text not in self.observed_behaviors:
            self.observed_behaviors.append(text)

    def record_protocol_evidence(self, text: str) -> None:
        if text not in self.protocol_evidence:
            self.protocol_evidence.append(text)

    def record_limitation(self, text: str) -> None:
        if text not in self.verification_limitations:
            self.verification_limitations.append(text)

    def _flush_log(self) -> None:
        if self._log_handle is not None:
            self._log_handle.flush()

    def _close_log(self) -> None:
        if self._log_handle is not None:
            self._log_handle.close()
            self._log_handle = None

    def close(self) -> None:
        process = self.process
        if process is None:
            self._close_log()
            return
        forced = False
        if process.poll() is None and self.hwnd is not None:
            _USER32.PostMessageW(wintypes.HWND(self.hwnd), 0x0010, 0, 0)
        if process.poll() is None:
            try:
                process.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                forced = True
                process.terminate()
                try:
                    process.wait(timeout=2.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=2.0)
        self._close_log()
        if forced:
            raise DesktopAutomationTimeout(
                "O encerramento normal do jogo expirou; o processo teve de ser terminado."
            )
