###### Requirements
* [Python](https://www.python.org/downloads/) *(**Note:** This script was developed to be used with the latest version of Python.)*

###### Information
This bot will automatically connect to a random EFNet server and idle in a channel.

The bot is designed to be very minimal, secure, and trustless by nature.

This means anyone can run a copy of your script on their server to help build your botnet.

All commands must be prefixed with @all or the bots nick and will work in PM or the configured channel.

Nicks added to the MONITOR list will attempt to jupe as the nick becomes available.

You can use [this](https://github.com/acidvegas/random/blob/master/irc/identd.py) identd script with this.

###### Commands
| Command | Description |
| --- | --- |
| id | Send bot identity |
| raw \<data> | Send \<data> to server |
| rawd \<data> | Send \<data> to server delayed |
| monitor list | Return MONITOR list |
| monitor reset | Reset MONITOR list |
| monitor \<+/->\<nicks> | Add (+) or Remove (-) \<nicks> from MONITOR list. *(Can be a single nick or comma seperated list)* |