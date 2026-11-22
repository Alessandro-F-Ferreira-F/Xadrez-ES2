from __future__ import annotations

import os
import sys
from datetime import datetime
from pathlib import Path

from desktop_automation import enable_dpi_awareness


ROOT = Path(__file__).resolve().parent


def main() -> int:
    enable_dpi_awareness()
    run_name = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    run_dir = ROOT / "reports" / run_name
    run_dir.mkdir(parents=True, exist_ok=False)
    os.environ["ACCEPTANCE_RUN_DIR"] = str(run_dir)
    os.chdir(ROOT)

    import pytest

    arguments = list(sys.argv[1:])
    if not any(arg.startswith("--junitxml") for arg in arguments):
        arguments.extend(["--junitxml", str(run_dir / "junit.xml")])
    if not arguments or arguments[0].startswith("--"):
        arguments.insert(0, "tests")

    print(f"Evidências e relatórios: {run_dir}")
    return int(pytest.main(arguments))


if __name__ == "__main__":
    raise SystemExit(main())
