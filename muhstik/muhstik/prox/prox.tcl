# {{{ pub  [CS ] SOCKS proxy gathering off phpBB [et al] sources and {DNS,R,...}BL matching
array unset {v:sites} {}
# {{{ RBL to query
set {v:sites:rbl} [list		\
	rbl.efnet.org		\
	dnsbl.swiftbl.net	\
	ircbl.ahbl.org		\
	dnsbl.dronebl.org	\
	tor.dnsbl.sectoor.de	\
	dnsbl.njabl.org]
# }}}
array unset {v:proxies}
array unset {v:proxies:rbl}
# {{{ dcsproxy.com
array set {v:sites} [list dcsproxy [list				\
		[list "http://www.dcsproxy.com/login.php?do=login"	\
			[list						\
#				vb_login_username	"insectcrew"	\
				vb_login_lusername	"szucsy"	\
				do			"login"		\
#				vb_login_md5password	"7e7cb40bdbfd8d4c512eda43e748b3ba"		\
				vb_login_md5password	"966bf1eaaaf07917faa5192115a10988"		\
#				vb_login_md5password_utf	"7e7cb40bdbfd8d4c512eda43e748b3ba"	\
				vb_login_md5password_utf	"966bf1eaaaf07917faa5192115a10988"	\
				cookieuser		1		\
				s			""		\
			]						\
		]							\
		"http://www.dcsproxy.com/socks-lists/"			\
		"http://www.dcsproxy.com/login.php?do=logout"		\
		{<div>.+?</div>}					\
		{<a href="(.+?)"}					\
	]]
# }}}
# {{{ proxy-heaven.blogspot.com
array set {v:sites} [list proxy-heaven [list			\
		[list]							\
		"http://proxy-heaven.blogspot.com/"			\
		""							\
		{<h3 class=.+?>.+?</h3>}				\
		{(?i)href='(.+?socks.+?)'}				\
	]]
# }}}
set {v:filter} {:23|443|(?:[18]0)?8[08]|312[78]|6588|9050$}
# }}}
# {{{ pub  [CS ] SOCKS proxy gathering off phpBB [et al] sources and {DNS,R,...}BL matching
setudef flag socks
array unset {v%cookies}

# {{{ bind  evnt  -|- loaded v%socks:ini
bind evnt -|- loaded v%socks:ini
proc v%socks:ini args {
	global {v:sites}
	foreach {- - tid}	\
		[join [lsearch -all -inline -regexp [utimers] {v%socks:daily}]] {
		killutimer ${tid} }
	foreach t [array names {v:sites}] {
		foreach c [channels] {
			if {![channel get ${c} socks]} { continue }
			catch {utimer 86400 [list v%socks:daily ${c} ${t}]}	;# XXX tunable
		}
	}
}
# }}}
# {{{ utimer 86400 [list v%socks:daily [ ... ]]
proc v%socks:daily {c t} {
	catch {v:socks * * * ${c} ${t}}
	utimer 86400 [list v%socks:daily ${c} ${t}]
}
# }}}
# {{{ bind  pub  S|- .socks v:socks
bind pub S|- .socks v:socks
proc v:socks {n u h c t} {
	global {v:sites} {v:proxies} {v:filter} {v%cookies}
	if {![channel get ${c} socks]} { return }
	if {![string length [set site [last [array get {v:sites} ${t}]]]]} {
		v%blog "[v%ifx johne] unknown site `${t}'" ${c}; return }

	set llogin [lindex ${site} 0]; set turl [lindex ${site} 1]; set lourl [lindex ${site} 2]
	set tp0 [lindex ${site} 3]; set tp1 [lindex ${site} 4]
	if { [llength ${llogin}] && \
	    ![string length [last [array get {v%cookies} ${site}]]]} {
		set lurl [lindex ${llogin} 0]; set lbody [lindex ${llogin} 1]
		v%fetch -cookies POST ${t} ${lurl} ${lbody} }

	set socks {}
	foreach thr [regexp -all -inline ${tp0} [v%fetch -- GET ${t} ${turl}]] {
		if {[regexp -- ${tp1} ${thr} {} thrurl]} {
			set thtml [v%fetch -- GET ${t} ${turl}]
			foreach {- s} [regexp -all -inline {\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}:\d+} ${thtml}] {
				lappend socks ${s}
			}
		}
	}

	set socks [lsearch -not -all -inline -regexp ${socks} ${v:filter}]
	if {![llength ${socks}]} { v%blog "[v%ifx johne/${t}] No socks found. FML" ${c} }\
	else { v%blog "[v%ifx johne/${t}] Done. [llength ${socks}] socks found." ${c} }
	array set {v:proxies} [list ${t} ${socks}]
	if {[string length ${lourl}]} { catch {v%fetch -cookies GET ${lourl}}; }
	return 1
}
# }}}
# {{{ bind  pub  S|- .matchbl v:socks:matchbl
bind pub S|- .matchbl v:socks:matchbl
proc v:socks:matchbl {n u h c t {zidx 0}} {
	global {v:proxies} {v:proxies:rbl} {v:sites:rbl}
	if {![channel get ${c} socks]} { return }
	if {${zidx} == 0} { array set {v:proxies:rbl} [list ${t} [concat [last [array get {v:proxies} ${t}]]]] }
	if {![llength [set proxies [last [array get {v:proxies:rbl} ${t}]]]]} {
		v%blog "[v%ifx johne/rbl] don't have n e proxies for `${t}'" ${c}; return }

	if {![string length [set zone [lindex ${v:sites:rbl} ${zidx}]]]} { return }
	set nproxies [llength ${proxies}]; set n 0
	v%blog "[v%ifx johne/rbl] ${t}: Matching ${nproxies} proxies against ${zone}" ${c}
	array set {v:proxies:rbl} [list ${t} {}]
	foreach ip ${proxies} {
		incr n
		if {[catch {set ha [apply format [join [list %d.%d.%d.%d [lreverse [scan ${ip} %d.%d.%d.%d]]]]].${zone}}] != 0} {
			if {${n} >= ${nproxies}} {
				v%dns:rbl - - - ${c} ${t} - ${zone} [incr zidx] [list ${n} ${h} ${u} ${c} ${t}]
			}
		} elseif {${n} >= ${nproxies}} {
			dnslookup ${ha}							\
				v%dns:rbl	${c} ${t} ${ip} ${zone} [incr zidx]	\
						[list ${n} ${h} ${u} ${c} ${t}]
		} else { dnslookup ${ha} v%dns:rbl ${c} ${t} ${ip} }
	}
}
# }}}

# {{{ Ancillary
proc v%fetch {{cookies ""} {method ""} site url {body ""}} {
	global {v%cookies}
	set method [string tolower ${method}]
	set cookie [last [array get {v%cookies} ${site}]]
	# N.B.	Tcl is the most worthless language there is
	set html [apply	::httpx::${method}	\
			[list -headers ${url} [list Cookie [list ${cookie}]] ${body}]]
	set status [lindex ${html} 0]

	if {[string first 20 ${status} 0] != 0 && \
	    [string first 30 ${status} 0] != 0} { return -code return }	;# TODO
	if {${cookies} == "-cookies"} {
		set hdr [lindex ${html} 1]; set cookies ""
		foreach idx [lsearch -all -regexp ${hdr} {Set-Cookie}] {
			append cookies "[lindex ${hdr} [expr ${idx} + 1]]; "
		}; if {[string length ${cookies}]} {
			array set {v%cookies} [list ${site} ${cookies}] }
	}; last ${html}
}

proc v%dns:rbl {ip host s c t oip {zone ""} {last 0} {pargs ""}} {
	global {v:proxies:rbl}
	if {${last}} {
		v%dns:rbl:fini ${t} ${c} ${zone}; v:socks:matchbl - - - ${c} ${t} ${last}
	} elseif {!${s}} { array set {v:proxies:rbl} [list ${t} [concat [last [array get {v:proxies:rbl} ${t}]] ${oip}]] }
}

proc v%dns:rbl:fini {t c zone} {
	global {v:proxies:rbl}
	set n [llength [last [array get {v:proxies:rbl} ${t}]]]
	v%blog "[v%ifx johne/rbl] ${t}!${zone}: ${n} unlisted." ${c}
}
# }}}
# }}}
# }}}
