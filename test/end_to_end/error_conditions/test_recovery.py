import subprocess
from pathlib import Path
from end_to_end.helper import diaria, key_path
import pytest
import uuid

DUMP_FILE = "/tmp/diaria_dump"


@pytest.mark.skip(reason="Namespaces break in CI test environment currently")
def test_no_keys(diaria: Path, tmp_path: Path):
    key_path = tmp_path / "keys"
    key_path.mkdir(parents=True)

    entry_text = str(uuid.uuid4())
    entry_file = tmp_path / "plaintext_entry"
    with open(entry_file, "w", encoding="utf-8") as f:
        f.write(entry_text)
    with pytest.raises(subprocess.CalledProcessError) as e_info:
        output = subprocess.run(
            [
                diaria,
                "--keys",
                key_path,
                "--entries",
                tmp_path / "entries",
                "add",
                "--editor",
                f"sh -c 'cat {entry_file.absolute()} > % '",
            ],
            check=True,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            encoding="utf-8",
        )
    with open(DUMP_FILE, "r", encoding="utf-8") as f:
        assert f.read() == entry_text


def test_no_keys_no_sandbox(diaria: Path, tmp_path: Path):
    key_path = tmp_path / "keys"
    key_path.mkdir(parents=True)

    entry_text = str(uuid.uuid4())
    entry_file = tmp_path / "plaintext_entry"
    with open(entry_file, "w", encoding="utf-8") as f:
        f.write(entry_text)
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
                f"sh -c 'cat {entry_file.absolute()} > % '",
            ],
            check=True,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            encoding="utf-8",
        )
    with open(DUMP_FILE, "r", encoding="utf-8") as f:
        assert f.read() == entry_text


# def test_inaccessible_output(diaria: Path, tmp_path: Path):
#     key_path = tmp_path / "keys"
#     key_path.mkdir(parents=True)

#     entry_text = str(uuid.uuid4())
#     entry_file = tmp_path / "plaintext_entry"
#     with open(entry_file, "w", encoding="utf-8") as f:
#         f.write(entry_text)
#     with pytest.raises(subprocess.CalledProcessError) as e_info:
#         output = subprocess.run(
#             [
#                 diaria,
#                 "--keys",
#                 key_path,
#                 "--entries",
#                 tmp_path / "entries",
#                 "add",
#                 "--output",
#                 ""
#                 "--no-sandbox",
#                 "--editor",
#                 f"sh -c 'cat {entry_file.absolute()} > % '",
#             ],
#             check=True,
#             stderr=subprocess.PIPE,
#             stdout=subprocess.PIPE,
#             encoding="utf-8",
#         )
#     with open(DUMP_FILE, "r", encoding="utf-8") as f:
#         assert f.read() == entry_text
