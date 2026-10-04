#include "../include/uci.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../include/types.h"
#include "../include/piece.h"
#include "../include/square.h"
#include "../include/board.h"
#include "../include/fen.h"
#include "../include/move.h"
#include "../include/movegen.h"
#include "../include/makemove.h"
#include "../include/io.h"
#include "../include/log.h"

/* ------------------------------------------------------------------------- *
 * Constantes do protocolo (protocolo.md secoes 3 e 4)
 * ------------------------------------------------------------------------- */

#define UCI_ID_NAME   "Xadrez-ES2 0.1"
#define UCI_ID_AUTHOR "Equipe Xadrez-ES2"

/* Limites do transporte. UCI_LINE_MAX e o teto que o contrato promete; a
   capacidade do buffer e dois bytes maior porque read_line() repassa 'cap' para
   o fgets, que le no maximo cap-1 bytes: para caber 8192 bytes de conteudo MAIS
   o '\n' e o '\0' precisamos de cap == 8194.

   O que isso da, exatamente: linha de ate 8193 bytes e aceita, de 8194 em diante
   read_line devolve -1 e a resposta e 'line-too-long'. O byte de folga e
   deliberado -- o contrato garante que 8192 SEMPRE cabe, e aceitar um byte a
   mais nao quebra nenhum cliente. O que importa e que linha longa demais nunca
   vira comando truncado processado em silencio. */
#define UCI_LINE_MAX      8192
#define UCI_LINE_CAP      (UCI_LINE_MAX + 2)

/* Teto de lances em 'position ... moves'. O teto de tokens cobre o pior caso
   legitimo -- "position fen <6 campos> moves <1024 lances>" = 1033 tokens --
   com folga; bater nele significa tokens demais de verdade, nao uma posicao
   valida grande. */
#define UCI_MAX_POSITION_MOVES 1024
#define UCI_MAX_TOKENS         (UCI_MAX_POSITION_MOVES + 16)

/* Semente do sorteio do 'go'. Fixa de proposito: protocolo.md 5.7 promete que a
   mesma sequencia de comandos produz sempre a mesma resposta, e e isso que
   torna uma sessao do cliente reproduzivel a partir do arquivo de --trace. */
#define UCI_RNG_SEED 0x9E3779B97F4A7C15ull

/* ------------------------------------------------------------------------- *
 * Estado do modulo
 *
 * Tudo aqui e file-static, e nao local do laco, por dois motivos: o buffer de
 * linha mais o vetor de tokens passam de 17 KB somados, e o build de debug liga
 * -Wframe-larger-than=16384; e o protocolo e estritamente single-thread (ver a
 * divida 1 em uci.h), entao estado de modulo aqui nao e estado compartilhado.
 * ------------------------------------------------------------------------- */

typedef struct {
    Board board;        /* a ultima posicao ACEITA -- o invariante da secao 2.3 */
    u64   rng_state;    /* sorteio do 'go'; estado zero e proibido (xorshift) */
} UciSession;

static UciSession session;
static char       line_buf[UCI_LINE_CAP];
static String     tokens[UCI_MAX_TOKENS];
static FILE      *trace_fp = NULL;

/* ------------------------------------------------------------------------- *
 * Saida: um unico caminho
 *
 * Toda linha de protocolo sai por uci_send(). Isso garante tres coisas de uma
 * vez: a descarga explicita acontece sempre, o arquivo de --trace nunca fica
 * incompleto, e o '\n' nao depende de quem chama se lembrar dele.
 * ------------------------------------------------------------------------- */

static void uci_send(const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

static void uci_send(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    fputc('\n', stdout);

    /* setvbuf(_IOLBF) resolveria isto num Unix, mas no CRT da Microsoft _IOLBF
       se comporta como buffer cheio (protocolo.md secao 13). O fflush por linha
       e o que faz a resposta chegar quando a stdout e um pipe -- sem ele o motor
       fica vivo, correto e mudo, que e o pior modo de falhar. */
    fflush(stdout);

    if (trace_fp != NULL) {
        va_start(ap, fmt);
        fputs("< ", trace_fp);
        vfprintf(trace_fp, fmt, ap);
        va_end(ap);
        fputc('\n', trace_fp);
        fflush(trace_fp);
    }
}

static void uci_trace_recv(const char *line) {
    if (trace_fp == NULL) return;
    fprintf(trace_fp, "> %s\n", line);
    fflush(trace_fp);
}

/* protocolo.md secao 6: 'error <codigo> [detalhes]'. Um erro nunca substitui a
   resposta de outro comando -- ele aparece antes dela. */
static void uci_error(const char *code, const char *detail) {
    if (detail != NULL) {
        uci_send("error %s %s", code, detail);
    } else {
        uci_send("error %s", code);
    }
}

/* ------------------------------------------------------------------------- *
 * Tokens
 * ------------------------------------------------------------------------- */

static bool tok_eq(String t, const char *literal) {
    return string_eq(t, string_from_cstr(literal));
}

static bool tok_to_cstr(String t, char *out, size_t cap) {
    if (t.len == 0 || t.len >= cap) return false;
    memcpy(out, t.data, t.len);
    out[t.len] = '\0';
    return true;
}

/* Copia truncada, para POR UM TOKEN DO CLIENTE numa mensagem de erro. Um token
   pode ter milhares de bytes; a linha de erro nao pode. */
static void tok_to_cstr_shown(String t, char *out, size_t cap) {
    size_t n = (t.len < cap - 1) ? t.len : cap - 1;
    memcpy(out, t.data, n);
    out[n] = '\0';
}

/* ------------------------------------------------------------------------- *
 * Perguntas sobre a posicao
 *
 * Tudo static e dentro deste modulo. O motor ainda nao tem 'in_check' nem funcao
 * de estado de partida (sao a Etapa 9 do next_steps.md); quando tiver, estas
 * tres funcoes viram chamadas de uma linha e o resto do arquivo nao muda.
 * ------------------------------------------------------------------------- */

/* ATENCAO ao terceiro argumento de is_square_attacked: ele e o DONO da casa (o
   defensor), nao o atacante. A assinatura planejada era '(b, sq, by)'; a
   implementada e '(b, sq, side)' com side = quem defende. Passar o atacante por
   habito inverte o resultado sem nenhum aviso do compilador.
   Ver project_context.md secao 3. */
static bool uci_in_check(const Board *b) {
    int ksq = b->king_square[b->side_to_move];
    if (SQ_OFFBOARD(ksq)) return false;   /* is_square_attacked tem assert aqui */
    return is_square_attacked(b, ksq, b->side_to_move);
}

/*
 * Defesa de fronteira contra o Bug #1 do project_context.md secao 5.
 *
 * 'fen_parse' aceita uma FEN em que o lado que NAO joga esta em xeque -- uma
 * posicao que nao pode existir numa partida. Dela o filtro de legalidade deixa
 * capturar o rei (a captura nao deixa o rei de quem joga em xeque), o
 * 'king_square' do capturado vira SQ_NONE, e a geracao seguinte aborta num
 * assert (ou, com -DNDEBUG, le fora do tabuleiro).
 *
 * A correcao definitiva e dentro do 'fen_parse' e nao e minha para fazer. Aqui a
 * posicao e recusada ANTES de ser aceita, o que fecha o caminho pelo protocolo:
 * nenhuma FEN que entre por 'position fen' pode mais alcancar esse estado.
 */
static bool uci_position_is_legal(const Board *b) {
    Color waiting;

    if (SQ_OFFBOARD(b->king_square[WHITE]) || SQ_OFFBOARD(b->king_square[BLACK])) {
        return false;
    }
    waiting = (b->side_to_move == WHITE) ? BLACK : WHITE;
    return !is_square_attacked(b, b->king_square[waiting], waiting);
}

typedef struct {
    const char *result;   /* "*", "1-0", "0-1", "1/2-1/2" -- tokens do PGN */
    const char *reason;   /* "none", "checkmate", "stalemate", "fifty-moves" */
    int         check_sq; /* casa do rei do lado a jogar se em xeque; senao SQ_NONE */
} UciStatus;

/*
 * 'b' nao e const porque generate_legal_moves aplica e desfaz lances. Nao e
 * defeito da assinatura: e o preco do filtro aplica-e-testa.
 *
 * Precedencia (protocolo.md secao 5.6): mate e afogamento vem ANTES da regra dos
 * 50 lances. Um lance que da mate no centesimo meio-lance e mate, nao empate.
 */
static void uci_status(Board *b, UciStatus *out) {
    MoveList legal;
    bool check;

    generate_legal_moves(b, &legal);
    check = uci_in_check(b);

    out->check_sq = check ? b->king_square[b->side_to_move] : SQ_NONE;
    out->result   = "*";
    out->reason   = "none";

    if (legal.count == 0) {
        if (check) {
            out->reason = "checkmate";
            /* quem esta em xeque e sem lance perdeu */
            out->result = (b->side_to_move == WHITE) ? "0-1" : "1-0";
        } else {
            out->reason = "stalemate";
            out->result = "1/2-1/2";
        }
    } else if (b->halfmove_clock >= 100) {
        /* Simplificacao deliberada, registrada no protocolo.md secao 5.6: na FIDE
           os 50 lances dependem de reivindicacao; aqui encerram sozinhos. */
        out->reason = "fifty-moves";
        out->result = "1/2-1/2";
    }
}

/* ------------------------------------------------------------------------- *
 * Sessao
 * ------------------------------------------------------------------------- */

/*
 * Volta para a posicao inicial. Monta por 'fen_parse(START_FEN)' e nao por
 * 'board_new': o parser e a unica rotina que preenche os seis campos da FEN e o
 * cache de 'king_square' de uma vez, e e a mesma que valida as posicoes que
 * chegam do cliente -- um caminho, nao dois.
 *
 * E aqui que o historico de posicoes da partida vai morar quando a repeticao
 * tripla entrar (etapa P6): uma lista de chaves 'u64' (ou os quatro primeiros
 * campos da FEN, ate o Zobrist existir), zerada neste ponto e estendida a cada
 * lance aplicado por 'position'. Nao e um 'Undo' -- a decisao de 10/09 vale.
 */
static void uci_session_reset(void) {
    if (!fen_parse(START_FEN, &session.board)) {
        /* Inalcancavel: START_FEN e constante e o round-trip do fen.c e testado.
           Se acontecer, o motor esta quebrado de um jeito que mentir nao ajuda. */
        LOG_ERROR("START_FEN rejeitada por fen_parse -- motor inconsistente");
    }
    session.rng_state = UCI_RNG_SEED;
}

/* xorshift64* proprio, e nao rand(), para que a sequencia do 'go' dependa so da
   semente e nao da libc: o mesmo --trace tem de se reproduzir no Windows e no
   Linux. Estado zero e o unico proibido, e UCI_RNG_SEED nao e zero. */
static u64 uci_rand(void) {
    u64 x = session.rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    session.rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

/* ------------------------------------------------------------------------- *
 * Comandos
 * ------------------------------------------------------------------------- */

static void uci_cmd_uci(void) {
    uci_send("id name %s", UCI_ID_NAME);
    uci_send("id author %s", UCI_ID_AUTHOR);
    /* As linhas 'option ...' entram aqui na etapa P5 (setoption Difficulty). */
    uci_send("uciok");
}

static void uci_cmd_state(void) {
    UciStatus st;
    char fen[MAX_FEN_STRING];
    char coord[3];

    uci_status(&session.board, &st);
    fen_write(&session.board, fen);

    /* A chave 'fen' e sempre a ultima e ocupa o resto da linha, porque a FEN tem
       espacos. O parser do cliente conta com isso (protocolo.md secao 4). */
    if (st.check_sq == SQ_NONE) {
        uci_send("state check - result %s reason %s fen %s",
                 st.result, st.reason, fen);
    } else {
        sq_to_coord(st.check_sq, coord);
        uci_send("state check %s result %s reason %s fen %s",
                 coord, st.result, st.reason, fen);
    }
}

static void uci_cmd_legalmoves(void) {
    MoveList legal;
    /* Cada entrada e ' ' + no maximo 5 caracteres. MAX_MOVES e 256 e o maximo
       legal numa posicao real e 218, entao isto e folgado por construcao. */
    char buf[MAX_MOVES * (MOVE_STR_SIZE + 1) + 1];
    char mv[MOVE_STR_SIZE];
    size_t len = 0;

    generate_legal_moves(&session.board, &legal);

    for (int i = 0; i < legal.count; i++) {
        size_t n;
        move_to_str(legal.moves[i], mv);
        n = strlen(mv);
        if (len + n + 2 > sizeof(buf)) {
            /* Inalcancavel: 218 lances e o maximo legal numa posicao real, e o
               buffer cabe 256. Se disparar, o bug esta no gerador, nao aqui --
               mas truncar em silencio transformaria esse bug numa linha de
               lances levemente curta, que ninguem nota. */
            LOG_ERROR("legalmoves nao cabe no buffer apos %d lances", i);
            break;
        }
        buf[len++] = ' ';
        memcpy(buf + len, mv, n);
        len += n;
    }
    buf[len] = '\0';

    /* Sem lance legal a linha e so a palavra -- e o contrato, e o cliente nao
       deve usar isso como fim de partida: quem diz isso e o 'result' do 'state'
       (na regra dos 50 lances ainda ha lances e a partida acabou). */
    uci_send("legalmoves%s", buf);
}

/*
 * 'position' e ATOMICO (protocolo.md secao 2.3): ou a posicao inteira e aceita,
 * ou nada muda. E por isso que tudo e montado num Board de rascunho e a sessao
 * so e tocada na ultima linha -- se o quinto lance de uma lista for ilegal, o
 * motor fica exatamente onde estava, nao depois do quarto.
 */
static void uci_cmd_position(int argc) {
    Board scratch;
    int i;

    if (argc < 2) {
        uci_error("bad-command", "position");
        return;
    }

    if (tok_eq(tokens[1], "startpos")) {
        if (!fen_parse(START_FEN, &scratch)) {
            uci_error("bad-fen", NULL);
            return;
        }
        i = 2;
    } else if (tok_eq(tokens[1], "fen")) {
        char fen[MAX_FEN_STRING];
        const char *begin;
        const char *end;
        size_t fen_len;

        /* Sempre os seis campos (protocolo.md secao 4). Faltar campo e comando
           malformado, nao FEN ruim -- o cliente precisa dos dois diagnosticos
           separados, porque um e bug dele e o outro e dado invalido. */
        if (argc < 8) {
            uci_error("bad-command", "position");
            return;
        }
        for (int k = 2; k < 8; k++) {
            if (tok_eq(tokens[k], "moves")) {
                uci_error("bad-command", "position");
                return;
            }
        }

        /* A FEN e a FATIA da linha que vai do primeiro ao sexto campo, com os
           separadores originais no meio. Juntar palavra por palavra seria o
           caminho do 'join_args', que hoje nao insere separador (Bug #5): a FEN
           inicial sairia como "rnbqkbnr/...wKQkq-01" e seria rejeitada. Fatiar
           nao tem esse problema e nao copia duas vezes. */
        begin   = tokens[2].data;
        end     = tokens[7].data + tokens[7].len;
        fen_len = (size_t)(end - begin);
        if (fen_len >= sizeof(fen)) {
            uci_error("bad-fen", NULL);
            return;
        }
        memcpy(fen, begin, fen_len);
        fen[fen_len] = '\0';

        if (!fen_parse(fen, &scratch)) {
            /* O motivo detalhado vai para a stderr, pelo LOG_ERROR do fen.c. */
            uci_error("bad-fen", NULL);
            return;
        }
        if (!uci_position_is_legal(&scratch)) {
            LOG_ERROR("FEN valida na forma mas impossivel numa partida "
                      "(lado que nao joga em xeque, ou rei ausente): %s", fen);
            uci_error("bad-fen", NULL);
            return;
        }
        i = 8;
    } else {
        uci_error("bad-command", "position");
        return;
    }

    if (i < argc) {
        if (!tok_eq(tokens[i], "moves")) {
            uci_error("bad-command", "position");
            return;
        }
        i++;
        if (argc - i > UCI_MAX_POSITION_MOVES) {
            uci_error("too-many-moves", NULL);
            return;
        }
    }

    for (int index = 0; i < argc; i++, index++) {
        char      uci_move[MOVE_STR_SIZE];
        char      shown[20];
        MoveList  legal;
        Undo      u;
        int       found;

        /* Um lance tem 4 ou 5 caracteres; qualquer outro tamanho nem chega ao
           movelist_find. Para o cliente o diagnostico e o mesmo: esse token nao
           e um lance legal nesta posicao. */
        if (!tok_to_cstr(tokens[i], uci_move, sizeof(uci_move))) {
            tok_to_cstr_shown(tokens[i], shown, sizeof(shown));
            uci_send("error illegal-move %s %d", shown, index);
            return;
        }

        /* movelist_find sobre a lista LEGAL, e nao sobre a pseudo-legal: e o
           gerador que sabe se 'e5d6' e captura comum ou en passant, porque foi
           ele que marcou a flag. E o retorno e INDICE: 0 e resultado valido e -1
           e 'nao encontrado'. Tratar como booleano ja foi bug aqui. */
        generate_legal_moves(&scratch, &legal);
        found = movelist_find(&legal, uci_move);
        if (found < 0) {
            tok_to_cstr_shown(tokens[i], shown, sizeof(shown));
            uci_send("error illegal-move %s %d", shown, index);
            return;
        }

        make_move(&scratch, legal.moves[found], &u);
    }

    session.board = scratch;   /* o unico ponto em que a posicao aceita muda */
}

/*
 * 'go'. A escolha do lance esta isolada nesta funcao justamente porque ela e o
 * que muda na etapa P5: hoje sorteia da lista legal, amanha chama a IA.
 */
static Move uci_pick_move(const MoveList *legal) {
    return legal->moves[(int)(uci_rand() % (u64)legal->count)];
}

static void uci_cmd_go(void) {
    MoveList legal;
    Move     chosen;
    char     mv[MOVE_STR_SIZE];
    bool     is_legal = false;

    /* Os parametros ('movetime', 'depth', 'wtime', ...) sao aceitos e ignorados
       nesta etapa, como a spec UCI manda para o que o motor nao implementa. */

    generate_legal_moves(&session.board, &legal);
    if (legal.count == 0) {
        uci_send("bestmove 0000");
        return;
    }

    chosen = uci_pick_move(&legal);

    /* Conferir que o lance escolhido esta na lista legal, inclusive em release
       (RF-M3: o motor nunca devolve lance ilegal). Hoje e redundante -- o lance
       saiu da propria lista. O valor e amanha: quando a fonte do lance for a IA,
       esta varredura e a unica coisa entre um bug de busca e um lance ilegal no
       tabuleiro do cliente. Custa uma varredura de no maximo 218 entradas. */
    for (int i = 0; i < legal.count; i++) {
        if (legal.moves[i] == chosen) {
            is_legal = true;
            break;
        }
    }
    if (!is_legal) {
        LOG_ERROR("lance escolhido fora da lista legal -- recusado");
        uci_send("bestmove 0000");
        return;
    }

    move_to_str(chosen, mv);
    uci_send("bestmove %s", mv);
}

static void uci_cmd_d(void) {
    char fen[MAX_FEN_STRING];

    /* board_print escreve direto na stdout, em formato livre e para humanos. Nao
       passa pelo uci_send, logo nao aparece no --trace -- de proposito: o trace e
       o dialogo do protocolo, e o desenho do tabuleiro nao faz parte dele. O
       cliente nao deve usar este comando. */
    board_print(&session.board);
    fflush(stdout);

    fen_write(&session.board, fen);
    uci_send("fen %s", fen);
}

/* ------------------------------------------------------------------------- *
 * Laco
 * ------------------------------------------------------------------------- */

/* Devolve false quando o motor deve encerrar ('quit'). */
static bool uci_dispatch(void) {
    int argc = string_split(line_buf, tokens, UCI_MAX_TOKENS);

    if (argc <= 0) return true;   /* linha vazia ou so espacos: ignora */

    /* string_split para ao encher o vetor, entao bater no teto significa que a
       linha podia ter mais tokens do que foi lido -- e processar o prefixo seria
       aceitar um comando truncado. */
    if (argc >= UCI_MAX_TOKENS) {
        uci_error("too-many-moves", NULL);
        return true;
    }

    if      (tok_eq(tokens[0], "uci"))        uci_cmd_uci();
    else if (tok_eq(tokens[0], "isready"))    uci_send("readyok");
    else if (tok_eq(tokens[0], "ucinewgame")) uci_session_reset();
    else if (tok_eq(tokens[0], "position"))   uci_cmd_position(argc);
    else if (tok_eq(tokens[0], "legalmoves")) uci_cmd_legalmoves();
    else if (tok_eq(tokens[0], "state"))      uci_cmd_state();
    else if (tok_eq(tokens[0], "go"))         uci_cmd_go();
    else if (tok_eq(tokens[0], "d"))          uci_cmd_d();
    else if (tok_eq(tokens[0], "quit"))       return false;
    /* Comando desconhecido: silencio. E a regra da spec UCI, e e ela que permite
       plugar o motor numa GUI pronta (Cute Chess, Arena) sem implementar a
       especificacao inteira. */

    return true;
}

/* Devolve false em erro de uso dos argumentos. */
static bool uci_parse_args(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--trace") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "uci: --trace exige um nome de arquivo\n");
                return false;
            }
            i++;
            trace_fp = fopen(argv[i], "w");
            if (trace_fp == NULL) {
                /* Nao e motivo para o motor nao subir: o cliente esta do outro
                   lado do pipe esperando o 'uciok', e perder o log e menos grave
                   que nao ter motor. Aviso na stderr e segue sem trace. */
                fprintf(stderr, "uci: nao foi possivel abrir '%s' para trace\n",
                        argv[i]);
            }
        } else {
            fprintf(stderr, "uci: argumento ignorado: '%s'\n", argv[i]);
        }
    }
    return true;
}

int uci_main(int argc, char **argv) {
    /* A linha mais importante do arquivo. Quando a stdout e um terminal a libc
       usa buffer de linha; quando e um PIPE -- o caso do cliente rodando o motor
       como subprocesso -- ela troca para buffer de bloco de 4 KB, a resposta
       fica presa, e nao ha sintoma nenhum para depurar. O uci_send ainda da
       fflush por linha, porque no Windows _IOLBF nao basta. */
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (!uci_parse_args(argc, argv)) return 2;

    uci_session_reset();

    for (;;) {
        int r = read_line(line_buf, UCI_LINE_CAP);

        /* read_line devolve 0 em EOF, 1 em linha lida e -1 em linha longa demais
           (e nesse caso ja descartou o resto dela).

           O EOF tem de ENCERRAR o laco. Este e o ponto exato do Bug #3 do
           project_context.md: 'read_word' testa so o -1, e em EOF devolve a
           palavra anterior outra vez COM SUCESSO -- um cliente que fecha o pipe
           faria o motor repetir o ultimo comando para sempre. Por isso o laco
           usa read_line direto, e nao read_word. */
        if (r == 0) break;
        if (r == -1) {
            uci_error("line-too-long", NULL);
            continue;
        }

        uci_trace_recv(line_buf);
        if (!uci_dispatch()) break;
    }

    if (trace_fp != NULL) {
        fclose(trace_fp);
        trace_fp = NULL;
    }
    return 0;
}
