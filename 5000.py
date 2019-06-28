#!/usr/bin/env python
# 5000 IRC Bot - Developed by acidvegas in Python (https://acid.vegas/trollbots)

'''
Requirements
	* Python (https://www.python.org/downloads/)
	Note: This script was developed to be used with the latest version of Python.

Information:
	This bot requires network operator privledges in order to use the SAJOIN command.
	The bot will idle in the #5000 channel and a channel defined in the config.
	Anyone who joins the #5000 channel will be force joined into 5000 random channels.
	It will announce in the channel defined in the config who joins the #5000 channel.
	The command .kills can be used to see how many people have been 5000'd.
'''

import os,random,socket,ssl,time,threading

nickserv_password='CHANGEME'
operator_password='CHANGEME'

def randstr():return ''.join(random.sample('aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ',random.randint(4,10)))
def unicode():
	msg=''
	for i in range(random.randint(150,200)):msg+=chr(random.randint(0x1000,0x3000))
	return msg
def attack(nick):
	try:
		nicklist.append(nick)
		raw(f'PRIVMSG #superbowl :I am fucking the shit out of {nick} right now...')
		current=str(int(open(kill_file).read())+1)
		with open(kill_file,'w') as kills_file:kills_file.write(current)
		for i in range(200):
			channels=','.join(('#'+randstr() for x in range(25)))
			raw(f'SAJOIN {nick} {channels}')
			raw(f'PRIVMSG #5000 :{unicode()} oh got {nick} what is happening {unicode()}')
			raw(f'PRIVMSG {nick} :{unicode()} oh got {nick} what is happening {unicode()}')
			time.sleep(0.4)
	except:pass
	finally:
		if nick in nicklist:
			nicklist.remove(nick)
def raw(msg):sock.send(bytes(msg+'\r\n','utf-8'))
kill_file=os.path.join(os.path.dirname(os.path.realpath(__file__)),'kills.log')
last=0
nicklist=list()
if not os.path.isfile(kill_file):open(kill_file,'w').write('0')
while True:
	try:
		sock=ssl.wrap_socket(socket.socket(socket.AF_INET,socket.SOCK_STREAM))
		sock.connect(('localhost',6697))
		raw(f'USER 5000 0 * :I CAN SHOW YOU THE WORLD')
		raw('NICK FUCKYOU')
		while True:
			try:
				data=sock.recv(1024).decode('utf-8')
				for line in (line for line in data.split('\r\n') if len(line.split())>=2):
					print('{0} | [~] - {1}'.format(time.strftime('%I:%M:%S'),line))
					args=line.split()
					if line.startswith('ERROR :Closing Link:'):raise Exception('Connection has closed.')
					elif args[0]=='PING':raw('PONG '+args[1][1:])
					elif args[1]=='001':
						raw('MODE FUCKYOU +BDd')
						raw('PRIVMSG NickServ IDENTIFY FUCKYOU '+nickserv_password)
						raw('OPER 5000 '+operator_password)
						raw('JOIN #superbowl')
						raw('JOIN #5000')
					elif args[1]=='401':
						if args[3] in nicklist:
							nicklist.remove(args[3])
					elif args[1]=='JOIN' and len(args)==3:
						nick=args[0].split('!')[0][1:]
						if args[2][1:]=='#5000' and nick not in ('ak','ChanServ','FUCKYOU') and len(nicklist)<3 and nick not in nicklist:
							threading.Thread(target=attack,args=(nick,)).start()
					elif args[1]=='PRIVMSG' and len(args)==4:
						if ' '.join(args[3:])[1:]=='.kills' and time.time()-last>3:
							raw('PRIVMSG #superbowl :'+open(kill_file).read())
							last=time.time()
			except (UnicodeDecodeError,UnicodeEncodeError):pass
	except:sock.close()
	finally:time.sleep(15)
