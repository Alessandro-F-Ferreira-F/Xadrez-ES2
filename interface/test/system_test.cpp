#include "board.hpp"
#include "engine_bridge.hpp"
#include "../../tools/ctest/test_api.h"

#include <chrono>
#include <iostream>
#include <string>

#define CHECK(cond, msg) ASSERT_MSG((cond), "%s", (msg))

static const char* ENGINE_BIN = "../Backend/build/main.out";

static std::string readBestMove(EngineBridge& bridge) {
    for (int i = 0; i < 16; ++i) {
        std::string line = bridge.readCommand();
        if (line.empty() || line.rfind("bestmove", 0) == 0) {
            return line;
        }
    }
    return "";
}

// 1. Teste de Sistema - Recuperacao (Slide 60):
// Forca situacoes extremas (binario invalido, comando corrompido, lance ilegal e reinicio do pipe)
// e verifica se o sistema se recupera integralmente sem perder o estado valido.
TEST(system_recovery) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    // 1.1 Tentativa de abrir executavel inexistente: o processo filho encerra com exit(1) e EOF no pipe
    CHECK(bridge.start("/caminho/inexistente/engine_fantasma"), "Fork ocorre mesmo com binario invalido");
    CHECK(bridge.readCommand().empty(), "Pipe de processo invalido deve retornar EOF vazio sem travar");
    CHECK(bridge.stop(), "Deve recolher o processo invalido e limpar os descritores");

    // 1.2 Recuperacao: sobe o motor valido na mesma instancia de EngineBridge
    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real apos falha anterior");
    CHECK(bridge.sendCommand("isready") && bridge.readCommand() == "readyok", "Motor operacional");

    // Configura uma posicao valida inicial (1. e4 e5)
    board.processUCICommand("position startpos moves e2e4 e7e5");
    CHECK(bridge.sendCommand("position startpos moves e2e4 e7e5"), "Envia posicao valida");

    // 1.3 Injeta comando malformado e lance ilegal no motor
    CHECK(bridge.sendCommand("position comando_invalido"), "Envia comando malformado");
    std::string err1 = bridge.readCommand();
    CHECK(err1 == "error bad-command position", "Motor deve reportar error bad-command position");

    CHECK(bridge.sendCommand("position startpos moves e2e4 e7e5 e4e6"), "Envia lance ilegal e4e6");
    std::string err2 = bridge.readCommand();
    CHECK(err2.find("error illegal-move e4e6") != std::string::npos, "Motor deve reportar error illegal-move");

    // 1.4 Verifica atomicidade e recuperacao: a posicao anterior (1. e4 e5) deve continuar intacta no motor
    CHECK(bridge.sendCommand("state"), "Consulta state apos erros");
    std::string stateAfterErr = bridge.readCommand();
    CHECK(stateAfterErr.find("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w") != std::string::npos,
          "Estado do motor permaneceu intacto apos comandos invalidos");

    // Sistema segue jogando normalmente
    CHECK(bridge.sendCommand("go"), "Envia go apos recuperacao");
    std::string best = readBestMove(bridge);
    CHECK(best.rfind("bestmove ", 0) == 0 && best != "bestmove 0000", "Recebe bestmove valido apos recuperacao");
    board.processUCICommand(best);
    CHECK(board.getMoveHistory().size() == 3, "Tabuleiro aplicou o 3o lance normalmente");

    bridge.stop();
}

// 2. Teste de Sistema - Estresse/Carga e Desempenho RNF01 (< 200 ms por turno) (Slides 60 e 71)
// Submete Interface + Pipe + Engine a 40 meios-lances seguidos medindo latencia maxima e media.
TEST(system_stress_and_latency_rnf01) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");
    CHECK(bridge.sendCommand("ucinewgame"), "Inicia nova partida");

    const int totalPlies = 40;
    double maxTurnMs = 0.0;
    double sumTurnMs = 0.0;
    int completedPlies = 0;

    for (int ply = 0; ply < totalPlies; ++ply) {
        // Sincroniza posicao acumulada
        std::string posCmd = "position startpos";
        if (!board.getMoveHistory().empty()) {
            posCmd += " moves";
            for (const auto& m : board.getMoveHistory()) {
                posCmd += " " + m;
            }
        }
        CHECK(bridge.sendCommand(posCmd), "Envia position no teste de estresse");

        // Consulta state e legalmoves (como a interface faz a cada turno)
        CHECK(bridge.sendCommand("state"), "Envia state");
        std::string stateLine = bridge.readCommand();
        if (stateLine.find("result *") == std::string::npos) {
            break; // Partida terminou naturalmente antes dos 40 meios-lances
        }

        CHECK(bridge.sendCommand("legalmoves"), "Envia legalmoves");
        board.processUCICommand(bridge.readCommand());
        CHECK(!board.getLegalMoves().empty(), "Deve haver lances legais para partida em andamento");

        // Pede lance ao motor e aplica no ChessBoard (mede latencia RNF01 da resposta do motor)
        auto t0 = std::chrono::steady_clock::now();
        CHECK(bridge.sendCommand("go"), "Envia go");
        std::string bestLine = readBestMove(bridge);
        auto t1 = std::chrono::steady_clock::now();
        CHECK(bestLine.rfind("bestmove ", 0) == 0 && bestLine != "bestmove 0000", "Deve receber bestmove valido");
        board.processUCICommand(bestLine);

        double elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (elapsedMs > maxTurnMs) {
            maxTurnMs = elapsedMs;
        }
        sumTurnMs += elapsedMs;
        completedPlies++;
    }

    double avgTurnMs = (completedPlies > 0) ? (sumTurnMs / completedPlies) : 0.0;
    std::cout << "    [METRICA RNF01] " << completedPlies << " meios-lances executados | "
              << "Media: " << avgTurnMs << " ms | Max: " << maxTurnMs << " ms (Limite medio RNF01: 200 ms)"
              << std::endl;

    CHECK(completedPlies > 0, "Deve ter executado lances no teste de carga");
    CHECK(avgTurnMs < 200.0, "Latencia media por turno deve ser < 200 ms (RNF01)");
    CHECK(maxTurnMs < 650.0, "Latencia de pico por turno dentro da tolerancia de escalonamento SO");

    bridge.stop();
}

int main() {
    return test_run_all();
}
