import os
import pytest
from pathlib import Path
import subprocess
import warnings


@pytest.fixture
def diaria():
    p = os.environ.get("DIARIA")
    if p is not None:
        return Path(p)

    build_dir = Path(__file__).parent.parent.parent / "build"
    if not build_dir.exists():
        raise RuntimeError(
            'Environment variable "DIARIA" not specified. Build directory not found'
        )

    if (build_dir / "diaria").exists():
        warnings.warn(
            UserWarning(
                f'Environment variable "DIARIA" not specified. Using {build_dir/"diaria"}'
            )
        )

        return build_dir / "diaria"
    if (build_dir / "dev" / "diaria").exists():
        warnings.warn(
            UserWarning(
                f'Environment variable "DIARIA" not specified. Using {build_dir/"dev"/"diaria"}'
            )
        )

        return build_dir / "dev" / "diaria"

    recursive_build_dir = [
        (x / "diaria")
        for x in build_dir.iterdir()
        if x.is_dir() and (x / "diaria").is_file()
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
