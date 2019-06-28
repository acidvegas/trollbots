#!/usr/bin/env python
# SpiderWeb IRC Bot - Developed by acidvegas in Python (https://acid.vegas/trollbots)

'''
Requirements
	* Python (https://www.python.org/downloads/)
	Note: This script was developed to be used with the latest version of Python.

Information
	This bot requires network operator privledges in order to use the `SAJOIN` command.
	The bot will idle in the channel defined in the config. Anyone who tries to part from the channel will be force joined back into it.
'''

import re,socket,ssl,time

nickserv_password='CHANGEME'
operator_password='CHANGEME'

def raw(msg):sock.send(bytes(msg+'\r\n','utf-8'))
while True:
	try:
		sock=ssl.wrap_socket(socket.socket(socket.AF_INET,socket.SOCK_STREAM))
		sock.connect(('localhost',6697))
		raw(f'USER spider 0 * :CAUGHT IN THE WEB')
		raw('NICK spider')
		while True:
			try:
				data=sock.recv(1024).decode('utf-8')
				for line in (line for line in data.split('\r\n') if len(line.split())>=2):
					print('{0} | [~] - {1}'.format(time.strftime('%I:%M:%S'), line))
					args=line.split()
					if line.startswith('ERROR :Closing Link:'):raise Exception('Connection has closed.')
					elif args[0]=='PING':raw('PONG '+args[1][1:])
					elif args[1]=='001':
						raw('MODE spider +BDd')
						raw('PRIVMSG NickServ IDENTIFY spider '+nickserv_password)
						raw('OPER spider '+operator_password)
						raw('JOIN #spiderweb')
					elif args[1] == 'PART' and len(args) >= 3:
						if args[2]=='#spiderweb':
							nick = args[0].split('!')[0][1:]
							raw(f'SAJOIN {nick} #spiderweb')
							time.sleep(1.5)
							raw(f'PRIVMSG #spiderweb :HA HA HA! IM A BIG ASSHOLE SPIDER AND {nick} IS CAUGHT IN MY SPIDER WEB!!!')
			except (UnicodeDecodeError,UnicodeEncodeError):pass
	except:sock.close()
	finally:time.sleep(15)
