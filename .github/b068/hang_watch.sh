#!/usr/bin/env bash
# B-068 hunt: runs beside a test step. Every second it samples CPU steal/pressure; a test process
# alive past HANG_S gets an all-thread gdb backtrace plus per-thread /proc state, and past KILL_S a
# second snapshot and SIGKILL, so the rest of the suite still runs on this runner.
set -u

out_dir=$1
hang_s=${HANG_S:-150}
kill_s=${KILL_S:-330}
mkdir -p "$out_dir"

snapshot() {
    local pid=$1 tag=$2
    {
        echo "=== pid=$pid tag=$tag age_s=$(ps -o etimes= -p "$pid" | tr -d ' ') at=$(date -u +%FT%T.%NZ)"
        tr '\0' ' ' < "/proc/$pid/cmdline"
        echo
        echo '--- threads (tid state wchan name)'
        for task in /proc/"$pid"/task/*; do
            echo "${task##*/} $(awk '{print $3}' "$task/stat") $(cat "$task/wchan" 2>/dev/null) $(cat "$task/comm")"
        done
        echo '--- gdb'
        sudo gdb -p "$pid" -batch -nx -ex 'set pagination off' -ex 'info threads' -ex 'thread apply all bt' 2>&1
    } > "$out_dir/hang-$pid-$tag.txt"
    echo "::warning::B-068 watch: pid $pid $tag snapshot -> $out_dir/hang-$pid-$tag.txt"
}

declare -A snapped
while true; do
    {
        printf '%s ' "$(date +%s.%N)"
        grep -E '^cpu ' /proc/stat | tr '\n' ' '
        tr '\n' ' ' < /proc/pressure/cpu 2>/dev/null
        echo
    } >> "$out_dir/cpu_samples.log"

    for pid in $(pgrep -f '/test_[a-z_]+( |$)'); do
        age=$(ps -o etimes= -p "$pid" 2>/dev/null | tr -d ' ')
        if [ -z "$age" ]; then
            continue
        fi
        if [ "$age" -ge "$hang_s" ] && [ -z "${snapped[$pid]:-}" ]; then
            snapped[$pid]=1
            snapshot "$pid" hang
        fi
        if [ "$age" -ge "$kill_s" ]; then
            snapshot "$pid" before-kill
            kill -9 "$pid"
        fi
    done
    sleep 1
done
