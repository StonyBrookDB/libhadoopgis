#! /bin/bash

if [ ! $# == 1 ]; then
    echo "Usage: $0 [log_id]"
    exit 0
fi

optinput="-input /user/aaji/knn/cell -input /user/aaji/knn/bv/tilePartition"
OUTDIR=/data2/ablimit/hadooplog


# reco=$(date +%F-%k-%M)
reco=$1

sudo -u hdfs hdfs dfs -rm -r /user/aaji/knnout

for reducecount in 200 180 160 140 120 100 80 60 40 20 10
do
    START=$(date +%s)

    sudo -u hdfs hadoop jar hadoop-streaming-2.0.0-mr1-cdh4.0.0.jar -mapper tMapper -reducer tReducer -file tMapper -file tReducer ${optinput} -output /user/aaji/knnout -numReduceTasks ${reducecount} -verbose -cmdenv LD_LIBRARY_PATH=/home/aaji/softs/lib:$LD_LIBRARY_PATH -jobconf mapred.job.name="vor_tile_${reducecount}"  -jobconf mapred.task.timeout=36000000


    END=$(date +%s)
    DIFF=$(( $END - $START ))
    echo "$1,${reducecount},${DIFF}" >> voronoi.tile.${reco}.log

    # sudo -u hdfs hdfs dfs -copyToLocal /user/aaji/joinout ${OUTDIR}/mjoin_${1}_${reducecount}
    sudo -u hdfs hdfs dfs -rm -r /user/aaji/knnout

done

