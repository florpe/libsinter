set -euo pipefail

for vdir in $(find . -maxdepth 1 -regex '\./v[0-9]+\.[0-9]+' -printf "%f" | sort) ; do
    jq --compact-output --slurp --arg vdir "$vdir" \
        '{($vdir): {"headers": .[0], "opcodes": .[1], "operations": .[2], "structs": .[3]}}' \
        "./$vdir/headers.json" \
        "./$vdir/opcodes.json" \
        "./$vdir/operations.json" \
        "./$vdir/structs.json"
done | jq --compact-output 'reduce . as $v ({}; . + $v)' > protocol.json

