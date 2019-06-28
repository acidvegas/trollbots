#!/usr/bin/env python
# Jupiter IRC Botnet - Developed by acidvegas in Python (https://acid.vegas/jupiter)

'''
for python 3.6.4 nerds with classic debian problems LOL
'''

import random
import re
import socket
import ssl
import time
import threading

# Connection
servers = (
	'efnet.port80.se',       # IPv6
	'efnet.portlane.se',     # IPv6
	'irc.choopa.net',        # IPv6
	'irc.colosolutions.net',
	'irc.du.se',
	'irc.efnet.fr',          # IPv6
	'irc.efnet.nl',
	'irc.homelien.no',       # IPv6
	'irc.inet.tele.dk',      # IPv6
	'irc.mzima.net',         # IPv6
	'irc.nordunet.se',       # IPv6
	'irc.prison.net',
	'irc.underworld.no',     # IPv6
	'irc.servercentral.net'
)
ipv6    = False
vhosts  = None # Use (line.rstrip() for line in open('vhosts.txt','r').readlines() if line) for reading from a file.
channel = '#jupiter'
key     = None

# Settings
admin = 'nick!user@host' # Can use wildcards (Must be in nick!user@host format)
id    = 'TEST'

# Formatting Control Characters / Color Codes
bold        = '\x02'
italic      = '\x1D'
underline   = '\x1F'
reverse     = '\x16'
reset       = '\x0f'
white       = '00'
black       = '01'
blue        = '02'
green       = '03'
red         = '04'
brown       = '05'
purple      = '06'
orange      = '07'
yellow      = '08'
light_green = '09'
cyan        = '10'
light_cyan  = '11'
light_blue  = '12'
pink        = '13'
grey        = '14'
light_grey  = '15'

def color(msg, foreground, background=None):
	return '\x03{0},{1}{2}{3}'.format(foreground,background,msg,reset) if background else '\x03{0}{1}{2}'.format(foreground,msg,reset)

def debug(msg):
	print('{0} | [~] - {1}'.format(get_time(), msg))

def error(msg, reason=None):
	print('{0} | [!] - {1} ({2})'.format(get_time(), msg, reason)) if reason else print('{0} | [!] - {1}'.format(get_time(), msg))

def get_time():
	return time.strftime('%I:%M:%S')

def is_admin(ident):
	return re.compile(admin.replace('*','.*')).search(ident)

def random_nick():
	prefix = random.choice(['st','sn','cr','pl','pr','qu','br','gr','sh','sk','kl','wr']+list('bcdfgklmnprstvwz'))
	midfix = random.choice(('aeiou'))+random.choice(('aeiou'))+random.choice(('bcdfgklmnprstvwz'))
	suffix = random.choice(['ed','est','er','le','ly','y','ies','iest','ian','ion','est','ing','led']+list('abcdfgklmnprstvwz'))
	return prefix+midfix+suffix

class clone(threading.Thread):
	def __init__(self, server, vhost):
		self.bots     = list()
		self.monlist  = list()
		self.hostlist = list()
		self.nickname = random_nick()
		self.server   = server
		self.sock     = None
		self.vhost    = vhost
		threading.Thread.__init__(self)

	def run(self):
		self.connect()

	def connect(self):
		try:
			self.create_socket()
			self.sock.connect((server, 6697))
			self.raw('USER {0} 0 * :{1}'.format(random_nick(), random_nick()))
			self.nick(self.nickname)
		except socket.error as ex:
			error('Failed to connect to IRC server.', ex)
			self.event_disconnect()
		else:
			self.listen()

	def create_socket(self):
		if ipv6:
			check = [ip[4][0] for ip in socket.getaddrinfo(server,6667) if ':' in ip[4][0]]
			family = socket.AF_INET6 if check else socket.AF_INET
		else:
			family = socket.AF_INET
		self.sock = socket.socket(family, socket.SOCK_STREAM)
		if self.vhost:
			self.sock.bind((self.vhost,0))
		self.sock = ssl.wrap_socket(self.sock)

	def event_connect(self):
		if self.monlist:
			self.monitor('+', self.monlist)
		self.join_channel(channel, key)

	def event_disconnect(self):
		self.sock.close()
		time.sleep(86400+random.randint(1800,3600))
		self.connect()

	def event_end_of_names(self, chan):
		pass

	def event_kick(self, nick, chan, kicked):
		if kicked == nickname:
			time.sleep(random.randint(5,15))
			self.join_channel(channel, key) if chan == channel else self.join_channel(chan)

	def event_nick(self, nick, new_nick):
		if nick == self.nickname:
			self.nickname = new_nick
			if new_nick in self.monlist:
				self.monitor('C')
				self.monlist = list()
		elif nick in self.monlist:
			self.nick(nick)

	def event_nick_in_use(self, nick, target_nick):
		if nick == '*':
			self.nickname = random_nick()
			self.nick(self.nickname)

	def event_message(self, ident, nick, target, msg):
		if is_admin(ident):
			args = msg.split()
			if args[0] in ('@all',self.nickname):
				if len(args) >= 2:
					if args[1] == 'id':
						self.sendmsg(target, id)
					elif args[1] == 'op' and self.bots and target[:1] == '#' and args[0] == self.nickname:
						for item in [self.bots[i:i + 4] for i in range(0, len(self.bots), 4)]:
							self.mode(target, '+{0} {1}'.format('o'*len(item), ' '.join(item)))
					elif args[1] == 'sync':
						self.bots = list()
						self.raw('NAMES ' + channel)
				if len(args) >= 3:
					if args[1] == 'raw':
						data = ' '.join(args[2:]).replace('%n', self.nickname)
						self.raw(data)
					elif args[1] == 'rawd':
						data = ' '.join(args[2:]).replace('%n', self.nickname)
						threading.Thread(target=self.raw_delay, args=(data,)).start()
					elif args[1] == 'host' and len(args) == 3: # Don't know what we are doing with this yet
						if args[2] == 'list' and self.hostlist:
							self.sendmsg(target, '[{0}] {1}'.format(color('Hostmasks', light_blue), ', '.join(self.hostlist)))
						elif args[2] == 'reset' and self.hostlist:
							self.sendmsg(target, '{0} nick(s) have been {1} from the host list.'.format(color(str(len(self.hostlist)), cyan), color('removed', red)))
							self.hostlist = list()
						elif args[2][:1] == '+':
							hosts = [host for host in set(args[2][1:].split(',')) if host not in self.hostlist]
							if hosts:
								self.hostlist += hosts
								self.sendmsg(target, '{0} nick(s) have been {1} to the host list.'.format(color(str(len(hosts)), cyan), color('added', green)))
						elif args[2][:1] == '-':
							hosts = [host for host in set(args[2][1:].split(',')) if host in self.hostlist]
							if hosts:
								for host in hosts:
									self.hostlist.remove(host)
								self.sendmsg(target, '{0} nick(s) have been {1} from the host list.'.format(color(str(len(hosts)), cyan), color('removed', red)))
					elif args[1] == 'monitor' and len(args) == 3:
						if args[2] == 'list' and self.monlist:
							self.sendmsg(target, '[{0}] {1}'.format(color('Monitor', light_blue), ', '.join(self.monlist)))
						elif args[2] == 'reset' and self.monlist:
							self.monitor('C')
							self.sendmsg(target, '{0} nick(s) have been {1} from the monitor list.'.format(color(str(len(self.monlist)), cyan), color('removed', red)))
							self.monlist = list()
						elif args[2][:1] == '+':
							nicks = [mon_nick for mon_nick in set(args[2][1:].split(',')) if mon_nick not in self.monlist]
							if nicks:
								self.monlist += nicks
								self.monitor('+', nicks)
								self.sendmsg(target, '{0} nick(s) have been {1} to the monitor list.'.format(color(str(len(nicks)), cyan), color('added', green)))
						elif args[2][:1] == '-':
							nicks = [mon_nick for mon_nick in set(args[2][1:].split(',')) if mon_nick in self.monlist]
							if nicks:
								for mon_nick in nicks:
									self.monlist.remove(mon_nick)
								self.monitor('-', nicks)
								self.sendmsg(target, '{0} nick(s) have been {1} from the monitor list.'.format(color(str(len(nicks)), cyan), color('removed', red)))
		elif target == self.nickname:
			if msg.startswith('\x01ACTION'):
				self.sendmsg(channel, '[{0}] {1}{2}{3} * {4}'.format(color('PM', red), color('<', grey), color(nick, yellow), color('>', grey), msg[8:][:-1]))
			elif msg[:1] != '\001':
				self.sendmsg(channel, '[{0}] {1}{2}{3} {4}'.format(color('PM', red), color('<', grey), color(nick, yellow), color('>', grey), msg))

	def event_mode(self, nick, chan, modes):
		pass # Don't know what we are doing with this yet.

	def event_names(self, chan, nicks):
		for name in nicks:
			if name[:1] in '~!@%&+':
				name = name[1:]
			if name not in self.bots:
				self.bots.append(name)

	def event_quit(self, nick):
		if nick in self.monlist:
			self.nick(nick)

	def event_who(self, ident):
		self.bots.append(ident)

	def handle_events(self, data):
		args = data.split()
		if data.startswith('ERROR :Closing Link:'):
			raise Exception('Connection has closed.')
		elif data.startswith('ERROR :Reconnecting too fast'):
			raise Exception('Connection has closed. (throttled)')
		elif args[0] == 'PING':
			self.raw('PONG ' + args[1][1:])
		elif args[1] == '001': # RPL_WELCOME
			self.event_connect()
		elif args[1] == '315': # RPL_ENDOFWHO
			pass
		elif args[1] == '352' and len(args) >= 8: # RPL_WHOREPLY
			user = args[4]
			host = args[5]
			nick = args[7]
			self.event_who('{0}!{1}@{2}'.format(nick, user, host))
		elif args[1] == '353' and len(args) >= 6: #RPL_NAMREPLY
			chan  = args[4].lower()
			names = ' '.join(args[5:]).lower()[1:].split()
			self.event_names(chan, names)
		elif args[1] == '366' and len(args) >= 4: # RPL_ENDOFNAMES
			chan = args[3].lower()
			self.event_end_of_names(chan)
		elif args[1] == '433' and len(args) >= 4: # ERR_NICKNAMEINUSE
			nick = args[2]
			target_nick = args[3]
			self.event_nick_in_use(nick, target_nick)
		elif args[1] == '731' and len(args) >= 4: # RPL_MONOFFLINE
			nick = args[3][1:]
			self.nick(nick)
		elif args[1] == 'MODE' and len(args) >= 4:
			nick  = args[0].split('!')[0][1:]
			chan  = args[2]
			modes = ' '.join(args[:3])
			self.event_mode(nick, chan, modes)
		elif args[1] == 'NICK' and len(args) == 3:
			nick = args[0].split('!')[0][1:]
			new_nick = args[2][1:]
			self.event_nick(nick, new_nick)
		elif args[1] == 'PRIVMSG' and len(args) >= 4:
			ident  = args[0][1:]
			nick   = args[0].split('!')[0][1:]
			target = args[2]
			msg    = ' '.join(args[3:])[1:]
			self.event_message(ident, nick, target, msg)
		elif args[1] == 'QUIT':
			nick = args[0].split('!')[0][1:]
			self.event_quit(nick)

	def join_channel(self, chan, key=None):
		self.raw('JOIN {0} {1}'.format(chan, key)) if key else self.raw('JOIN ' + chan)

	def listen(self):
		while True:
			try:
				data = self.sock.recv(1024).decode('utf-8')
				for line in (line for line in data.split('\r\n') if len(line.split()) >= 2):
					debug(line)
					self.handle_events(line)
			except (UnicodeDecodeError,UnicodeEncodeError):
				pass
			except Exception as ex:
				error('Unexpected error occured.', ex)
				break
		self.event_disconnect()

	def mode(self, target, mode):
		self.raw('MODE {0} {1}'.format(target, mode))

	def monitor(self, action, nicks=None):
		self.raw('MONITOR {0} {1}'.format(action, ','.join(nicks))) if nicks else self.raw('MONITOR ' + action)

	def nick(self, nick):
		self.raw('NICK ' + nick)

	def raw(self, data):
		self.sock.send(bytes(data + '\r\n', 'utf-8'))
		time.sleep(0.5)

	def raw_delay(self, data):
		time.sleep(random.randint(300,900))
		self.raw(data)

	def sendmsg(self, target, msg):
		self.raw('PRIVMSG {0} :{1}'.format(target, msg))

# Main
if type(vhosts) == list:
	for vhost in vhosts:
		for server in servers:
			for i in range(5):
				clone(server, vhost).start()
				time.sleep(random.randint(300,900))
else:
	for server in servers:
		for i in range(5):
			clone(server, vhosts).start()
			time.sleep(random.randint(300,900))
while True:input('')
