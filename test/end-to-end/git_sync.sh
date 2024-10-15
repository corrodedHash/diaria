# RUN: sh %s 2>%t.stderr | tee %t | %{filecheck} %s
# Test that executable can be found

TMPDIR=$(mktemp -d)

RECV_DIARIA="$DIARIA --keys "$TMPDIR/keys" --entries "$TMPDIR/entries2""
DIARIA="$DIARIA --keys "$TMPDIR/keys" --entries "$TMPDIR/entries""

BARE_REPO=$(mktemp -d)

create_keys() {
    # CHECK: Writing keys to
    echo Writing keys to $TMPDIR/keys
    echo abc | $DIARIA  init

    KEY_ENTRY_COUNT=$(ls -1 "$TMPDIR/keys" | wc -l)
    # CHECK: Number of key entries: 3
    echo Number of key entries: $KEY_ENTRY_COUNT
}

create_git_repo() {
    (
        cd $BARE_REPO
        git -c init.defaultBranch=main init --bare
    )
    (
        mkdir $TMPDIR/entries
        cd $TMPDIR/entries
        # CHECK: Initializing sending repo
        echo Initializing sending repo
        git -c init.defaultBranch=main init
        git config user.email "testuser@llvm.org"
        git config user.name "Testuser"
        git remote add origin $BARE_REPO
        touch init
        git add *
        git commit -m "Init"
        git push -u origin main
    )
}
create_receiving_git_repo(){
    mkdir $TMPDIR/entries2
    (
        cd $TMPDIR/entries2

        # CHECK: Initializing receiving repo
        echo Initializing receiving repo
        git -c init.defaultBranch=main init
        git config user.email "testuser@llvm.org"
        git config user.name "Testuser"
        git remote add origin $BARE_REPO
        git pull origin main
    )
}
create_entry() {

    ENTRY_TEXT=$(head -c 24 /dev/urandom | base64)
    # CHECK: Writing entry "[[ENTRYTEXT:[a-zA-Z0-9\+/]{32}]]"
    echo "Writing entry \"${ENTRY_TEXT}\""
    ENTRY_TEXT_FILE=$(mktemp)
    echo ${ENTRY_TEXT} > ${ENTRY_TEXT_FILE}
    $DIARIA add --input "${ENTRY_TEXT_FILE}"

    DIARY_ENTRY_COUNT=$(find $TMPDIR/entries -name '*.diaria' | wc -l)

    # CHECK: Number of diary entries: 1
    echo Number of diary entries: $DIARY_ENTRY_COUNT

    # CHECK: ?? {{.*}}.diaria
    (
        cd $TMPDIR/entries
        git status --porcelain
    )
}



create_keys
create_git_repo
create_receiving_git_repo
create_entry
$DIARIA repo sync || exit 1
$RECV_DIARIA repo sync || exit 1


#CHECK: [[ENTRYTEXT]]
echo abc | $RECV_DIARIA read $(find $TMPDIR/entries -name '*.diaria') || exit 1
