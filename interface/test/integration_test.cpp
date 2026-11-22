#include "board.hpp"
#include "engine_bridge.hpp"
#include "../../tools/ctest/test_api.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#define CHECK(cond, msg) ASSERT_MSG((cond), "%s", (msg))

#ifdef _WIN32
static const char* ENGINE_BIN = "../Backend/build/main.exe";
#else
static const char* ENGINE_BIN = "../Backend/build/main.out";
#endif

static std::string readBestMove(EngineBridge& bridge) {
    for (int i = 0; i < 16; ++i) {
        std::string line = bridge.readCommand();
        if (line.empty() || line.rfind("bestmove", 0) == 0) {
            return line;
        }
    }
    return "";
}

// 1. Integracao: Handshake UCI + Carregamento dos 20 lances legais iniciais no ChessBoard
TEST(integration_uci_handshake_and_legalmoves) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");
    CHECK(bridge.sendCommand("uci"), "Envia uci");

    bool uciOk = false;
    for (int i = 0; i < 10; ++i) {
        if (bridge.readCommand() == "uciok") {
            uciOk = true;
            break;
        }
    }
    CHECK(uciOk, "Motor real deve responder uciok");

    CHECK(bridge.sendCommand("position startpos"), "Envia position startpos");
    CHECK(bridge.sendCommand("legalmoves"), "Envia legalmoves");

    std::string legalLine = bridge.readCommand();
    board.processUCICommand(legalLine);
    CHECK(board.getLegalMoves().size() == 20, "Posicao inicial deve ter exatamente 20 lances legais");

    bridge.stop();
}

// 2. Integracao Incremental: Clique do Usuario no ChessBoard -> Pipe -> Resposta do Motor -> Tabuleiro
TEST(integration_mouse_click_turn_with_engine) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");
    CHECK(bridge.sendCommand("isready") && bridge.readCommand() == "readyok", "Motor pronto");

    // 1o clique em e2 (x=450, y=650): consulta legalmoves na engine, seleciona peca e destaca destinos
    bool turnCompleted = board.playTurnWithEngine(450.f, 650.f, bridge);
    CHECK(!turnCompleted, "1o clique apenas seleciona a peca em e2");
    CHECK(board.getSelectedCol() == 4 && board.getSelectedRow() == 6, "Casa e2 selecionada");
    CHECK(board.getHighlightedSquares().size() == 2, "Motor informou 2 destinos legais para e2 (e3 e e4)");

    // 2o clique em e4 (x=450, y=450): aplica e2e4, envia position + go ao motor e aplica bestmove das pretas
    turnCompleted = board.playTurnWithEngine(450.f, 450.f, bridge);
    CHECK(turnCompleted, "2o clique em e4 deve completar o turno Jogador + Engine");
    CHECK(board.getMoveHistory().size() == 2, "Historico deve ter 2 lances (e2e4 + resposta da engine)");
    CHECK(board.getMoveHistory()[0] == "e2e4", "1o lance do historico deve ser e2e4");
    CHECK(board.getCurrentTurn() == PieceColor::WHITE, "Vez voltou para WHITE apos resposta da engine");
    CHECK(!board.getLegalMoves().empty(), "Lista de lances legais atualizada para o turno 2");

    // Turno 2 (regressao): seleciona cavalo branco em g1 (x=650, y=750) e move para f3 (x=550, y=550)
    turnCompleted = board.playTurnWithEngine(650.f, 750.f, bridge);
    CHECK(!turnCompleted, "1o clique do turno 2 apenas seleciona g1");
    CHECK(board.getSelectedCol() == 6 && board.getSelectedRow() == 7, "g1 selecionado no turno 2");
    CHECK(board.getHighlightedSquares().size() == 3, "g1 deve ter 3 destinos legais (e2, f3 e h3) apos 1. e4");

    turnCompleted = board.playTurnWithEngine(550.f, 550.f, bridge);
    CHECK(turnCompleted, "2o clique do turno 2 em f3 deve completar o turno 2");
    CHECK(board.getMoveHistory().size() == 4, "Historico deve ter 4 lances apos 2 turnos");
    CHECK(board.getMoveHistory()[2] == "g1f3", "3o lance do historico deve ser g1f3");

    bridge.stop();
}

// 3. Integracao: Partida ate Xeque-Mate (Mate do Louco) sincronizada entre ChessBoard e Engine
TEST(integration_fools_mate_state) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");

    const std::string cmd = "position startpos moves f2f3 e7e5 g2g4 d8h4";
    board.processUCICommand(cmd);
    CHECK(bridge.sendCommand(cmd), "Envia sequencia do Mate do Louco para o motor");

    CHECK(bridge.sendCommand("state"), "Consulta estado da partida no motor");
    std::string stateLine = bridge.readCommand();
    CHECK(stateLine.find("check e1") != std::string::npos, "Rei branco em e1 deve estar em xeque");
    CHECK(stateLine.find("result 0-1") != std::string::npos, "Resultado deve ser 0-1 (vitoria das pretas)");
    CHECK(stateLine.find("reason checkmate") != std::string::npos, "Motivo deve ser checkmate");
    board.processUCICommand(stateLine);
    CHECK(board.isGameOver(), "ChessBoard deve marcar fim de jogo apos processar state de mate");
    CHECK(board.isInCheck() && board.getCheckCol() == 4 && board.getCheckRow() == 7,
          "ChessBoard deve marcar rei branco em e1 em xeque");
    CHECK(board.getGameResult() == "0-1" && board.getGameReason() == "checkmate",
          "ChessBoard deve guardar resultado e motivo do mate");

    CHECK(bridge.sendCommand("legalmoves"), "Consulta lances legais no mate");
    board.processUCICommand(bridge.readCommand());
    CHECK(board.getLegalMoves().empty(), "Nenhum lance legal disponivel em xeque-mate");

    CHECK(bridge.sendCommand("go"), "Pede go em posicao sem lances");
    std::string best = readBestMove(bridge);
    CHECK(best == "bestmove 0000", "Motor deve devolver bestmove 0000 no mate");
    board.processUCICommand(best);

    bridge.stop();
}

// 5. Integracao (Issue #7): Jogador aplica xeque-mate via playTurnWithEngine e interface detecta o fim de jogo
TEST(integration_checkmate_via_play_turn) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");

    // 1. e4 g5 2. d4 f6 (prepara 3. Qh5# - Mate do Louco para as brancas)
    const std::string setup = "position startpos moves e2e4 g7g5 d2d4 f7f6";
    board.processUCICommand(setup);
    CHECK(bridge.sendCommand(setup), "Envia pre-mate para o motor");
    CHECK(bridge.sendCommand("legalmoves"), "Consulta lances legais antes de Qh5#");
    board.processUCICommand(bridge.readCommand());

    // 1o clique em d1 (col 3, row 7 -> x=350, y=750)
    CHECK(!board.playTurnWithEngine(350.f, 750.f, bridge), "1o clique seleciona a dama em d1");
    // 2o clique em h5 (col 7, row 3 -> x=750, y=350): aplica d1h5#
    CHECK(board.playTurnWithEngine(750.f, 350.f, bridge), "2o clique aplica Qh5# e atualiza state");

    CHECK(board.isGameOver(), "playTurnWithEngine deve detectar fim de jogo apos Qh5#");
    CHECK(board.getGameResult() == "1-0", "Resultado deve ser 1-0 (vitoria das brancas)");
    CHECK(board.getGameReason() == "checkmate", "Motivo deve ser checkmate");
    CHECK(board.isInCheck() && board.getCheckCol() == 4 && board.getCheckRow() == 0,
          "Rei preto em e8 (4, 0) deve estar destacado em xeque");
    CHECK(board.getMoveHistory().size() == 5, "Motor nao deve jogar apos tomar mate (5 lances no total)");

    bridge.stop();
}

// 4. Integracao: Partida inteira (da posicao inicial ate o fim de jogo real: mate, afogamento ou 50 lances)
TEST(integration_full_game_until_end) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");
    CHECK(bridge.sendCommand("ucinewgame"), "Inicia nova partida");
    CHECK(bridge.sendCommand("setoption name Nivel value 1"), "Configura Nivel 1 para partida completa deterministica");

    std::string finalState;
    const int maxPlies = 600;
    int plies = 0;

    for (; plies < maxPlies; ++plies) {
        std::string posCmd = "position startpos";
        if (!board.getMoveHistory().empty()) {
            posCmd += " moves";
            for (const auto& m : board.getMoveHistory()) {
                posCmd += " " + m;
            }
        }
        CHECK(bridge.sendCommand(posCmd), "Envia position");
        CHECK(bridge.sendCommand("state"), "Consulta state");

        finalState = bridge.readCommand();
        if (finalState.find("result *") == std::string::npos) {
            break; // Jogo terminou (1-0, 0-1 ou 1/2-1/2)
        }

        CHECK(bridge.sendCommand("go"), "Pede proximo lance");
        std::string best = readBestMove(bridge);
        CHECK(best.rfind("bestmove ", 0) == 0 && best != "bestmove 0000", "Lance valido enquanto result == *");
        board.processUCICommand(best);
    }

    CHECK(plies < maxPlies, "Partida deve terminar dentro do limite de 600 meios-lances");
    CHECK(finalState.find("result *") == std::string::npos, "Estado final nao pode ser '*'");
    CHECK((bridge.sendCommand("go") && readBestMove(bridge) == "bestmove 0000") ||
          finalState.find("reason fifty-moves") != std::string::npos ||
          finalState.find("reason repetition") != std::string::npos ||
          finalState.find("reason insufficient-material") != std::string::npos,
          "No fim de jogo, ou nao ha lances (bestmove 0000) ou foi empate por regra");

    std::cout << "    [FIM DE JOGO] Partida completa encerrada em " << plies
              << " meios-lances | " << finalState << std::endl;

    bridge.stop();
}

// 6. Integracao (Issue #8): Botoes do HUD Lateral (Desistir e Nova Partida) sincronizados com a Engine
TEST(integration_hud_buttons_with_engine) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");

    // Joga 1o turno: e2 -> e4
    board.playTurnWithEngine(450.f, 650.f, bridge);
    CHECK(board.playTurnWithEngine(450.f, 450.f, bridge), "Joga 1. e4 e recebe resposta");
    CHECK(board.getFormattedMoveHistory().size() == 1, "1 linha completa no historico do HUD");

    // Clica em Desistir no HUD via playTurnWithEngine
    const sf::FloatRect& resignRect = board.getResignButtonBounds();
    board.playTurnWithEngine(resignRect.left + 10.f, resignRect.top + 10.f, bridge);
    CHECK(board.isGameOver() && board.getGameReason() == "resign", "Partida encerrada por desistencia");

    // Clica em Nova Partida no HUD via playTurnWithEngine: reseta tabuleiro e ressincroniza engine
    const sf::FloatRect& newGameRect = board.getNewGameButtonBounds();
    board.playTurnWithEngine(newGameRect.left + 10.f, newGameRect.top + 10.f, bridge);
    CHECK(!board.isGameOver(), "Nova Partida desbloqueia o jogo");
    CHECK(board.getMoveHistory().empty(), "Historico zerado");
    CHECK(board.getLegalMoves().size() == 20, "Engine ressincronizada com os 20 lances iniciais");

    bridge.stop();
}

// 7. Integracao (Issue #9): Modo Local PvP valida regras no motor sem acionar a IA (go)
TEST(integration_local_pvp_mode_without_ai) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");
    GameConfig cfg;
    cfg.mode = GameMode::LOCAL_PVP;
    cfg.timeLimitMinutes = 5;
    board.startMatchWithEngine(cfg, bridge);

    CHECK(board.getLegalMoves().size() == 20, "Inicio do modo local tem 20 lances legais");

    // Brancas jogam e2 -> e4
    CHECK(!board.playTurnWithEngine(450.f, 650.f, bridge), "Seleciona e2");
    CHECK(board.playTurnWithEngine(450.f, 450.f, bridge), "Brancas jogam e2e4");
    CHECK(board.getMoveHistory().size() == 1, "No modo local, IA nao joga em seguida");
    CHECK(board.getCurrentTurn() == PieceColor::BLACK, "Turno passa para as Pretas");
    CHECK(board.getLegalMoves().size() == 20, "Motor retorna 20 lances legais para as Pretas");

    // Pretas jogam e7 -> e5
    CHECK(!board.playTurnWithEngine(450.f, 150.f, bridge), "Seleciona e7");
    CHECK(board.playTurnWithEngine(450.f, 350.f, bridge), "Pretas jogam e7e5");
    CHECK(board.getMoveHistory().size() == 2, "Historico tem os 2 lances humanos");
    CHECK(board.getCurrentTurn() == PieceColor::WHITE, "Turno volta para as Brancas");
    CHECK(!board.getLegalMoves().empty(), "Motor segue sincronizado com lances legais");

    bridge.stop();
}

// 8. Integracao (Issue #9): Jogar de Pretas contra IA (Tabuleiro Invertido + IA faz 1o lance)
TEST(integration_vs_ai_as_black_flipped_board) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");
    GameConfig cfg;
    cfg.mode = GameMode::VS_AI;
    cfg.difficulty = AIDifficulty::EASY;
    cfg.playerColor = PieceColor::BLACK;
    cfg.timeLimitMinutes = 10;
    board.startMatchWithEngine(cfg, bridge);

    CHECK(board.isBoardFlipped(), "Tabuleiro invertido ao jogar de Pretas");
    CHECK(board.needsAIMove(), "IA das Brancas deve fazer o 1o lance");

    // Executa o 1o lance da IA (Brancas)
    board.executeAIMove(bridge);
    CHECK(board.getMoveHistory().size() == 1, "IA executou o 1o lance das Brancas");
    CHECK(board.getCurrentTurn() == PieceColor::BLACK, "Vez do jogador humano (Pretas)");
    CHECK(!board.getLegalMoves().empty(), "Lances legais das Pretas carregados");

    // Pega o 1o lance legal das Pretas e converte para cliques em pixels invertidos
    std::string replyMove = board.getLegalMoves()[0];
    int fromCol = -1, fromRow = -1, toCol = -1, toRow = -1;
    CHECK(board.parseSquare(replyMove.substr(0, 2), fromCol, fromRow), "Origem valida");
    CHECK(board.parseSquare(replyMove.substr(2, 2), toCol, toRow), "Destino valido");

    float fx = (7 - fromCol) * 100.f + 50.f;
    float fy = (7 - fromRow) * 100.f + 50.f;
    float tx = (7 - toCol) * 100.f + 50.f;
    float ty = (7 - toRow) * 100.f + 50.f;

    CHECK(!board.playTurnWithEngine(fx, fy, bridge), "Seleciona peca preta via pixel invertido");
    CHECK(board.playTurnWithEngine(tx, ty, bridge), "Executa lance das Pretas e recebe resposta da IA");
    CHECK(board.getMoveHistory().size() == 3, "Historico contem lance da IA, das Pretas e resposta da IA");
    CHECK(board.getCurrentTurn() == PieceColor::BLACK, "Turno retorna para as Pretas");

    bridge.stop();
}

// 9. Integracao (Issue #9): Niveis de Dificuldade (Facil, Medio, Dificil) enviados via UCI
TEST(integration_vs_ai_difficulty_levels) {
    EngineBridge bridge;
    ChessBoard board(800.f);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");

    const AIDifficulty levels[3] = { AIDifficulty::EASY, AIDifficulty::MEDIUM, AIDifficulty::HARD };
    for (int i = 0; i < 3; ++i) {
        GameConfig cfg;
        cfg.mode = GameMode::VS_AI;
        cfg.difficulty = levels[i];
        cfg.playerColor = PieceColor::WHITE;
        board.startMatchWithEngine(cfg, bridge);

        board.playTurnWithEngine(450.f, 650.f, bridge);
        CHECK(board.playTurnWithEngine(450.f, 450.f, bridge), "Humano joga 1. e4 e IA responde no nivel selecionado");
        CHECK(board.getMoveHistory().size() == 2, "IA respondeu com 1 lance valido");
        CHECK(!board.getLegalMoves().empty(), "Motor retornou lances legais apos resposta da IA");
    }

    bridge.stop();
}

// 10. Integracao: Fluxo Assincrono da IA com Delay Natural e Cancelamento ao Desistir
TEST(integration_async_ai_move_and_cancel_on_resign) {
    EngineBridge bridge;
    ChessBoard board(800.f);
    board.setAsyncAIEnabled(true);

    CHECK(bridge.start(ENGINE_BIN), "Deve iniciar o motor real");

    GameConfig cfg;
    cfg.mode = GameMode::VS_AI;
    cfg.difficulty = AIDifficulty::EASY;
    cfg.playerColor = PieceColor::WHITE;
    board.startMatchWithEngine(cfg, bridge);

    // 1. e4: retorno deve ser imediato com a IA pensando em background
    auto t0 = std::chrono::steady_clock::now();
    board.playTurnWithEngine(450.f, 650.f, bridge);
    CHECK(board.playTurnWithEngine(450.f, 450.f, bridge), "1. e4 aceito imediatamente");
    CHECK(board.isAIThinking(), "IA deve iniciar pensamento assincrono sem travar");
    CHECK(board.getMoveHistory().size() == 1, "Resposta da IA ainda nao entra antes do delay");

    // Polling de frame ate a IA concluir o delay natural (1.2s..1.8s)
    for (int step = 0; step < 150 && board.isAIThinking(); ++step) {
        board.updateAI(bridge);
        if (board.isAIThinking()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    double waitedSec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    CHECK(!board.isAIThinking(), "IA deve concluir dentro do prazo esperado");
    CHECK(waitedSec >= 1.15, "Delay natural minimo (~1.2s) deve ser respeitado");
    CHECK(board.getMoveHistory().size() == 2, "Lance da IA aplicado apos o delay");
    CHECK(!board.getLegalMoves().empty(), "Lances legais disponiveis para o 2o turno");

    // 2o turno: joga um lance legal e desiste ENQUANTO a IA pensa
    std::string secondMove = board.getLegalMoves()[0];
    int fc = -1, fr = -1, tc = -1, tr = -1;
    CHECK(board.parseSquare(secondMove.substr(0, 2), fc, fr), "Origem do 2o lance valida");
    CHECK(board.parseSquare(secondMove.substr(2, 2), tc, tr), "Destino do 2o lance valido");
    board.playTurnWithEngine(fc * 100.f + 50.f, fr * 100.f + 50.f, bridge);
    CHECK(board.playTurnWithEngine(tc * 100.f + 50.f, tr * 100.f + 50.f, bridge), "2o lance aplicado");
    CHECK(board.isAIThinking(), "IA pensando no 2o turno");

    const sf::FloatRect& resignRect = board.getResignButtonBounds();
    board.playTurnWithEngine(resignRect.left + 5.f, resignRect.top + 5.f, bridge);
    CHECK(board.isGameOver() && board.getGameReason() == "resign", "Desistencia imediata enquanto IA pensa");

    board.waitForPendingAI();
    CHECK(board.getMoveHistory().size() == 3, "Lance atrasado da IA e descartado apos desistencia");
    CHECK(bridge.sendCommand("isready") && bridge.readCommand() == "readyok",
          "Pipe do motor permanece sincronizado apos cancelar/aguardar IA");

    // Fecha o alerta de desistencia via botao [X] usando playTurnWithEngine
    CHECK(board.isAlertVisible(), "Alerta de desistencia visivel antes de fechar");
    const sf::FloatRect& closeRect = board.getCloseButtonBounds();
    CHECK(!board.playTurnWithEngine(closeRect.left + 5.f, closeRect.top + 5.f, bridge),
          "Clique no [X] via playTurnWithEngine apenas fecha o alerta");
    CHECK(!board.isAlertVisible(), "Alerta fechado apos clicar no [X] via playTurnWithEngine");

    // Clica em Nova Partida pos-jogo com matchConfigured == true (reinicia com mesma configuracao)
    const sf::FloatRect& newGameRect = board.getNewGameButtonBounds();
    CHECK(!board.playTurnWithEngine(newGameRect.left + 5.f, newGameRect.top + 5.f, bridge),
          "Clique em Nova Partida reinicia a partida configurada");
    CHECK(!board.isGameOver() && board.getMoveHistory().empty(),
          "Partida reiniciada com historico zerado e desbloqueada");

    // Joga 1. e4 e tenta clicar no tabuleiro enquanto a IA pensa (deve ser bloqueado)
    board.playTurnWithEngine(450.f, 650.f, bridge);
    CHECK(board.playTurnWithEngine(450.f, 450.f, bridge), "1. e4 aceito apos reinicio");
    CHECK(board.isAIThinking(), "IA pensando apos 1. e4");
    CHECK(!board.playTurnWithEngine(350.f, 650.f, bridge),
          "Cliques no tabuleiro devem ser ignorados enquanto a IA pensa");
    board.waitForPendingAI();

    bridge.stop();
}

int main() {
    return test_run_all();
}
