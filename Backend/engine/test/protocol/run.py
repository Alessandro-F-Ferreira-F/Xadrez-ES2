#!/usr/bin/env python3
"""
Teste do protocolo: roda cada NN-nome.in no motor e compara a stdout com o
NN-nome.out correspondente (protocolo.md secao 11.1).

    python3 test/protocol/run.py [caminho/do/motor]

Sem argumento, procura build/main.out e build/main.exe a partir de engine/.

Tres coisas que este script faz de proposito:

1.  Compara as linhas 'legalmoves' como CONJUNTO. A ordem dos lances nao faz
    parte do contrato (protocolo.md secao 5.5), e amarrar o teste a ela faria
    qualquer mudanca na ordem de geracao parecer uma regressao.

2.  Nunca compara o 'bestmove' contra uma string fixa. O lance e sorteado; o que
    o contrato promete e que ele e LEGAL. Entao o teste pergunta 'legalmoves' na
    mesma posicao e confere que o bestmove esta na lista -- que e exatamente o
    requisito (o motor nunca devolve lance ilegal), e nao um retrato do sorteio.

3.  Faz o teste de transporte com a STDIN ABERTA (secao 11.2). Um
    'printf ... | motor' nao detecta buffer preso: quando a stdin fecha o motor
    termina, e o que estava no buffer e despejado na saida do processo. O teste
    passa e o cliente -- que mantem a stdin aberta -- trava.

A stderr do motor e ignorada: por contrato ela e diagnostico para humanos, em
formato livre e instavel.
"""

import re
import subprocess
import sys
import tempfile
import threading
import queue
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ENGINE_DIR = HERE.parent.parent


def find_engine(argv):
    if len(argv) > 1:
        path = Path(argv[1])
        if not path.exists():
            sys.exit(f"motor nao encontrado: {path}")
        return path
    for name in ("build/main.out", "build/main.exe"):
        path = ENGINE_DIR / name
        if path.exists():
            return path
    sys.exit("motor nao encontrado: rode 'make' em engine/ primeiro")


def run_engine(engine, stdin_text, timeout=10.0):
    """Roda o motor com stdin_text e devolve (linhas da stdout, codigo de saida)."""
    proc = subprocess.run(
        [str(engine)],
        input=stdin_text,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        timeout=timeout,
    )
    lines = [ln.rstrip("\r") for ln in proc.stdout.split("\n")]
    while lines and lines[-1] == "":
        lines.pop()
    return lines, proc.returncode


def lines_match(expected, got):
    """Linhas 'legalmoves' batem como conjunto; o resto, literalmente."""
    if expected == got:
        return True
    e, g = expected.split(), got.split()
    if e and g and e[0] == "legalmoves" and g[0] == "legalmoves":
        return sorted(e[1:]) == sorted(g[1:])
    return False


DIVIDE_LINE = re.compile(r"^\S+: \d+$")


def normalize(lines):
    """Ordena cada bloco contiguo de linhas 'lance: contagem' ('go perft').

    Como 'legalmoves', a ordem dos lances da raiz nao faz parte do contrato; o que
    importa e o CONJUNTO de pares lance/contagem e o total que vem depois.
    """
    out, run = [], []
    for ln in lines:
        if DIVIDE_LINE.match(ln):
            run.append(ln)
        else:
            out.extend(sorted(run))
            run = []
            out.append(ln)
    out.extend(sorted(run))
    return out


def check_transcripts(engine):
    failures = []
    cases = sorted(HERE.glob("*.in"))
    if not cases:
        sys.exit("nenhuma transcricao .in encontrada")

    for case in cases:
        expected_file = case.with_suffix(".out")
        if not expected_file.exists():
            failures.append(f"{case.name}: falta {expected_file.name}")
            continue

        expected = normalize(expected_file.read_text().splitlines())
        try:
            got, code = run_engine(engine, case.read_text())
            got = normalize(got)
        except subprocess.TimeoutExpired:
            failures.append(f"{case.name}: o motor nao terminou (travou, ou nao "
                            f"respondeu ao quit)")
            continue

        if code != 0:
            failures.append(f"{case.name}: codigo de saida {code}, esperado 0")

        if len(expected) != len(got) or not all(
            lines_match(e, g) for e, g in zip(expected, got)
        ):
            detail = [f"{case.name}: saida diferente do esperado"]
            for i in range(max(len(expected), len(got))):
                e = expected[i] if i < len(expected) else "<nada>"
                g = got[i] if i < len(got) else "<nada>"
                mark = "  " if (i < len(expected) and i < len(got)
                                and lines_match(e, g)) else "!!"
                detail.append(f"   {mark} esperado: {e}")
                detail.append(f"   {mark} obtido:   {g}")
            failures.append("\n".join(detail))
        else:
            print(f"ok    {case.name}")

    return failures


def check_transport_open_stdin(engine):
    """Secao 11.2: um comando, e a resposta chega SEM fechar a stdin."""
    proc = subprocess.Popen(
        [str(engine)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    proc.stdin.write("isready\n")
    proc.stdin.flush()

    answer = []
    reader = threading.Thread(target=lambda: answer.append(proc.stdout.readline()),
                             daemon=True)
    reader.start()
    reader.join(2.0)

    failures = []
    if not answer or answer[0].strip() != "readyok":
        failures.append("transporte: sem 'readyok' em 2 s com a stdin aberta -- "
                        "a saida esta presa no buffer (falta setvbuf/fflush)")
    else:
        print("ok    transporte com a stdin aberta")

    # E o fim da stdin encerra o motor, em vez de repetir o ultimo comando?
    proc.stdin.close()
    try:
        code = proc.wait(timeout=2.0)
        if code != 0:
            failures.append(f"EOF: o motor saiu com codigo {code}, esperado 0")
        else:
            print("ok    EOF na stdin encerra o motor")
    except subprocess.TimeoutExpired:
        proc.kill()
        failures.append("EOF: o motor nao terminou no fim da stdin (Bug #3: "
                        "read_word devolve a palavra anterior em EOF)")
    return failures


def check_go_is_legal(engine):
    """O bestmove tem de estar na lista legal da mesma posicao."""
    positions = [
        "startpos",
        "startpos moves e2e4 e7e5",
        "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    ]
    failures = []
    for pos in positions:
        script = f"position {pos}\nlegalmoves\ngo movetime 10\nquit\n"
        try:
            lines, _ = run_engine(engine, script)
        except subprocess.TimeoutExpired:
            failures.append(f"go: o motor travou em 'position {pos}'")
            continue

        legal = next((ln.split()[1:] for ln in lines
                      if ln.startswith("legalmoves")), None)
        best = next((ln.split()[1] for ln in lines
                     if ln.startswith("bestmove") and len(ln.split()) > 1), None)

        if legal is None or best is None:
            failures.append(f"go: falta legalmoves ou bestmove em 'position {pos}'")
        elif best not in legal:
            failures.append(f"go: bestmove ILEGAL '{best}' em 'position {pos}' "
                            f"-- RF-M3 violado")
        else:
            print(f"ok    go devolve lance legal ({best}) em 'position {pos}'")
    return failures


def check_search_options(engine):
    failures = []
    # No nivel facil, depth explicito deve superar a profundidade 2 do perfil.
    for command, expected_depth in (("go depth 3", 3), ("go depth 2 movetime 0", 2),
                                    ("go movetime 0 depth 2", 2)):
        lines, code = run_engine(engine, "setoption name Nivel value 1\nstate\n"
                                 + command + "\nstate\nquit\n")
        states = [ln for ln in lines if ln.startswith("state ")]
        infos = [ln for ln in lines if ln.startswith("info depth ")]
        if code or len(states) != 2 or states[0] != states[1] or len(infos) != 1 \
                or not infos[0].startswith(f"info depth {expected_depth} score cp "):
            failures.append(f"busca: limite/imutabilidade incorreto em {command}: {lines}")

    invalid = ("go depth", "go depth 0", "go depth 65", "go depth -1", "go depth x",
               "go movetime", "go movetime -1", "go movetime 2147483648",
               "go depth 1 depth 2", "go movetime 1 movetime 2",
               "setoption name Nivel value 0", "setoption name Nivel value 4",
               "setoption name Nivel value -1", "setoption name Nivel value x",
               "setoption name Nivel value", "setoption name Nivel value 2 extra")
    lines, code = run_engine(engine, "\n".join(invalid) + "\nisready\nquit\n")
    expected = ["error bad-command " + cmd.split()[0] for cmd in invalid] + ["readyok"]
    if code or lines != expected:
        failures.append(f"busca: validacao de parametros incorreta: {lines}")

    for command in ("go movetime 40", "go depth 64 movetime 40"):
        lines, code = run_engine(engine, "setoption name Nivel value 1\n" + command + "\nquit\n")
        info = next((ln for ln in lines if ln.startswith("info depth ")), "")
        # movetime permite superar a profundidade 2 do facil; com os dois
        # limites, o prazo deve interromper muito antes de depth 64.
        if code or not info or not 2 < int(info.split()[2]) < 64:
            failures.append(f"busca: movetime nao substitui/limita o perfil: {command}: {lines}")

    # Mate em um para as duas cores: score na perspectiva de quem joga.
    for fen in ("7k/8/5KQ1/8/8/8/8/8 w - - 0 1",
                "8/8/8/8/8/5kq1/8/7K b - - 0 1"):
        lines, code = run_engine(engine, f"position fen {fen}\ngo depth 2\nquit\n")
        if code or not any("score mate 1 " in ln for ln in lines):
            failures.append(f"busca: faltou score mate 1 em {fen}: {lines}")

    # Semente fixa e repetivel apos ucinewgame, com limites sem relogio.
    script = "setoption name Nivel value 1\ngo depth 2\ngo depth 2\n"
    first, _ = run_engine(engine, script + "quit\n")
    repeated, _ = run_engine(engine, script + "ucinewgame\n" + script + "quit\n")
    if repeated != first + first:
        failures.append("busca: ucinewgame nao reinicia a sequencia reproduzivel")
    if not failures:
        print("ok    limites de busca, scores, erros, imutabilidade e semente fixa")
    return failures


def check_draws(engine):
    failures = []
    knight_cycle = "g1f3 g8f6 f3g1 f6g8"
    cases = [
        ("fen 7k/8/8/8/8/8/8/K7 w - - 0 1", "insufficient-material"),
        ("fen 7k/8/8/8/8/8/8/KR6 w - - 100 1", "fifty-moves"),
        ("fen 7k/6Q1/5K2/8/8/8/8/8 b - - 100 1", "checkmate"),
        ("fen 7k/5Q2/6K1/8/8/8/8/8 b - - 100 1", "stalemate"),
        (f"startpos moves {knight_cycle} {knight_cycle}", "repetition"),
        (f"startpos moves {knight_cycle}", "none"),
    ]
    # Direitos de roque perdidos distinguem a posicao inicial dos ciclos seguintes.
    castle = "h1h2 h8h7 h2h1 h7h8"
    base = "fen r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1 moves "
    cases += [(base + " ".join([castle] * n), reason)
              for n, reason in ((2, "none"), (3, "repetition"))]
    # En passant sem captura legal (inclusive peao cravado) nao distingue posicoes.
    cycle = "b1c3 h8g8 c3b1 g8h8"
    cases += [("fen 7k/8/8/3p4/8/8/8/KN6 w - d6 0 1 moves "
               + " ".join([cycle] * 2), "repetition")]
    base = "fen 7k/8/8/3pP3/8/8/8/KN6 w - d6 0 1 moves "
    cases += [(base + " ".join([cycle] * n), reason)
              for n, reason in ((2, "none"), (3, "repetition"))]
    pinned = "b1c3 a8b8 c3b1 b8a8"
    cases += [("fen k3r3/8/8/3pP3/8/8/8/1N2K3 w - d6 0 1 moves "
               + " ".join([pinned] * 2), "repetition")]
    for position, reason in cases:
        lines, code = run_engine(engine, f"position {position}\nstate\ngo depth 1 movetime 1\nquit\n")
        if code or not any(f" reason {reason} fen " in ln for ln in lines) \
                or (reason != "none" and lines[-1] != "bestmove 0000"):
            failures.append(f"empate: esperado {reason} em {position}: {lines}")

    lines, code = run_engine(engine, f"position startpos moves {knight_cycle} {knight_cycle}\n"
                             "state\nposition startpos moves e2e4 e2e4\nstate\n"
                             "position fen 4k3/8/8/8/8/8/4R3/4K3 w - - 0 1\nstate\n"
                             "position startpos\nstate\nucinewgame\nstate\nquit\n")
    states = [ln for ln in lines if ln.startswith("state ")]
    if code or len(states) != 5 or states[0] != states[1] or states[1] != states[2] \
            or "reason none" not in states[3] or states[3] != states[4]:
        failures.append(f"empate: historico nao e atomico/resetavel: {lines}")
    if not failures:
        print("ok    empates, precedencia do mate, repeticao, roque e en passant efetivo")
    return failures


def check_level_response(engine):
    """Mede comando/resposta em pipe aberto, excluindo o spawn do processo."""
    failures = []
    proc = subprocess.Popen([str(engine)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL, text=True)
    answers = queue.Queue()
    def read():
        for line in proc.stdout:
            answers.put(line.strip())
    reader = threading.Thread(target=read, daemon=True)
    reader.start()
    def send(command):
        proc.stdin.write(command + "\n")
        proc.stdin.flush()
    try:
        kiwi = "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
        # O primeiro go usa o padrao medio. Os demais cobrem os tres niveis.
        for level, ceiling in ((None, 200), (1, 200), (2, 200), (3, 1000)):
            for position in ("startpos", kiwi):
                if level is not None:
                    send(f"setoption name Nivel value {level}")
                send(f"position {position}")
                send("isready")
                if answers.get(timeout=2) != "readyok":
                    failures.append("nivel: falha na preparacao")
                    return failures
                start = time.perf_counter()
                send("go")
                info = answers.get(timeout=2)
                best = answers.get(timeout=2)
                elapsed = (time.perf_counter() - start) * 1000
                if not info.startswith("info depth ") or not best.startswith("bestmove ") \
                        or best == "bestmove 0000" or elapsed >= ceiling:
                    failures.append(f"nivel {level or 'padrao'}: {elapsed:.1f} ms, teto {ceiling}: {info}, {best}")
                else:
                    print(f"ok    nivel {level or 'padrao'} em {position.split()[0]}: {elapsed:.1f} ms < {ceiling} ms")
    except queue.Empty:
        failures.append("nivel: sem resposta em 2 s com stdin aberta")
    finally:
        proc.stdin.close()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        proc.stdout.close()
    return failures


def check_trace_and_uci_edges(engine):
    """Cobre --trace, argumentos de linha de comando, d, line-too-long e bordas de position."""
    failures = []

    # 1. --trace valido grava entrada e saida + comando d
    with tempfile.TemporaryDirectory() as tmp:
        trace_path = Path(tmp) / "uci_trace.log"
        res = subprocess.run([str(engine), "--trace", str(trace_path)],
                             input="uci\nd\nisready\nquit\n",
                             capture_output=True, text=True, timeout=5)
        if res.returncode != 0 or "readyok" not in res.stdout:
            failures.append(f"trace: retorno/saida invalida: code={res.returncode}")
        elif not trace_path.is_file():
            failures.append("trace: arquivo de trace nao foi criado")
        else:
            content = trace_path.read_text(encoding="utf-8", errors="replace")
            if "> uci" not in content or "< uciok" not in content:
                failures.append(f"trace: conteudo incompleto: {content[:120]}")

        # 2. --trace sem caminho retorna codigo 2
        res_missing = subprocess.run([str(engine), "--trace"],
                                     input="", capture_output=True, text=True, timeout=5)
        if res_missing.returncode != 2:
            failures.append(f"trace sem arg: esperado exit 2, veio {res_missing.returncode}")

        # 3. --trace em diretorio inexistente e argumento desconhecido seguem operando
        bad_trace = Path(tmp) / "inexistente" / "trace.log"
        res_warn = subprocess.run([str(engine), "--trace", str(bad_trace), "--desconhecido"],
                                  input="isready\nquit\n",
                                  capture_output=True, text=True, timeout=5)
        if res_warn.returncode != 0 or res_warn.stdout.strip() != "readyok":
            failures.append(f"trace aviso/arg ignorado falhou: {res_warn.returncode}, {res_warn.stdout!r}")

    # 4. line-too-long (> 8192 bytes) e bordas de position
    too_many = " ".join(["e2e4"] * 1025)
    long_fen_tok = "a" * 260
    script = (
        ("a" * 8300) + "\n"
        "position outro\n"
        "position fen 4k3/8/8/8/8/8/8/4K3 w - moves e2e4\n"
        f"position fen {long_fen_tok} w - - 0 1\n"
        f"position startpos moves {too_many}\n"
        "position startpos moves lance_muito_longo_1234567890\n"
        "isready\n"
        "quit\n"
    )
    lines, code = run_engine(engine, script)
    expected_prefixes = [
        "error line-too-long",
        "error bad-command position",
        "error bad-command position",
        "error bad-fen",
        "error too-many-moves",
        "error illegal-move ",
        "readyok",
    ]
    if code != 0 or len(lines) != len(expected_prefixes) or any(
        not got.startswith(exp) for got, exp in zip(lines, expected_prefixes)
    ):
        failures.append(f"uci bordas: esperado {expected_prefixes}, veio {lines}")

    if not failures:
        print("ok    trace, argumentos CLI, d, line-too-long e bordas de position")
    return failures


def main():
    engine = find_engine(sys.argv)
    print(f"motor: {engine}\n")

    failures = check_transcripts(engine)
    failures += check_transport_open_stdin(engine)
    failures += check_go_is_legal(engine)
    failures += check_search_options(engine)
    failures += check_draws(engine)
    failures += check_trace_and_uci_edges(engine)
    failures += check_level_response(engine)

    print()
    if failures:
        for f in failures:
            print(f"FALHOU {f}")
        print(f"\n{len(failures)} falha(s)")
        return 1
    print("todos os testes de protocolo passaram")
    return 0


if __name__ == "__main__":
    sys.exit(main())
