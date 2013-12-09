#! /bin/bash

usage(){
    echo -e "spjoin-amazonemr.sh  [options]\n \
    --inputa \t hdfs directory path for join input data A (table a )\n \
    --inputb \t hdfs directory path for join input data B (table b )\n \
    --output \t hdfs directory path for output data \n \
    --predicate \t join predicate [contains | intersects | touches | crosses | within | dwithin] \n \
    --geoma \t index of the geometry field of table A\n \
    --geomb \t index of the geometry field \n \
    --worker \t number of reduce tasks to utlize \n \
    --verbose \t [optional] show verbose output \n \
    --help \t show this information.
    "
    exit 1
}

# Reset all variables that might be set
# path to the elastic-mapreduce CLI
enginepath=~/elastic-mapreduce
ldpath=/usr/local/lib:/usr/lib:$LD_LIBRARY_PATH
reducecount=20
inputdira=""
inputdirb=""
outputdir=""
verbose=""
predicate=""
gidxa=""
gidxb=""

while :
do
    case $1 in
	-h | --help | -\?)
	    usage;
	    #  Call your Help() or usage() function here.
	    exit 0      # This is not an error, User asked help. Don't do "exit 1"
	    ;;
	-n | --worker)
	    reducecount=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--worker=*)
	    reducecount=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-p | --predicate)
	    predicate=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--predicate=*)
	    predicate=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-i | --inputa)
	    inputdira=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--inputa=*)
	    inputdira=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-j | --inputb)
	    inputdirb=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--inputb=*)
	    inputdirb=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-o | --output)
	    outputdir=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--output=*)
	    outputdir=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-g | --geoma)
	    gidxa=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--geoma=*)
	gidxa=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-h | --geomb)
	    gidxb=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--geomb=*)
	gidxb=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-v | --verbose)
	    # Each instance of -v adds 1 to verbosity
	    verbose="-verbose"
	    shift
	    ;;
	--) # End of all options
	    shift
	    break
	    ;;
	-*)
	    echo "WARN: Unknown option (ignored): $1" >&2
	    shift
	    ;;
	*)  # no more options. Stop while loop
	    break
	    ;;
    esac
done

# Suppose some options are required. Check that we got them.
if [ ! "$predicate" ] ; then
    echo "ERROR: join predicate is missing. See --help" >&2
    exit 1
fi

if [ ! "$gidxa" ] || [ ! "$gidxb" ] ; then
    echo "ERROR: geometry field index is missing. See --help" >&2
    exit 1
fi


if [ ! "$inputdira" ] || [ ! "$inputdirb" ] || [ ! "$outputdir" ]; then
    echo "ERROR: missing option. See --help" >&2
    exit 1
fi

randomfilename=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)
tempdir=s3://cciemory/hadoopgis/output/${randomfilename}

reducecmd="resque st_${predicate} ${gidxa} ${gidxb}"

ruby ${enginepath}/elastic-mapreduce --create --stream --args --cacheFile "${inputdirb}#hgskewinput" --alive --num-instances=4 --enable-debugging --log-uri s3://cciemorylog/ --master-instance-type=m1.medium  --name "Joiner MR"  --mapper s3://cciemory/program/tagmapper.py ${inputdira} --reducer "s3://cciemory/program/${reducecmd}" --input ${inputdira} --output ${tempdir}  --jobconf mapred.reduce.tasks=${reducecount} --bootstrap-action "s3://cciemory/bootstrap/bootcopygeosspatial.sh"

# If the class path becomes an issue, manually set the environment variable: -cmdenv LD_LIBRARY_PATH=${ldpath}

