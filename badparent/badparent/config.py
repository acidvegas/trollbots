#!/usr/bin/env python
# BadParent IRC Bot - Developed by acidvegas in Python (https://acid.vegas/trollbots)
# config.py

class connection:
	server  = 'irc.server.com'
	port    = 6667
	proxy   = None # IP:PORT Socks 5
	ipv6    = False
	ssl     = False
	vhost   = None
	channel = '#chats'
	key     = None

class cert:
	key      = None
	file     = None
	password = None

class ident:
	nickname = 'BadParent'
	username = 'badparent'
	realname = 'acid.vegas/badparent'

class login:
	network  = None
	nickserv = None

class throttle:
	concurrency = 3
	pm          = 1.5
	threads     = 100
	timeout     = 15
