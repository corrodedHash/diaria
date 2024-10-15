# RUN: sh %s 2>%t.stderr | tee %t | %{filecheck} %s
# Test that executable can be found
TMPDIR=$(mktemp -d)
# CHECK: Writing keys to
echo Writing keys to $TMPDIR/keys
echo abc | $DIARIA --keys $TMPDIR/keys init

mkdir -p $TMPDIR/entries
$DIARIA --keys $TMPDIR/keys --entries $TMPDIR/entries add --input "/dev/null/nonexistent"

DIARY_ENTRY_COUNT=$(ls -1 $TMPDIR/entries | wc -l)
# CHECK: Number of diary entries: 0
echo Number of diary entries: $DIARY_ENTRY_COUNT