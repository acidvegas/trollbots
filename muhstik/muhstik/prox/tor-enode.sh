#!/bin/sh
# <http://www.sectoor.de/tor.php>
#  -- by vxp
#

hname="${1:?missing hostname}"
nick="${2:?missing nickname}"

for _addr in `dnsip ${hname} 2>/dev/null`
do
	host -t TXT `echo "${_addr}" |\
		     awk -F. '{print $4 "." $3 "." $2 "." $1; }'`.tor.dnsbl.sectoor.de	\
		>/dev/null 2>&1

	[ $? -eq 0 ] && { echo "${2} is a Tor using faggot (listed in sectoor's Tor DNSBL)"; };
done

exit 1;

# vim:ts=8 sw=8 noexpandtab

