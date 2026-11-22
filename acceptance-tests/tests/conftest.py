from __future__ import annotations

import json
import os
import re
import time
from datetime import datetime
from pathlib import Path
from typing import Any, Dict, Iterator, List

import pytest

from desktop_automation import (
    DesktopAutomationUnavailable,
    GameSession,
    ROOT,
    enable_dpi_awareness,
)


_RUN_DIR: Path | None = None


def _slug(value: str) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "-", value).strip("-.") or "scenario"


def pytest_configure(config: pytest.Config) -> None:
    global _RUN_DIR
    enable_dpi_awareness()
    configured = os.environ.get("ACCEPTANCE_RUN_DIR")
    if configured:
        _RUN_DIR = Path(configured).resolve()
    else:
        run_name = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
        _RUN_DIR = ROOT / "acceptance-tests" / "reports" / run_name
    _RUN_DIR.mkdir(parents=True, exist_ok=True)
    config.addinivalue_line(
        "markers",
        "acceptance(description): desktop acceptance scenario and expected behavior",
    )


@pytest.fixture
def game(request: pytest.FixtureRequest) -> Iterator[GameSession]:
    assert _RUN_DIR is not None
    session = GameSession(_RUN_DIR, request.node.name)
    request.node._acceptance_session = session
    if os.name != "nt":
        pytest.skip(
            "Automação de janela Win32 indisponível fora do Windows; "
            "use uma sessão gráfica Windows interativa."
        )
    try:
        session.launch()
    except DesktopAutomationUnavailable as exc:
        pytest.skip(str(exc))
    try:
        yield session
    finally:
        phases = getattr(request.node, "_acceptance_phase_reports", [])
        if any(phase.failed for phase in phases) and session.process is not None:
            session.save_capture("falha-estado-final")
        session.close()


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item: pytest.Item, call: pytest.CallInfo[Any]):
    result = yield
    report = result.get_result()
    reports = getattr(item, "_acceptance_phase_reports", [])
    reports.append(report)
    item._acceptance_phase_reports = reports


def _test_result(item: pytest.Item) -> Dict[str, Any]:
    marker = item.get_closest_marker("acceptance")
    description = (
        marker.kwargs.get("description", "")
        if marker is not None
        else (item.function.__doc__ or "").strip()
    )
    phases = getattr(item, "_acceptance_phase_reports", [])
    failed = [phase for phase in phases if phase.failed]
    skipped = [phase for phase in phases if phase.skipped]
    status = "FAIL" if failed else ("SKIP" if skipped else "PASS")
    reason = "\n".join(
        str(getattr(phase, "longreprtext", phase.longrepr))
        for phase in failed
    )
    session = getattr(item, "_acceptance_session", None)
    evidence: List[str] = []
    log_file = None
    observed_behavior: List[str] = []
    protocol_evidence: List[str] = []
    limitations: List[str] = []
    if session is not None:
        evidence = [
            path.relative_to(ROOT).as_posix()
            if path.is_relative_to(ROOT)
            else str(path)
            for path in session.evidence
        ]
        log_file = (
            session.log_path.relative_to(ROOT).as_posix()
            if session.log_path.is_relative_to(ROOT)
            else str(session.log_path)
        )
        observed_behavior = list(session.observed_behaviors)
        protocol_evidence = list(session.protocol_evidence)
        limitations = list(session.verification_limitations)
    return {
        "name": item.name,
        "nodeid": item.nodeid,
        "result": status,
        "expected": description,
        "failure_reason": reason,
        "duration_seconds": round(sum(phase.duration for phase in phases), 3),
        "observed_behavior": observed_behavior,
        "protocol_evidence": protocol_evidence,
        "limitations": limitations,
        "evidence": evidence,
        "log_file": log_file,
    }


def _write_reports(session: pytest.Session, exitstatus: int) -> None:
    assert _RUN_DIR is not None
    results = [_test_result(item) for item in session.items]
    payload = {
        "generated_at": datetime.now().astimezone().isoformat(),
        "exit_code": int(exitstatus),
        "summary": {
            "passed": sum(row["result"] == "PASS" for row in results),
            "failed": sum(row["result"] == "FAIL" for row in results),
            "skipped": sum(row["result"] == "SKIP" for row in results),
            "total": len(results),
        },
        "tests": results,
    }
    (_RUN_DIR / "report.json").write_text(
        json.dumps(payload, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )

    lines = [
        "# Relatório de Testes de Aceitação",
        "",
        f"- Gerado: {payload['generated_at']}",
        f"- Código de saída do pytest: {payload['exit_code']}",
        (
            "- Resumo: "
            f"{payload['summary']['passed']} PASS, "
            f"{payload['summary']['failed']} FAIL, "
            f"{payload['summary']['skipped']} SKIP "
            f"({payload['summary']['total']} cenários)"
        ),
        "",
    ]
    for row in results:
        lines.extend(
            [
                f"## {row['name']} — {row['result']}",
                "",
                f"- Esperado: {row['expected'] or 'Não informado'}",
                f"- Duração: {row['duration_seconds']:.3f}s",
            ]
        )
        if row["observed_behavior"]:
            lines.extend(["- Comportamento observado:"])
            lines.extend(f"  - {entry}" for entry in row["observed_behavior"])
        if row["protocol_evidence"]:
            lines.extend(["- Evidência protocolar (respostas já registradas pela interface):"])
            lines.extend(f"  - `{entry}`" for entry in row["protocol_evidence"])
        if row["limitations"]:
            lines.extend(["- Limitações da verificação:"])
            lines.extend(f"  - {entry}" for entry in row["limitations"])
        if row["failure_reason"]:
            reason = row["failure_reason"].strip()
            lines.extend(["- Motivo da falha:", "", "```text", reason, "```"])
        if row["evidence"]:
            lines.append("- Evidências:")
            for path in row["evidence"]:
                evidence_path = Path(path)
                source_path = (
                    evidence_path if evidence_path.is_absolute() else ROOT / evidence_path
                )
                relative_to_run = (
                    source_path.relative_to(_RUN_DIR).as_posix()
                    if source_path.is_relative_to(_RUN_DIR)
                    else evidence_path.as_posix()
                )
                lines.append(
                    f"  - [{evidence_path.name}]({relative_to_run})"
                )
        else:
            lines.append("- Evidências: nenhuma captura foi produzida.")
        if row["log_file"]:
            log_link = f"logs/{Path(row['log_file']).name}"
            lines.append(
                f"- Log da aplicação: [{Path(row['log_file']).name}]({log_link})"
            )
        lines.append("")

    (_RUN_DIR / "report.md").write_text("\n".join(lines), encoding="utf-8")


def pytest_sessionfinish(session: pytest.Session, exitstatus: int) -> None:
    _write_reports(session, exitstatus)
