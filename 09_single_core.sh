#!/bin/bash

echo ""
echo "Job start time:"  
date
echo ""
echo "Removing (*)_heads_export.dat files and feflow_antamina.log from last run"
rm -f /home/axa/feflow_files/import+export/*t.*
rm -f /home/axa/feflow_files/feflow_antamina.log

inc=1
model_count=2

for ((x=1 ; x<=model_count; x++ )); do
	echo "-------------------------- START OF SIMULATION $x ---------------------------"
	args="-log feflow_antamina.log -work /home/axa/feflow_files $x.fem"
	feflow70c $args 
	echo""
	echo "************************** END OF SIMULATION $x *****************************"
	echo ""

newname=$(($inc + $x))
mv /home/axa/feflow_files/femdata/$x.fem  /home/axa/feflow_files/femdata/$newname.fem
done

echo "Renaming model from $x to initial name 1.fem"
mv /home/axa/feflow_files/femdata/$x.fem  /home/axa/feflow_files/femdata/1.fem
echo ""
echo "Job end time: date"  
date
echo ""
echo "END of batch runs"
