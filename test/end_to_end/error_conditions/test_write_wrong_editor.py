import subprocess
from pathlib import Path
from end_to_end.helper import diaria, key_path
import pytest


def test_write_wrong_editor(diaria: Path, key_path: Path, tmp_path: Path):
    with pytest.raises(subprocess.CalledProcessError) as e_info:
        output = subprocess.run(
            [
                diaria,
                "--keys",
                key_path,
                "--entries",
                tmp_path / "entries",
                "add",
                "--no-sandbox",
                "--editor",
                "/dev/null %",
            ],
            check=True,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            encoding="utf-8",
        )
    assert "Editor did not terminate successfully" in e_info.value.stderr
