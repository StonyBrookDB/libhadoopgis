#! /bin/bash

usage(){
    echo -e "spjoin.sh  [options]\n \
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
enginepath=/usr/local/bin/resque
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
tempdir=/tmp/${randomfilename}

reducecmd="resque st_${predicate} ${gidxa} ${gidxb}"

echo -e "starting the job\n" 
echo -e "--------------[join stage]----------------\n" 
# actual job is performed here

hadoop jar contrib/streaming/hadoop-streaming.jar -mapper 'tagmapper.py ${inputdira} ${inputdirb}' -reducer '${reducecmd}' -file tagmapper.py -file ${enginepath} -input ${inputdira} -input ${inputdirb} -output ${tempdir} -numReduceTasks ${reducecount} ${verbose} -jobconf mapred.job.name="hadoopgis-joinjob-${inputdira}-${inputdirb}" -jobconf mapred.task.timeout=360000000 -cmdenv LD_LIBRARY_PATH=${ldpath}

succ=$?

if [[ $succ != 0 ]] ; then
    echo -e "\n\njoin task has failed. \nPlease check the output result for debugging."
    exit $succ
fi

# deduplication job 
echo -e "--------------[dedup stage]----------------\n" 

hadoop jar contrib/streaming/hadoop-streaming.jar -mapper 'cat -' -reducer 'uniq ' -input ${tempdir} -output ${outputdir} -numReduceTasks ${reducecount} ${verbose} -jobconf mapred.job.name="hadoopgis-joinjob-dedup" -jobconf mapred.task.timeout=360000000

succ=$?

if [[ $succ != 0 ]] ; then
    echo -e "\n\n deduplication stage has failed. \nPlease check the output result for debugging."
    exit $succ
fi

hadoop fs -rmr ${tempdir}

echo "\n\njoin task has finished successfully."

