# RUN: sh %s | tee %t | %{filecheck} %s
# Test that executable can be found
TMPDIR=$(mktemp -d)
# CHECK: Writing keys to
echo Writing keys to $TMPDIR
$DIARIA -p abc --keys $TMPDIR init

ENTRY_COUNT=$(ls -1 $TMPDIR | wc)
# CHECK: Number of entries: 3
echo Number of entries: $ENTRY_COUNT