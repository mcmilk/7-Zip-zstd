#!/usr/bin/env python3
"""Kanzi archive quality matrix for the 7-Zip integration."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
CI_LEVELS = list(range(10))
CI_BLOCKS = [
    "default",
    "1k",
    "2k",
    "4k",
    "8k",
    "16k",
    "32k",
    "64k",
    "128k",
    "256k",
    "512k",
    "1m",
    "2m",
    "4m",
    "8m",
    "16m",
    "32m",
    "64m",
]
CI_CHECKSUMS = ["none", "32", "64"]
CI_THREADS = ["default", "1", "2", "4", "8", "16", "32", "64"]
CI_MAX_JOBS = 4


@dataclass(frozen=True)
class CommandResult:
    exit_code: int
    stdout: str
    stderr: str
    duration_ms: int
    timeout: bool


def timestamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def default_output() -> Path:
    root = os.environ.get("RUNNER_TEMP")
    if root:
        return Path(root) / "kanzi-quality" / timestamp()
    return Path.cwd() / "tmp" / "kanzi-quality" / timestamp()


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", errors="replace")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def run_command(argv: list[str], timeout_seconds: int, cwd: Path | None = None) -> CommandResult:
    start = time.monotonic()
    try:
        proc = subprocess.run(
            argv,
            cwd=str(cwd) if cwd else None,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_seconds,
        )
        return CommandResult(
            proc.returncode,
            proc.stdout,
            proc.stderr,
            int((time.monotonic() - start) * 1000),
            False,
        )
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout if isinstance(exc.stdout, str) else ""
        stderr = exc.stderr if isinstance(exc.stderr, str) else ""
        return CommandResult(124, stdout, stderr, int((time.monotonic() - start) * 1000), True)


def run_command_with_input(
    argv: list[str],
    timeout_seconds: int,
    input_text: str,
    cwd: Path | None = None,
) -> CommandResult:
    start = time.monotonic()
    try:
        proc = subprocess.run(
            argv,
            cwd=str(cwd) if cwd else None,
            input=input_text,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_seconds,
        )
        return CommandResult(
            proc.returncode,
            proc.stdout,
            proc.stderr,
            int((time.monotonic() - start) * 1000),
            False,
        )
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout if isinstance(exc.stdout, str) else ""
        stderr = exc.stderr if isinstance(exc.stderr, str) else ""
        return CommandResult(124, stdout, stderr, int((time.monotonic() - start) * 1000), True)


def block_size_bytes(value: str) -> int:
    text = value.strip().lower()
    multiplier = 1
    if text.endswith("k"):
        multiplier = 1024
        text = text[:-1]
    elif text.endswith("m"):
        multiplier = 1024 * 1024
        text = text[:-1]
    elif text.endswith("g"):
        multiplier = 1024 * 1024 * 1024
        text = text[:-1]

    size = int(text) * multiplier
    if size <= 0 or size & (size - 1):
        raise argparse.ArgumentTypeError("block sizes must be powers of two")
    return size


def block_token(value: str) -> str:
    return f"b{block_size_bytes(value).bit_length() - 1}"


def default_block_token(level: int) -> str:
    if level == 6:
        return "b23"
    if level in (7, 8):
        return "b24"
    if level == 9:
        return "b25"
    return "b22"


def parse_csv(value: str, default: list[str]) -> list[str]:
    if not value:
        return default
    return [item.strip() for item in value.split(",") if item.strip()]


def parse_levels(value: str, default: list[int]) -> list[int]:
    if not value:
        return default
    if value.lower() == "all":
        return list(range(10))
    levels = [int(item.strip()) for item in value.split(",") if item.strip()]
    for level in levels:
        if level < 0 or level > 9:
            raise argparse.ArgumentTypeError("levels must be in 0..9")
    return levels


def create_payloads(root: Path) -> dict[str, Path]:
    root.mkdir(parents=True, exist_ok=True)

    empty = root / "empty.bin"
    empty.write_bytes(b"")

    tiny = root / "tiny.txt"
    tiny.write_text("kanzi\n", encoding="utf-8")

    text = root / "text.txt"
    text.write_text(("kanzi text payload\n" * 2048), encoding="utf-8")

    binary = root / "matrix.bin"
    binary.write_bytes(bytes(((i * 131 + 17) & 0xFF) for i in range(128 * 1024)))

    tree = root / "tree"
    (tree / "sub").mkdir(parents=True, exist_ok=True)
    (tree / "root.txt").write_text("root\n" * 128, encoding="utf-8")
    (tree / "sub" / "nested.bin").write_bytes(bytes(range(256)) * 32)

    return {
        "empty": empty,
        "tiny": tiny,
        "text": text,
        "binary": binary,
        "tree": tree,
    }


def relative_hashes(path: Path) -> dict[str, str]:
    if path.is_file():
        return {path.name: sha256(path)}
    return {
        item.relative_to(path).as_posix(): sha256(item)
        for item in sorted(path.rglob("*"))
        if item.is_file()
    }


def extracted_root(output_dir: Path, source: Path) -> Path:
    candidate = output_dir / source.name
    return candidate if candidate.exists() else output_dir


def parse_method(stdout: str) -> str:
    method = ""
    for line in stdout.splitlines():
        if line.startswith("Method = "):
            value = line.split("=", 1)[1].strip()
            if "KANZI" in value:
                method = value
    return method


def method_parts(level: int, block: str, checksum: str) -> tuple[str, list[str]]:
    parts = ["kanzi"]
    tokens: list[str] = [default_block_token(level)]
    if block != "default":
        parts.append(f"b{block}")
        tokens[0] = block_token(block)
    if checksum != "none":
        parts.append(f"check{checksum}")
        tokens.append(f"c{checksum}")
    return ":".join(parts), tokens


def build_cases(
    payloads: dict[str, Path],
    levels: list[int],
    blocks: list[str],
    checksums: list[str],
    threads: list[str],
) -> list[dict[str, Any]]:
    cases: list[dict[str, Any]] = [
        {"id": "scenario-empty", "source": payloads["empty"], "args": ["-m0=kanzi", "-mx3"], "tokens": [], "method_optional": True},
        {"id": "scenario-tiny", "source": payloads["tiny"], "args": ["-m0=kanzi", "-mx3"], "tokens": ["l3", "b22"]},
        {"id": "scenario-text", "source": payloads["text"], "args": ["-m0=kanzi", "-mx3"], "tokens": ["l3", "b22"]},
        {"id": "scenario-binary", "source": payloads["binary"], "args": ["-m0=kanzi", "-mx3"], "tokens": ["l3", "b22"]},
        {"id": "scenario-tree", "source": payloads["tree"], "args": ["-m0=kanzi", "-mx3"], "tokens": ["l3", "b22"]},
        {
            "id": "scenario-stdin",
            "kind": "stdin",
            "source": payloads["text"],
            "args": ["-m0=kanzi", "-mx3"],
            "stdin_name": "stdin.txt",
            "tokens": ["l3", "b22"],
        },
        {
            "id": "scenario-update-delete",
            "kind": "update_delete",
            "source": payloads["text"],
            "args": ["-m0=kanzi:b1m:check32", "-mx5"],
            "tokens": ["l5", "b20", "c32"],
        },
    ]

    for level in levels:
        for block in blocks:
            for checksum in checksums:
                method, tokens = method_parts(level, block, checksum)
                for thread in threads:
                    args = [f"-m0={method}", f"-mx{level}"]
                    decode_args: list[str] = []
                    suffix = [f"mx{level}", f"b{block}", f"c{checksum}"]
                    if thread != "default":
                        args.append(f"-mmt={thread}")
                        decode_args.append(f"-mmt={thread}")
                        suffix.append(f"t{thread}")
                    else:
                        suffix.append("tdefault")
                    cases.append(
                        {
                            "id": "param-" + "-".join(suffix),
                            "source": payloads["binary"],
                            "args": args,
                            "decode_args": decode_args,
                            "tokens": [f"l{level}", *tokens],
                        }
                    )

    return cases


def finalize_case(
    case: dict[str, Any],
    source: Path,
    before: dict[str, str],
    after: dict[str, str],
    method: str,
    command_results: dict[str, CommandResult],
    command_argvs: dict[str, list[str]],
    output: Path,
    case_dir: Path,
    logs_dir: Path,
    log_passing_cases: bool,
) -> dict[str, Any]:
    case_id = str(case["id"])
    expected_tokens = ["KANZI", *case["tokens"]]
    method_match = all(token.lower() in method.lower() for token in expected_tokens)
    if case.get("method_optional") and not method:
        method_match = True
    hash_match = before == after

    status = "PASS"
    reason = ""
    for name, result in command_results.items():
        if result.timeout:
            status = "TIMEOUT"
            reason = f"{name} timed out"
            break
        if result.exit_code != 0:
            status = "APP_FAIL"
            reason = f"{name} exit code {result.exit_code}"
            break
    if status == "PASS" and not method_match:
        status = "METHOD_FAIL"
        reason = f"method {method!r} missing {expected_tokens}"
    if status == "PASS" and not hash_match:
        status = "HASH_FAIL"
        reason = "extracted hashes differ"

    result = {
        "schema_version": SCHEMA_VERSION,
        "case_id": case_id,
        "kind": str(case.get("kind", "roundtrip")),
        "status": status,
        "failure_reason": reason,
        "source": str(source),
        "args": case["args"],
        "decode_args": list(case.get("decode_args", [])),
        "method": method,
        "expected_method_tokens": expected_tokens,
        "hash_match": hash_match,
        "duration_ms": sum(item.duration_ms for item in command_results.values()),
        "exit_codes": {name: item.exit_code for name, item in command_results.items()},
        "commands": command_argvs,
    }
    if status != "PASS" or log_passing_cases:
        for name, command in command_results.items():
            write_text(logs_dir / f"{case_id}.{name}.stdout.txt", command.stdout)
            write_text(logs_dir / f"{case_id}.{name}.stderr.txt", command.stderr)
        write_json(output / "results" / f"{case_id}.json", result)
    else:
        shutil.rmtree(case_dir)
    return result


def run_case(
    seven_zip: Path,
    case: dict[str, Any],
    output: Path,
    timeout_seconds: int,
    log_passing_cases: bool,
) -> dict[str, Any]:
    case_id = str(case["id"])
    kind = str(case.get("kind", "roundtrip"))
    source = Path(case["source"]).resolve()
    case_dir = output / "work" / case_id
    logs_dir = output / "logs"
    archive = case_dir / f"{case_id}.7z"
    out_dir = case_dir / "out"
    case_dir.mkdir(parents=True, exist_ok=True)
    decode_args = list(case.get("decode_args", []))

    if kind == "stdin":
        stdin_name = str(case["stdin_name"])
        add_cmd = [str(seven_zip), "a", "-t7z", *case["args"], f"-si{stdin_name}", "--", str(archive)]
        list_cmd = [str(seven_zip), "l", "-slt", "--", str(archive)]
        test_cmd = [str(seven_zip), "t", *decode_args, "--", str(archive)]
        extract_cmd = [str(seven_zip), "x", "-y", f"-o{out_dir}", *decode_args, "--", str(archive)]
        command_results = {
            "add": run_command_with_input(add_cmd, timeout_seconds, source.read_text(encoding="utf-8")),
            "list": run_command(list_cmd, timeout_seconds),
            "test": run_command(test_cmd, timeout_seconds),
            "extract": run_command(extract_cmd, timeout_seconds),
        }
        before = {stdin_name: sha256(source)}
        extracted = out_dir / stdin_name
        after = {stdin_name: sha256(extracted)} if extracted.exists() else {}
        return finalize_case(
            case,
            source,
            before,
            after,
            parse_method(command_results["list"].stdout),
            command_results,
            {"add": add_cmd, "list": list_cmd, "test": test_cmd, "extract": extract_cmd},
            output,
            case_dir,
            logs_dir,
            log_passing_cases,
        )

    if kind == "update_delete":
        src_dir = case_dir / "src"
        src_dir.mkdir(parents=True, exist_ok=True)
        first = src_dir / "first.txt"
        second = src_dir / "second.txt"
        first.write_text("first file removed after update\n" * 128, encoding="utf-8")
        second.write_text("second file kept after update\n" * 128, encoding="utf-8")
        add_cmd = [str(seven_zip), "a", "-t7z", *case["args"], "--", str(archive), first.name]
        update_cmd = [str(seven_zip), "u", "-t7z", *case["args"], "--", str(archive), second.name]
        delete_cmd = [str(seven_zip), "d", "--", str(archive), first.name]
        list_cmd = [str(seven_zip), "l", "-slt", "--", str(archive)]
        test_cmd = [str(seven_zip), "t", *decode_args, "--", str(archive)]
        extract_cmd = [str(seven_zip), "x", "-y", f"-o{out_dir}", *decode_args, "--", str(archive)]
        command_results = {
            "add": run_command(add_cmd, timeout_seconds, cwd=src_dir),
            "update": run_command(update_cmd, timeout_seconds, cwd=src_dir),
            "delete": run_command(delete_cmd, timeout_seconds, cwd=src_dir),
            "list": run_command(list_cmd, timeout_seconds),
            "test": run_command(test_cmd, timeout_seconds),
            "extract": run_command(extract_cmd, timeout_seconds),
        }
        before = {second.name: sha256(second)}
        extracted = out_dir / second.name
        after = {second.name: sha256(extracted)} if extracted.exists() else {}
        return finalize_case(
            case,
            src_dir,
            before,
            after,
            parse_method(command_results["list"].stdout),
            command_results,
            {
                "add": add_cmd,
                "update": update_cmd,
                "delete": delete_cmd,
                "list": list_cmd,
                "test": test_cmd,
                "extract": extract_cmd,
            },
            output,
            case_dir,
            logs_dir,
            log_passing_cases,
        )

    add_cmd = [str(seven_zip), "a", "-t7z", *case["args"], "--", str(archive), source.name]
    list_cmd = [str(seven_zip), "l", "-slt", "--", str(archive)]
    test_cmd = [str(seven_zip), "t", *decode_args, "--", str(archive)]
    extract_cmd = [str(seven_zip), "x", "-y", f"-o{out_dir}", *decode_args, "--", str(archive)]
    command_results = {
        "add": run_command(add_cmd, timeout_seconds, cwd=source.parent),
        "list": run_command(list_cmd, timeout_seconds),
        "test": run_command(test_cmd, timeout_seconds),
        "extract": run_command(extract_cmd, timeout_seconds),
    }
    root = extracted_root(out_dir, source)
    return finalize_case(
        case,
        source,
        relative_hashes(source),
        relative_hashes(root) if root.exists() else {},
        parse_method(command_results["list"].stdout),
        command_results,
        {"add": add_cmd, "list": list_cmd, "test": test_cmd, "extract": extract_cmd},
        output,
        case_dir,
        logs_dir,
        log_passing_cases,
    )


def summarize(output: Path, results: list[dict[str, Any]], metadata: dict[str, Any]) -> dict[str, Any]:
    failures = [
        {
            "case_id": item["case_id"],
            "status": item["status"],
            "failure_reason": item["failure_reason"],
        }
        for item in results
        if item["status"] != "PASS"
    ]
    counts: dict[str, int] = {}
    kinds: dict[str, int] = {}
    for item in results:
        counts[item["status"]] = counts.get(item["status"], 0) + 1
        kind = str(item.get("kind", "roundtrip"))
        kinds[kind] = kinds.get(kind, 0) + 1

    summary = {
        "schema_version": SCHEMA_VERSION,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "status": "FAIL" if failures else "PASS",
        "total": len(results),
        "status_counts": counts,
        "coverage": {
            "case_kinds": kinds,
            "case_ids": [item["case_id"] for item in results],
        },
        "metadata": metadata,
        "failures": failures,
    }
    write_json(output / "summary.json", summary)
    write_text(output / "summary.txt", json.dumps(summary, indent=2))
    return summary


def append_github_summary(summary: dict[str, Any]) -> None:
    summary_file = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_file:
        return
    counts = summary.get("status_counts", {})
    total = int(summary.get("total", 0))
    passed = int(counts.get("PASS", 0))
    failures = summary.get("failures", [])
    lines = [
        "## Kanzi codec quality",
        "",
        f"- Status: {summary.get('status', 'UNKNOWN')}",
        f"- Cases: {passed}/{total} passed",
    ]
    if failures:
        lines.extend(["", "### Failures"])
        for failure in failures[:20]:
            lines.append(f"- {failure['case_id']}: {failure['failure_reason']}")
        remaining = len(failures) - 20
        if remaining > 0:
            lines.append(f"- {remaining} more failure(s) in the job log")
    try:
        with Path(summary_file).open("a", encoding="utf-8") as handle:
            handle.write("\n".join(lines) + "\n")
    except OSError as exc:
        print(f"Could not write GitHub step summary: {exc}", file=sys.stderr)


def trim_success_output(output: Path, log_passing_cases: bool) -> None:
    if log_passing_cases:
        return

    for name in ("payloads", "work", "results"):
        path = output / name
        if path.exists():
            shutil.rmtree(path)


def resolve_seven_zip(value: Path) -> Path:
    text = str(value)
    if os.sep not in text and (os.altsep is None or os.altsep not in text):
        found = shutil.which(text)
        if found:
            return Path(found).resolve()
    return value.resolve()


def default_jobs() -> int:
    if os.environ.get("GITHUB_ACTIONS") == "true":
        return max(1, min(CI_MAX_JOBS, os.cpu_count() or 1))
    return 1


def parse_jobs(value: str) -> int:
    if value.lower() == "auto":
        return default_jobs()
    jobs = int(value)
    if jobs < 1:
        raise argparse.ArgumentTypeError("jobs must be >= 1")
    return jobs


def run_cases(
    seven_zip: Path,
    cases: list[dict[str, Any]],
    output: Path,
    timeout_seconds: int,
    log_passing_cases: bool,
    jobs: int,
) -> list[dict[str, Any]]:
    if jobs == 1:
        return [
            run_case(seven_zip, case, output, timeout_seconds, log_passing_cases)
            for case in cases
        ]

    with ThreadPoolExecutor(max_workers=jobs) as executor:
        return list(
            executor.map(
                lambda case: run_case(seven_zip, case, output, timeout_seconds, log_passing_cases),
                cases,
            )
        )


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Run Kanzi 7z quality coverage.")
    parser.add_argument("--sevenzip", type=Path, default=os.environ.get("Z7_PATH", "7z"))
    parser.add_argument("--output", type=Path, default=default_output())
    parser.add_argument("--levels", default="")
    parser.add_argument("--blocks", default="")
    parser.add_argument("--checksums", default="")
    parser.add_argument("--threads", default="")
    parser.add_argument("--jobs", default="auto")
    parser.add_argument("--timeout-seconds", type=int, default=120)
    parser.add_argument("--log-passing-cases", action="store_true")
    args = parser.parse_args(argv)

    seven_zip = resolve_seven_zip(args.sevenzip)
    if not seven_zip.is_file():
        print(f"7z executable not found: {seven_zip}", file=sys.stderr)
        return 2

    output = args.output.resolve()
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    info = run_command([str(seven_zip), "i"], args.timeout_seconds)
    write_text(output / "logs" / "7z-i.stdout.txt", info.stdout)
    write_text(output / "logs" / "7z-i.stderr.txt", info.stderr)
    if info.exit_code != 0 or "KANZI" not in info.stdout or "4F71107" not in info.stdout:
        summary = {
            "schema_version": SCHEMA_VERSION,
            "status": "FAIL",
            "total": 0,
            "status_counts": {},
            "metadata": {"seven_zip": str(seven_zip)},
            "failures": [{"case_id": "method-visible", "status": "METHOD_FAIL", "failure_reason": "KANZI is not listed"}],
        }
        write_json(output / "summary.json", summary)
        print(json.dumps(summary, indent=2))
        return 2

    levels = parse_levels(args.levels, CI_LEVELS)
    blocks = parse_csv(args.blocks, CI_BLOCKS)
    checksums = parse_csv(args.checksums, CI_CHECKSUMS)
    threads = parse_csv(args.threads, CI_THREADS)
    jobs = parse_jobs(args.jobs)
    payloads = create_payloads(output / "payloads")
    all_cases = build_cases(payloads, levels, blocks, checksums, threads)
    results = run_cases(seven_zip, all_cases, output, args.timeout_seconds, args.log_passing_cases, jobs)
    summary = summarize(
        output,
        results,
        {
            "seven_zip": str(seven_zip),
            "seven_zip_sha256": sha256(seven_zip),
            "levels": levels,
            "blocks": blocks,
            "checksums": checksums,
            "threads": threads,
            "default_block_tokens": {str(level): default_block_token(level) for level in levels},
            "all_cases": len(all_cases),
            "jobs": jobs,
            "output": str(output),
        },
    )
    if summary["status"] == "PASS":
        trim_success_output(output, args.log_passing_cases)
    append_github_summary(summary)
    print(json.dumps(summary, indent=2))
    return 0 if summary["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
