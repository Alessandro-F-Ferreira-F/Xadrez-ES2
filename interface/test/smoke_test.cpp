#include "board.hpp"
#include "engine_bridge.hpp"
#include "../../tools/ctest/test_api.h"

#include <iostream>
#include <string>

TEST(interface_smoke_engine_pipe) {
    const std::string enginePath = "../engine/build/main.out";

    EngineBridge bridge;
    ASSERT_MSG(bridge.start(enginePath), "Nao foi possivel iniciar o processo da engine: %s", enginePath.c_str());

    ASSERT_MSG(bridge.sendCommand("uci"), "Falha ao enviar comando 'uci'");

    bool gotUciOk = false;
    for (int i = 0; i < 20; ++i) {
        std::string line = bridge.readCommand();
        if (line.empty()) {
            break;
        }
        if (line == "uciok") {
            gotUciOk = true;
            break;
        }
    }
    ASSERT_MSG(gotUciOk, "Nao recebeu 'uciok' da engine");

    ASSERT_MSG(bridge.sendCommand("isready"), "Falha ao enviar comando 'isready'");
    std::string readyResp = bridge.readCommand();
    ASSERT_MSG(readyResp == "readyok", "Esperava 'readyok', recebeu: '%s'", readyResp.c_str());

    ASSERT_MSG(bridge.stop(), "Falha ao encerrar o processo da engine");

    // Smoke test basico da estrutura de tabuleiro (sem abrir janela grafica)
    ChessBoard board;
    board.processUCICommand("position startpos moves e2e4 e7e5");
}

int main() {
    return test_run_all();
}
