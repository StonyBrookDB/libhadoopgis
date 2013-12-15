#! /bin/bash

# small cluster to process tweet filter task 
/usr/local/emrcli/elastic-mapreduce --create --alive --stream --enable-debugging --num-instances=2 --instance-type=m1.medium --master-instance-type=m1.medium --name 'tweetSearcher'  --bootstrap-action 's3://aaji/scratch/awsjoin/bootcopy.sh' --region us-east-1 --log-uri 's3://aaji/scratch/logs' --with-termination-protection --key-pair aaji --mapper 's3://aaji/scratch/deps/bins/skewresque st_contains 2 1 ' --reducer 's3://aaji/scratch/awsjoin/hgdeduplicater.py cat' --input "s3://aaji/scratch/data/tweet.dump.tsv" --output "s3://aaji/scratch/temp/tweet" --cache "s3://aaji/scratch/data/atl.stores.tsv#hgskewinput" 

# --instance-group master --instance-type m1.medium --instance-count 1 \
# --instance-group core   --instance-type m1.medium --instance-count 5  --bid-price 0.028 \
# --instance-group task   --instance-type c1.xlarge --instance-count 14 --bid-price 0.028

