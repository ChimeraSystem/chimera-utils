import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import tempfile

ABI = "chimera-1"
ARCH = "x86_64"
NAME = re.compile(r"[a-z0-9][a-z0-9+.-]{0,63}\Z")
SHA256 = re.compile(r"[0-9a-f]{64}\Z")
MAX_ARCHIVE = 512 * 1024 * 1024
MAX_PAYLOAD = 1024 * 1024 * 1024


class Error(Exception):
    pass


def require(condition, message):
    if not condition:
        raise Error(message)


def package_name(name):
    require(isinstance(name, str) and NAME.fullmatch(name), f"invalid package name: {name!r}")
    return name


def dependencies(value):
    require(isinstance(value, list) and len(value) <= 256, "invalid dependency list")
    names = [package_name(name) for name in value]
    require(len(names) == len(set(names)), "duplicate dependency")
    return names


def payload_path(value):
    require(isinstance(value, str) and 0 < len(value) <= 240 and "\\" not in value and "\0" not in value,
            "invalid payload path")
    path = PurePosixPath(value)
    require(not path.is_absolute() and all(part not in ("", ".", "..") for part in value.split("/")),
            f"unsafe payload path: {value}")
    require(value.startswith(("bin/", "usr/bin/", "usr/lib/", "usr/libexec/", "usr/share/", "usr/include/",
                              "Applications/", "System/Library/", "Library/Fonts/")),
            f"unsupported install path: {value}")
    return value


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as source:
        while block := source.read(128 * 1024):
            value.update(block)
    return value.hexdigest()


def sync_directory(path):
    fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def atomic_json(path, data, mode=0o600):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix="." + path.name, dir=path.parent)
    try:
        with os.fdopen(fd, "w") as output:
            json.dump(data, output, sort_keys=True)
            output.write("\n")
            output.flush()
            os.fchmod(output.fileno(), mode)
            os.fsync(output.fileno())
        os.replace(name, path)
        sync_directory(path.parent)
    finally:
        Path(name).unlink(missing_ok=True)


def read_json(path, limit=4 * 1024 * 1024):
    with Path(path).open("rb") as source:
        data = source.read(limit + 1)
    require(len(data) <= limit, f"metadata too large: {path}")

    def unique(items):
        result = {}
        for key, value in items:
            require(key not in result, f"duplicate JSON key: {key}")
            result[key] = value
        return result

    try:
        return json.loads(data, object_pairs_hook=unique)
    except (ValueError, UnicodeError) as error:
        raise Error(f"invalid JSON in {path}: {error}") from None
