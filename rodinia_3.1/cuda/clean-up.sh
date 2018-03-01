rm *.txt 
find . -name bugs-found.txt -type f -delete

for dir in */
do
    name=$(echo $dir | awk -F'/' '{print $1}')
    make -C $name dranoclean
    make -C $name clean
done
