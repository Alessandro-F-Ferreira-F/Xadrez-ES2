#!/usr/bin/env bash
set -euo pipefail

python3 - <<'PY'
import datetime
import glob
import os
import sys


def parse_gcov(path):
    source = None
    covered = 0
    total = 0
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            parts = line.split(":", 2)
            if len(parts) < 3:
                continue
            count = parts[0].strip()
            lineno = parts[1].strip()
            text = parts[2].strip()
            if lineno == "0":
                if text.startswith("Source:"):
                    source = text[len("Source:"):].strip()
                continue
            if count == "-":
                continue
            # Linhas com contador numerico (ex.: "42" ou "42*") foram executadas
            clean_count = count.rstrip("*")
            if clean_count.isdigit():
                total += 1
                if int(clean_count) > 0:
                    covered += 1
            elif count.startswith("#####") or count.startswith("====="):
                total += 1
    return source, covered, total


def collect_dir(cov_dir, prefix):
    files = {}
    if not os.path.isdir(cov_dir):
        return files
    for gcov_path in sorted(glob.glob(os.path.join(cov_dir, "*.gcov"))):
        source, covered, total = parse_gcov(gcov_path)
        if not source or source.startswith("/") or total == 0:
            continue
        norm = os.path.normpath(os.path.join(prefix, source))
        # Evita duplicar se o mesmo header aparecer em dois .gcov: mantem maior cobertura
        if norm in files:
            prev_cov, prev_tot = files[norm]
            files[norm] = (max(prev_cov, covered), max(prev_tot, total))
        else:
            files[norm] = (covered, total)
    return files


all_files = {}
all_files.update(collect_dir("Backend/build/cov", "Backend"))
all_files.update(collect_dir("interface/build/cov", "interface"))

modules = {
    "Backend/engine": [0, 0, 0],  # covered, total, num_files
    "Backend/IA": [0, 0, 0],
    "interface": [0, 0, 0],
}

for path, (cov, tot) in all_files.items():
    p = path.replace("\\", "/")
    if p.startswith("Backend/engine/src/") or p.startswith("Backend/engine/include/"):
        mod = "Backend/engine"
    elif (p.startswith("Backend/IA/src/") or p.startswith("Backend/IA/header/")) and not p.endswith(("stubs.c", "teste.c")):
        mod = "Backend/IA"
    elif p.startswith("interface/src/") or p.startswith("interface/include/"):
        mod = "interface"
    else:
        continue
    modules[mod][0] += cov
    modules[mod][1] += tot
    modules[mod][2] += 1

print()
print("==========================================================================")
print("            RELATORIO CONSOLIDADO DE COBERTURA POR MODULO (gcov)          ")
print("==========================================================================")
print(f"{'Modulo':<20} | {'Arquivos':>8} | {'Cobertas':>10} | {'Executaveis':>11} | {'Cobertura':>10}")
print("-" * 74)

grand_cov = 0
grand_tot = 0
grand_files = 0

for mod in ("Backend/engine", "Backend/IA", "interface"):
    cov, tot, nfiles = modules[mod]
    pct = (100.0 * cov / tot) if tot > 0 else 0.0
    grand_cov += cov
    grand_tot += tot
    grand_files += nfiles
    print(f"{mod:<20} | {nfiles:>8d} | {cov:>10d} | {tot:>11d} | {pct:>9.2f}%")

print("-" * 74)
grand_pct = (100.0 * grand_cov / grand_tot) if grand_tot > 0 else 0.0
print(f"{'TOTAL PROJETO':<20} | {grand_files:>8d} | {grand_cov:>10d} | {grand_tot:>11d} | {grand_pct:>9.2f}%")
print("==========================================================================")
print("Arquivos .gcov detalhados em: Backend/build/cov/ e interface/build/cov/")

out_md = os.path.join("tools", "coverage.md")
now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
md_lines = [
    "# Relatório Consolidado de Cobertura de Testes (`gcov`)",
    "",
    f"_Gerado automaticamente por `make coverage` em {now_str}._",
    "",
    "| Módulo | Arquivos | Linhas Cobertas | Linhas Executáveis | Cobertura |",
    "| :--- | :---: | :---: | :---: | :---: |",
]
for mod in ("Backend/engine", "Backend/IA", "interface"):
    cov, tot, nfiles = modules[mod]
    pct = (100.0 * cov / tot) if tot > 0 else 0.0
    md_lines.append(f"| `{mod}` | {nfiles} | {cov} | {tot} | **{pct:.2f}%** |")
md_lines.append(
    f"| **TOTAL PROJETO** | **{grand_files}** | **{grand_cov}** | **{grand_tot}** | **{grand_pct:.2f}%** |"
)
md_lines.extend([
    "",
    "## Como reproduzir",
    "",
    "```sh",
    "make coverage",
    "```",
    "",
    "Os arquivos `.gcov` detalhados (linha a linha) ficam em `Backend/build/cov/` e `interface/build/cov/`.",
    "",
])
with open(out_md, "w", encoding="utf-8") as f:
    f.write("\n".join(md_lines))
print(f"Relatorio Markdown salvo em: {out_md}")
print()

if grand_tot == 0:
    sys.exit(1)
PY
