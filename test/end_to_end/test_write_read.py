import subprocess
from pathlib import Path
from .helper import diaria, key_path
import uuid


def test_write_read(diaria: Path, key_path: Path, tmp_path: Path):
    entry_text = str(uuid.uuid4())
    entry_file = tmp_path / "plaintext_entry"
    with open(entry_file, "w", encoding="utf-8") as f:
        f.write(entry_text)
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
    subprocess.run(
        [
            *diaria_cmd_base,
            "add",
            "--input",
            entry_file,
        ],
        check=True,
    )
    assert len(list(entry_path.iterdir())) == 1
    diary_file = list(entry_path.iterdir())[0]
    read_output = subprocess.run(
        [
            *diaria_cmd_base,
            "read",
            diary_file.absolute(),
        ],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    )
    assert read_output.stdout.strip() == entry_text


def test_write_read_interactive(diaria: Path, key_path: Path, tmp_path: Path):
    entry_text = str(uuid.uuid4())
    entry_file = tmp_path / "plaintext_entry"
    with open(entry_file, "w", encoding="utf-8") as f:
        f.write(entry_text)
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
    subprocess.run(
        [
            *diaria_cmd_base,
            "add",
            "--editor",
            f"sh -c 'V=\"%\"; echo Writing to $V; cat {entry_file.absolute()} > $V'",
        ],
        check=True,
    )
    assert len(list(entry_path.iterdir())) == 1
    diary_file = list(entry_path.iterdir())[0]
    read_output = subprocess.run(
        [
            *diaria_cmd_base,
            "read",
            diary_file.absolute(),
        ],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    )
    assert read_output.stdout.strip() == entry_text
