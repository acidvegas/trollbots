#!/usr/bin/env python
# EFknockr (EFK) - Developed by acidvegas in Python (https://acid.vegas/trollbots)
# efknockr.py

'''

"The most annoying script IRC has ever seen"

'''

import argparse
import concurrent.futures
import os
import random
import socket
import ssl
import string
import threading
import time

class settings:
    mass_hilite = True # Hilite all the users in a channel before parting.
    part_msg    = 'Smell ya l8r'
    proxy       = None # Proxy should be a Socks5 in IP:PORT format.
    register    = True # Register with NickServ before joining channels.
    vhost       = None

class throttle:
    channels = 3   # Maximum number of channels to be flooding at once.
    delay    = 31  # Delay before sending registering nick (if enabled) and sending /LIST.
    join     = 3   # Delay between each channel join.
    message  = 0.5 # Delay between each message sent to a channel.
    threads  = 100 # Maximum number of threads running.
    timeout  = 15  # Timeout for all sockets.
    users    = 10  # Minimum number of users required in a channel.

bad_numerics = {
	'404' : 'ERR_CANNOTSENDTOCHAN',
	'405' : 'ERR_TOOMANYCHANNELS',
	'470' : 'ERR_LINKCHANNEL',
	'471' : 'ERR_CHANNELISFULL',
	'473' : 'ERR_INVITEONLYCHAN',
	'474' : 'ERR_BANNEDFROMCHAN',
	'475' : 'ERR_BADCHANNELKEY',
	'477' : 'ERR_NEEDREGGEDNICK',
	'489' : 'ERR_SECUREONLYCHAN',
	'519' : 'ERR_TOOMANYUSERS',
	'520' : 'ERR_OPERONLY'
}

def debug(msg):
	print(f'{get_time()} | [~] - {msg}')

def error(msg, reason=None):
	print(f'{get_time()} | [!] - {msg} ({reason})') if reason else print(f'{get_time()} | [!] - {msg}')

def error_exit(msg):
	raise SystemExit(f'{get_time()} | [!] - {msg}')

def get_time():
	return time.strftime('%I:%M:%S')

def rnd():
	return ''.join(random.choice(string.ascii_letters) for _ in range(random.randint(4, 8)))

class clone:
	def __init__(self, server):
		self.channels = {'all':list(), 'bad':list(), 'current':list(), 'nicks':dict()}
		self.server   = server
		self.sock     = None
		self.port     = 6697 # Fallback to 6667 if SSL fails

	def attack(self):
		try:
			while self.channels['all']:
				while len(self.channels['current']) >= throttle.channels:
					time.sleep(1)
				chan = random.choice(self.channels['all'])
				self.raw('JOIN ' + chan)
				time.sleep(throttle.join)
				if chan in self.channels['all']:
					self.channels['all'].remove(chan)
			debug('Finished knocking all channels on ' + self.server)
		except Exception as ex:
			error('Error occured in the attack loop!', ex)
		finally:
			self.event_disconnect()

	def connect(self):
		try:
			self.create_socket()
			self.sock.connect((self.server, self.port))
			self.raw('USER {rnd()} 0 * :{rnd()}')
			self.raw('NICK ' + rnd())
		except socket.error:
			if self.port == 6697:
				self.port = 6667
				self.connect()
			else:
				self.event_disconnect()
		else:
			self.listen()

	def create_socket(self):
		if settings.proxy:
			proxy_server, proxy_port = settings.proxy.split(':')
			self.sock = socks.socksocket()
			self.sock.setproxy(socks.PROXY_TYPE_SOCKS5, proxy_server, int(proxy_port))
		else:
			self.sock = socket.socket(socket.AF_INET6) if ':' in self.server else socket.socket()
			if settings.vhost:
				self.sock.bind((settings.vhost, 0))
			if self.port == 6697:
				self.sock = ssl.wrap_socket(self.sock)
		self.sock.settimeout(throttle.timeout)

	def event_connect(self, name, daemon):
		debug(f'Connected to {name} ({daemon})')
		self.server = name
		time.sleep(throttle.delay) # high delay could cause a ping out, asyncio would help
		if settings.register:
			self.sendmsg('NickServ', f'REGISTER {rnd()} {rnd()}@gmail.com')
		self.raw('LIST >' + str(throttle.users))

	def event_disconnect(self):
		self.sock.close()

	def event_end_of_list(self):
		if self.channels['all']:
			debug('Found {0:,} channels on {1}'.format(len(self.channels['all']), self.server))
			threading.Thread(target=self.attack).start()
		else:
			error('Found zero channels on ' + self.server)
			self.event_disconnect()

	def event_end_of_names(self, chan):
		self.channels['current'].append(chan)
		debug(f'Knocking {chan} channel on {self.server}...')
		try:
			for line in msg_lines:
				if chan in self.channels['bad']:
					break
				self.sendmsg(chan, line)
				time.sleep(throttle.message)
			if chan in self.channels['nicks']:
				self.channels['nicks'][chan] = ' '.join(self.channels['nicks'][chan])
				if len(self.channels['nicks'][chan]) <= 400:
					self.sendmsg(chan, self.channels['nicks'][chan])
				else:
					while len(self.channels['nicks'][chan]) > 400:
						if chan in self.channels['bad']:
							break
						segment = self.channels['nicks'][chan][:400]
						segment = segment[:-len(segment.split()[len(segment.split())-1])]
						self.sendmsg(chan, segment)
						self.channels['nicks'][chan] = self.channels['nicks'][chan][len(segment):]
						time.sleep(throttle.message)
			self.raw(f'PART {chan} :{settings.part_msg}')
		except Exception as ex:
			error('Error occured in the attack loop!', ex)
		finally:
			if chan in self.channels['current']:
				self.channels['current'].remove(chan)
			if chan in self.channels['bad']:
				self.channels['bad'].remove(chan)
			if chan in self.channels['nicks']:
				del self.channels['nicks'][chan]

	def event_names(self, chan, names):
		if settings.mass_hilite:
			if chan not in self.channels['nicks']:
				self.channels['nicks'][chan] = list()
			for name in names:
				if name[:1] in '~!@%&+:':
					name = name[1:]
				if name != 'ChanServ' and name not in self.channels['nicks'][chan]:
					self.channels['nicks'][chan].append(name)

	def handle_events(self, data):
		args = data.split()
		if data.startswith('ERROR :Closing Link:'):
			raise Exception('Connection has closed.')
		elif args[0] == 'PING':
			self.raw('PONG ' + args[1][1:])
		elif args[1] == '004': # RPL_MYINFO
			server_name   = args[3]
			server_daemon = args[4]
			self.event_connect(server_name, server_daemon)
		elif args[1] == '266': # RPL_GLOBALUSERS
			global_users = args[3] # might use this for something in the future
		elif args[1] == '322' and len(args) >= 5: # RPL_LIST
			chan  = args[3]
			users = args[4] # might use this for something in the future
			self.channels['all'].append(chan)
		elif args[1] == '323': # RPL_LISTEND
			self.event_end_of_list()
		elif args[1] == '353' and len(args) >= 6: # RPL_NAMREPLY
			chan  = args[4]
			names = ' '.join(args[5:])[2:].split()
			self.event_names(chan, names)
		elif args[1] == '366' and len(args) >= 4: # RPL_ENDOFNAMES
			chan = args[3]
			threading.Thread(target=self.event_end_of_names, args=(chan,)).start()
		elif args[1] == '433': # ERR_NICKNAMEINUSE
			self.raw('NICK ' + rnd())
		elif args[1] == '464': # ERR_PASSWDMISMATCH
			error('Network has a password.', self.server)
		elif args[1] == '465': # ERR_YOUREBANNEDCREEP
			error('K-Lined.', self.server)
		elif args[1] in bad_numerics and len(args) >= 4:
			chan = args[3]
			if chan not in self.channels['bad']:
				self.channels['bad'].append(chan)
				error(f'Failed to knock {chan} channel on {self.server}', bad_numerics[args[1]])

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

	def sendmsg(self, target, msg):
		self.raw(f'PRIVMSG {target} :{msg}')

# Main
print('#'*56)
print('#{:^54}#'.format(''))
print('#{:^54}#'.format('EFknockr (EFK)'))
print('#{:^54}#'.format('Developed by acidvegas in Python'))
print('#{:^54}#'.format('https://acid.vegas/trollbots'))
print('#{:^54}#'.format(''))
print('#'*56)
parser = argparse.ArgumentParser(usage='%(prog)s <msg_file> <targets_file>')
parser.add_argument('msg_file',     help='file to send contents')
parser.add_argument('targets_file', help='file with a list of ip addresses')
args = parser.parse_args()
if settings.proxy:
	try:
		import socks
	except ImportError:
		error_exit('Missing PySocks module! (https://pypi.python.org/pypi/PySocks)')
if os.path.isfile(args.msg_file) and os.path.isfile(args.targets_file):
	msg_lines = [line.rstrip() for line in open(args.msg_file,     encoding='utf8', errors='replace').readlines() if line]
	targets   = [line.rstrip() for line in open(args.targets_file, encoding='utf8', errors='replace').readlines() if line]
else:
	error_exit('File is missing or is a directory!')
debug(f'Loaded {len(targets):,} targets.')
random.shuffle(targets)
with concurrent.futures.ThreadPoolExecutor(max_workers=throttle.threads) as executor:
	checks = [executor.submit(clone(target).connect) for target in targets]
	concurrent.futures.wait(checks)
debug('EFknockr has finished knocking.')