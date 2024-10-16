# RUN: sh %s | tee %t | %{filecheck} %s
# Test that old diary entries stored can still be read

#CHECK: Eons... pass like days
$DIARIA -p password --keys entry_data/ read $(pwd)/entry_data/eternal.diaria

#CHECK: This runway is covered with the last pollen from the last flowers available anywhere on Earth.
$DIARIA -p password --keys entry_data/ read $(pwd)/entry_data/long.diaria
