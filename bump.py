#!/usr/bin/env python3
import sys
import re
import json
import os

SEMVER_REGEX = r"^\d+\.\d+\.\d+(-[0-9A-Za-z.-]+)?(\+[0-9A-Za-z.-]+)?$"

def get_current_version(root_dir):
    manifest_path = os.path.join(root_dir, ".release-please-manifest.json")
    if os.path.isfile(manifest_path):
        with open(manifest_path, "r", encoding="utf-8") as f:
            data = json.load(f)
            return data.get(".", "unknown")
    return "unknown"

def update_library_properties(file_path, new_version):
    if not os.path.isfile(file_path):
        return False
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    updated = re.sub(r"^version=.*$", f"version={new_version}", content, flags=re.MULTILINE)
    with open(file_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(updated)
    return True

def update_library_json(file_path, new_version):
    if not os.path.isfile(file_path):
        return False
    with open(file_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["version"] = new_version
    with open(file_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2)
        f.write("\n")
    return True

def update_release_manifest(file_path, new_version):
    if not os.path.isfile(file_path):
        return False
    with open(file_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    data["."] = new_version
    with open(file_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2)
        f.write("\n")
    return True

def update_mcp_server_header(file_path, new_version):
    if not os.path.isfile(file_path):
        return False
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    updated = re.sub(
        r'(MCPServer\(const String& name = "[^"]*", const String& version = ")[^"]*("\);)',
        rf'\g<1>{new_version}\g<2>',
        content
    )
    with open(file_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(updated)
    return True

def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print("Usage: python bump.py <new_version>")
        print("Example: python bump.py 0.2.0")
        sys.exit(1 if len(sys.argv) < 2 else 0)

    new_version = sys.argv[1].strip()
    if not re.match(SEMVER_REGEX, new_version):
        print(f"Error: '{new_version}' is not a valid semantic version (e.g. 1.0.0, 0.2.1).")
        sys.exit(1)

    root_dir = os.path.abspath(os.path.dirname(__file__))
    current_version = get_current_version(root_dir)

    print(f"Bumping version: {current_version} -> {new_version}")

    files_to_update = [
        ("library.properties", os.path.join(root_dir, "library.properties"), update_library_properties),
        ("library.json", os.path.join(root_dir, "library.json"), update_library_json),
        (".release-please-manifest.json", os.path.join(root_dir, ".release-please-manifest.json"), update_release_manifest),
        ("src/mcp/MCPServer.h", os.path.join(root_dir, "src", "mcp", "MCPServer.h"), update_mcp_server_header),
    ]

    for name, path, updater in files_to_update:
        if updater(path, new_version):
            print(f"  [UPDATED] {name}")
        else:
            print(f"  [SKIPPED] {name} (file not found)")

    print(f"\nSuccessfully bumped project to version {new_version}!")

if __name__ == "__main__":
    main()
