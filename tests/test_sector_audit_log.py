import importlib.util
from pathlib import Path
import unittest
import sys

sys.dont_write_bytecode = True

spec = importlib.util.spec_from_file_location('audit', Path(__file__).resolve().parents[1] / 'android/analyze-sector-audit.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class AuditLogTest(unittest.TestCase):
    def test_unfinished_scan_is_not_evidence_of_agreement(self):
        report = audit.analyze('[sector-audit] begin frame=1\n')
        self.assertEqual(report['completed'], [])
        self.assertEqual(report['incomplete'], [1])

    def test_all_four_queries_and_end_required(self):
        log = '[sector-audit] begin frame=1\n'
        for query in ('down/static', 'up/static', 'down/portals', 'up/portals'):
            log += f'[sector-audit] frame=1 query={query} tree=5 reference=5 match=1\n'
        self.assertEqual(audit.analyze(log)['completed'], [])
        self.assertEqual(audit.analyze(log + '[sector-audit] end frame=1\n')['completed'], [1])

    def test_mismatch_retained(self):
        log = '[sector-audit] begin frame=7\n[sector-audit] frame=7 query=down/static tree=-1 reference=2 match=0\n'
        self.assertEqual(audit.analyze(log)['mismatches'], [{'frame': 7, 'query': 'down/static'}])


if __name__ == '__main__':
    unittest.main()
