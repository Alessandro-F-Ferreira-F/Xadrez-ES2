#include "engine_bridge.hpp"
#include "board.hpp"

#include <iostream>
#include <string>

static int fail(const std::string& msg) {
    std::cerr << "[FAIL] " << msg << std::endl;
    return 1;
}

int main(int argc, char* argv[]) {
    const std::string enginePath = (argc > 1) ? argv[1] : "../Backend/build/main.out";

    std::cout << "[SMOKE] Iniciando motor: " << enginePath << std::endl;

    EngineBridge bridge;
    if (!bridge.start(enginePath)) {
        return fail("Nao foi possivel iniciar o processo da engine.");
    }

    if (!bridge.sendCommand("uci")) {
        bridge.stop();
        return fail("Falha ao enviar comando 'uci'.");
    }

    bool gotUciOk = false;
    for (int i = 0; i < 20; ++i) {
        std::string line = bridge.readCommand();
        if (line.empty()) {
            break;
        }
        std::cout << "[SMOKE] <- " << line << std::endl;
        if (line == "uciok") {
            gotUciOk = true;
            break;
        }
    }

    if (!gotUciOk) {
        bridge.stop();
        return fail("Nao recebeu 'uciok' da engine.");
    }

    if (!bridge.sendCommand("isready")) {
        bridge.stop();
        return fail("Falha ao enviar comando 'isready'.");
    }

    std::string readyResp = bridge.readCommand();
    std::cout << "[SMOKE] <- " << readyResp << std::endl;
    if (readyResp != "readyok") {
        bridge.stop();
        return fail("Esperava 'readyok', recebeu: '" + readyResp + "'");
    }

    if (!bridge.stop()) {
        return fail("Falha ao encerrar o processo da engine.");
    }

    // Smoke test básico da estrutura de tabuleiro (sem abrir janela gráfica)
    ChessBoard board;
    board.processUCICommand("position startpos moves e2e4 e7e5");

    std::cout << "[PASS] Smoke test concluido com sucesso!" << std::endl;
    return 0;
}
