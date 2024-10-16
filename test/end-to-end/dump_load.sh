# RUN: sh %s 2>%t.stderr | tee %t | %{filecheck} %s
# Test that executable can be found
set -e
TMPDIR=$(mktemp -d)
KEYDIR=$TMPDIR/keys
REPODIR=$TMPDIR/entries
REPODIR2=$TMPDIR/entries2
DUMPDIR1=$TMPDIR/dump1
DUMPDIR2=$TMPDIR/dump2

# CHECK: Writing keys to
echo Writing keys to $KEYDIR
$DIARIA -p abc --keys $KEYDIR init

ENTRY_COUNT=$(ls -1 $KEYDIR | wc -l)
# CHECK: Number of entries: 3
echo Number of entries: $ENTRY_COUNT

mkdir $DUMPDIR1
echo "1st testentry" >> $DUMPDIR1/a.txt
echo "2nd testentry" >> $DUMPDIR1/b.dump
echo "3rd testentry" >> $DUMPDIR1/c.txt

$DIARIA --keys $KEYDIR --entries $REPODIR repo load $DUMPDIR1

# CHECK: Dump 1 loaded: 3 entries
echo "Dump 1 loaded: $(ls -1 $DUMPDIR1 | wc -l) entries"

$DIARIA -p abc --keys $KEYDIR --entries $REPODIR repo dump $DUMPDIR2

# CHECK: Dumped: 3 entries
echo "Dumped: $(ls -1 $DUMPDIR2 | wc -l) entries"

$DIARIA --keys $KEYDIR --entries $REPODIR2 repo load $DUMPDIR2

# CHECK: Dump 2 loaded: 3 entries
echo "Dump 2 loaded: $(ls -1 $DUMPDIR1 | wc -l) entries"

# CHECK: 1st testentry
$DIARIA -p abc --keys $KEYDIR --entries $REPODIR2 read $REPODIR2/a.diaria
# CHECK: 2nd testentry
$DIARIA -p abc --keys $KEYDIR --entries $REPODIR2 read $REPODIR2/b.diaria
# CHECK: 3rd testentry
$DIARIA -p abc --keys $KEYDIR --entries $REPODIR2 read $REPODIR2/c.diaria
