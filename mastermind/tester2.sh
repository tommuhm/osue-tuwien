#! /bin/sh

#run this in shell to get test case file:
#perl -le '@c = ("b","d","g","o","r","v","w");for $a (@c){for $b(@c){for $c(@c){for $d(@c){for $e(@c){print "$a$b$c$d$e"}}}}}' >> testcasefile

port=1280

rm outputofcli.txt

while read line           
do           
    ./server $port $line &
    i=$(./client localhost $port | grep -o '[0-9]*' | cut -c 1-3)
    if [ $i -gt 7 ]
    	then
    	echo "$i $line" >> outputofcli.txt
    fi
done < perm.txt
