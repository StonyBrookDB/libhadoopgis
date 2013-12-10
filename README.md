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
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/3.png "Create a cluster")

4. Configure cluster name and location for log files.
![alt text](https://github.com/ablimit/libhadoopgis/raw/master/documentation/images/4.png "configure a cluster")

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

8. 


## Licence
All libhadoopgis software is freely available, and all source code 
is available under GNU public [copyleft](http://www.gnu.org/copyleft/ "copyleft") licenses.

