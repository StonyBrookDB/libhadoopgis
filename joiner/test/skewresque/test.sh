#!/usr/bin/env bash

if [ -e ../../skewresque ];
then 
  cp ../../skewresque ./
else
  echo "missing skewresque exe in current directory."
  exit 1; 
fi

cp ../../../data/atl.stores.tsv hgskewinput
cp ../../../data/tweet.dump.tsv .

cat tweet.dump.tsv | ./skewresque --p st_within -i 2 -j 1 -d 1 -f 1,2:5,6 > skew.out.tsv
cat tweet.dump.tsv | ./skewresque --p st_within -i 2 -j 1 -d 1 > skew.out.tsv

rc=$?
if [ $rc -eq 0 ];then
  echo -e "\n\njoin task has finished successfully."
else
  echo -e "\nERROR: skewjoin failed. "
fi

rm -f hgskewinput

exit $rc ;

