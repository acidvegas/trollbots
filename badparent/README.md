###### Requirements
* [Python](https://www.python.org/downloads/) *(**Note:** This script was developed to be used with the latest version of Python.)*
* [PySocks](https://pypi.python.org/pypi/PySocks) *(**Optional:** For using the `--proxy` setting.)*

###### Information
The parent bot will join a channel, parse the entire nicklist, and maintain it during joins, quits, nick changes, etc.

The child bot clones will use either proxies or virtual hosts to connect and PM the nicklist.

Nicks that have the usermode +g *(callerid)*, +D *(privdeaf)*, and +R *(regonlymsg)* will be removed from the nicklist.