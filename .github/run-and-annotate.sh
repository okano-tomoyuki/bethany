#!/usr/bin/env bash
# CI の手順を実行し、失敗したら出力の末尾を GitHub の注釈(::error::)にする(docs/adr/0056)。
# 注釈は実行の一覧・API から読めるため、ログを開かずに失敗の理由が分かる。
#   .github/run-and-annotate.sh <説明> <コマンド> [引数...]
set -u
title="$1"
shift
log="$(mktemp)"
"$@" 2>&1 | tee "$log"
status=${PIPESTATUS[0]}
if [ "$status" -ne 0 ]; then
  # 注釈の本文の改行は %0A で表す。
  body="$(tail -n 40 "$log" | sed -e 's/%/%25/g' -e 's/\r//g' | awk '{ printf "%s%%0A", $0 }')"
  echo "::error title=$title::$body"
fi
rm -f "$log"
exit "$status"
