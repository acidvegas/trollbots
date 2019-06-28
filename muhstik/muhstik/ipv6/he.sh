ipv4addr="AUTO"
_tunnelid=""
md5pass="" # echo -n ${PW} | md5sum
userid=""

tunnel_detail="http://ipv4.tunnelbroker.net/tunnel_detail.php?tid=${_tunnelid}" # &ajax={true,false}
nsupdate="http://ipv4.tunnelbroker.net/nsupdate.php?tid=${_tunnelid}"
ipv4_end="https://ipv4.tunnelbroker.net/ipv4_end.php?ip=${ipv4addr}&pass=${md5pass}&apikey=${userid}&tid=${_tunnelid}"
# -or-: https://USERNAME:PASSWORD@ipv4.tunnelbroker.net/ipv4_end.php?tid=TUNNELID (auto-detect IP)
#       https://USERNAME:PASSWORD@ipv4.tunnelbroker.net/ipv4_end.php?tid=TUNNELID&ip=IPV4ADDR
