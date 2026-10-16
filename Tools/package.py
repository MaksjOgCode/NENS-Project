#!/usr/bin/env python3
"""Native desktop Shipping packaging through Unreal Automation Tool (Python 3)."""
import argparse
import platform
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', required=True, type=Path, help='UE root containing Engine/')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    target = {'Windows': 'Win64', 'Linux': 'Linux', 'Darwin': 'Mac'}[platform.system()]
    root = Path(__file__).resolve().parents[1]
    uat = args.engine.resolve() / 'Engine/Build/BatchFiles' / ('RunUAT.bat' if target == 'Win64' else 'RunUAT.sh')
    if not uat.is_file():
        parser.error(f'RunUAT not found: {uat}')
    if not (root / 'Content/Maps/Startup.umap').is_file():
        parser.error('Startup.umap is missing')
    output = (args.output or root / 'Packaged' / target).resolve()
    command = [str(uat), 'BuildCookRun', f'-project={root / "LudumDare.uproject"}',
               '-noP4', '-utf8output', f'-platform={target}', '-clientconfig=Shipping',
               '-build', '-cook', '-map=/Game/Maps/Startup', '-stage', '-pak', '-iostore',
               '-compressed', '-package', '-archive', f'-archivedirectory={output}']
    print(subprocess.list2cmdline(command), flush=True)
    if not args.dry_run:
        subprocess.run(command, cwd=root, check=True)


if __name__ == '__main__':
    main()
