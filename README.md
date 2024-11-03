# Diaria

![Continuous Integration](https://github.com/corrodedHash/diaria/actions/workflows/ci.yml/badge.svg?branch=main)

Tool for writing and maintaining diary entries

## Security Model

Diary entries are encrypted both with a symmetric key as well as an asymmetric key.

The symmetric key is stored unencrypted, while the private key for asymmetric encryption is
itself symmetrically encrypted via a password.  
This way, new diary entries can be created without the need for entering a password, while
reading requires a password.

The symmetric encryption of the entries adds a level of security at rest, when synced to a storage
server where the symmetric key is not backed up to.  
Especially for long term entries, the symmetric encryption used is more likely to be resistant against quantum computation attacks.

## Many thanks to
CMake project template by [cmake-init](https://github.com/friendlyanon/cmake-init)

Easy to use, hard to misuse encryption librare [libsodium](https://doc.libsodium.org/)