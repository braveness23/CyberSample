#!/usr/bin/env bash
# Guardrail for this public repo: fail if phone numbers or e-mail addresses creep into tracked text files.
cd "$(dirname "$0")/.."
bad=$(git ls-files -z | xargs -0 grep -I -n -E '[0-9]{3}[-. ][0-9]{3}-[0-9]{4}|[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[a-z]{2,}' -- 2>/dev/null | grep -v -E '^(ci/scrub-check.sh|LICENSE)' || true)
if [ -n "$bad" ]; then echo "possible private data:"; echo "$bad"; exit 1; fi
echo "scrub-check: clean"
