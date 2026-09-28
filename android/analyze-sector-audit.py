#!/usr/bin/env python3
"""Summarize completed sector audits; reject absent or incomplete evidence."""
import argparse
import json
import re
from pathlib import Path


def analyze(text):
    starts = set(map(int, re.findall(r'\[sector-audit\] begin frame=(\d+)', text)))
    ends = set(map(int, re.findall(r'\[sector-audit\] end frame=(\d+)', text)))
    queries = {}
    mismatches = []
    for frame, direction, model, match in re.findall(
            r'\[sector-audit\] frame=(\d+) query=(down|up)/(static|portals).*?match=(\d+)', text):
        frame = int(frame)
        queries.setdefault(frame, set()).add(f'{direction}/{model}')
        if match == '0':
            mismatches.append({'frame': frame, 'query': f'{direction}/{model}'})
    complete = {frame for frame in starts & ends if len(queries.get(frame, ())) == 4}
    return {'started': sorted(starts), 'completed': sorted(complete),
            'incomplete': sorted(starts - complete), 'mismatches': mismatches}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    report = analyze(args.log.read_text(encoding='utf-8', errors='replace'))
    print(json.dumps(report, indent=2))
    if not report['completed'] or report['incomplete']:
        return 2
    return 1 if report['mismatches'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
