#!/usr/bin/env python3
"""Summarizes glslang errors from build/essl_check by original shader file and line."""
import collections, pathlib, re, sys

out = pathlib.Path('build/essl_check')
sites = collections.OrderedDict()
for log in sorted(out.glob('*.log')):
    stem = log.name[:-4]
    files_list = out / (stem + ('.vert' if stem.endswith('.vs') else '.frag') + '.files')
    if not files_list.exists():
        continue
    files = files_list.read_text().splitlines()
    for line in log.read_text().splitlines():
        m = re.match(r'ERROR: (\d+):(\d+): (.*)', line)
        if not m or 'missing #endif' in line or 'compilation terminated' in line:
            continue
        src, ln, msg = int(m.group(1)), int(m.group(2)), m.group(3).strip()
        if src >= len(files):
            continue
        key = (files[src], ln, msg)
        sites.setdefault(key, []).append(stem)

limit = int(sys.argv[1]) if len(sys.argv) > 1 else 200
for (path, ln, msg), shaders in list(sites.items())[:limit]:
    text = pathlib.Path(path).read_text(errors='replace').splitlines()
    src = text[ln - 1].strip() if 0 < ln <= len(text) else '?'
    print(f"{path}:{ln}: {msg}  [{len(shaders)} shaders]\n    {src}")
print(f"{len(sites)} unique error sites")
