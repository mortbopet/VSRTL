# /bin/sh

cd test

find . -executable -type f -name "tst_*" | while read line; do
    ($line)
done