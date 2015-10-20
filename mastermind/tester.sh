#! /bin/sh

port=30000;

i=0;
filename=perm.txt

 
while read -r code
do
	#echo $i;
	((port++));
	./server $port $code >> output.txt &
	#sleep 1
	./client localhost $port > /dev/null 2>&1 &
	
done < "$filename"
wait



#for i in `seq 0 $(echo '8^5-1' | bc)`;
#do 
	#echo $i;

	#0 = bbbbb
	#1 = bbbbc

	#./server $port 

#done

#perl -le '@c = ("b", "d", "g", "o", "r", "s", "v", "w");
#          for $a (@c){for $b(@c){for $c(@c){for $d(@c){for $e(@c){
#            print "$a$b$c$d$e"}}}}}' >> perm.txt