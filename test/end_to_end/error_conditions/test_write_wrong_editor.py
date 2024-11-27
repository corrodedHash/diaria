import subprocess
from pathlib import Path
from end_to_end.helper import diaria, key_path


def test_write_wrong_editor(diaria: Path, key_path: Path, tmp_path: Path):
    output = subprocess.run(
        [
            diaria,
            "--keys",
            key_path,
            "--entries",
            tmp_path / "entries",
            "add",
            "--editor",
            "/dev/null %",
        ]
    )
    assert output.returncode != 0
