rm compile_time.txt  compile_log.txt
touch compile_log.txt
echo 'Compile time for GPUDrano:' > compile_time.txt
echo >> compile_time.txt

for dir in */
do
    name=$(echo $dir | awk -F'/' '{print $1}')
    echo '... compiling ' $name >> compile_log.txt
    # Log compilation time
    begin=$(date +%s%3N)
    make -C $name drano 2>> compile_log.txt
    end=$(date +%s%3N)
    # Output compilation time in log file 
    diff=$(echo "scale=3;($end - $begin)/1000" | bc -l | awk '{printf "%.3f", $0}' ) 
    printf "%-20s: %ss \n" $name $diff >> compile_time.txt
    echo >> compile_log.txt
    echo >> compile_log.txt
done
