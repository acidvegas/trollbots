[ $# -ne 1 ] && { print "Try again."; exit 1; }
tmpfile="shuffled.tmp"
shuf $1 > $tmpfile
mv $tmpfile $1 
