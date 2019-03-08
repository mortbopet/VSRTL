# /bin/sh

cd tst

find . -executable -type f -name "tst_*" | while read line; do
    ($line)
done