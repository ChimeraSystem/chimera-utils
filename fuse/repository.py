import os
from pathlib import Path
import re
import subprocess
import tempfile
from urllib.parse import urlparse

from common import (ABI, ARCH, Error, MAX_ARCHIVE, SHA256, dependencies, package_name,
                    read_json, require)

DEFAULT_INDEX = "https://raw.githubusercontent.com/ChimeraSystem/chimera-utils/main/packages/index.json"


class Transport:
    def __init__(self, helper="/usr/libexec/fuse-fetch", token=None, allow_http=False):
        self.helper, self.token, self.allow_http = helper, token, allow_http

    def fetch(self, url, destination, limit, accept="application/json"):
        parsed = urlparse(url)
        require(parsed.scheme == "https" or self.allow_http and parsed.scheme == "http",
                "repositories and downloads require HTTPS")
        require(parsed.hostname and not parsed.username and not parsed.password and not parsed.fragment,
                "invalid download URL")
        env = {**os.environ}
        env.pop("FUSE_GITHUB_TOKEN", None)
        if self.token:
            env["FUSE_GITHUB_TOKEN"] = self.token
        args = [str(self.helper), url, str(destination), str(limit), accept]
        if self.allow_http:
            args.append("--allow-http")
        result = subprocess.run(args, env=env, check=False)
        require(result.returncode == 0, f"download failed: {parsed.hostname}{parsed.path}")
        require(Path(destination).stat().st_size <= limit, "download exceeds size limit")

    def json(self, url, accept="application/json"):
        with tempfile.TemporaryDirectory(prefix="fuse-metadata-") as directory:
            path = Path(directory) / "metadata.json"
            self.fetch(url, path, 4 * 1024 * 1024, accept)
            return read_json(path)


class Repository:
    def __init__(self, transport, indexes, api="https://api.github.com"):
        self.transport, self.api = transport, api.rstrip("/")
        self.packages, self.releases = {}, {}
        for index in indexes:
            data = transport.json(index)
            require(isinstance(data, dict) and data.get("schema") == 1 and isinstance(data.get("packages"), list),
                    "unsupported package index")
            for package in data["packages"]:
                require(isinstance(package, dict), "invalid package entry")
                name = package_name(package.get("name"))
                require(name not in self.packages, f"package appears in multiple indexes: {name}")
                require(package.get("architecture") == ARCH and package.get("abi") == ABI,
                        f"incompatible architecture or ABI: {name}")
                require(isinstance(package.get("repository"), str) and
                        re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]*/[A-Za-z0-9][A-Za-z0-9_.-]*", package["repository"]),
                        f"invalid GitHub repository: {name}")
                require(isinstance(package.get("asset"), str) and
                        re.fullmatch(r"[a-zA-Z0-9+_.-]+\.tar\.gz", package["asset"]), f"invalid release asset: {name}")
                dependencies(package.get("dependencies"))
                for key in ("description", "license", "homepage", "maintainer"):
                    require(isinstance(package.get(key), str) and 0 < len(package[key]) <= 1024,
                            f"missing {key}: {name}")
                self.packages[name] = package
        require(len(self.packages) <= 10000, "too many packages")

    def order(self, requested):
        ordered, state, chain = [], {}, []

        def visit(name):
            require(name in self.packages, f"package not found: {name}")
            require(state.get(name) != 1, "dependency cycle: " + " -> ".join([*chain, name]))
            if state.get(name) == 2:
                return
            require(len(chain) < 128, "dependency graph too deep")
            state[name] = 1
            chain.append(name)
            for dep in self.packages[name]["dependencies"]:
                visit(dep)
            chain.pop()
            state[name] = 2
            ordered.append(name)

        for name in requested:
            visit(package_name(name))
        return ordered

    def latest(self, name):
        package = self.packages[name]
        repo = package["repository"]
        if repo not in self.releases:
            self.releases[repo] = self.transport.json(f"{self.api}/repos/{repo}/releases/latest")
        release = self.releases[repo]
        require(isinstance(release, dict) and release.get("draft") is False and release.get("prerelease") is False,
                f"no stable release for {name}")
        tag = release.get("tag_name")
        require(isinstance(tag, str) and re.fullmatch(r"v?[A-Za-z0-9][A-Za-z0-9+_.-]{0,62}", tag),
                f"invalid release version: {name}")
        assets = release.get("assets")
        require(isinstance(assets, list), f"release has no assets: {name}")
        matches = [asset for asset in assets if isinstance(asset, dict) and asset.get("name") == package["asset"]]
        require(len(matches) == 1, f"release {tag} has no unique {package['asset']}")
        asset = matches[0]
        checksum = asset.get("digest", "")
        require(isinstance(checksum, str) and checksum.startswith("sha256:") and SHA256.fullmatch(checksum[7:]),
                f"release asset has no SHA-256 digest: {name}")
        require(type(asset.get("size")) is int and 0 < asset["size"] <= MAX_ARCHIVE,
                f"invalid release asset size: {name}")
        require(asset.get("state") == "uploaded", f"release asset is not ready: {name}")
        url = asset.get("url")
        require(isinstance(url, str) and url.startswith(f"{self.api}/repos/{repo}/releases/assets/") and
                url[len(f"{self.api}/repos/{repo}/releases/assets/"):].isdigit(), f"invalid release asset URL: {name}")
        return {**package, "version": tag.removeprefix("v"), "tag": tag, "url": url,
                "sha256": checksum[7:], "download_size": asset["size"]}
