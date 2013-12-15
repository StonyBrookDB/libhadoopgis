# libhadoopgis
*libhadoopgis* is library version of [HadoopGIS](https://github.com/Hadoop-GIS/Hadoop-GIS) - a 
scalable and high performance spatial data warehousing system for running spatial queries on 
Hadoop. *libhadoopgis* comes with a set of easy to use scripts which enables you to run 
hadoopgis queries on Amazon EMR.

## Library Dependecies:
*libhadoopgis * relies on two libraries for performaing spatial data processing. An [EMR bootstrap action] (http://docs.aws.amazon.com/ElasticMapReduce/latest/DeveloperGuide/emr-plan-bootstrap.html) script is provided in the libhadopgis repository.

- GEOS 3.x (x >= 3)

- libspatialindex 1.8.x (x >= 0)

## AWS Dependecy (for CLI):
Amazon Elastic MapReduce Command Line Interface: [Amazon EMR CLI] (http://docs.aws.amazon.com/ElasticMapReduce/latest/DeveloperGuide/emr-cli-install.html)

## Steps to run from AWS EMR web interface:

### Source code compilation and configuration:

1. Cluster Creation with Amazon EMR CLI.
  1. Create a cluster instance of EMR (EC2) and login into it using ssh via the AWS EMR CLI interface.

  ```bash 
  ./elastic-mapreduce --create --alive --name "compilerMachine" --num-instances=1 --master-instance-type=m1.medium
  ```

  2. ssh to the cluster with jobflow ID created with the step above:

  ```bash
  ./elastic-mapreduce --ssh jobflow_id
  ```


2. Dowload and compile *libhadoopgis* on the cluster.

  ```bash
  git clone https://github.com/ablimit/libhadoopgis
  cd tiler
  make

  cd joiner
  make
  ```

3. Upload the libhadopgis binaries to Amazon S3.
  
  Example:
  ```bash
  hadoop fs -pu libhadoopgis s3://yourbucket/libhadoopgis
  ```


### Running *libhadoopgis* from AWS web interface
1. Login to the AWS Management Console and select Elastic MapReduce.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/1.png "Select EMR")

2. Create a cluster.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/2.png "Create a cluster")

3. Configure cluster name and location for log files.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/3.png "configure cluster")

4. Select the compatible AMI version (corresponding to different version of Hadoop).

5. Remove Hive and Pig installation as we do not need them.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/5.png "remove hive and pig")

6. Select your preferred Hardware Configuration.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/6.png "configure hardware")
  * The description of EC2 instances can be found on the [aws website](http://aws.amazon.com/ec2/instance-types/instance-details/).
  * Be aware of [The default settings] (http://docs.aws.amazon.com/ElasticMapReduce/latest/DeveloperGuide/TaskConfiguration.html) for the numbers of mappers and reducers.  
  * Be advised that while you can pick any number of reducers, the maximum CPU cores available for computing might not be less than the number of reducers. In addition, the amount of memory available for mapper jobs will decrease as the number of reducers increases while the number of core and task instances does not change.


7. Select a boostrap script (libspatialindex and geos and libhadoopgis installation).
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/7.png "bootstrap")

8. create a streaming job.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/8.png "streaming")

9. Enter the locations of mappers, reducers, input and output directory, as well as other arguments. In this example we will partition two datasets and perform a spatial join on them.
  * **Partition** (tiling) step.

    **Mapper**: S3 location of hgdeduplicater.py
    
    _Argument_: cat
   
    **Reducer**: hgtiler
    
    Arguments: -w minX –s minY – n maxY –e maxX (The minimum and maximum coordinates of the spatial universe)
    
    Arguments: -x numberOfXsplits –y numberOfYsplits –u uid –g gid
    
    Argument: -u : index of the uid field (counting from 1)
    
    Argument: -g : index of the geometry field (counting from 1)
   
    **Input location**: Location of the first data file on Amazon S3
   
    **Output location**: The directory of the output should not exist on S3. It will be created by the EMR job.
   
    **Other Arguments**: Specify the number of reduce tasks and other options as needed.
  
    Example: for a sample of OpenStreetMap data (OSM) (coordinates range: x: [-180, 180], y: [-90, 90]) – the geometry is the 5th field on every line. uid is the 1st field. and we want to split the data into 100 by 100 grids.
   
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/hgdeduplicater.py cat 
    Reducer: s3://yourbucket/libhadoopgis/tiler/hgtiler -w -180 -s -90 -n 90 -e 180 -x 100 -y 100 -u 1 -g 5
    Input location: s3://yourbucket/libhadoopgis/sampledata/osm.1.dat
    Output location: s3://yourbucket/outputtiler1/
    Argument: -numReduceTasks 20
    ```

  * **Spatial Query** step:
    Spatial queries run on the partitioned data, i.e the output from tiling step. To run spatial join queries, 
    users need to specify extra argument. Namely, the `spatial predicate` and indexes of geometry fields in 
    the datasets to be joined.
    
     **Mapper**: location of tagmapper.py on Amazon S3.
     
      First argument: directory name of the first partitioned dataset.
      Second argument: directory name of the second partitioned dataset.
    
     **Reducer**: s3://yourbucket/libhadoopgis/joiner/resque
      
      First argument: type of operation (see below for supported type).
    
      Second argument: index of the geometry field in the first dataset
    
      Third parameter: index of the geometry field in the second dataset
      Note: (these index of the geometry field are same as the positions of the geometry fields as in the partition step)
    
     **Input location**: S3 location of the partitioned data directory.
    
     **Output location**: S3 location for the output directory.
      Arguments: Specify the number of reduce tasks and the second input directory to


    Example:
    
    ```bash
    Mapper: s3://yourbucket/libhadoopgis/joiner/tagmapper.py outputtiler1 outputtiler2
    Reducer: s3://yourbucket/libhadoopgis/joiner/resque st_intersects 5 5
    Input location: s3://yourbucket/libhadoopgis/outputtiler1/
    Output location: s3://yourbucket/libhadoopgis/sampleout/
    Argument: -input s3://yourbucket/libhadoopgis/outputtiler2/ -numReduceTasks 10
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

##Optimization for Skewed Dataset

If you have skewed dataset, you can use the `skewresque` to reduce your query time. For example, let's say you have a dataset _A_ and _B_, and size of _B_ is much smaller than _A_, and you want to perform a spatial join on them. Then you can skip the partition step altogether, and perform the query directly on the raw dataset. Please see example below.

* Skewed spatial join:
   
  **Mapper**: location of the resqueskew program s3://cciemorypublic/libhadoopgis/program/skewresque
(Visible to public)
    
  Arguments: Similar to resque arguments above.
   
  **Reducer**: No reducer is needed.
   
  **Input location**: Location of the larger input file (tsv)
  
  **Output location**: The directory of the output should not exist on S3. It will be created by the EMR job.
   
  Arguments: Specify the number of reduce tasks, additional input directory (as for standard input) and path 
  to the small file used as the other operand in the join.
  ```bash
  Mapper: s3://yourbucket/libhadoopgis/joiner/skewresque st_intersects 2 1
  Reducer: None (Enter None)
  Input location: s3://yourbucket/libhadoopgis/sampledata/tweet.dump.tsv
  Output location: s3://yourbucket/libhadoopgis/skewjoinout/
  Arguments: -cacheFile s3://yourbucket/libhadoopgis/sampledata/fulton.tsv#hgskewinput -numReduceTasks 0
  ```


## Licence
All libhadoopgis software is freely available, and all source code 
is available under GNU public [copyleft](http://www.gnu.org/copyleft/ "copyleft") licenses.

