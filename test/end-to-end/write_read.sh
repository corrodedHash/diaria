# RUN: sh %s | tee %t | FileCheck %s
# Test that executable can be found
TMPDIR=$(mktemp -d)
# CHECK: Writing keys to
echo Writing keys to $TMPDIR/keys
echo abc | $DIARIA --keys $TMPDIR/keys init

KEY_ENTRY_COUNT=$(ls -1 $TMPDIR/keys | wc -l)
# CHECK: Number of key entries: 3
echo Number of key entries: $KEY_ENTRY_COUNT

ENTRY_TEXT=$(head -c 24 /dev/urandom | base64)
# CHECK: Writing entry "[[ENTRYTEXT:[a-zA-Z0-9\+/]{32}]]"
echo "Writing entry \"${ENTRY_TEXT}\""
$DIARIA --keys $TMPDIR/keys --entries $TMPDIR/entries add --input <(echo ${ENTRY_TEXT})

DIARY_ENTRY_COUNT=$(ls -1 $TMPDIR/entries | wc -l)
ls -R $TMPDIR
# CHECK: Number of diary entries: 1
echo Number of diary entries: $DIARY_ENTRY_COUNT

#CHECK: [[ENTRYTEXT]]
echo abc | $DIARIA --keys $TMPDIR/keys --entries $TMPDIR/entries read $(find $TMPDIR/entries -type f)
