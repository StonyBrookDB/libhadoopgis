#! /bin/bash

usage(){
    echo -e "tileData.sh  [options]\n \
    --input \t hdfs directory path for input data \n \
    --output \t hdfs directory path for output data \n \
    --north \t maximum coordinate value in the vertical direction [y axis] \n \
    --south \t minimum coordinate value in the vertical direction [y axis] \n \
    --east \t maximum coordinate value in the horizontal direction [x axis]\n \
    --north \t minimum coordinate value in the horizontal direction [x axis] \n \
    --xsplit \t number of splits in the horizontal direction \n \
    --ysplit \t number of splits in the vertical  direction \n \
    --geom \t index of the geometry field \n \
    --uid \t index of the uid field \n \
    --verbose \t [optional] show verbose output \n \
    --help \t show this information.
    "
    exit 1
}

# Reset all variables that might be set
inputdir=""
outputdir=""
verbose=""
west=""
south=""
north=""
east=""
xsplit=""
ysplit=""
gidx=""
uidx=""

while :
do
    case $1 in
	-h | --help | -\?)
	    usage;
	    #  Call your Help() or usage() function here.
	    exit 0      # This is not an error, User asked help. Don't do "exit 1"
	    ;;
	-w | --west)
	    west=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--west=*)
	    west=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-s | --south)
	    south=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--south=*)
	    south=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-n | --north)
	    north=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--north=*)
	    north=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-e | --east)
	    east=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--east=*)
	    east=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-x | --xsplit)
	    xsplit=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--xsplit=*)
	    xsplit=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-y | --ysplit)
	    ysplit=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--ysplit=*)
	    ysplit=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-i | --input)
	    inputdir=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--input=*)
	    inputdir=${1#*=}        # Delete everything up till "="
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
	-g | --geom)
	    gidx=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--geom=*)
	gidx=${1#*=}        # Delete everything up till "="
	    shift
	    ;;
	-u | --uid)
	    uidx=$2     # You might want to check if you really got FILE
	    shift 2
	    ;;
	--uid=*)
	uidx=${1#*=}        # Delete everything up till "="
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
if [ ! "$fidx" ]; then
    echo "ERROR: geometry field index is missing. See --help" >&2
    exit 1
fi

if [ ${fidx} -lt 1 ]; then
    echo "ERROR: geometry field index can not start a value less than 1."
    exit 1
fi

if [ ! "$inputdir" ] || [ ! "$outputdir" ]; then
    echo "ERROR: missing option. See --help" >&2
    exit 1
fi


if [ ! "$outputdir" ] || [ ! "$south" ] || [ ! "$north" ] || [ ! "$east" ] || [ ! "$west" ]; then
    echo -e "ERROR: coordinate values are not given. See --help" >&2
    exit 1
fi

if [ ! "$ysplit" ] || [ ! "$xsplit" ]; then
    echo -e "WARNING: tiling parameters (xsplit && ysplit) are not given.\t Will use defalut values (x=10, y=10)."
    xsplit=10
    ysplit=10
fi

mapprog="-w ${west}  -s ${south}  -n ${north}  -e ${east}  -x ${xsplit} -y ${ysplit} -u ${uidx} -g ${gidx}"
redprog="tilerreducer"
redtasks=20

echo -e "starting tile job\n" 
# actual job is performed here

./tiler $mapprog

# hadoop jar contrib/streaming/hadoop-streaming.jar -mapper ${mapprog} -reducer ${redprog} -file ${mapprog} -file ${redprog} -input ${inputdir} -output ${outputdir} -numReduceTasks ${reducecount} ${verbose} -jobconf mapred.job.name="hadoopgis-TileJob-${inputdir}"  -jobconf mapred.task.timeout=360000000
 
succ=$?

if [[ $succ != 0 ]] ; then
    echo -e "\n\nTiling task has failed. \nPlease check the output result for debugging."
    exit $succ
fi

echo "\n\nTiling task has finished successfully."

