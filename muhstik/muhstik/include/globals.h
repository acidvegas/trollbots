#ifndef GLOBAL_H
#define GLOBAL_H

#define VERSION "4.2.3"
/*
 * The above `#define' _must_ be line number 4 for the `pre-commit' hook to
 *     work, and the MOTD file to stay intact.
 */

/* {{{ List lengths */
#define MAX_AOPS 128
#define MAX_PROTS 128
#define MAX_JUPES 128
#define MAX_SHITS 128
#define MAX_CHANS 64
#define MAX_NICKS 1024
#define MAX_IDENTS 1024
#define MAX_NAMES 1024
#define MAX_GROUPS 16
#define MAX_CLONES 1024
#define MAX_BROTHERS 1024
#define MAX_MUTEX 2
/* }}} */
/* {{{ Internal commands */
#define ADDOP "+aop"
#define RMOP "-aop"
#define ADDPROT "+prot"
#define RMPROT "-prot"
#define ADDJUPE "+jupe"
#define RMJUPE "-jupe"
#define ADDSHIT "+shit"
#define RMSHIT "-shit"
#define ADDSCAN "+scan"
#define RMSCAN "-scan"
#define KICKBAN "kb"
#define MASSKICK "mk"
#define MASSKICKBAN "mkb"
#define TAKEOVER "to"
#define MASSOP "mo"
#define MASSDEOP "md"
#define MASSUNBAN "mu"
#define NICKS "nicks"
#define NICKLIST "nicklist"
#define ECHO "echo"
#define CHANKEY "chankey"
#define STATUS "stat"
#define MUTE "mute"
#define PEACE "peace"
#define AGGRESS "agg"
#define RANDOM "random"
#define KILL "kill"
#define SELECT "select"
#define LOAD "load"
/* }}} */
/* {{{ Messages */
#define KICK_REAS "/!\\ this channel has moved to irc.gnaa.eu #gnaa /!\\"
#define SHIT_REAS "/!\\ this channel has moved to irc.gnaa.eu #gnaa /!\\"
#define SETON "activated"
#define CLOSE "+milk 1488 killjews"
/* }}} */
/* {{{ Buffer sizes */
#define MINIBUF 32
#define MINIBUF_TXT "32"
#define MEDBUF 128
#define MEDBUF_TXT "128"
#define BIGBUF 1024
#define BIGBUF_TXT "1024"
/* }}} */

/* To avoid mixing sleep() and usleep() */
#define sleep(t)\
usleep (t*1000000)

#endif
