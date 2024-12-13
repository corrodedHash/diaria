import os
import pytest
from pathlib import Path
import subprocess
import warnings


@pytest.fixture
def diaria():
    cmake_exe_path = Path("src") / "cli" / "diaria"
    p = os.environ.get("DIARIA")
    if p is not None:
        return Path(p)

    build_dir = Path(__file__).parent.parent.parent / "build"
    if not build_dir.exists():
        raise RuntimeError(
            'Environment variable "DIARIA" not specified. Build directory not found'
        )

    if (build_dir / cmake_exe_path).exists():
        warnings.warn(
            UserWarning(
                f'Environment variable "DIARIA" not specified. Using {build_dir / cmake_exe_path}'
            )
        )

        return build_dir / cmake_exe_path
    if (build_dir / "dev" / cmake_exe_path).exists():
        warnings.warn(
            UserWarning(
                f'Environment variable "DIARIA" not specified. Using {build_dir / "dev" / cmake_exe_path}'
            )
        )

        return build_dir / "dev" / cmake_exe_path

    recursive_build_dir = [
        (x / cmake_exe_path)
        for x in build_dir.iterdir()
        if x.is_dir() and (x / cmake_exe_path).is_file()
    ]
    diaria_entry = max(
        [(x.stat().st_ctime_ns, x) for x in recursive_build_dir], key=lambda x: x[0]
    )
    warnings.warn(
        UserWarning(
            f'Environment variable "DIARIA" not specified. Using {diaria_entry[1]}'
        )
    )

    return diaria_entry[1]


@pytest.fixture
def key_path(diaria: Path, tmp_path: Path):
    key_path = tmp_path / "keys"
    key_path.mkdir(parents=True)
    subprocess.run([str(diaria), "-p", "abc", "--keys", key_path, "init"], check=True)
    assert len(list(key_path.iterdir())) == 3
    return key_path
