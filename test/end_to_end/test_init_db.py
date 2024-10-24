import subprocess
from pathlib import Path
from .helper import diaria


def test_init_db(diaria: Path, tmp_path: Path):
    key_path = tmp_path / "keys"
    key_path.mkdir(parents=True)
    subprocess.run([str(diaria), "-p", "abc", "--keys", key_path, "init"], check=True)
    assert len(list(key_path.iterdir())) == 3
