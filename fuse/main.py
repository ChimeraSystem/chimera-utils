#!/usr/bin/python3
import argparse
from contextlib import nullcontext
import os
from pathlib import Path
import shutil
import stat
import sys
import tarfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from archive import unpack
from common import Error, digest, package_name, read_json, require
from repository import DEFAULT_INDEX, Repository, Transport
from transaction import Store


def token_for(root):
    token = os.environ.get("GITHUB_TOKEN")
    path = root / "private/etc/fuse/github-token"
    if not token and path.exists():
        info = path.lstat()
        require(stat.S_ISREG(info.st_mode) and info.st_uid == os.geteuid() and info.st_mode & 0o077 == 0,
                "github-token must be owned by the current user and have mode 0600")
        with path.open() as source:
            token = source.read(1025).strip()
    require(not token or len(token) <= 1024 and all(33 <= ord(c) <= 126 for c in token), "invalid GitHub token")
    return token


def indexes_for(args, root):
    if args.index:
        return args.index
    path = root / "private/etc/fuse/repositories.json"
    if not path.exists():
        return [DEFAULT_INDEX]
    data = read_json(path)
    require(isinstance(data, dict) and data.get("schema") == 1 and
            isinstance(data.get("repositories"), list) and 0 < len(data["repositories"]) <= 32 and
            all(isinstance(url, str) for url in data["repositories"]), "invalid repositories.json")
    return data["repositories"]


def installed_plan(repository, names, store, records):
    order = repository.order(names)
    packages, pending = {}, []
    for name in order:
        package = repository.latest(name)
        packages[name] = package
        old = records.get(name)
        current = old and old.get("version") == package["version"] and old.get("archive_sha256") == package["sha256"] and store.intact(old)
        action = "installed" if current else "upgrade" if old else "install"
        print(f"  {action:9} {name:20} {package['version']:16} {package['download_size']:10} bytes")
        print("            dependencies: " + (", ".join(package["dependencies"]) or "none"))
        if not current:
            pending.append(name)
    print(f"Download: {sum(packages[name]['download_size'] for name in pending)} bytes; {len(pending)} package(s)", flush=True)
    return packages, pending


def install(args, repository, store, records):
    print("Installation plan (dependencies first):", flush=True)
    packages, pending = installed_plan(repository, args.packages, store, records)
    if args.command == "plan" or args.dry_run or not pending:
        return
    if not args.yes:
        require(sys.stdin.isatty(), "use --yes to install without an interactive terminal")
        if input("Proceed? [y/N] ").strip().lower() not in ("y", "yes"):
            print("Cancelled")
            return
    directory = store.temporary()
    manifests = []
    try:
        for name in pending:
            package = packages[name]
            stage = directory / name
            stage.mkdir()
            archive = stage / "download.tar.gz"
            print(f"Downloading {name} {package['version']}", flush=True)
            repository.transport.fetch(package["url"], archive, package["download_size"], "application/octet-stream")
            require(archive.stat().st_size == package["download_size"] and digest(archive) == package["sha256"],
                    f"archive checksum or size mismatch: {name}")
            manifests.append(unpack(archive, stage / "payload", package))
            archive.unlink()
        store.commit(directory, manifests, packages, records)
    finally:
        # keep failed journals and their backups for recovery on the next run
        if directory.exists() and not (directory / "journal.json").exists():
            shutil.rmtree(directory)
    for name in pending:
        print(f"Installed {name} {packages[name]['version']}", flush=True)


def main(argv=None):
    parser = argparse.ArgumentParser(prog="fuse", description="Install Chimera packages from GitHub Releases")
    parser.add_argument("command", choices=("install", "plan", "search", "info", "list", "verify", "update"))
    parser.add_argument("packages", nargs="*")
    parser.add_argument("--root", type=Path, default=Path("/"), help="install into another root directory")
    parser.add_argument("--index", action="append", help="use this repository index URL instead of system configuration")
    parser.add_argument("--yes", "-y", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--allow-http", action="store_true", help="allow an explicitly trusted HTTP repository")
    parser.add_argument("--api", default="https://api.github.com", help="GitHub API endpoint (or test fixture)")
    parser.add_argument("--fetch-helper", default="/usr/libexec/fuse-fetch", help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    if args.command in ("install", "plan", "info", "verify"):
        require(args.packages, "specify at least one package")
        for name in args.packages:
            package_name(name)
    else:
        require(args.command == "search" or not args.packages, "this command takes no package names")
    root = args.root.resolve()
    writing = args.command == "install" and not args.dry_run
    require(not writing or root != Path("/") or os.geteuid() == 0, "installation requires root; use sudo fuse install")
    store = Store(root)
    with store.lock() if writing else nullcontext():
        records = store.records()
        if args.command == "list":
            for name, record in records.items():
                print(f"{name:20} {record['version']:16} {record.get('description', '')}")
            return 0
        if args.command == "verify":
            for name in args.packages:
                require(name in records, f"package is not installed: {name}")
                require(store.intact(records[name]), f"installed payload changed: {name}")
                print(f"{name}: verified")
            return 0
        transport = Transport(args.fetch_helper, token_for(root), args.allow_http)
        repository = Repository(transport, indexes_for(args, root), args.api)
        if args.command == "update":
            print(f"Repository index checked: {len(repository.packages)} package(s)")
        elif args.command == "search":
            query = " ".join(args.packages).lower()
            for name, package in sorted(repository.packages.items()):
                if query in (name + " " + package["description"]).lower():
                    print(f"{name:20} {package['description']}")
        elif args.command == "info":
            for name in args.packages:
                require(name in repository.packages, f"package not found: {name}")
                package = repository.latest(name)
                for key in ("name", "version", "description", "repository", "license", "homepage", "maintainer", "architecture", "abi", "asset", "sha256"):
                    print(f"{key:14} {package[key]}")
                print("dependencies   " + (", ".join(package["dependencies"]) or "none"))
        else:
            install(args, repository, store, records)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (Error, OSError, ValueError, tarfile.TarError) as error:
        sys.exit(f"fuse: {error}")
    except KeyboardInterrupt:
        sys.exit(130)
