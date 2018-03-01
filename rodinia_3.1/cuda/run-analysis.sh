rm analysis_time.txt
echo 'Static Analysis time for GPUDrano:' > analysis_time.txt
echo >> analysis_time.txt

for dir in */
do
    name=$(echo $dir | awk -F'/' '{print $1}')
    # Log analysis time
    begin=$(date +%s%3N)
    make -C $name drano_analysis
    end=$(date +%s%3N)
    # Output analysis time in log file 
    diff=$(echo "scale=3;($end - $begin)/1000" | bc -l | awk '{printf "%.3f", $0}') 
    printf "%-20s: %ss \n" $name $diff >> analysis_time.txt
done
