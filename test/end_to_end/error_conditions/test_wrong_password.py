import subprocess
from pathlib import Path
from ..helper import diaria, key_path
import uuid


def test_find_executable(diaria: Path, key_path: Path, tmp_path: Path):
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
    ]
    subprocess.run(
        [
            *diaria_cmd_base,
            "--password",
            "abc",
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
            "--password",
            "wrong",
            "read",
            diary_file.absolute(),
        ],
        stdout=subprocess.PIPE,
        encoding="utf-8",
    )
    assert read_output.returncode != 0
    # assert read_output.stdout.strip() == entry_text
