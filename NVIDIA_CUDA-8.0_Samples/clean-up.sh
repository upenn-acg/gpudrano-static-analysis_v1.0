rm *.txt 
find . -name bugs-found.txt -type f -delete
find . -name *.ll -type f -delete
find . -name log_* -type f -delete

#for dir in $(find ./ -type d)
#do
#    name=$(echo $dir | awk -F'/' '{print $1}')
#    make -C $name dranoclean
#    make -C $name clean
#done
