#!/usr/bin/env bash
# B-068 hunt: record what this runner is, then refuse runner images the hang was never seen on.
# The hang and the stall-ridden slow runs so far came only from image 20260907.292.1 (0 of 10 runs
# on 20260831.284.1); the image is likely a proxy for the VM pool, hence the hardware dump.
set -u

out_dir=$1
mkdir -p "$out_dir"

image="${ImageVersion:-}"
if [ -z "$image" ] && [ -r /imagegeneration/imagedata.json ]; then
    image=$(python3 -c '
import json, re
for group in json.load(open("/imagegeneration/imagedata.json")):
    if group.get("group") == "Runner Image":
        match = re.search(r"Version: (\S+)", group.get("detail", ""))
        if match:
            print(match.group(1))
' 2>/dev/null)
fi

{
    echo "image=${image:-unknown}"
    echo "kernel=$(uname -r)"
    echo "nproc=$(nproc)"
    lscpu | grep -E '^(Model name|CPU\(s\)|Thread\(s\) per core|Core\(s\) per socket|Socket\(s\)|Hypervisor vendor|Virtualization type|L3 cache|CPU MHz)'
    echo "clocksource=$(cat /sys/devices/system/clocksource/clocksource0/current_clocksource 2>/dev/null)"
    echo "cgroup_cpu_max=$(cat /sys/fs/cgroup/cpu.max 2>/dev/null)"
    echo "timerslack_ns=$(cat /proc/self/timerslack_ns 2>/dev/null)"
    echo "proc_stat_cpu=$(grep -E '^cpu ' /proc/stat)"
    echo "pressure_cpu=$(tr '\n' ' ' < /proc/pressure/cpu 2>/dev/null)"
    free -m | head -2
} | tee "$out_dir/fingerprint.txt"

{
    echo '### Runner fingerprint'
    echo '```'
    cat "$out_dir/fingerprint.txt"
    echo '```'
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"

case "$image" in
    '' | unknown)
        echo "::warning::runner image unknown, continuing"
        ;;
    *)
        if [ "${image%%.*}" -lt 20260907 ]; then
            echo "::error::runner image $image predates 20260907, where B-068 was never seen; rerun this leg"
            exit 1
        fi
        ;;
esac
