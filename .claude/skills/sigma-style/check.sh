#!/bin/sh
# Sigma 코드 서식 규약 — 1차 기계검사 헬퍼.
#
# 사용법:  check.sh <file|dir>
#   - <dir> 은 하위까지 재귀(-r) 검사한다.
#   - build/sak 이 없으면 cmake 로 구성·빌드한 뒤 실행한다(이후엔 재사용).
#
# 종료코드:  0 위반 없음 / 1 위반 있음 / 2 사용법·경로 오류.
#   (위반이 있어 1 로 끝나는 것은 정상 동작이다.)

set -e

if [ "$#" -ne 1 ]; then
	echo "usage: check.sh <file|dir>" >&2
	exit 2
fi

# 이 스크립트는 .claude/skills/sigma-style/ 에 있으므로 프로젝트 루트는 세 단계 위.
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)
BIN="$ROOT/build/sak"

if [ ! -x "$BIN" ]; then
	cmake -S "$ROOT" -B "$ROOT/build" >/dev/null
	cmake --build "$ROOT/build" >/dev/null
fi

TARGET=$1

if [ -d "$TARGET" ]; then
	exec "$BIN" -r "$TARGET"
else
	exec "$BIN" "$TARGET"
fi
