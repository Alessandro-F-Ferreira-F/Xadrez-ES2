#include "engine_bridge.hpp"
#include "board.hpp"
#include "../../tools/ctest/test_api.h"

#include <iostream>
#include <string>

#ifdef _WIN32
static std::string g_engine_path = "../Backend/build/main.exe";
#else
static std::string g_engine_path = "../Backend/build/main.out";
#endif

TEST(smoke_engine_handshake_and_board) {
    EngineBridge bridge;
    ASSERT_MSG(bridge.start(g_engine_path), "Nao foi possivel iniciar o processo da engine: %s", g_engine_path.c_str());
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

    if (!gotUciOk) {
        bridge.stop();
    }
    ASSERT_MSG(gotUciOk, "Nao recebeu 'uciok' da engine");

    if (!bridge.sendCommand("isready")) {
        bridge.stop();
        ASSERT_MSG(false, "Falha ao enviar comando 'isready'");
    }

    std::string readyResp = bridge.readCommand();
    if (readyResp != "readyok") {
        bridge.stop();
    }
    ASSERT_EQ_STR(readyResp.c_str(), "readyok");
    ASSERT_MSG(bridge.stop(), "Falha ao encerrar o processo da engine");

    // Smoke test basico da estrutura de tabuleiro (sem abrir janela grafica)
    ChessBoard board;
    board.processUCICommand("position startpos moves e2e4 e7e5");
    ASSERT_EQ_U64(board.getMoveHistory().size(), 2);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        g_engine_path = argv[1];
    }
    return test_run_all();
}
