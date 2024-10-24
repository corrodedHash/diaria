import subprocess
from pathlib import Path
from .helper import diaria, key_path
import datetime

def test_summarize(diaria: Path, key_path: Path, tmp_path: Path): 
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
    for i in range(0,41):
        timestamp = (datetime.datetime.now() - datetime.timedelta(days=i)).replace(microsecond=0).isoformat()

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
                f"{timestamp}.diaria"

            ],
            check=True,
        )
    assert len(list(entry_path.iterdir())) == 41
    summarize_output = subprocess.run(
        [
            *diaria_cmd_base,
            "summarize",
            "--long"
        ],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    ).stdout
    assert "--1--" in summarize_output
    assert "--7--" in  summarize_output
    assert "--31--" in  summarize_output