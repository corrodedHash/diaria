import subprocess
from pathlib import Path
import pytest
from .helper import diaria, key_path


def test_empty_entry(diaria: Path, key_path: Path, tmp_path: Path):
    entry_path = tmp_path / "entries"
    diaria_cmd_base: list[Path | str] = [
        diaria,
        "--keys",
        key_path,
        "--entries",
        entry_path,
        "--password",
        "abc",
    ]
    with pytest.raises(subprocess.CalledProcessError):
        subprocess.run(
            [
                *diaria_cmd_base,
                "add",
                "--editor",
                f"touch %",
            ],
            check=True,
        )
    # No entry created with empty plaintext
    assert not entry_path.exists()


@pytest.mark.parametrize('whitespace_type', [" ", "\t", "\n", "\r", " \t\n\r"])
def test_empty_whitespace(diaria: Path, key_path: Path, tmp_path: Path, whitespace_type: str):
    entry_path = tmp_path / "entries"
    diaria_cmd_base: list[Path | str] = [
        diaria,
        "--keys",
        key_path,
        "--entries",
        entry_path,
        "--password",
        "abc",
    ]
    with pytest.raises(subprocess.CalledProcessError):
        subprocess.run(
            [
                *diaria_cmd_base,
                "add",
                "--editor",
                f"sh -c 'echo -n \"{whitespace_type}\" > %'",
            ],
            check=True,
        )
    # No entry created with empty plaintext
    assert not entry_path.exists()
