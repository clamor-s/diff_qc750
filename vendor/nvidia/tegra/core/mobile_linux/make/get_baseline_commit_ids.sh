#Generate Git commands that will sync a customer's Android tree to the correct Commit ID
#!/bin/bash

for i in `find . -type d -name .git |  sed -e "s/.\.git//"`
do
    echo cd $i
    cd $i
    hash=`git log | head -1 | cut -c8-`
    echo git reset --hard $hash
    echo cd -
    cd - >/dev/null
done
