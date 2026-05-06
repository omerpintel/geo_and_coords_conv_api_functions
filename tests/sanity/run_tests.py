"""
GeoPoint CI Test Suite Runner
=============================
Cross-platform script that builds, tests, and generates an HTML validation report.

Usage:
    python run_tests.py                    # Auto-detect platform, Debug build
    python run_tests.py --config Release   # Release build
    python run_tests.py --asan             # Enable AddressSanitizer (Linux/macOS only)
    python run_tests.py --build-dir ./build  # Custom build directory

Outputs:
    test_report.html  — Full HTML validation report
    Exit code 0       — All checks passed
    Exit code 1       — One or more checks failed
"""

import subprocess
import sys
import os
import platform
import time
import re
import argparse
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Optional
from datetime import datetime


# ============================================================================
# Data Structures
# ============================================================================

@dataclass
class TestResult:
    name: str
    passed: bool
    detail: str = ""


@dataclass
class TestSuite:
    name: str
    binary: str
    results: List[TestResult] = field(default_factory=list)
    total_passed: int = 0
    total_failed: int = 0
    duration_sec: float = 0.0
    raw_output: str = ""
    coverage_status: Optional[str] = None


@dataclass
class BuildResult:
    success: bool
    warnings: List[str] = field(default_factory=list)
    errors: List[str] = field(default_factory=list)
    duration_sec: float = 0.0
    raw_output: str = ""


@dataclass
class ValidationReport:
    timestamp: str
    platform_info: str
    compiler_info: str
    build_config: str
    build_result: Optional[BuildResult] = None
    test_suites: List[TestSuite] = field(default_factory=list)
    dll_exports: List[str] = field(default_factory=list)
    dll_exports_valid: bool = False
    asan_enabled: bool = False
    overall_pass: bool = False


# ============================================================================
# Utilities
# ============================================================================

def run_command(cmd: List[str], cwd: str = None, timeout: int = 300) -> tuple:
    """Run a command and return (returncode, stdout, stderr)."""
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=cwd,
            timeout=timeout
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "TIMEOUT"
    except FileNotFoundError:
        return -1, "", f"Command not found: {cmd[0]}"


def find_binary(build_dir: str, name: str) -> Optional[str]:
    """Find a built binary in the build directory."""
    if platform.system() == "Windows":
        name += ".exe"

    for root, dirs, files in os.walk(build_dir):
        if name in files:
            return os.path.join(root, name)
    return None


def find_library(build_dir: str) -> Optional[str]:
    """Find the shared library in the build directory."""
    if platform.system() == "Windows":
        ext = ".dll"
    elif platform.system() == "Darwin":
        ext = ".dylib"
    else:
        ext = ".so"

    lib_name = f"api_functions{ext}" if platform.system() == "Windows" else f"libapi_functions{ext}"

    for root, dirs, files in os.walk(build_dir):
        if lib_name in files:
            return os.path.join(root, lib_name)
    return None


# ============================================================================
# Build Phase
# ============================================================================

def configure_and_build(project_root: str, build_dir: str, config: str, asan: bool) -> BuildResult:
    """Configure and build the project with CMake."""
    print(f"\n{'='*60}")
    print(f"  BUILD PHASE ({config})")
    print(f"{'='*60}")

    start = time.time()

    # Configure
    cmake_args = [
        "cmake", "-S", project_root, "-B", build_dir,
        f"-DCMAKE_BUILD_TYPE={config}"
    ]
    if asan and platform.system() != "Windows":
        cmake_args.append("-DENABLE_ASAN=ON")

    print(f"  Configuring: {' '.join(cmake_args)}")
    rc, stdout, stderr = run_command(cmake_args)
    if rc != 0:
        return BuildResult(
            success=False,
            errors=[stderr or stdout],
            duration_sec=time.time() - start,
            raw_output=stdout + "\n" + stderr
        )

    # Build
    build_cmd = ["cmake", "--build", build_dir, "--config", config]
    print(f"  Building:    {' '.join(build_cmd)}")
    rc, stdout, stderr = run_command(build_cmd)

    combined_output = stdout + "\n" + stderr
    duration = time.time() - start

    # Parse warnings/errors
    warnings = []
    errors = []
    for line in combined_output.splitlines():
        line_lower = line.lower()
        if "error" in line_lower and ("lnk" in line_lower or "c1" in line_lower or "fatal" in line_lower):
            errors.append(line.strip())
        elif "warning" in line_lower and ("c4" in line_lower or "-w" in line_lower):
            warnings.append(line.strip())

    success = rc == 0 and len(errors) == 0

    status = "PASS" if success else "FAIL"
    print(f"  Result:      [{status}] ({duration:.1f}s, {len(warnings)} warnings, {len(errors)} errors)")

    return BuildResult(
        success=success,
        warnings=warnings,
        errors=errors,
        duration_sec=duration,
        raw_output=combined_output
    )


# ============================================================================
# Test Phase
# ============================================================================

def run_test_binary(binary_path: str, suite_name: str) -> TestSuite:
    """Run a test binary and parse its output."""
    suite = TestSuite(name=suite_name, binary=binary_path)

    if not binary_path or not os.path.exists(binary_path):
        suite.results.append(TestResult(name="Binary Not Found", passed=False, detail=f"Could not find: {binary_path}"))
        suite.total_failed = 1
        return suite

    print(f"\n  Running: {suite_name}")
    start = time.time()
    rc, stdout, stderr = run_command([binary_path])
    suite.duration_sec = time.time() - start
    suite.raw_output = stdout + "\n" + stderr

    # Parse output
    for line in stdout.splitlines():
        line = line.strip()
        if line.startswith("[PASS]"):
            test_name = line[7:].strip()
            suite.results.append(TestResult(name=test_name, passed=True))
            suite.total_passed += 1
        elif line.startswith("[FAIL]"):
            test_name = line[7:].strip()
            suite.results.append(TestResult(name=test_name, passed=False, detail=line))
            suite.total_failed += 1
        elif "[SUCCESS] 100% Logic Coverage" in line:
            suite.coverage_status = line.strip()
        elif "[WARNING]" in line and "Coverage" in line:
            suite.coverage_status = line.strip()

    status = "PASS" if suite.total_failed == 0 and rc == 0 else "FAIL"
    print(f"  Result:  [{status}] {suite.total_passed} passed, {suite.total_failed} failed ({suite.duration_sec:.1f}s)")

    return suite


# ============================================================================
# DLL Export Validation
# ============================================================================

EXPECTED_EXPORTS = [
    "isInsidePolygon",
    "doesLineIntersectPolygon",
    "GeoToNed",
    "NedToGeo",
]


def validate_dll_exports(lib_path: str) -> tuple:
    """Validate that all expected symbols are exported from the shared library."""
    if not lib_path or not os.path.exists(lib_path):
        return [], False

    exports = []

    if platform.system() == "Windows":
        # Try dumpbin first (requires VS developer prompt)
        rc, stdout, stderr = run_command(["dumpbin", "/EXPORTS", lib_path])
        if rc == 0:
            for line in stdout.splitlines():
                for func in EXPECTED_EXPORTS:
                    if func in line and func not in exports:
                        exports.append(func)
        else:
            # Fallback: use ctypes to verify functions are loadable
            try:
                import ctypes
                dll_dir = os.path.dirname(lib_path)
                os.add_dll_directory(dll_dir)
                lib = ctypes.CDLL(lib_path)
                for func in EXPECTED_EXPORTS:
                    try:
                        getattr(lib, func)
                        exports.append(func)
                    except AttributeError:
                        pass
            except Exception:
                pass
    else:
        rc, stdout, stderr = run_command(["nm", "-D", lib_path])
        if rc == 0:
            for line in stdout.splitlines():
                for func in EXPECTED_EXPORTS:
                    if func in line and " T " in line and func not in exports:
                        exports.append(func)

    valid = all(f in exports for f in EXPECTED_EXPORTS)
    return exports, valid


# ============================================================================
# HTML Report Generation
# ============================================================================

def generate_html_report(report: ValidationReport, output_path: str):
    """Generate a comprehensive HTML validation report."""

    def status_badge(passed: bool) -> str:
        if passed:
            return '<span class="badge pass">PASS</span>'
        return '<span class="badge fail">FAIL</span>'

    def status_class(passed: bool) -> str:
        return "pass" if passed else "fail"

    # Count totals
    total_tests = sum(s.total_passed + s.total_failed for s in report.test_suites)
    total_passed = sum(s.total_passed for s in report.test_suites)
    total_failed = sum(s.total_failed for s in report.test_suites)

    # Build test suite HTML sections
    suite_sections = ""
    for suite in report.test_suites:
        suite_pass = suite.total_failed == 0
        rows = ""
        for r in suite.results:
            icon = "&#10004;" if r.passed else "&#10008;"
            cls = "pass" if r.passed else "fail"
            detail = f'<span class="detail">{r.detail}</span>' if r.detail and not r.passed else ""
            rows += f'<tr class="{cls}"><td class="icon">{icon}</td><td>{r.name}</td><td>{detail}</td></tr>\n'

        coverage_html = ""
        if suite.coverage_status:
            cov_pass = "100%" in suite.coverage_status
            coverage_html = f"""
            <div class="coverage {'pass' if cov_pass else 'fail'}">
                <strong>Coverage:</strong> {suite.coverage_status}
            </div>"""

        suite_sections += f"""
        <div class="suite">
            <h3>{status_badge(suite_pass)} {suite.name}
                <span class="meta">({suite.total_passed + suite.total_failed} tests, {suite.duration_sec:.1f}s)</span>
            </h3>
            {coverage_html}
            <table class="results">
                <thead><tr><th></th><th>Test</th><th>Details</th></tr></thead>
                <tbody>{rows}</tbody>
            </table>
        </div>
        """

    # DLL exports section
    dll_rows = ""
    for func in EXPECTED_EXPORTS:
        found = func in report.dll_exports
        icon = "&#10004;" if found else "&#10008;"
        cls = "pass" if found else "fail"
        dll_rows += f'<tr class="{cls}"><td class="icon">{icon}</td><td>{func}</td></tr>\n'

    # Build warnings/errors
    build_warnings_html = ""
    if report.build_result and report.build_result.warnings:
        items = "".join(f"<li>{w}</li>" for w in report.build_result.warnings)
        build_warnings_html = f'<details><summary>Warnings ({len(report.build_result.warnings)})</summary><ul class="log">{items}</ul></details>'

    build_errors_html = ""
    if report.build_result and report.build_result.errors:
        items = "".join(f"<li>{e}</li>" for e in report.build_result.errors)
        build_errors_html = f'<details><summary>Errors ({len(report.build_result.errors)})</summary><ul class="log error">{items}</ul></details>'

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>GeoPoint Validation Report</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{ font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f7fa; color: #333; padding: 2rem; }}
        .container {{ max-width: 1100px; margin: 0 auto; }}
        h1 {{ font-size: 1.8rem; margin-bottom: 0.5rem; }}
        h2 {{ font-size: 1.3rem; margin: 1.5rem 0 0.75rem; border-bottom: 2px solid #e1e4e8; padding-bottom: 0.4rem; }}
        h3 {{ font-size: 1.05rem; margin-bottom: 0.5rem; }}
        .header {{ background: #fff; border-radius: 8px; padding: 1.5rem 2rem; box-shadow: 0 1px 3px rgba(0,0,0,0.1); margin-bottom: 1.5rem; }}
        .header .overall {{ font-size: 1.4rem; margin-top: 0.5rem; }}
        .meta {{ font-size: 0.85rem; color: #666; font-weight: normal; }}
        .badge {{ display: inline-block; padding: 2px 10px; border-radius: 4px; font-size: 0.8rem; font-weight: 700; text-transform: uppercase; }}
        .badge.pass {{ background: #d4edda; color: #155724; }}
        .badge.fail {{ background: #f8d7da; color: #721c24; }}
        .summary-grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 1rem; margin: 1rem 0; }}
        .summary-card {{ background: #fff; border-radius: 8px; padding: 1rem 1.2rem; box-shadow: 0 1px 3px rgba(0,0,0,0.08); text-align: center; }}
        .summary-card .value {{ font-size: 1.8rem; font-weight: 700; }}
        .summary-card .label {{ font-size: 0.8rem; color: #666; text-transform: uppercase; }}
        .summary-card.pass .value {{ color: #28a745; }}
        .summary-card.fail .value {{ color: #dc3545; }}
        .suite {{ background: #fff; border-radius: 8px; padding: 1.2rem 1.5rem; margin-bottom: 1rem; box-shadow: 0 1px 3px rgba(0,0,0,0.08); }}
        .coverage {{ padding: 0.5rem 0.8rem; border-radius: 4px; margin-bottom: 0.75rem; font-size: 0.9rem; }}
        .coverage.pass {{ background: #d4edda; }}
        .coverage.fail {{ background: #f8d7da; }}
        table.results {{ width: 100%; border-collapse: collapse; font-size: 0.85rem; }}
        table.results th {{ text-align: left; padding: 0.4rem 0.6rem; background: #f8f9fa; border-bottom: 1px solid #e1e4e8; }}
        table.results td {{ padding: 0.3rem 0.6rem; border-bottom: 1px solid #f0f0f0; }}
        table.results tr.fail td {{ background: #fff5f5; }}
        td.icon {{ width: 24px; text-align: center; }}
        tr.pass td.icon {{ color: #28a745; }}
        tr.fail td.icon {{ color: #dc3545; }}
        .detail {{ color: #dc3545; font-size: 0.8rem; }}
        .section {{ background: #fff; border-radius: 8px; padding: 1.2rem 1.5rem; margin-bottom: 1rem; box-shadow: 0 1px 3px rgba(0,0,0,0.08); }}
        details {{ margin-top: 0.5rem; }}
        summary {{ cursor: pointer; font-size: 0.9rem; color: #555; }}
        ul.log {{ list-style: none; font-family: 'Fira Code', monospace; font-size: 0.75rem; margin-top: 0.5rem; max-height: 200px; overflow-y: auto; background: #f8f9fa; padding: 0.5rem; border-radius: 4px; }}
        ul.log li {{ padding: 2px 0; white-space: pre-wrap; word-break: break-all; }}
        ul.log.error li {{ color: #dc3545; }}
        .info-table {{ width: 100%; font-size: 0.85rem; }}
        .info-table td {{ padding: 0.3rem 0.6rem; }}
        .info-table td:first-child {{ font-weight: 600; width: 180px; }}
        .footer {{ text-align: center; font-size: 0.75rem; color: #999; margin-top: 2rem; }}
    </style>
</head>
<body>
<div class="container">
    <div class="header">
        <h1>GeoPoint Validation Report</h1>
        <p class="meta">{report.timestamp} | {report.platform_info} | {report.compiler_info}</p>
        <p class="overall">{status_badge(report.overall_pass)} Overall: {"ALL CHECKS PASSED" if report.overall_pass else "FAILURES DETECTED"}</p>
    </div>

    <div class="summary-grid">
        <div class="summary-card {'pass' if report.build_result and report.build_result.success else 'fail'}">
            <div class="value">{"&#10004;" if report.build_result and report.build_result.success else "&#10008;"}</div>
            <div class="label">Build</div>
        </div>
        <div class="summary-card {'pass' if total_failed == 0 else 'fail'}">
            <div class="value">{total_passed}/{total_tests}</div>
            <div class="label">Tests Passed</div>
        </div>
        <div class="summary-card {'pass' if report.dll_exports_valid else 'fail'}">
            <div class="value">{len(report.dll_exports)}/{len(EXPECTED_EXPORTS)}</div>
            <div class="label">API Exports</div>
        </div>
        <div class="summary-card {'pass' if not report.build_result or len(report.build_result.warnings) == 0 else 'fail'}">
            <div class="value">{len(report.build_result.warnings) if report.build_result else 0}</div>
            <div class="label">Warnings</div>
        </div>
    </div>

    <h2>Build</h2>
    <div class="section">
        <table class="info-table">
            <tr><td>Configuration</td><td>{report.build_config}</td></tr>
            <tr><td>ASan Enabled</td><td>{"Yes" if report.asan_enabled else "No"}</td></tr>
            <tr><td>Duration</td><td>{report.build_result.duration_sec:.1f}s</td></tr>
            <tr><td>Warnings</td><td>{len(report.build_result.warnings) if report.build_result else 0}</td></tr>
            <tr><td>Errors</td><td>{len(report.build_result.errors) if report.build_result else 0}</td></tr>
        </table>
        {build_warnings_html}
        {build_errors_html}
    </div>

    <h2>Test Suites</h2>
    {suite_sections}

    <h2>DLL/Shared Library Exports</h2>
    <div class="section">
        <p>{status_badge(report.dll_exports_valid)} All expected API functions exported</p>
        <table class="results" style="margin-top: 0.75rem;">
            <thead><tr><th></th><th>Function</th></tr></thead>
            <tbody>{dll_rows}</tbody>
        </table>
    </div>

    <h2>Environment</h2>
    <div class="section">
        <table class="info-table">
            <tr><td>Platform</td><td>{report.platform_info}</td></tr>
            <tr><td>Compiler</td><td>{report.compiler_info}</td></tr>
            <tr><td>CMake Build Type</td><td>{report.build_config}</td></tr>
            <tr><td>AddressSanitizer</td><td>{"Enabled" if report.asan_enabled else "Disabled"}</td></tr>
            <tr><td>Report Generated</td><td>{report.timestamp}</td></tr>
        </table>
    </div>

    <p class="footer">Generated by GeoPoint CI Test Suite Runner</p>
</div>
</body>
</html>"""

    with open(output_path, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"\n  Report saved to: {output_path}")


# ============================================================================
# Main Orchestration
# ============================================================================

def detect_compiler_info(build_dir: str) -> str:
    """Try to detect compiler version from CMake cache."""
    cache_path = os.path.join(build_dir, "CMakeCache.txt")
    if os.path.exists(cache_path):
        with open(cache_path, "r") as f:
            for line in f:
                if "CMAKE_CXX_COMPILER_ID" in line:
                    compiler_id = line.split("=")[-1].strip()
                if "CMAKE_CXX_COMPILER_VERSION" in line:
                    compiler_ver = line.split("=")[-1].strip()
                    return f"{compiler_id} {compiler_ver}" if 'compiler_id' in dir() else compiler_ver
    return "Unknown"


def main():
    parser = argparse.ArgumentParser(description="GeoPoint CI Test Suite Runner")
    parser.add_argument("--config", default="Debug", choices=["Debug", "Release", "RelWithDebInfo"],
                        help="CMake build configuration (default: Debug)")
    parser.add_argument("--asan", action="store_true",
                        help="Enable AddressSanitizer (Linux/macOS only)")
    parser.add_argument("--build-dir", default=None,
                        help="Custom build directory (default: auto-detect)")
    parser.add_argument("--output", default="test_report.html",
                        help="Output HTML report path (default: test_report.html)")
    args = parser.parse_args()

    # Resolve paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(os.path.dirname(script_dir))  # Up from tests/sanity/ to project root

    if args.build_dir:
        build_dir = os.path.abspath(args.build_dir)
    else:
        build_dir = os.path.join(project_root, "out", "build", "ci")

    output_path = os.path.join(project_root, args.output)

    print("=" * 60)
    print("  GEOPOINT CI TEST SUITE")
    print("=" * 60)
    print(f"  Project Root: {project_root}")
    print(f"  Build Dir:    {build_dir}")
    print(f"  Config:       {args.config}")
    print(f"  ASan:         {'ON' if args.asan else 'OFF'}")
    print(f"  Output:       {output_path}")

    # Initialize report
    report = ValidationReport(
        timestamp=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        platform_info=f"{platform.system()} {platform.release()} ({platform.machine()})",
        compiler_info="",
        build_config=args.config,
        asan_enabled=args.asan,
    )

    # --- Phase 1: Build ---
    report.build_result = configure_and_build(project_root, build_dir, args.config, args.asan)

    if not report.build_result.success:
        print("\n  BUILD FAILED — cannot continue with tests.")
        report.overall_pass = False
        report.compiler_info = detect_compiler_info(build_dir)
        generate_html_report(report, output_path)
        return 1

    report.compiler_info = detect_compiler_info(build_dir)

    # --- Phase 2: Run Tests ---
    print(f"\n{'='*60}")
    print(f"  TEST PHASE")
    print(f"{'='*60}")

    # Find test binaries
    geo_tests_bin = find_binary(build_dir, "geo_unit_tests")
    coords_tests_bin = find_binary(build_dir, "coords_conv_tests")

    # Run each suite
    geo_suite = run_test_binary(geo_tests_bin, "Geometric Unit Tests (isInsidePolygon + doesLineIntersectPolygon)")
    coords_suite = run_test_binary(coords_tests_bin, "Coordinate Conversion & Robustness Tests")

    report.test_suites = [geo_suite, coords_suite]

    # --- Phase 3: DLL Export Validation ---
    print(f"\n{'='*60}")
    print(f"  DLL EXPORT VALIDATION")
    print(f"{'='*60}")

    lib_path = find_library(build_dir)
    if lib_path:
        print(f"  Library: {lib_path}")
        report.dll_exports, report.dll_exports_valid = validate_dll_exports(lib_path)
        status = "PASS" if report.dll_exports_valid else "FAIL"
        print(f"  Result:  [{status}] {len(report.dll_exports)}/{len(EXPECTED_EXPORTS)} functions exported")
    else:
        print("  Library not found — skipping export validation")
        report.dll_exports_valid = False

    # --- Phase 4: Overall Verdict ---
    all_tests_pass = all(s.total_failed == 0 for s in report.test_suites)
    any_tests_ran = any((s.total_passed + s.total_failed) > 0 for s in report.test_suites)
    build_ok = report.build_result.success
    zero_warnings = len(report.build_result.warnings) == 0

    report.overall_pass = build_ok and all_tests_pass and any_tests_ran and report.dll_exports_valid

    # --- Generate Report ---
    print(f"\n{'='*60}")
    print(f"  GENERATING REPORT")
    print(f"{'='*60}")
    generate_html_report(report, output_path)

    # --- Final Summary ---
    total_passed = sum(s.total_passed for s in report.test_suites)
    total_failed = sum(s.total_failed for s in report.test_suites)

    print(f"\n{'='*60}")
    print(f"  FINAL SUMMARY")
    print(f"{'='*60}")
    print(f"  Build:        {'PASS' if build_ok else 'FAIL'} ({len(report.build_result.warnings)} warnings)")
    print(f"  Tests:        {total_passed} passed, {total_failed} failed")
    print(f"  DLL Exports:  {'PASS' if report.dll_exports_valid else 'FAIL'}")
    print(f"  Overall:      {'PASS' if report.overall_pass else 'FAIL'}")
    print(f"{'='*60}\n")

    return 0 if report.overall_pass else 1


if __name__ == "__main__":
    sys.exit(main())
