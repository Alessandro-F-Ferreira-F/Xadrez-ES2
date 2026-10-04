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

import subprocess
import sys
import threading
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

        expected = expected_file.read_text().splitlines()
        try:
            got, code = run_engine(engine, case.read_text())
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


def main():
    engine = find_engine(sys.argv)
    print(f"motor: {engine}\n")

    failures = check_transcripts(engine)
    failures += check_transport_open_stdin(engine)
    failures += check_go_is_legal(engine)

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
