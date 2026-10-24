import hashlib
import json
import os
from pathlib import PurePosixPath
import posixpath
import tarfile

from common import (ABI, ARCH, MAX_PAYLOAD, SHA256, dependencies, package_name,
                    payload_path, require)


def validate_manifest(manifest, package):
    require(isinstance(manifest, dict) and manifest.get("schema") == 1, "unsupported package manifest")
    require(package_name(manifest.get("name")) == package["name"], "package name mismatch")
    for key in ("version", "architecture", "abi"):
        require(manifest.get(key) == package[key], f"package {key} mismatch: {package['name']}")
    require(manifest["architecture"] == ARCH and manifest["abi"] == ABI, "incompatible package")
    require(set(dependencies(manifest.get("dependencies"))) == set(package["dependencies"]),
            f"dependency metadata changed: {package['name']}")
    files = manifest.get("files")
    require(isinstance(files, list) and 0 < len(files) <= 50000, "invalid file list")
    entries, total = {}, 0
    for entry in files:
        require(isinstance(entry, dict), "invalid file metadata")
        path = payload_path(entry.get("path"))
        require(path not in entries, f"duplicate payload path: {path}")
        if entry.get("type") == "file":
            require(type(entry.get("size")) is int and 0 <= entry["size"] <= MAX_PAYLOAD, "invalid file size")
            require(isinstance(entry.get("sha256"), str) and SHA256.fullmatch(entry["sha256"]), "invalid file digest")
            require(entry.get("mode") in (0o644, 0o755), "only 0644 and 0755 file modes are supported")
            total += entry["size"]
        else:
            require(entry.get("type") == "symlink", "only files and symlinks are supported")
            target = entry.get("target")
            require(isinstance(target, str) and 0 < len(target) <= 240 and "\0" not in target and "\\" not in target,
                    "invalid symlink target")
            resolved = target.lstrip("/") if target.startswith("/") else posixpath.normpath(str(PurePosixPath(path).parent / target))
            payload_path(resolved)
        entries[path] = entry
    require(total <= MAX_PAYLOAD, "unpacked payload exceeds size limit")
    for path in entries:
        require(not any(str(parent) in entries for parent in PurePosixPath(path).parents),
                f"payload file is also a parent directory: {path}")
    return entries


def unpack(archive, destination, package):
    with tarfile.open(archive, mode="r|gz") as source:
        first = next(iter(source), None)
        require(first is not None and first.name == "manifest.json" and first.isfile() and
                0 < first.size <= 1024 * 1024, "archive must start with manifest.json")
        try:
            manifest = json.loads(source.extractfile(first).read())
        except (ValueError, UnicodeError):
            require(False, "invalid archive manifest")
        entries = validate_manifest(manifest, package)
        seen = set()
        for member in source:
            require(not member.pax_headers and not member.sparse, "extended tar headers are unsupported")
            if member is first:
                continue
            require(member.name.startswith("payload/"), f"unexpected archive entry: {member.name}")
            relative = payload_path(member.name[len("payload/"):])
            require(relative in entries and relative not in seen, f"unexpected or duplicate archive entry: {relative}")
            entry = entries[relative]
            output = destination / relative
            output.parent.mkdir(parents=True, exist_ok=True)
            if entry["type"] == "symlink":
                require(member.issym() and member.linkname == entry["target"] and member.size == 0,
                        f"symlink metadata mismatch: {relative}")
                output.symlink_to(entry["target"])
            else:
                require(member.isfile() and member.size == entry["size"], f"file metadata mismatch: {relative}")
                checksum = hashlib.sha256()
                stream = source.extractfile(member)
                remaining = member.size
                with output.open("xb") as target:
                    while remaining:
                        block = stream.read(min(128 * 1024, remaining))
                        require(block, f"truncated payload: {relative}")
                        target.write(block)
                        checksum.update(block)
                        remaining -= len(block)
                    target.flush()
                    os.fchmod(target.fileno(), entry["mode"])
                    os.fsync(target.fileno())
                require(checksum.hexdigest() == entry["sha256"], f"file digest mismatch: {relative}")
            seen.add(relative)
        require(seen == set(entries), "archive is missing payload files")
        return manifest
