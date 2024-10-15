# RUN: bash %s 2>%t.stderr | tee %t | %{filecheck} %s

TMPDIR=$(mktemp -d)
KEYDIR=$TMPDIR/keys
ENTRYDIR=$TMPDIR/entries
# CHECK: Writing keys to
echo Writing keys to $KEYDIR
echo hunter2 | $DIARIA --keys $KEYDIR init

$DIARIA --keys $KEYDIR --entries $ENTRYDIR add --input <(echo This is a test)
ENTRY=$(find $ENTRYDIR -type f)

TMPFILE=$TMPDIR/clear
echo hunter2 | $DIARIA --keys $KEYDIR read $ENTRY -o >(cat > $TMPFILE)
#CHECK: This is a test
cat $TMPFILE