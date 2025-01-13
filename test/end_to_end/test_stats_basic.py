import subprocess
from pathlib import Path
from .helper import diaria, key_path
import datetime


def test_stats_basic(diaria: Path, key_path: Path, tmp_path: Path):
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

    entry_templates = [datetime.datetime(2020, 8, 7), datetime.datetime(1931, 5, 2)]
    for i, timestamp in enumerate(entry_templates):
        timestamp = timestamp.isoformat()

        entry_file = tmp_path / f"plaintext_entry_{i}"
        with open(entry_file, "w", encoding="utf-8") as f:
            f.write(f"--{i}--")

        subprocess.run(
            [
                *diaria_cmd_base,
                "add",
                "--input",
                entry_file,
                "--output",
                entry_path / f"{timestamp}.diaria",
            ],
            check=True,
        )
    assert len(list(entry_path.iterdir())) == len(entry_templates)
    stats_output = subprocess.run(
        [*diaria_cmd_base, "stats"],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    ).stdout
    assert "1931" in stats_output
    assert "2020" in stats_output
    assert not "2021" in stats_output


def test_stats_no_dir(diaria: Path, key_path: Path, tmp_path: Path):
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
        [*diaria_cmd_base, "stats"],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    )


def test_stats_empty(diaria: Path, key_path: Path, tmp_path: Path):
    entry_path = tmp_path / "entries"
    entry_path.mkdir()
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
        [*diaria_cmd_base, "stats"],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    )
