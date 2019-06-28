source config.sh

rm "/home/literalka/git/muhstik/vhosts"

for (( i = 0; i < 100; i++ )); do
	if [ "$SUBNET_TYPE" -eq "48" ]; then
		RAND_IP=`echo \`ipgen\`:\`ipgen\`:\`ipgen\`:\`ipgen\`:\`ipgen 1 65534\``
	else # Assume /64
		RAND_IP=`echo \`ipgen\`:\`ipgen\`:\`ipgen\`:\`ipgen 1 65534\``
	fi
	ip -6 addr add ${EXT_IP6_PREFIX}:${RAND_IP}/3 dev ${DEV_NAME} preferred_lft 0
	echo "${EXT_IP6_PREFIX}:${RAND_IP}" >> /home/literalka/git/muhstik/vhosts
done
