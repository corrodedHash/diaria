import subprocess
from pathlib import Path
from ..helper import diaria, key_path


def test_find_executable(diaria: Path, key_path: Path, tmp_path: Path):
    entry_path = tmp_path/"entries"
    entry_path.mkdir()
    output = subprocess.run(
        [
            diaria,
            "--keys",
            key_path,
            "--entries",
            entry_path,
            "add",
            "--input",
            "/dev/null/nonexistent",
        ]
    )
    assert output.returncode != 0
    assert len(list(entry_path.iterdir())) == 0
