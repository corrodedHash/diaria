# RUN: bash %s 2>%t.stderr | tee %t | %{filecheck} %s
# Test that executable can be found
TMPDIR=$(mktemp -d)
# CHECK: Writing keys to
echo Writing keys to $TMPDIR/keys
$DIARIA -p abc --keys "$TMPDIR/keys" init

KEY_ENTRY_COUNT=$(ls -1 "$TMPDIR/keys" | wc -l)
# CHECK: Number of key entries: 3
echo Number of key entries: $KEY_ENTRY_COUNT

NOW="$(date +%s)"

for i in `seq 40`; do
    CURUNIXEPOCH=$(( ${NOW} - $(( 86400 * $i )) ))
    OUTFILENAME="$(date "--date=@${CURUNIXEPOCH}" "+%FT%H:%M:%S.diaria")"
    # echo $OUTFILENAME
    $DIARIA --keys "$TMPDIR/keys" --entries "$TMPDIR/entries" add --input <(echo This is the entry number $i.) --output "${OUTFILENAME}"
done

# CHECK: Number of entries: 40
echo Number of entries: $(ls -1 "$TMPDIR/entries" | wc -l)

$DIARIA -p abc --keys "$TMPDIR/keys" --entries "$TMPDIR/entries" repo summarize --long

# CHECK: Reading entry from
# CHECK-NEXT: This is the entry number 1.
# CHECK: Reading entry from
# CHECK-NEXT: This is the entry number 7.
# CHECK: Reading entry from
# CHECK-NEXT: This is the entry number 31.