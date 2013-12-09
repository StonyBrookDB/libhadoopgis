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
Note: *If you are planning to use AWS web interface, you can ignore this section*
Amazon Elastic MapReduce Command Line Interface: [Amazon EMR CLI] (http://docs.aws.amazon.com/ElasticMapReduce/latest/DeveloperGuide/emr-
cli-install.html)
Amazon S3 Client for file manipulation and/or transfer: [s3cmd] (http://s3tools.org/s3cmd)

## Licence
All Hadopo-GIS software is freely available, and all source code 
is available under GNU public [copyleft](http://www.gnu.org/copyleft/ "copyleft") licenses.

## Example:

