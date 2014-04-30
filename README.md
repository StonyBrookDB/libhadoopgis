# libhadoopgis
*libhadoopgis* is library version of [HadoopGIS](https://github.com/Hadoop-GIS/Hadoop-GIS) - a 
scalable and high performance spatial data warehousing system for running spatial queries on 
Hadoop. *libhadoopgis* comes with a set of easy to use scripts which enables you to run 
hadoopgis queries on Amazon EMR.

## Library Dependencies:
*libhadoopgis* relies on two libraries for performing spatial data processing. An [EMR bootstrap action] (http://docs.aws.amazon.com/ElasticMapReduce/latest/DeveloperGuide/emr-plan-bootstrap.html) script is provided in the libhadopgis repository.

- GEOS 3.x (x >= 3)

- libspatialindex 1.8.x (x >= 0)

## Amazon Web Services (AWS) Dependency (for Command Line Interface):
Amazon Elastic MapReduce Command Line Interface (CLI): [Amazon EMR CLI] (http://docs.aws.amazon.com/ElasticMapReduce/latest/DeveloperGuide/emr-cli-install.html)

## Steps to run from AWS EMR web interface:

### Source code compilation and configuration:

1. Cluster Creation with Amazon EMR CLI.
  1. Create a cluster instance in EMR (EC2 nodes) and login into it using ssh via the AWS EMR CLI interface.

  ```bash 
  ./elastic-mapreduce --create --alive --name "compilerMachine" --num-instances=1 --master-instance-type=m1.medium
  ```

  2. ssh to the cluster with jobflow ID created with the step above:

  ```bash
  ./elastic-mapreduce --ssh jobflow_id
  ```


2. Dowload and compile *libhadoopgis* on the cluster.
  This step only need to be done every time you start the cluster. Therefore, you can put this in the _boostrap action_. 
  ```bash
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
  
  git clone https://github.com/ablimit/libhadoopgis
  cd tiler
  make

  cd joiner
  make
  ```
  However, this bootstrap is heavy weight. A lightweight bootstrap action would be: compile and install the dependencies once; copy libraries to S3 (`hadoop fs -put /usr/local/lib/lib* s3://_yourbucket_/libs/`); and create a bootstrap action which only downloads these libraries from S3 to the cluster `bootstrap.light.sh`.

3. Upload the libhadopgis binaries to Amazon S3.
  
  Example (using AWS Command Line Interface):
  ```bash
  aws s3 sync libhadoopgis s3://yourbucket/libhadoopgis
  ```


### Running *libhadoopgis* from AWS web interface
1. Login to the AWS Management Console and select Elastic MapReduce.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/1.png "Select EMR")

2. Create a cluster.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/2.png "Create a cluster")

3. Configure the cluster name and a location for log files.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/3.png "Configure cluster")

4. Select a compatible AMI version (corresponding to different version of Hadoop).

5. Remove the default Hive and Pig installation as we do not need them.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/5.png "Remove hive and pig")

6. Select your preferred Hardware Configuration.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/6.png "Configure hardware")
  * The description of EC2 instances can be found on the [AWS website](http://aws.amazon.com/ec2/instance-types/instance-details/).
  * Be aware of [The default settings] (http://docs.aws.amazon.com/ElasticMapReduce/latest/DeveloperGuide/TaskConfiguration.html) for the default numbers of mappers and reducers.  
  * Be advised that while you can pick any number of reducers, the maximum CPU cores available for computing might not be less than the number of reducers. In addition, the amount of memory available for mapper jobs will decrease as the number of reducers increases while the number of core and task instances does not change.


7. Select a boostrap script (libspatialindex and geos and libhadoopgis installation).
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/7.png "bootstrap")

8. Create a streaming job.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/8.png "streaming")

9. Enter the locations of mappers, reducers, input and output directory, as well as other arguments. In this example we will partition two datasets and perform a spatial join on them.
  * **Partition** (tiling) step.

    **Mapper**: Amazon S3 location of *hgdeduplicater.py*
    
    _Argument_: cat
   
    **Reducer**: Amazon S3 location of *hgtiler* followed by its arguments
    
    Arguments: -w minX –s minY – n maxY –e maxX (The minimum and maximum coordinates of the spatial universe)
    
    Argument: -x numberOfXsplits Number of splits across the horizontal direction.
    
    Argument: –y numberOfYsplits Number of splits across the vertical direction.
    
    Argument: -u uidNum: index of the uid field (counting from 1).
    
    Argument: -g gidNum: index of the geometry field (counting from 1).
    
   
    **Input location**: Amazon S3 Location of the first data file on Amazon S3. Note that the fields should be seperated by tabs (tsv)!
   
    **Output location**: The directory of the output should not exist on Amazon S3. It will be created by the EMR job.
   
    **Other Arguments**: Specify the number of reduce tasks and other options as needed.
  
    Example 1: 
      for a sample of OpenStreetMap (OSM) data (coordinates range: x: [-180, 180], y: [-90, 90]), the geometry is the 5th field on every line, uid is the 1st field, and we want to split the data into a 100 by 100 grid.
   
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/hgdeduplicater.py cat 
    Reducer: s3://yourbucket/libhadoopgis/tiler/hgtiler -w -180 -s -90 -n 90 -e 180 -x 100 -y 100 -u 1 -g 5
    Input location: s3://yourbucket/libhadoopgis/sampledata/osm.1.dat
    Output location: s3://yourbucket/outputtiler1/
    Argument: -numReduceTasks 20
    ```
    
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/hgdeduplicater.py cat 
    Reducer: s3://yourbucket/libhadoopgis/tiler/hgtiler -w -180 -s -90 -n 90 -e 180 -x 100 -y 100 -u 1 -g 5
    Input location: s3://yourbucket/libhadoopgis/sampledata/osm.2.dat
    Output location: s3://yourbucket/outputtiler2/
    Argument: -numReduceTasks 20
    ```
    
    Example 2:
      for a sample of Pathology image (PAIS) data (coordinates range: x: [0, 100000], y: [0, 100000]) – the geometry is the 11th field on every line. uid is the 1st field. and we want to split the data into a 20 by 20 grid.
    
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/hgdeduplicater.py cat 
    Reducer: s3://yourbucket/libhadoopgis/tiler/hgtiler -w 0 -s 0 -n 100000 -e 100000 -x 20 -y 20 -u 1 -g 11
    Input location: s3://yourbucket/libhadoopgis/sampledata/pais.1.dat
    Output location: s3://yourbucket/outputpaistiler1/
    Argument: -numReduceTasks 20
    ```
    
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/hgdeduplicater.py cat 
    Reducer: s3://yourbucket/libhadoopgis/tiler/hgtiler -w 0 -s 0 -n 100000 -e 100000 -x 20 -y 20 -u 1 -g 11
    Input location: s3://yourbucket/libhadoopgis/sampledata/pais.2.dat
    Output location: s3://yourbucket/outputpaistiler2/
    Argument: -numReduceTasks 20
    ```

  * **Spatial Query** step:
    Spatial queries run on the partitioned data, i.e the output from tiling step. To run spatial join queries, 
    users need to specify extra argument. Namely, the `spatial predicate` and indexes of geometry fields in 
    the datasets to be joined.
    
     **Mapper**: Amazon S3 location of *tagmapper.py* on Amazon S3 followed by 1 or 2 arguments.
      First argument: directory name of the first partitioned dataset (output directory from the first partitioning step).

      Second argument: directory name of the second partitioned dataset (output directory from the second partitioning step).
      
      Note that the directory names could just be the relative path on Amazon S3 (not necessary full path), but directory names should be distinct.
      
    
     **Reducer**: Amazon S3 location of *resque* followed with arguments.
     Argument: -i gid1 Index of the geometry field in the first data set. Index value starts from 1.
     
     Argument: -j gid2 Index of the geometry field in the second data set. Index value starts from 1.
     
     Note: (these indices of the geometry field are same as the positions of the geometry fields as in the partition step)
     
     Argument: -p st_predicate Type of operation (see below for supported type).
     
     Argument: -d distance Used together with st_dwithin predicate to indicates the join distance.This field has no effect o other join predicates.
     
     Optional argument: -f f1,2,3..:1,2,3 Optional output field parameter. Fields from different dataset are separated with a colon (:), and fields from the same dataset are separated with a comma (,). For example: if we want to only output fields 1,3, and 5 from the first dataset (indicated with param -i), and output fields 1, 2, and 9 from the second dataset (indicated with param -j)  then we can provide an option such as: -f 1,3,5:1,2,9
     
    
     **Input location**: S3 location of the first partitioned data directory (data set 1).
    
    
     **Output location**: S3 location for the output directory.
     
     Arguments: Specify the number of reduce tasks and the second input directory to the second data set (data set 2)
     -input s3FullPathSecondDataSet
     -numReduceTasks someNum


    Example - OSM dataset
    (output all fields):
    
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/tagmapper.py outputtiler1 outputtiler2
    Reducer: s3://yourbucket/libhadoopgis/joiner/resque -p st_intersects -i 5 -j 5
    Input location: s3://yourbucket/libhadoopgis/outputtiler1/
    Output location: s3://yourbucket/libhadoopgis/sampleout/
    Argument: -input s3://yourbucket/libhadoopgis/outputtiler2/ -numReduceTasks 10
    ```
    
    Example - PAIS dataset 
    (output only user-selected fields: 1,2,4, 5 from data set 1 and 1,2,11 from data set 2):
    
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/tagmapper.py outputpaistiler1 outputpaistiler2
    Reducer: s3://yourbucket/libhadoopgis/joiner/resque -p st_dwithin -i 11 -j 11 -d 1 -f 1,2,4,5,11:1,2,11
    Input location: s3://yourbucket/libhadoopgis/outputpaistiler1/
    Output location: s3://yourbucket/libhadoopgis/sampleout/
    Argument: -input s3://yourbucket/libhadoopgis/outputpaistiler2/ -numReduceTasks 10
    ```
    
    Full list of supported spatial join predicates:
    
    ```bash
    st_intersects
    st_touches
    st_crosses
    st_contains
    st_adjacent
    st_disjoint
    st_equals
    st_dwithin 
    st_within
    st_overlaps
    ```
  * **Boundary-handling (Duplicate-removal)** step:
  * 
    Since spatial processing is performed indepedently across regions or tiles, duplicates might occur and therefore duplicates should be eliminated to achieve consistent and correct results.
    
     **Mapper**: Amazon S3 location of *hgdeduplicater.py* on Amazon S3 followed by the argument *cat*.
    
     **Reducer**: Amazon S3 location of *hgdeduplicater.py* on Amazon S3 followed by the argument *uniq*.
    
     **Input location**: Amazon S3 location of the output directory from the previous spatial join step.
    
     **Output location**: Amazon S3 location for the final output directory.

    Example - OSM dataset
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/hgdeduplicater.py cat
    Reducer: s3://yourbucket/libhadoopgis/joiner/hgdeduplicater.py uniq
    Input location: s3://yourbucket/libhadoopgis/sampleout/
    Output location: s3://yourbucket/libhadoopgis/samplefinalout/
    Argument: -numReduceTasks 20
    ```


##Optimization for Skewed Dataset

If you have skewed dataset, you can use the `skewresque` to reduce your query time. For example, let's say you have a dataset _A_ and _B_, and size of _B_ is much smaller than _A_, and you want to perform a spatial join on them. Then you can skip the partition step altogether, and perform the query directly on the raw dataset. Please see example below.

* Skewed spatial join:
   
  **Mapper**: location of the resqueskew program s3://cciemorypublic/libhadoopgis/program/skewresque
(Visible to public)
    
  Arguments: Similar to the resque arguments above.

  Argument: -i The index of the geometry field in the first data set. Index value starts from 1.

  Argument: -j The index of the geometry field in the second data set. Index value starts from 1.
  
     Note: (these index of the geometry field are same as the positions of the geometry fields as in the partition step)

  Argument: -p type of operation (see below for supported type).
  
  Argument: -d Used together with st_dwithin predicate to indicates the join distance.This field has no effect o other join predicates.
  
  Optional argument: -f Optional output field parameter. Fields from different dataset are separated with a colon (:), and fields from the same dataset are separated with a comma (,). For example: if we want to only output fields 1, 3, and 5 from the first dataset (indicated with param -i), and output fields 1, 2, and 9 from the second dataset (indicated with param -j)  then we can provide an option such as: -f 1,3,5:1,2,9

  **Reducer**: No reducer is needed.
   
  **Input location**: Location of the larger input file (tsv). Note that fields should be seperated by tabs!
  
  **Output location**: The directory of the output should not exist on S3. It will be created by the EMR job.
   
  **Arguments**: 
  Argument: -numReduceTasks 0 Set the number of reduce tasks as 0
  
  Argument: -cacheFile s3PathFileName#hgskewinput path to the small file used as the other operand in the join (Note that the small file will be renamed to hgskewinput for MapReduce in local directories).
  
  Optinal argument: -input SomePath additional input directories as additional parts of the first dataset if needed (as for standard input)

  Example:
  ```bash
  Mapper: s3://yourbucket/libhadoopgis/joiner/skewresque -p st_intersects -i 2 -j 1
  Reducer: None (Enter None)
  Input location: s3://yourbucket/libhadoopgis/sampledata/tweet.dump.tsv
  Output location: s3://yourbucket/libhadoopgis/skewjoinout/
  Arguments: -cacheFile s3://yourbucket/libhadoopgis/sampledata/fulton.tsv#hgskewinput -numReduceTasks 0
  ```


## Licence
All libhadoopgis software is freely available, and all source code 
is available under GNU public [copyleft](http://www.gnu.org/copyleft/ "copyleft") licenses.

