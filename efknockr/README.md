# EFknockr
> The most annoying script IRC has ever seen!

## Requirements
* [Python](https://www.python.org/downloads/) *(latest version)*
* [PySocks](https://pypi.python.org/pypi/PySocks) *(optional for using the --proxy setting)*

## Information
For each server defined in the config, the bot will connnect, join every channel, send the lines from msg.txt, mass hilight everyone, and then part.

For questions, comments, & concerns, please join [#5000](irc://irc.supernets.org/%265000) on [irc.supernets.org](irc://irc.supernets.org/%265000)

## Todo
* Make use of the [asyncio](https://docs.python.org/3/library/asyncio.html) library.
* Convert text with random unicode & combining diacritics & maybe zero-width characters.
* Dynamic msg files using replacers defined in config.
* Detect global/channel user counts.
* Weechat dcc exploit.
* PM every nick collected after knocking.
* Create an output log of all networks, channels, nicks, and more.

## Warning
This script is for testing against your own server(s). I am not responsible for how the public uses these.