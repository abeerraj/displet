#!/bin/bash

# source job pool file
. external/misc/job_pool.sh

# initialize the job pool
job_pool_init 6 0

# matlab call function
matlab () {
  job_pool_run matlab -nodisplay -nosplash -nodesktop -nojvm -singleCompThread -r "$1; exit;"
}

# compare
for img in `seq 0 194`;
do
    matlab "callMain($img)"
done

# shut down the job pool
job_pool_shutdown