#ifndef UCI_H
#define UCI_H

/*
 * uci.h -- a camada de protocolo do motor.
 *
 * Implementa o contrato de 'protocolo.md' v1: um subconjunto do UCI (uci,
 * isready, ucinewgame, position, go, quit) mais as extensoes do projeto
 * (legalmoves, state, d). Uma mensagem por linha, pela stdin/stdout.
 *
 * A fronteira e' estreita de proposito: este modulo e' o unico lugar do motor
 * que sabe que existe um cliente do outro lado do pipe. Ele valida tudo o que
 * vem de fora e o nucleo (make_move, generate_legal_moves) continua confiando
 * na entrada -- e' o contrato "fronteira valida, nucleo confia" do
 * project_context.md §3.
 *
 * Quem e' dono do que: o CLIENTE e' dono da partida (guarda o historico e o
 * reenvia inteiro a cada mudanca); o motor e' um oraculo sem estado de partida
 * entre comandos, com uma excecao declarada -- 'state', 'legalmoves' e 'go' se
 * referem sempre a' ultima posicao aceita por 'position'.
 *
 * DIVIDAS CONSCIENTES desta versao, para nao ficarem implicitas:
 *
 *  1. O laco nao le a stdin enquanto calcula o 'go'. A spec UCI exige isso para
 *     atender 'stop', e nao existe 'stop' na versao 1 do protocolo. Comandos
 *     enviados durante um 'go' esperam no pipe e sao processados depois do
 *     'bestmove', em ordem. Resolve-se com uma thread leitora, quando a busca
 *     da IA for longa o bastante para importar.
 *  2. 'go' sorteia um lance legal com semente fixa -- a IA de 'IA/' ainda nao
 *     esta ligada ao build do motor, e 'escolherJogada' nao recebe profundidade
 *     nem tempo. E' o comportamento que 'protocolo.md' §5.7 prescreve ate' a
 *     etapa P5, nao um atalho.
 *  3. 'repetition' e 'insufficient-material' nao sao emitidos como motivo de
 *     fim de partida (etapa P6). Repeticao exige o historico de posicoes, que
 *     e' desta camada guardar -- ver uci_session_reset().
 *  4. 'go perft' (etapa P6) nao existe: 'perft' mora em 'main.c', que nao tem
 *     header. Quando sair para um 'perft.h' proprio, o comando e' uma linha.
 */

/*
 * Le comandos da stdin e responde na stdout ate' 'quit' ou fim da entrada.
 * Devolve 0 em encerramento normal (as duas formas) e != 0 so' em erro de uso
 * dos argumentos de linha de comando.
 *
 * Argumentos reconhecidos:
 *   --trace <arquivo>   grava cada linha recebida ("> ...") e enviada ("< ...")
 *
 * Pre-condicao: init_square_tables() ja' foi chamada.
 */
int uci_main(int argc, char **argv);

#endif /* UCI_H */
