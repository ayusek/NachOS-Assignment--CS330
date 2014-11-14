#!/bin/bash
touch output_data.csv
touch temp_ouput.txt
> output_data.csv

for (( i = 64 ; i <= 74 ; i=$i + 1 ))
do
#Inside the for loop
echo "doing for "$i
sed -i "87s/.*/#define Q4 $i/" ../machine/stats.h
bash ~/OS_run.sh > /dev/null 2>&1
>temp_output.txt
./nachos -F getq4_batch.txt >> temp_output.txt //Running the same batch file here
val=`grep "CPU utilization" temp_output.txt`
echo $val
val=${val//[!0-9\.]/}
echo "$i,$val" >> output_data1.csv
done
rm temp_output.txt
echo "done"
