from contextlib import contextmanager
import fcntl
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import tempfile

from common import Error, atomic_json, digest, package_name, read_json, require, sync_directory
from archive import validate_manifest


class Store:
    def __init__(self, root):
        self.root = Path(root).resolve()
        self.state = self.root / "var/db/fuse"
        self.receipts = self.root / "Library/Receipts/fuse"

    def target(self, relative, mkdir=False):
        require(isinstance(relative, str) and not relative.startswith("/") and
                all(part not in ("", ".", "..") for part in relative.split("/")), "unsafe installed path")
        path = self.root
        for part in PurePosixPath(relative).parts[:-1]:
            parent = path
            path = path / part
            if mkdir and not path.exists() and not path.is_symlink():
                path.mkdir(mode=0o755)
                sync_directory(parent)
            if path.exists() or path.is_symlink():
                require(not path.is_symlink() and path.is_dir(), f"unsafe parent directory: {path}")
        return self.root / relative

    @contextmanager
    def lock(self):
        self.root.mkdir(parents=True, exist_ok=True)
        self.target("var/db/fuse/lock", mkdir=True)
        os.chmod(self.state, 0o700)
        fd = os.open(self.state / "lock", os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW, 0o600)
        try:
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError:
                raise Error("another Fuse operation is running") from None
            self.recover()
            yield
        finally:
            os.close(fd)

    def records(self):
        self.target("Library/Receipts/fuse/unused")
        records = {}
        if not self.receipts.exists():
            return records
        for path in sorted(self.receipts.glob("*.json")):
            require(not path.is_symlink(), "receipt must not be a symlink")
            data = read_json(path, 16 * 1024 * 1024)
            require(isinstance(data, dict) and data.get("schema") == 1 and
                    package_name(data.get("name")) == path.stem, f"invalid receipt: {path}")
            validate_manifest(data, data)
            records[data["name"]] = data
        return records

    def intact(self, receipt):
        for entry in receipt["files"]:
            target = self.target(entry["path"])
            if entry["type"] == "symlink":
                if not target.is_symlink() or os.readlink(target) != entry["target"]:
                    return False
            elif target.is_symlink() or not target.is_file() or digest(target) != entry["sha256"] or stat.S_IMODE(target.stat().st_mode) != entry["mode"]:
                return False
        return True

    def rollback(self, directory, journal):
        for entry in reversed(journal["operations"]):
            target = self.target(entry["path"], mkdir=True)
            backup = directory / "backup" / entry["path"]
            if backup.exists() or backup.is_symlink():
                os.replace(backup, target)
                sync_directory(target.parent)
                sync_directory(backup.parent)
            elif not entry["existed"]:
                target.unlink(missing_ok=True)
                sync_directory(target.parent)
        journal["status"] = "rolled-back"
        atomic_json(directory / "journal.json", journal)

    def recover(self):
        self.target("var/db/fuse/transactions/unused", mkdir=True)
        for directory in sorted((self.state / "transactions").iterdir()):
            require(directory.is_dir() and not directory.is_symlink(), "invalid transaction directory")
            path = directory / "journal.json"
            if path.exists():
                journal = read_json(path, 32 * 1024 * 1024)
                require(journal.get("schema") == 1 and journal.get("status") in ("pending", "committed", "rolled-back"),
                        "invalid recovery journal")
                if journal["status"] == "pending":
                    print("Recovering interrupted installation", flush=True)
                    self.rollback(directory, journal)
            shutil.rmtree(directory)
        sync_directory(self.state / "transactions")

    def temporary(self):
        parent = self.state / "transactions"
        directory = Path(tempfile.mkdtemp(prefix="install-", dir=parent))
        sync_directory(parent)
        return directory

    def commit(self, directory, manifests, packages, records):
        owners = {entry["path"]: name for name, record in records.items() for entry in record["files"]}
        destinations = {}
        for manifest in manifests:
            name = manifest["name"]
            for entry in manifest["files"]:
                path = entry["path"]
                require(path not in destinations, f"packages both install {path}")
                require(path not in owners or owners[path] == name, f"{path} belongs to {owners.get(path)}")
                destinations[path] = directory / name / "payload" / path
            old = records.get(name)
            if old:
                new_paths = {entry["path"] for entry in manifest["files"]}
                for entry in old["files"]:
                    if entry["path"] not in new_paths:
                        destinations[entry["path"]] = None
            receipt = {**manifest, "repository": packages[name]["repository"],
                       "archive_sha256": packages[name]["sha256"], "release_tag": packages[name]["tag"]}
            path = f"Library/Receipts/fuse/{name}.json"
            staged = directory / name / "receipt.json"
            atomic_json(staged, receipt, mode=0o644)
            destinations[path] = staged
        journal = {"schema": 1, "status": "pending", "operations": []}
        try:
            for path, staged in destinations.items():
                target = self.target(path, mkdir=True)
                existed = target.exists() or target.is_symlink()
                require(not existed or target.is_symlink() or target.is_file(), f"destination is not a file: {target}")
                backup = self.target(str((directory / "backup" / path).relative_to(self.root)), mkdir=True)
                journal["operations"].append({"path": path, "existed": existed})
                atomic_json(directory / "journal.json", journal)
                if existed:
                    os.replace(target, backup)
                    sync_directory(backup.parent)
                    sync_directory(target.parent)
                if staged is not None:
                    os.replace(staged, target)
                    sync_directory(target.parent)
            journal["status"] = "committed"
            atomic_json(directory / "journal.json", journal)
        except BaseException:
            self.rollback(directory, journal)
            raise
        shutil.rmtree(directory)
        sync_directory(self.state / "transactions")
