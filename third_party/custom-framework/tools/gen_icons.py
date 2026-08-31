#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from pathlib import Path

CSS_RE = re.compile(r"\.fa-([a-zA-Z0-9-]+)")
PROP_RE = re.compile(r"\{--fa:('(?:\\.|[^'])*'|\"(?:\\.|[^\"])*\")\}")


def css_unescape(raw: str) -> int:
    if len(raw) >= 2 and raw[0] in "'\"" and raw[-1] == raw[0]:
        raw = raw[1:-1]
    if not raw:
        return 0
    if raw[0] != "\\":
        return ord(raw[0])
    body = raw[1:]
    hex_digits = ""
    for ch in body:
        if ch in "0123456789abcdefABCDEF":
            hex_digits += ch
            if len(hex_digits) == 6:
                break
        else:
            break
    if hex_digits:
        return int(hex_digits, 16)
    if body:
        return ord(body[0])
    return 0


def to_ident(name: str) -> str:
    parts = [p for p in name.replace("-", "_").split("_") if p]
    ident = "".join(p[:1].upper() + p[1:] for p in parts)
    if not ident:
        ident = "Unknown"
    if ident[0].isdigit():
        ident = "_" + ident
    reserved = {
        "And", "Or", "Not", "Xor", "Default", "True", "False", "Null",
        "Class", "Template", "Export", "Import", "New", "Delete", "This",
        "Private", "Public", "Protected", "Virtual", "Inline", "Operator",
        "Register", "Switch", "Case", "Return", "Break", "Continue",
        "Namespace", "Using", "Try", "Catch", "Throw", "Const", "Static",
    }
    if ident in reserved:
        ident = ident + "_"
    return ident


def parse(css: str) -> list[tuple[str, str, int]]:
    entries: list[tuple[str, str, int]] = []
    seen_ident: set[str] = set()
    for block in css.split("}"):
        if "--fa:" not in block:
            continue
        names = CSS_RE.findall(block)
        match = PROP_RE.search(block + "}")
        if not names or not match:
            continue
        code = css_unescape(match.group(1))
        if code <= 0:
            continue
        for name in names:
            ident = to_ident(name)
            if ident in seen_ident:
                continue
            seen_ident.add(ident)
            entries.append((ident, name, code))
    entries.sort(key=lambda item: item[0])
    return entries


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    css_path = root / "assets" / "icons" / "fontawesome" / "fontawesome.css"
    header = root / "include" / "ur" / "icons.hpp"
    if not css_path.exists():
        print("missing", css_path, file=sys.stderr)
        return 1
    entries = parse(css_path.read_text(encoding="utf-8", errors="ignore"))
    header.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <cstddef>",
        "#include <cstdint>",
        "",
        "namespace ur {",
        "namespace icons {",
        "",
        "enum class Icon : std::uint32_t {",
        "    None = 0,",
    ]
    for ident, _name, code in entries:
        lines.append(f"    {ident} = {code},")
    lines += [
        "};",
        "",
        "struct Glyph {",
        "    const char* name;",
        "    Icon icon;",
        "};",
        "",
        "inline constexpr Glyph kCatalog[] = {",
    ]
    for ident, name, _code in entries:
        lines.append(f'    {{ "{name}", Icon::{ident} }},')
    lines += [
        "};",
        "",
        "inline constexpr std::size_t kCatalogCount = sizeof(kCatalog) / sizeof(kCatalog[0]);",
        "",
        "inline constexpr const char* name(Icon icon) {",
        "    for (std::size_t i = 0; i < kCatalogCount; ++i) {",
        "        if (kCatalog[i].icon == icon)",
        "            return kCatalog[i].name;",
        "    }",
        "    return \"\";",
        "}",
        "",
        "}",
        "}",
        "",
    ]
    header.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {header} ({len(entries)} icons)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
