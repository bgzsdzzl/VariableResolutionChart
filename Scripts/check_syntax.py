#!/usr/bin/env python3
"""Lightweight C++ syntax checks for the VRC plugin.

Catches:
  - Unbalanced brackets: () [] {}
  - Mixed tabs and spaces in indentation

Skips:
  - Comments and string/char literals
  - Preprocessor directives and UE macros
  - Types, templates, and anything that needs a real compiler

This is intentionally not a real parser. It is a cheap guard that runs
in CI without installing Unreal Engine.
"""

import sys
from pathlib import Path


def strip_comments_and_strings(text):
    """Blank out comments and literals so bracket counting stays accurate."""
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            if j == -1:
                break
            out.append(' ' * (j - i))
            i = j
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            if j == -1:
                break
            chunk = text[i:j + 2]
            out.append(''.join('\n' if ch == '\n' else ' ' for ch in chunk))
            i = j + 2
            continue
        if c in ('"', "'"):
            quote = c
            j = i + 1
            while j < n:
                if text[j] == '\\':
                    j += 2
                    continue
                if text[j] == quote:
                    j += 1
                    break
                if text[j] == '\n':
                    break
                j += 1
            chunk = text[i:j]
            out.append(''.join('\n' if ch == '\n' else ' ' for ch in chunk))
            i = j
            continue
        out.append(c)
        i += 1
    return ''.join(out)


def check_brackets(clean, path):
    errors = []
    pairs = {')': '(', ']': '[', '}': '{'}
    stack = []
    line = 1
    for c in clean:
        if c == '\n':
            line += 1
        elif c in '([{':
            stack.append((c, line))
        elif c in ')]}':
            if not stack:
                errors.append(f"{path}:{line}: unmatched '{c}'")
            else:
                open_c, open_line = stack.pop()
                if open_c != pairs[c]:
                    errors.append(
                        f"{path}:{line}: '{c}' closes '{open_c}' opened at line {open_line}"
                    )
    for open_c, open_line in stack:
        errors.append(f"{path}:{open_line}: unclosed '{open_c}'")
    return errors


def check_indent(text, path):
    errors = []
    for lineno, line in enumerate(text.splitlines(), 1):
        if not line:
            continue
        lead = line[:len(line) - len(line.lstrip(' \t'))]
        if '\t' in lead and ' ' in lead:
            errors.append(f"{path}:{lineno}: mixed tabs and spaces in indentation")
    return errors


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('Source')
    if not root.exists():
        print(f"error: {root} not found", file=sys.stderr)
        return 1

    skip = {'Binaries', 'Intermediate', 'Saved', '.git'}
    files = sorted(
        p for p in root.rglob('*')
        if p.suffix in ('.h', '.cpp', '.inl')
        and not any(part in skip for part in p.parts)
    )
    if not files:
        print("no source files found", file=sys.stderr)
        return 1

    errors = []
    for p in files:
        text = p.read_text(encoding='utf-8', errors='replace')
        clean = strip_comments_and_strings(text)
        errors += check_brackets(clean, str(p))
        errors += check_indent(text, str(p))

    if errors:
        print('\n'.join(errors))
        print(f"\n{len(errors)} issue(s) in {len(files)} file(s)")
        return 1

    print(f"OK: {len(files)} file(s) checked")
    return 0


if __name__ == '__main__':
    sys.exit(main())