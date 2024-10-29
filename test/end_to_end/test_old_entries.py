import subprocess
from pathlib import Path
from .helper import diaria, key_path

def test_old_entries(diaria: Path, tmp_path: Path):
    key_path = Path(__file__).parent / "entry_data"
    entry_path = Path(__file__).parent / "entry_data"
    read_cmd: list[str | Path] = [
        diaria,
        "--keys",
        key_path,
        "--entries",
        entry_path,
        "--password",
        "password",
        "read",
    ]
    read_output = subprocess.run(
        [
            *read_cmd,
            entry_path / "eternal.diaria",
        ],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    ).stdout
    assert read_output.strip() == "Eons... pass like days"

    read_output = subprocess.run(
        [
            *read_cmd,
            entry_path / "long.diaria",
        ],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    ).stdout
    assert (
        "This runway is covered with the last pollen from the last flowers available anywhere on Earth."
        in read_output
    )
