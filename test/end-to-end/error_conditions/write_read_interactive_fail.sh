# RUN: sh %s 2>&1 | tee %t | %{filecheck} %s

TMPDIR=$(mktemp -d)
# CHECK: Writing keys to
echo Writing keys to $TMPDIR/keys
$DIARIA -p abc --keys "$TMPDIR/keys" init

KEY_ENTRY_COUNT=$(ls -1 "$TMPDIR/keys" | wc -l)
# CHECK: Number of key entries: 3
echo Number of key entries: $KEY_ENTRY_COUNT

$DIARIA --keys "$TMPDIR/keys" --entries "$TMPDIR/entries" add --editor "/dev/null %"

# CHECK-NOT: Entry directory was created
test -d "$TMPDIR/entries" && echo Entry directory was created
true
