#!/bin/bash
touch output_data.csv
touch temp_ouput.txt
touch batch.txt
> output_data.csv

bash ~/OS_run.sh > /dev/null 2>&1

echo "\begin{table}    "
echo "\centering"
echo ""
echo "\begin{tabular}{|c|c|c|}"
echo "\hline"
echo "{\bf Scheduling Algorithm Used} & {\bf CPU Utilization} & {\bf Average Waiting Time} \\\\"
echo "\hline"

for (( i = 1 ; i <= 10 ; i=$i + 1 ))
do
#echo "doing algorithm "$i
echo $i > batch.txt
cat $1 >> batch.txt
> temp_output.txt
./nachos -F batch.txt >> temp_output.txt //Running the same batch file here
val=`grep "CPU utilization" temp_output.txt`
val=${val//[!0-9\.]/}
val1=`grep "average waiting time:" temp_output.txt`
val1=${val1//[!0-9\.]/}
echo "$i & $val  & $val1 \\\\ \\hline " 
done

rm temp_output.txt
rm batch.txt

echo "\end{tabular}"

echo "\captionof{table}{} \label{tab:}" 
echo "\end{table}"
