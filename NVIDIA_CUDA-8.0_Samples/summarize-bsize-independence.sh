printf "=================================================================================================\n"
printf "|                          %-69s|\n"    "GPU Drano Static Analysis Results"
printf "--" "-------------------------------------------------------------------------------------------------\n"
printf "| %-35s | %-20s | %-20s | %-10s|\n" "Program" "# Kernels" "# IndpKernels" "Runtime"
printf "--" "-------------------------------------------------------------------------------------------------\n"

timesFile="analysis_time.txt"
# Create log to write misc info.
rm analysis_log.txt
touch analysis_log.txt

#Read times file.
times=$(cat $timesFile)

for i in $(find ./ -type d | sort -d)
do
    # If no log file is found skip this entry.
    exists=$(ls $i/log_* 2> /dev/null)
    returnStatus=$?

    if [[ $returnStatus -ne 0 ]]
	then
	echo "Warning: " $i "No log files found. Skipping" >> analysis_log.txt
	continue
    fi
    # Join all log files in the directory. Get rid of -- pattern. Sort and print unique entries.
    totalKernels=$(grep "Function:" $i/log_* | sort -u | wc -l)
    indepKernels=$(grep "block-size independent" $i/log_* | sort -u | wc -l) 
    
    # Print information
    dir=$i #$(echo $i |sed s/"\/"//)
    
    #Find the correct time for this benchmark based on the name." 
    while read line; do
	bm=$(echo $line | awk -F' :' '{print $1}')
	if [[ "$bm" == "$dir" ]]
	then
	    time=$(echo $line | awk -F' : ' '{print $2}')
	fi
    done <$timesFile

    # Print this row!
    printf "| %-35s | %-20s | %-20s | %-10s|\n" $dir $totalKernels $indepKernels $time
    printf "--" "-------------------------------------------------------------------------------------------------\n"

done
