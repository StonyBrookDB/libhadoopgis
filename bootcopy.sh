#! /bin/bash

your_s3_bucket=aaji/scratch
cd
echo "hadoop fs -get s3://${your_s3_bucket}/s3cfg .s3cfg"
hadoop fs -get s3://${your_s3_bucket}/s3cfg .s3cfg

mkdir -p /tmp/lib
mkdir -p /tmp/include

s3cmd sync s3://${your_s3_bucket}/deps/libs/ /tmp/lib/
s3cmd sync s3://${your_s3_bucket}/deps/includes/ /tmp/include/

sudo cp /tmp/lib/* /usr/lib/
sudo cp -r /tmp/include/* /usr/include/

# these lines are not necessary, but just to check if the compilation is ok.
s3cmd get s3://${your_s3_bucket}/awsjoin/resque.cpp ./
s3cmd get s3://${your_s3_bucket}/awsjoin/skewresque.cpp ./
s3cmd get s3://${your_s3_bucket}/awsjoin/makefile ./

make
sudo make install


