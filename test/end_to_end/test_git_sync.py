import subprocess
from pathlib import Path
from .helper import diaria, key_path
import uuid


def repo_init_bare(p: Path):
    subprocess.run(
        "git -c init.defaultBranch=main init --bare", check=True, cwd=p, shell=True
    )


def repo_init_1(p: Path, bare_repo: Path):
    commands = [
        "git -c init.defaultBranch=main init",
        'git config user.name "Testuser"',
        'git config user.email "testuser@llvm.org"',
        f"git remote add origin {bare_repo.absolute()}",
        "touch init.txt",
        "git add init.txt",
        'git commit -m "Init"',
        "git push -u origin main",
    ]
    for c in commands:
        subprocess.run(c, check=True, shell=True, cwd=p)


def repo_init_2(p: Path, bare_repo: Path):
    commands = [
        "git -c init.defaultBranch=main init",
        'git config user.name "Testuser"',
        'git config user.email "testuser@llvm.org"',
        f"git remote add origin {bare_repo.absolute()}",
        "git pull --set-upstream origin main",
    ]
    for c in commands:

        subprocess.run(c, check=True, shell=True, cwd=p)


def test_git_sync(diaria: Path, key_path: Path, tmp_path: Path):

    entry_1_path = tmp_path / "entries1"
    entry_2_path = tmp_path / "entries2"
    entry_bare_path = tmp_path / "entries_bare"

    entry_1_path.mkdir()
    entry_2_path.mkdir()
    entry_bare_path.mkdir()
    repo_init_bare(entry_bare_path)
    repo_init_1(entry_1_path, entry_bare_path)
    repo_init_2(entry_2_path, entry_bare_path)
    diaria_cmd_base: list[Path | str] = [
        diaria,
        "--keys",
        key_path,
        "--password",
        "abc",
    ]

    entry_text = str(uuid.uuid4())
    entry_file = tmp_path / "plaintext_entry"
    with open(entry_file, "w", encoding="utf-8") as f:
        f.write(entry_text)
    subprocess.run(
        [
            *diaria_cmd_base,
            "--entries",
            entry_1_path.absolute(),
            "add",
            "--input",
            entry_file,
        ],
        check=True,
    )

    subprocess.run([*diaria_cmd_base, "--entries", entry_1_path.absolute(), "sync"])
    subprocess.run([*diaria_cmd_base, "--entries", entry_2_path.absolute(), "sync"])

    synced_entry_file = [x for x in entry_2_path.iterdir() if x.suffix == (".diaria")][
        0
    ]
    read_output = subprocess.run(
        [
            *diaria_cmd_base,
            "--entries",
            entry_2_path.absolute(),
            "read",
            synced_entry_file,
        ],
        check=True,
        stdout=subprocess.PIPE,
        encoding="utf-8",
    ).stdout
    assert read_output.strip() == entry_text
