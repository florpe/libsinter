set -euo pipefail

for vdir in $(find . -maxdepth 1 -regex '\./v[0-9]+\.[0-9]+' -printf "%f" | sort) ; do
    jq --compact-output --slurp --arg vdir "$vdir" \
        '{"headers": .[0], "opcodes": .[1], "operations": .[2], "structs": .[3]}' \
        "./$vdir/headers.json" \
        "./$vdir/opcodes.json" \
        "./$vdir/operations.json" \
        "./$vdir/structs.json" \
        > "protocol-$vdir.json"
done
cp "protocol-$vdir.json" protocol-latest.json

