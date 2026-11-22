
import subprocess
import time
from pathlib import Path

import pygetwindow as gw
import pyautogui


GAME_EXE = Path(
    r"C:\Users\RODRIGUES\Documents\XadrezES\Xadrez-ES2\interface\build\xadrez.exe"
)

WINDOW_TITLE = "Xadrez - ES2"


def main():
    if not GAME_EXE.exists():
        print("[ERRO] Executável não encontrado.")
        return

    print("[INFO] Iniciando o jogo...")

    process = subprocess.Popen(
        [str(GAME_EXE)],
        cwd=GAME_EXE.parent.parent
    )

    try:
        print("[INFO] Aguardando a janela abrir...")
        time.sleep(3)

        windows = gw.getWindowsWithTitle(WINDOW_TITLE)

        if not windows:
            print("[ERRO] Janela do xadrez não encontrada.")
            return

        window = windows[0]

        print("\n[SUCESSO] Janela encontrada!")
        print(f"Título: {window.title}")
        print(f"Posição X: {window.left}")
        print(f"Posição Y: {window.top}")
        print(f"Largura: {window.width}")
        print(f"Altura: {window.height}")

        # Captura inicial
        print("\n[INFO] Capturando tela inicial...")
        time.sleep(1)

        output = Path("screenshots")
        output.mkdir(exist_ok=True)

        screenshot = pyautogui.screenshot()
        screenshot.save(output / "poc_window.png")

        print("[SUCESSO] Captura inicial salva.")

        # Calcula a posição do botão em relação à janela
        relative_x = 567
        relative_y = 436

        click_x = window.left + relative_x
        click_y = window.top + relative_y

        print("\n[INFO] Preparando clique no botão Jogar...")
        time.sleep(2)

        pyautogui.click(click_x, click_y)

        print("[SUCESSO] Clique realizado no botão Jogar!")

        time.sleep(2)

        # Captura após a interação
        screenshot = pyautogui.screenshot()
        screenshot.save(output / "menu_jogar.png")

        print("[SUCESSO] Evidência salva em screenshots/menu_jogar.png")

    except KeyboardInterrupt:
        print("\n[INFO] Teste interrompido.")

    finally:
        process.terminate()
        print("[INFO] Processo do jogo encerrado.")


if __name__ == "__main__":
    main()
