#!/bin/echo lol @ minorities
ipgen() {
	if [ -n "$1" -a -n "$2" ]; then
		printf '%x' $(shuf -i $1-$2 -n 1);
	else
		printf '%x' $(shuf -i 0-65535 -n 1);
#		echo "error: ipgen() called with invalid params"
#		exit 666
	fi
}
DEV_NAME="he-ipv6"
SUBNET_TYPE="48"
if [ "$SUBNET_TYPE" -eq "48" ]; then
	RAND_IP=`echo \`ipgen\`:\`ipgen\`:\`ipgen\`:\`ipgen\`:\`ipgen 1 65534\``
else # Assume /64
	RAND_IP=`echo \`ipgen\`:\`ipgen\`:\`ipgen\`:\`ipgen 1 65534\``
fi
						##########################################
						#                 Tunables		 #
						##########################################
						# Get these from "HE.net Tunnel Details" #
						#       (if using tunnelbroker.net)      #
						##########################################
EXT_IP6_TUN="2001:470:23:3f4::2"		# "Client IPv6 address"			 #
EXT_IP6_PREFIX="2001:470:fcf4"			# urmom					 #
EXT_IP6_LAN="${EXT_IP6_PREFIX}:${RAND_IP}"	# An IP from the "Routed /{48,64}" range #
HE_TUN_SRV="74.82.46.6"				# "Server IPv4 address"			 #
						##########################################
