#!/usr/bin/env python
# Surge IRC Flooder - Developed by acidvegas in Python (https://acid.vegas/trollbots)
# config.py

class connection:
	server    = 'irc.server.com'
	port      = 6667
	ipv6      = False
	ssl       = False
	password  = None
	channel   = '#chats'
	key       = None

class attacks:
	channel  = ['action','color','ctcp','msg','nick','notice','part','topic']
	message  = 'SURGE SURGE SURGE SURGE SURGE'
	nicklist = ['ctcp','invite','notice','private']

class throttle:
	attack      = 3
	concurrency = 3
	threads     = 100
	rejoin      = 3
	timeout     = 15