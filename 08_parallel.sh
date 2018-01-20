#!/bin/bash

# 1 VARIABLES (modify only model_count, simul_jobs & work_path)
model_count=9
simul_jobs=3
work_path='/home/axa/feflow_files'
switch=1

# 2 START TIME
echo ""; echo "Job start time: "; date; echo ""

# 3 DELETE LAST SIMULATION EXPORT AND LOG FILES
echo "Removing (*)_heads_export.dat files and LOG files from last run"
rm -f "$work_path"/import+export/*t.*
rm -f "$work_path"/feflow_parallel.log

# 4 BIG LOOP
# 4.1 CONCATENATE FEM FILE NAMES AND ARGUMENTS TO GENERATE FEFLOW PARALLEL JOBS
for (( x=1 ; x<=$(($model_count/$simul_jobs)); x++ )); do   # number of parallel batches
  if [[ "$switch" == 1 ]]
    then
      for (( x1=1 ; x1 <= simul_jobs ; x1++)); do
        args='feflow70c -log feflow_parallel.log -work '$work_path' '$x1'.fem'
        name_vector[$x1]=$x1
        command="$command $args &"
        switch=$(($switch+1))
      done
    else
      for (( x2=1 ; x2 <= simul_jobs ; x2++)); do
        args='feflow70c -log feflow_parallel.log -work '$work_path' '${name_vector[$x2]}'.fem'
        command="$command $args &"
      done       
  fi
# 4.2 FEFLOW PARALLEL BATCH RUN 
  echo "";  echo "--------------------- START OF PARALLEL BATCH $x ---------------------"; echo ""
  
  command="$command wait"
  echo command = $command
  eval $command
# echo "running simulations ${name_vector[0]}.fem and ${name_vector [1]}.fem"
  command=''
 
  echo ""; echo "********************* END OF PARALLEL BATCH $x ************************"; echo ""
  
# 4.3 SET NEW NAMES FOR NEXT PARALLEL BATCH RUN

  for (( x3=1 ; x3<=simul_jobs ; x3++ )); do
    newname=$((${name_vector[x3]} + $simul_jobs))
    mv "$work_path"/femdata/${name_vector[x3]}.fem "$work_path"/femdata/$newname.fem
    name_vector[x3]=$newname
  done

done  #from line 17 END OF BIG LOOP

# 5 SET ORIGINAL NAMES FOR NEXT RUN
echo "Renaming models to initial names"

for (( x3=1 ; x3<=simul_jobs ; x3++ )); do
  echo "setting name from ${name_vector[x3]}.fem to $x3.fem" 
  mv "$work_path"/femdata/${name_vector[x3]}.fem "$work_path"/femdata/$x3.fem
done

# 6 END TIME
echo ""; echo "Job END time: "; date; echo ""
echo "parallel.sh FILE END"
