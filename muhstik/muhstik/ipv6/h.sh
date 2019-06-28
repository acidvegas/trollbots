#!/bin/echo lol @ minorities
# $Id: h.sh 11 2011-05-29 literalka  $
#
# {{{ Relevant WWW site links:
#	[0] HOWTO
#	[0a] http://mirrors.bieringer.de/Linux+IPv6-HOWTO/
#	[1] Wiki
#	[1a] http://en.wikipedia.org/wiki/IPv6
#	[1b] http://en.wikipedia.org/wiki/IPv6_address
#	[1c] http://en.wikipedia.org/wiki/Tunneling_protocol
#	[1d] http://en.wikipedia.org/wiki/List_of_IPv6_tunnel_brokers
#	[2] RFC
#	[2a] http://tools.ietf.org/html/rfc2460
#	[2b] http://tools.ietf.org/html/rfc3053
#	[2c] http://tools.ietf.org/html/rfc3964
#	[3] Tunnel Brokers
#	[3a] http://www.tunnelbroker.net/
#	[3b] http://tbroker.mybsd.org.my/
#	[4] Amusing
#	[4a] http://blogs.pcmag.com/securitywatch/2010/12/ipv6_will_worsen_the_spam_and.php
#	[4b] http://jl.ly/Email/v6bl.html
#	[5] DNS
#	[5a] http://member.wide.ad.jp/~fujiwara/v6rev.html
#	[5b] http://freedns.afraid.org/
#	[5c] http://dns.he.net/
#	[5d] http://freedns.afraid.org/reverse/instructions.php
#	[6] VPS Hosts
#	[6a] http://www.hostgator.com/
# }}}
# {{{ Notes:
# 1) Add ``EXT_IP6_TUN'' IP with /3 so it auto-adds a route to 2000::/3 to the
#	tunnel device so that there is no need to explicitly add that route.
# 2) Add ``EXT_IP6_LAN'' IP with /3 so the route to the LAN prefix/64 is not
#	auto-added to the tunnel device.
# 3) You could also add IPs with /128, but adding IPs with /3 auto-adds a route
#	to 2000::/3 to the tunnel device, and with the ``EXT_IP6_TUN'' IP having
#	that route auto-added already, the routing table is kept clean.
# 4) Last IP address added with "preferred_lft forever" is used as the default
#	IP for new outgoing connections.
# 5) That being said, "preferred_lft x" is optional, defaulting to
#	"preferred_lft forever".
# 6) Add additional IPs using:
#       ip -6 addr add 2001:470:___:___::___/3 dev $DEV_NAME
# 7) Do not add any CIDRs, just add "straight up" IPs.
# }}}
# {{{ Suggestions:
# 1) If using tunnelbroker [3a], delegate the rDNS to ``dyn.ip6arpa.co.cc'', it
#	/should/ give proper forward and reverse DNS, for "max sperg". Make sure
#	it works first though, (i.e. freenode's DNS probably sucks).
# 2) <@h> ipv6 youtube + /48 = lol viewcounts
# 3)  "...if you sent a billion messages a second, each with its own address,
#	it would take about a thousand years to use all the addresses in a
#	/64" [4b], thus, I would suggest sending one billion messages per second
#	using different IP addresses.
# 4) Run your own rDNS (see [5a], requires root or sudo(8))
# 5) If you have 0.01 USD on Paypal and a valid US phone number, just get a
#	Hostgator [6a] VPS with 4 ips on it using promo code "austin" (dunno how
#	long this will be valid), you'd probably be able to run a DNS server on
#	it, with 4 IPs.
# 6) If you use FreeDNS [5b], you can set up a ``vhost'' by delegating your rDNS
#	to FreeDNS' servers and setting up an ``IPv6 Reverse'' [5d].
# 7) 19:07:20 <+h> I'm still trying to get ARIN to give me 14:88:/32
#	19:07:24 <+h> would be awesome as fuck
# 8) IPv6 botnets are relatively unknown, based on my own experience. I suggest
#	fixing this with an "IPv6 Awareness" program of somesort.
# 9) Try to use as many "levels" (TODO: find the right word for this) of an IPv6
#	IP as possible: when banning, many channel operators will ban a single
#	IP, others will ban 2001:470:* (all of HE.net), and even some others
#	will ban, say, a /64 when you're {ab,}using a /48, leaving unbanned IP
#	addresses. 
# }}}
# {{{ `telnet(1) route-server.he.net`
# [AS6939/HURRICANE-IPV6]
# ``tunnelbroker.net'' tunnel server information. Current as of 2011-03-29.
#
# Location                  IPv4                  IPv6
#---------------------     ----------------      ------------------------
# North America
#  PAIX Seattle             216.218.252.176       2001:470:0:3d::1
#  PAIX Palo Alto           216.218.252.165       2001:470:0:1b::1
#  Equinix San Jose         216.218.252.164       2001:470:0:1a::1
#  Hurricane Fremont 1      216.218.252.161       2001:470:0:23::1
#  Hurricane Fremont 2      216.218.252.162       2001:470:0:24::1
#  Hurricane San Jose       216.218.252.163       2001:470:0:19::1
#  Equinix Los Angeles      216.218.252.166       2001:470:0:1c::1
#  One Wilshire Los Angeles 216.218.252.178       2001:470:0:6c::1
#  Equinix Chicago          216.218.252.168       2001:470:0:16::1
#  Equinix Dallas           216.218.252.167       2001:470:0:1d::1
#  PAIX Toronto             216.218.252.147       2001:470:0:99::1
#  Telehouse New York       216.218.252.170       2001:470:0:12::1
#  PAIX New York            216.218.252.171       2001:470:0:13::1
#  TelX New York            216.218.252.148       2001:470:0:9f::1
#  Equinix Ashburn          216.218.252.169       2001:470:0:17::1
#  TelX Atlanta             216.218.252.150       2001:470:0:a7::1
#  NOTA Miami               216.218.252.177       2001:470:0:4a::1
#  Telx Phoenix             216.218.252.156       2001:470:0:154::1
#  Pittock Portland         216.218.252.159       2001:470:0:157::1
#  Comfluent Denver	    216.218.252.158	  2001:470:0:155::1
#  Level3 Kansas City	    216.218.252.157	  2001:470:0:156::1
#  Oak Tower Kansas City    216.218.252.181	  2001:470:0:178::1
#  Minnesota Gateway	    216.218.252.185	  2001:470:0:ab::1
# Europe
#  Telehouse London         216.218.252.172       2001:470:0:d::1
#  NIKHEF Amsterdam         216.218.252.173       2001:470:0:e::1
#  Interxion Frankfurt      216.218.252.174       2001:470:0:2a::1
#  Interxion Paris          216.218.252.175       2001:470:0:2b::1
#  Telehouse Paris	    216.218.252.184 	  2001:470:0:1ae::1
#  Equinix Zurich           216.218.252.153       2001:470:0:10c::1
#  TeleCity Stockholm       216.218.252.154       2001:470:0:10f::1
# Asia
#  Mega-I Hong Kong         216.218.252.180       2001:470:0:c2::1
#  Equinix Tokyo            216.218.252.151       2001:470:0:10a::1
#  Equinix Singapore	    216.218.252.179	  2001:470:0:169::1
# }}}
# {{{ Prerequisites and supported platforms:
#  Tested on:   bash(1) 4.0.33(1)-release on Ubuntu 2.6.31-23.74-generic
#  Requires:    bash(1), test(1), printf(1), echo(1), and ip(8)
#  May Need:    su(1), sudo(8), modprobe(8)
# }}}
# {{{ rcslog
# $Log: h.sh $
# Revision 11 2011/05/29 21:44:23  literalka
# Move config into its own file
#
# Revision 10 2011/04/27 01:10:19  literalka
# Added some small fucking retarded change in some obscure comment somewhere
# Added two more small fucking useless changes in an even more obscure comment
#	And then I added a small stupid fucking change to one of those comments
#
# Revision 9 2011/04/21 22:52:04  literalka
# Small documentation updates
#
# Revision 8   2011/03/29 13:02:45  literalka
# Updated `telnet(1) route-server.he.net`
# Updated "Tested on"
#
# Revision 7   2011/02/10 12:21:45  literalka
# Add adjustable params to ipgen()
# Check for ``SUBNET_TYPE''
#
# Revision 6   2011/02/09 22:13:12  literalka
# Wrote ipgen(), replacing use of ``RANDOM''
#
# Revision 5   2011/02/09 20:08:12  literalka
# `telnet(1) route-server.he.net`
#
# Revision 4   2011/02/09 15:32:18  literalka
# Require bash(1)
#
# Revision 3   2011/02/04 12:56:24  literalka
# Replaced all instances of "he-ipv6" with ``DEV_NAME''
#
# Revision 2   2011/02/03 19:15:48  literalka
# rm useless "if" statement
#
# Revision 1   2011/01/22 18:05:48  literalka
# Initial revision
# }}}
#  Last update: Wed Apr 27 2011
#   -- by Leon Kaiser of the GNAA
#	<literalka@gnaa.eu>
if [ -z "${BASH_VERSION}" ]; then
	echo "error: use bash(1)" # require bash(1)
	exit 1488
fi
#modprobe ipv6

source config.sh

ip tunnel add $DEV_NAME mode sit remote $HE_TUN_SRV ttl 255
ip link set $DEV_NAME up
ip -6 addr add $EXT_IP6_TUN/3 dev $DEV_NAME preferred_lft 0
ip -6 addr add $EXT_IP6_LAN/3 dev $DEV_NAME preferred_lft forever
echo "Added route to IP ${EXT_IP6_LAN}"
# tunnelbroker.net includes the following lines in their suggested
#  "Linux-route2" config...
#	ip route add ::/0 dev he-ipv6
#	ip -f inet6 addr
