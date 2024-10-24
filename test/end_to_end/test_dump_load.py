import subprocess
from pathlib import Path
from .helper import diaria, key_path
import uuid


def test_load_dump(diaria: Path, key_path: Path, tmp_path: Path):
    entry_1_path = tmp_path / "entries1"
    dump_1_path = tmp_path / "dump1"
    dump_2_path = tmp_path / "dump2"

    dump_1_path.mkdir()
    dump_2_path.mkdir()

    for i in range(4):
        entry_text = str(uuid.uuid4())
        entry_file = dump_1_path / f"entry_{i:02}"
        with open(entry_file, "w", encoding="utf-8") as f:
            f.write(entry_text)

    subprocess.run(
        [
            diaria,
            "--entries",
            entry_1_path.absolute(),
            "--keys",
            key_path,
            "--password",
            "abc",
            "load",
            dump_1_path,
        ]
    )
    assert len(list(entry_1_path.iterdir())) == 4
    subprocess.run(
        [
            diaria,
            "--entries",
            entry_1_path.absolute(),
            "--keys",
            key_path,
            "--password",
            "abc",
            "dump",
            dump_2_path,
        ]
    )

    for x in dump_2_path.iterdir():
        with open(x, "r", encoding="utf-8") as sink, open(
            dump_1_path / x.with_suffix("").name, "r", encoding="utf-8"
        ) as source:
            assert sink.read() == source.read()
