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
    diaria_cmd_base = generate_cmd_base(diaria, key_path, entry_path)

    subprocess.run(
        [
            *diaria_cmd_base,
            "add",
            "--input",
            entry_file,
        ],
        check=True,
    )
    [diary_file] = list(entry_path.iterdir())
    diaria_check_entry(diaria_cmd_base, diary_file.absolute(), entry_text)


def test_write_read_interactive(diaria: Path, key_path: Path, tmp_path: Path):
    entry_path = tmp_path / "entries"
    entry_text = str(uuid.uuid4())
    entry_file = tmp_path / "plaintext_entry"
    with open(entry_file, "w", encoding="utf-8") as f:
        f.write(entry_text)
    diaria_cmd_base = generate_cmd_base(diaria, key_path, entry_path)

    subprocess.run(
        [
            *diaria_cmd_base,
            "add",
            "--editor",
            f"sh -c 'V=\"%\"; echo Writing to $V; cat {entry_file.absolute()} > $V'",
        ],
        check=True,
    )
    [diary_file] = list(entry_path.iterdir())
    diaria_check_entry(diaria_cmd_base, diary_file.absolute(), entry_text)


def test_write_read_interactive_unsandboxed(
    diaria: Path, key_path: Path, tmp_path: Path
):
    entry_text = str(uuid.uuid4())
    entry_file = tmp_path / "plaintext_entry"
    with open(entry_file, "w", encoding="utf-8") as f:
        f.write(entry_text)
    entry_path = tmp_path / "entries"
    diaria_cmd_base = generate_cmd_base(diaria, key_path, entry_path)

    subprocess.run(
        [
            *diaria_cmd_base,
            "add",
            "--no-sandbox",
            "--editor",
            f"sh -c 'V=\"%\"; echo Writing to $V; cat {entry_file.absolute()} > $V'",
        ],
        check=True,
    )
    [diary_file] = list(entry_path.iterdir())
    diaria_check_entry(diaria_cmd_base, diary_file.absolute(), entry_text)


def diaria_check_entry(cmd_base: list[Path | str], entry_file: Path, entry_text: str):
    read_output = subprocess.run(
        [
            *cmd_base,
            "read",
            entry_file,
        ],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    )
    assert read_output.stdout.strip() == entry_text


def generate_cmd_base(
    diaria: Path, key_path: Path, entry_path: Path
) -> list[Path | str]:
    return [
        diaria,
        "--keys",
        key_path,
        "--entries",
        entry_path,
        "--password",
        "abc",
    ]
