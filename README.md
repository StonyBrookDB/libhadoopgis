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

## Steps to Run on AWS EMR:

### Source code compilation and configuration:

1. Cluster Creation with Amazon EMR CLI.
  1. Create a cluster instance of EMR (EC2) and login into it using ssh via the AWS EMR CLI interface.

  ```bash 
  ./elastic-mapreduce --create --alive --name "compilerandtest" --num-instances=1 --master-instance-type=m1.medium
  ```

  2. ssh to the cluster with jobflow ID created with the step above:

  ```bash
  ./elastic-mapreduce --ssh –jobflow jobflow_id
  ```


2. Dowload and compile *libhadoopgis* on the cluster.

  ```bash
  git clone https://github.com/ablimit/libhadoopgis
  cd tiler
  make

  cd joiner
  make
  ```

3. Upload the libhadopgis folder to Amazon S3.


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

9. Enter the locations of mappers, reducers, input and output directory, as well as other arguments.
  * For tiling job (tiler):

    **Mapper**: s3://cciemory/program/cat
    
    **Reducer**: (need to enter parameters): s3://cciemory/program/hgtilerwitharg 
    
    **Input location**: valid location on Amazon s3.
    
    **Output location**: The directory of the output should not exist on S3. It will be created by the EMR job.
    
    **Arguments**: Specify the number of reduce tasks and other options as needed.

    Example:
    
    **Mapper**: `s3://cciemory/program/cat`
    
    **Reducer**: `s3://cciemory/program/hgtilerwitharg -w 0 -s 0  -n 100000 -e 100000 -x 25 -y 25 -u 1 -g 11`
    
    **Input location**: s3://cciemory/hadoopgis/sampleinput/
    
    **Output location**: s3://cciemory/hadoopgis/sampleout/
    
    **Arguments**:  -numReduceTasks 8
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/9.png "tiling job")

  * Spatial Join Query job:
    For running spatial join queries, users need to specify extra argument. Namely, the `spatial predicate` and indexes of geometry fields in the dataset to be joined.
    
    **Mapper**: location of tagmapper.py followed by arguments which are names of input files.
    
    **Reducer**: (need to enter parameters): s3://cciemory/program/resque
    
    **Input location**: The inputs file location on Amazon S3
    
    **Output location**: The directory of the output should not exist on Amazon S3. It will be created by the EMR job.
    
    **Arguments**: Specify the number of reduce tasks, additional input directory

    Example:
    
    **Mapper**: s3://cciemory/program/tagmapper.py s3://cciemory/hadoopgis/samplejoininput/dataset1.dat s3://cciemory/hadoopgis/samplejoininput/dataset2.dat
    
    **Reducer**: s3://cciemory/program/resque st_intersects 2 4
    
    **Input location**: s3://cciemory/hadoopgis/samplejoininput/
    
    **Output location**: s3://cciemory/hadoopgis/samplejoinout/
    
    **Arguments**: -numReduceTasks 20

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

## Licence
All libhadoopgis software is freely available, and all source code 
is available under GNU public [copyleft](http://www.gnu.org/copyleft/ "copyleft") licenses.

