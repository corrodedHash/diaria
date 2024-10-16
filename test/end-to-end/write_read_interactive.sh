# RUN: sh %s | tee %t | %{filecheck} %s

# This script also acts as its own interactive input
if [ -n "$1" ]; then
    echo "Testentry" > $1
    exit 0
fi

TMPDIR=$(mktemp -d)
# CHECK: Writing keys to
echo Writing keys to $TMPDIR/keys
$DIARIA -p abc --keys "$TMPDIR/keys" init

KEY_ENTRY_COUNT=$(ls -1 "$TMPDIR/keys" | wc -l)
# CHECK: Number of key entries: 3
echo Number of key entries: $KEY_ENTRY_COUNT

ENTRY_TEXT=$(head -c 24 /dev/urandom | base64)
# CHECK: Writing entry "[[ENTRYTEXT:Testentry]]"
echo "Writing entry \"Testentry\""
$DIARIA --keys "$TMPDIR/keys" --entries "$TMPDIR/entries" add --editor "sh $0 %"

DIARY_ENTRY_COUNT=$(ls -1 $TMPDIR/entries | wc -l)
ls -R $TMPDIR
# CHECK: Number of diary entries: 1
echo Number of diary entries: $DIARY_ENTRY_COUNT

#CHECK: [[ENTRYTEXT]]
$DIARIA -p abc --keys "$TMPDIR/keys" --entries "$TMPDIR/entries" read $(find $TMPDIR/entries -type f)
