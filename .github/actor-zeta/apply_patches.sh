#!/usr/bin/env bash
# Applies the given patches, in order, to the headers of the one actor-zeta package this job installed.
# Header-only edits reach every otterbrix TU; the prebuilt actor-zeta archive is not rebuilt.
set -euo pipefail

header=$(find ~/.conan2/p -path '*/p/include/actor-zeta/actor/cooperative_actor.hpp')
count=$(printf '%s\n' "$header" | grep -c . || true)
if [ "$count" -ne 1 ]; then
    echo "::error::expected exactly one actor-zeta package header, found $count"
    printf '%s\n' "$header"
    exit 1
fi

include_dir=${header%/actor-zeta/actor/cooperative_actor.hpp}
for patch_file in "$@"; do
    echo "applying $patch_file to $include_dir"
    patch -p1 --forward -d "$include_dir" < "$patch_file"
done
