#! /bin/bash

sudo apt-get -y install cmake

wget http://download.osgeo.org/geos/geos-3.3.9.tar.bz2
tar xvf geos-3.3.9.tar.bz2
cd geos-3.3.9
mkdir Release
cd Release
cmake ..
make
sudo make install

wget http://download.osgeo.org/libspatialindex/spatialindex-src-1.8.1.tar.bz2
tar xvf spatialindex-src-1.8.1.tar.bz2
cd spatialindex-src-1.8.1
mkdir Release
cd Release
cmake ..
make
sudo make install

cd boots
hadoop fs -get s3://your_s3_bucket/awsjoin/resque.cpp ./
hadoop fs -get s3://your_s3_bucket/awsjoin/skewresque.cpp ./
hadoop fs -get s3://your_s3_bucket/awsjoin/makefile ./
make
sudo make install

