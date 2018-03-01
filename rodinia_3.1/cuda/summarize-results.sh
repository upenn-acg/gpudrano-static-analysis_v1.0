printf "===========================================================\n"
printf "|            %-45s|\n" "GPUDrano Static Analysis Results"
printf "--" "-----------------------------------------------------------\n"

printf "| %-20s | %-20s | %-10s|\n" "Program" "Uncoalesced Accesses" "Runtime"
printf "--" "-----------------------------------------------------------\n"

timesFile="analysis_time.txt"
# Create log to write misc info.
rm analysis_log.txt
touch analysis_log.txt

#Read times file.
times=$(cat $timesFile)

for i in */
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
    perfBugs=$(grep "\-\-" $i/log_* | awk -F'--' '{print $2}' | sort -u)
    
    # Print information
    dir=$(echo $i |sed s/"\/"//)
    myFile="$dir""/bugs-found.txt"
    echo $dir "Uncoalesced bugs:" > $myFile
    printf "%s\n" "${perfBugs[@]}" >> $myFile
    bugNum=$(printf "%s\n" "${perfBugs[@]}" | wc -l)
    if [[ -z $perfBugs ]]
	then
	bugNum=0
    fi
    
    #Find the correct time for this benchmark based on the name." 
    while read line; do
	bm=$(echo $line | awk -F' :' '{print $1}')
	if [[ "$bm" == "$dir" ]]
	then
	    time=$(echo $line | awk -F' : ' '{print $2}')
	fi
    done <$timesFile

    # Print this row!
    printf "| %-20s | %-20s | %-10s|\n" $dir $bugNum $time
    printf "--" "-----------------------------------------------------------\n"

done
