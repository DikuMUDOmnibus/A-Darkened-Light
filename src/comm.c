/* ************************************************************************
*   File: comm.c                                        Part of CircleMUD *
*  Usage: Communication, socket handling, main(), central game loop       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __COMM_C__

#include "conf.h"
#include "sysdep.h"
#include "events.h"

#ifdef CIRCLE_MACINTOSH		/* Includes for the Macintosh */
# define SIGPIPE 13
# define SIGALRM 14
  /* GUSI headers */
# include <sys/ioctl.h>
  /* Codewarrior dependant */
# include <SIOUX.h>
# include <console.h>
#endif

#ifdef CIRCLE_WINDOWS		/* Includes for Win32 */
# ifdef __BORLANDC__
#  include <dir.h>
# else /* MSVC */
# include <direct.h>
# endif
# include <mmsystem.h>
#endif /* CIRCLE_WINDOWS */

#ifdef CIRCLE_AMIGA		/* Includes for the Amiga */
# include <sys/ioctl.h>
# include <clib/socket_protos.h>
#endif /* CIRCLE_AMIGA */

#ifdef CIRCLE_ACORN		/* Includes for the Acorn (RiscOS) */
# include <socklib.h>
# include <inetlib.h>
# include <sys/ioctl.h>
#endif

/*
 * Note, most includes for all platforms are in sysdep.h.  The list of
 * files that is included is controlled by conf.h for that platform.
 */

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "house.h"
#include "olc.h"
#include "clan.h"
#include "teleport.h"
//#include "class.h"
#include "screen.h"  /* added this line */

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#define UNDEFPERC	-32000

/* externs */
extern struct ban_list_element *ban_list;
extern int num_invalid;
extern char *GREETINGS;
extern const char circlemud_version[];
extern int circle_restrict;
extern int mini_mud;
extern int no_rent_check;
extern FILE *player_fl;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern char *DFLT_IP;
extern char *LOGNAME;
extern int MAX_PLAYERS;

// extern struct title_type titles[NUM_CLASSES][LVL_IMPL + 1];
extern struct room_data *world;	/* In db.c */
extern int top_of_world;	/* In db.c */
extern struct time_info_data time_info;		/* In db.c */
extern char help[];
extern int read_obj;
void pmail_check(void);
extern int  numgreetings;
extern char *greetings[MAX_GREETINGS];
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct index_data *mob_index;

/* local globals */
struct descriptor_data *descriptor_list = NULL;		/* master desc list */
struct txt_block *bufpool = 0;	/* pool of large output buffers */
int buf_largecount = 0;		/* # of large buffers which exist */
int buf_overflows = 0;		/* # of overflows of output */
int buf_switches = 0;		/* # of switches from small to large buf */
int circle_shutdown = 0;	/* clean shutdown */
int circle_reboot = 0;		/* reboot the game after a shutdown */
int circle_put_sign = 1;	/* whether to put sign on reboot */
int no_specials = 0;		/* Suppress ass. of special routines */
int max_players = 0;		/* max descriptors available */
int tics = 0;			/* for extern checkpointing */
int scheck = 0;			/* for syntax checking mode */
//int pulse = 0;
bool MOBTrigger = TRUE;		/* for MOBProgs	*/
extern int nameserver_is_slow;	/* see config.c */
extern int auto_save;		/* see config.c */
extern int autosave_time;	/* see config.c */
struct timeval null_time;	/* zero-valued time structure */
FILE *logfile = NULL;		/* Where to send the log messages. */
unsigned long pulse = 0;
static bool fCopyOver;          /* Are we booting in copyover mode? */
int  mother_desc;        /* Now a global */
int     port;
bool show_color_codes = FALSE;

/* functions in this file */
RETSIGTYPE reread_wizlists(int sig);
RETSIGTYPE unrestrict_game(int sig);
RETSIGTYPE reap(int sig);
RETSIGTYPE checkpointing(int sig);
RETSIGTYPE hupsig(int sig);
ssize_t perform_socket_read(socket_t desc, char *read_point,size_t space_left);
ssize_t perform_socket_write(socket_t desc, const char *txt,size_t length);
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);
void sanity_check(void);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_max_players(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
struct timeval *timediff(struct timeval a, struct timeval b);
struct timeval *timeadd(struct timeval a, struct timeval b);
void flush_queues(struct descriptor_data *d);
void nonblock(socket_t s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
char *make_prompt(struct descriptor_data *point);
void check_idle_passwords(void);
struct in_addr *get_bind_addr(void);
int parse_ip(const char *addr, struct in_addr *inaddr);
int set_sendbuf(socket_t s);
#if defined(POSIX)
sigfunc *my_signal(int signo, sigfunc * func);
#endif

/* extern fcnts */
void boot_world(void);
void parts_update(void); 
void affect_update(void);	/* In spells.c */
void perform_violence(void);
void mobile_activity(void);
void string_add(struct descriptor_data *d, char *str);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname);
void weather_and_time(int mode);
void train_upd(void); /* in trains.c */
void mprog_act_trigger(char *buf, struct char_data *mob, struct char_data *ch, struct obj_data *obj, void *vo);
void TeleportPulseStuff();
void heartbeat(int pulse);
void tick_grenade(void);
void reboot_wizlists(void);
void process_program_output(void);
int level_exp(int chclass, int level);
void init_descriptor (struct descriptor_data *newd, int desc);
int enter_player_game(struct descriptor_data *d);
void make_who2html(void);

#ifdef __CXREF__
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#undef FD_CLR
#define FD_ZERO(x)
#define FD_SET(x, y) 0
#define FD_ISSET(x, y) 0
#define FD_CLR(x, y)
#endif

/* *********************************************************************
*  main game loop and related stuff                                    *
********************************************************************* */

#if defined(CIRCLE_WINDOWS) || defined(CIRCLE_MACINTOSH)

/* Windows doesn't have gettimeofday, so we'll simulate it. */
/* The Mac doesn't have gettimeofday either. */
void gettimeofday(struct timeval *t, struct timezone *dummy)
{
#if defined(CIRCLE_WINDOWS)
  DWORD millisec = GetTickCount();
#elif defined(CIRCLE_MACINTOSH)
  unsigned long int millisec;
  millisec = (int)((float)TickCount() * 1000.0 / 60.0);
#endif

  t->tv_sec = (int) (millisec / 1000);
  t->tv_usec = (millisec % 1000) * 1000;
}

#endif	/* CIRCLE_WINDOWS || CIRCLE_MACINTOSH */

int main(int argc, char **argv)
{
  int pos = 1;
  char *dir;

  /* Initialize these to check for overruns later. */
  buf[MAX_STRING_LENGTH - 1] = buf1[MAX_STRING_LENGTH - 1] = MAGIC_NUMBER;
  buf2[MAX_STRING_LENGTH - 1] = arg[MAX_STRING_LENGTH - 1] = MAGIC_NUMBER;

#ifdef CIRCLE_MACINTOSH
  /*
   * ccommand() calls the command line/io redirection dialog box from
   * Codewarriors's SIOUX library
   */
  argc = ccommand(&argv);
  /* Initialize the GUSI library calls.  */
  GUSIDefaultSetup();
#endif

  port = DFLT_PORT;
  dir = DFLT_DIR;

  /*
   * It would be nice to make this a command line option but the parser uses
   * the log() function, maybe later. -gg
   */
  if (LOGNAME == NULL || *LOGNAME == '\0')
    logfile = fdopen(STDERR_FILENO, "w");
  else
    logfile = freopen(LOGNAME, "w", stderr);

  if (logfile == NULL) {
      printf("error opening log file %s: %s\n",
		LOGNAME ? LOGNAME : "stderr", strerror(errno));
      exit(1);
    }

  log(circlemud_version);

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'C': /* -C<socket number> - recover from copyover, this is the control socket */
	fCopyOver = TRUE;
	mother_desc = atoi(argv[pos]+2);
	break;
    case 'd':
      if (*(argv[pos] + 2))
	dir = argv[pos] + 2;
      else if (++pos < argc)
	dir = argv[pos];
      else {
	log("SYSERR: Directory arg expected after option -d.");
	exit(1);
      }
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      log("Running in minimized mode & with no rent check.");
      break;
    case 'c':
      scheck = 1;
      log("Syntax check mode enabled.");
      break;
    case 'q':
      no_rent_check = 1;
      log("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      circle_restrict = 1;
      log("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      log("Suppressing assignment of special routines.");
      break;
    default:
      log("SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      log("Usage: %s [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      log("SYSERR: Illegal port number %d.", port);
      exit(1);
    }
  }

  if (chdir(dir) < 0) {
    perror("SYSERR: Fatal error changing to data directory");
    exit(1);
  }
  log("Using %s as data directory.", dir);

/*
  if (!no_rent_check) {
    log("Checking rent files expiration.");
  }
*/
  
  if (scheck) {
    boot_world();
    log("Done.");
  } else {
    log("Running game on port %d.", port);
    init_game(port);
  }

  return 0;
}



/* Reload players after a copyover */
void copyover_recover() {
  struct descriptor_data *d;
  FILE *fp;
  char host[1024];
  struct char_file_u tmp_store;
  int desc, player_i;
  bool fOld;
  char name[MAX_INPUT_LENGTH];
  int loadroom = 0, temp;
	
  log ("Copyover recovery initiated");
	
  fp = fopen (COPYOVER_FILE, "r");
	
  if (!fp) { /* there are some descriptors open which will hang forever then ? */    
    perror ("copyover_recover:fopen");
    log ("Copyover file not found. Exitting.\n\r");
    exit (1);
  }

  unlink (COPYOVER_FILE); /* In case something crashes - doesn't prevent reading	*/
	
  for (;;) {
    fOld = TRUE;
    fscanf (fp, "%d %s %d %s\n", &desc, name, &loadroom, host);
    if (desc == -1) break;

    /* Write something, and check if it goes error-free */		
    if (write_to_descriptor (desc, "\n\rRestoring from copyover...\n\r") < 0) {
      close (desc); /* nope */
      continue;
    }
		
    /* create a new descriptor */
    CREATE (d, struct descriptor_data, 1);
    memset ((char *) d, 0, sizeof (struct descriptor_data));
    init_descriptor (d,desc); /* set up various stuff */
		
    strcpy(d->host, host);
    d->next = descriptor_list;
    descriptor_list = d;

    d->connected = CON_CLOSE;
	
    /* Now, find the pfile */
		
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;

    if ((player_i = load_char(name, &tmp_store)) >= 0) {
      store_to_char(&tmp_store, d->character);
      GET_PFILEPOS(d->character) = player_i;
      if (!PLR_FLAGGED(d->character, PLR_DELETED))
        REMOVE_BIT(PLR_FLAGS(d->character),PLR_WRITING | PLR_MAILING | PLR_CRYO);
      else
        fOld = FALSE;
    } else fOld = FALSE;
		
    if (!fOld) /* Player file not found?! */ {
       write_to_descriptor (desc, "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r");
       close_socket (d);			
    } else /* ok! */ {
      write_to_descriptor (desc, "\n\rCopyover recovery complete.\n\r");
      temp = GET_LOADROOM(d->character);
      GET_LOADROOM(d->character) = loadroom;
      enter_player_game(d);
      GET_LOADROOM(d->character) = temp;
      d->connected = CON_PLAYING;
      look_at_room(d->character, 0);
    }		
  }	
  fclose (fp);
}


void put_sign(void)
{
  int pipefd[2];
  int pid;
  FILE *fd;
  char *str = "MUD is down due to maintenance. Try again in a few minutes.\r\n";
  
  if ((fd = fopen(SIGN_PID_FILE, "w")) == NULL) {
    log("SYSERR: Cannot write %s, sign won't be started.", SIGN_PID_FILE);
    return;
  }
  log("Starting sign on port %d.", port);
  if (pipe(pipefd) < 0) {
    log("SYSERR: Cannot create pipe.\r\n");
    perror("pipe");
    return;
  }
  if ((pid = fork()) < 0) {
    log("SYSERR: Cannot fork.\r\n");
    perror("fork");
    return;
  }
  
  if (pid == 0) {
    close(pipefd[1]);
    dup2(pipefd[0], 0);
    close(pipefd[0]);
    sprintf(buf, "%d", port);
    execlp("../bin/sign", "../bin/sign", buf, "-", NULL);
    perror("exec");
  }
  
  close(pipefd[0]);
  write(pipefd[1], str, strlen(str));
  close(pipefd[1]);
  fprintf(fd, "%d\n", pid);
  fclose(fd);
//  while (wait(NULL) != pid);
}

int remove_sign(void) {
  return TRUE;
}

/* Init sockets, run game, and cleanup sockets */
void init_game(int port)
{
//  int mother_desc;

  /* We don't want to restart if we crash before we get up. */
  touch(KILLSCRIPT_FILE);

  circle_srandom(time(0));

  log("Finding player limit.");
  max_players = get_max_players();

//  log("Opening mother connection.");
//  mother_desc = init_socket(port);

  if (!fCopyOver) /* If copyover mother_desc is already set up */
    {
 	log ("Opening mother connection.");
 	mother_desc = init_socket (port);
    }
  //event_init();
  boot_db();

#ifdef CIRCLE_UNIX
  log("Signal trapping.");
  signal_setup();
#endif

  if (fCopyOver) /* reload players */
  copyover_recover();

  /* If we made it this far, we will be able to restart without problem. */
  remove(KILLSCRIPT_FILE);

  log("Entering game loop.");

  game_loop(mother_desc);

  Crash_save_all();
  clan_save(TRUE, NULL); /* Fido - Crash save and auto save clans */

  log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  CLOSE_SOCKET(mother_desc);
  fclose(player_fl);

  // if (circle_put_sign) put_sign();

  if (circle_reboot) {
    log("Rebooting.");
    exit(52);			/* what's so great about HHGTTG, anyhow? */
  }
  log("Normal termination of game.");
}



/*
 * init_socket sets up the mother descriptor - creates the socket, sets
 * its options up, binds it, and listens.
 */
int init_socket(int port)
{
  int s, opt;
  struct sockaddr_in sa;

#ifdef CIRCLE_WINDOWS
  {
    WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
      log("SYSERR: WinSock not available!");
      exit(1);
    }
    if ((wsaData.iMaxSockets - 4) < max_players) {
      max_players = wsaData.iMaxSockets - 4;
    }
    log("Max players set to %d", max_players);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
      log("SYSERR: Error opening network connection: Winsock error #%d",
	  WSAGetLastError());
      exit(1);
    }
  }
#else
  /*
   * Should the first argument to socket() be AF_INET or PF_INET?  I don't
   * know, take your pick.  PF_INET seems to be more widely adopted, and
   * Comer (_Internetworking with TCP/IP_) even makes a point to say that
   * people erroneously use AF_INET with socket() when they should be using
   * PF_INET.  However, the man pages of some systems indicate that AF_INET
   * is correct; some such as ConvexOS even say that you can use either one.
   * All implementations I've seen define AF_INET and PF_INET to be the same
   * number anyway, so the point is (hopefully) moot.
   */

  if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("SYSERR: Error creating socket");
    exit(1);
  }
#endif				/* CIRCLE_WINDOWS */

#if defined(SO_REUSEADDR) && !defined(CIRCLE_MACINTOSH)
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0){
    perror("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }
#endif

  set_sendbuf(s);

#if defined(SO_LINGER)
  {
    struct linger ld;

    ld.l_onoff = 0;
    ld.l_linger = 0;
    if (setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof(ld)) < 0) {
      perror("SYSERR: setsockopt LINGER");
      exit(1);
    }
  }
#endif

  /* Clear the structure */
  memset((char *)&sa, 0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr = *(get_bind_addr());

  if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    CLOSE_SOCKET(s);
    exit(1);
  }
  nonblock(s);
  listen(s, 5);
  return s;
}


int get_max_players(void)
{
#ifndef CIRCLE_UNIX
  return MAX_PLAYERS;
#else

  int max_descs = 0;
  const char *method;

/*
 * First, we'll try using getrlimit/setrlimit.  This will probably work
 * on most systems.  HAS_RLIMIT is defined in sysdep.h.
 */
#ifdef HAS_RLIMIT
  {
    struct rlimit limit;

    /* find the limit of file descs */
    method = "rlimit";
    if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling getrlimit");
      exit(1);
    }
    /* set the current to the maximum */
    limit.rlim_cur = limit.rlim_max;
    if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
      perror("SYSERR: calling setrlimit");
      exit(1);
    }
#ifdef RLIM_INFINITY
    if (limit.rlim_max == RLIM_INFINITY)
      max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
    else
      max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
#else
    max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
#endif
  }

#elif defined (OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  method = "OPEN_MAX";
  max_descs = OPEN_MAX;		/* Uh oh.. rlimit didn't work, but we have
				 * OPEN_MAX */
#elif defined (_SC_OPEN_MAX)
  /*
   * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
   * try the POSIX sysconf() function.  (See Stevens' _Advanced Programming
   * in the UNIX Environment_).
   */
  method = "POSIX sysconf";
  errno = 0;
  if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
    if (errno == 0)
      max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
    else {
      perror("SYSERR: Error calling sysconf");
      exit(1);
    }
  }
#else
  /* if everything has failed, we'll just take a guess */
  method = "random guess"
  max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
#endif

  /* now calculate max _players_ based on max descs */
  max_descs = MIN(MAX_PLAYERS, max_descs - NUM_RESERVED_DESCS);

  if (max_descs <= 0) {
    log("SYSERR: Non-positive max player limit!  (Set at %d using %s).",
	    max_descs, method);
    exit(1);
  }
  log("Setting player limit to %d using %s.", max_descs, method);
  return max_descs;
#endif /* CIRCLE_UNIX */
}



/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(int mother_desc)
{
  fd_set input_set, output_set, exc_set, null_set;
  struct timeval last_time, before_sleep, opt_time, process_time, now, timeout;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int missed_pulses, maxdesc, aliased, i;

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  FD_ZERO(&null_set);

  gettimeofday(&last_time, (struct timezone *) 0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
      
      log("No connections.  Going to sleep.");
      make_who2html();

      for (i = 0;i <= top_of_zone_table;i++)
        if (!zone_table[i].unloaded)
          unload_room_descs(i);
      
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *) 0, (fd_set *) 0, NULL) < 0) {
	if (errno == EINTR)
	  log("Waking up to process signal.");
	else
	  perror("Select coma");
      } else
	log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *) 0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);

    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
#ifndef CIRCLE_WINDOWS
      if (d->descriptor > maxdesc)
	maxdesc = d->descriptor;
#endif
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, we have completed all input, output and heartbeat
     * activity from the previous iteration, so we have to put ourselves
     * to sleep until the next 0.1 second tick.  The first step is to
     * calculate how long we took processing the previous iteration.
     */
    
    gettimeofday(&before_sleep, (struct timezone *) 0); /* current time */
    process_time = *timediff(before_sleep, last_time);

    /*
     * If we were asleep for more than one pass, count missed pulses and sleep
     * until we're resynchronized with the next upcoming pulse.
     */
    if (process_time.tv_sec == 0 && process_time.tv_usec < OPT_USEC) {
      missed_pulses = 0;
    } else {
      missed_pulses = process_time.tv_sec * PASSES_PER_SEC;
      missed_pulses += process_time.tv_usec / OPT_USEC;
      process_time.tv_sec = 0;
      process_time.tv_usec = process_time.tv_usec % OPT_USEC;
    }

    /* Calculate the time we should wake up */
    last_time = *timeadd(before_sleep, *timediff(opt_time, process_time));

    /* Now keep sleeping until that time has come */
    gettimeofday(&now, (struct timezone *) 0);
    timeout = *timediff(last_time, now);

    /* Go to sleep */
    do {
#ifdef CIRCLE_WINDOWS
      Sleep(timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#else
      if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0) {
	if (errno != EINTR) {
	  perror("SYSERR: Select sleep");
	  exit(1);
	}
      }
#endif /* CIRCLE_WINDOWS */
      gettimeofday(&now, (struct timezone *) 0);
      timeout = *timediff(last_time, now);
    } while (timeout.tv_usec || timeout.tv_sec);

    /* Poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("Select poll");
      return;
    }
    /* If there are new connections waiting, accept them. */
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);

    /* Kick out the freaky folks in the exception set and marked for close */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
	FD_CLR(d->descriptor, &input_set);
	FD_CLR(d->descriptor, &output_set);
	close_socket(d);
      }
    }

    /* Process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
	if (process_input(d) < 0)
	  close_socket(d);
    }

    /* Process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      /* Not combined to retain --(d->wait) behavior. -gg 2/20/98 */
      d->wait -= (d->wait > 0);
      if (d->wait > 0)
        continue;

      if (!get_from_q(&d->input, comm, &aliased))
        continue;

	if (d->character) {
	  /* Reset the idle timer & pull char back from void if necessary */
	  d->character->char_specials.timer = 0;
	if (STATE(d) == CON_PLAYING && GET_WAS_IN(d->character) != NOWHERE) {
	    if (d->character->in_room != NOWHERE)
	      char_from_room(d->character);
	    char_to_room(d->character, GET_WAS_IN(d->character));
	    GET_WAS_IN(d->character) = NOWHERE;
	    act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
	  }
	}
	d->wait = 1;
	d->has_prompt = 0;

	 /* reversed these top 2 if checks so that you can use the page_string */
	 /* function in the editor */
	if (d->showstr_count)	/* reading something w/ pager     */
	  show_string(d, comm);
	else if (d->str)		/* writing boards, mail, etc.     */
	  string_add(d, comm);
	else if (STATE(d) != CON_PLAYING)	/* in menus, etc. */
	  nanny(d, comm);
	else {			/* else: we're playing normally */
	  if (aliased)		/* to prevent recursive aliases */
	    d->has_prompt = 1;
	  else
	    if (perform_alias(d, comm))		/* run it through aliasing system */
	      get_from_q(&d->input, comm, &aliased);
	  
	  command_interpreter(d->character, comm);	/* send it to interpreter */
	}
      }
    

    /* Send queued output out to the operating system (ultimately to user) */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (*(d->output) && FD_ISSET(d->descriptor, &output_set)) {
        /* Output for this player is ready */
	if (process_output(d) < 0)
	  close_socket(d);
	else
	  d->has_prompt = 1;
      }
    }
    /* Print prompts for other descriptors who had no other output */
    for (d = descriptor_list; d; d = d->next) {
      if (!d->has_prompt) {
	write_to_descriptor(d->descriptor, make_prompt(d));
	d->has_prompt = 1;
      }
    }
    
    /* kick out folks in the CON_CLOSE or CON_DISCONNECT state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE || STATE(d) == CON_DISCONNECT)
	close_socket(d);
    }
    /*
     * Now, we execute as many pulses as necessary--just one if we haven't
     * missed any pulses, or make up for lost time if we missed a few
     * pulses by sleeping for too long.
     */
    missed_pulses++;

    if (missed_pulses <= 0) {
      log("SYSERR: **BAD** MISSED_PULSES NONPOSITIVE (%d), TIME GOING BACKWARDS!!", missed_pulses);
      missed_pulses = 1;
    }

    /* If we missed more than 30 seconds worth of pulses, just do 30 secs */
    if (missed_pulses > (30 * PASSES_PER_SEC)) {
      log("SYSERR: Missed %d seconds worth of pulses.", missed_pulses / PASSES_PER_SEC);
      missed_pulses = 30 * PASSES_PER_SEC;
    }

    /* Now execute the heartbeat functions */
    while (missed_pulses--)
      heartbeat(++pulse);

    /* Roll pulse over after 10 hours */
    if (pulse >= (600 * 60 * PASSES_PER_SEC))
      pulse = 0;

#ifdef CIRCLE_UNIX
    /* Update tics for deadlock protection (UNIX only) */
    tics++;
#endif
  }
}


void heartbeat(int pulse)
{
  static int mins_since_crashsave = 0;
  //event_process();
  process_program_output();
  
  if (!(pulse % (30 * PASSES_PER_SEC)))
    sanity_check();
    
  if (!(pulse % PULSE_ZONE))
    zone_update();

  if (!(pulse % (15 * PASSES_PER_SEC)))		/* 15 seconds */
    check_idle_passwords();

  if (!(pulse % PULSE_MOBILE))
    mobile_activity();

  if (!(pulse % PULSE_VIOLENCE)) {
    tick_grenade();
  }
  if (!(pulse % PULSE_REGEN)) {
    perform_violence();
    point_update();
    make_who2html();

  }

  if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
    affect_update();
    parts_update();
    weather_and_time(1);
    fflush(player_fl);
    blood_update();
  //mobile_activity();
    }
  
  if (!(pulse % PULSE_TRAINS))
         train_upd();
         
  if (auto_save && !(pulse % (30 * PASSES_PER_SEC))) {	/* 30 seconds */
    if (++mins_since_crashsave >= autosave_time) {
      mins_since_crashsave = 0;
      Crash_save_all();
      House_save_all();
      clan_save(YES, NULL); /* Fido - Crash save and auto save clans */
    }
  }
  
  if (!(pulse % (5 * 60 * PASSES_PER_SEC)))	/* 5 minutes */
    record_usage();
    
  if (!(pulse % (15 * PASSES_PER_SEC)))         /* 15 seconds */
    pmail_check();
    
  /* Do teleport stuff */
  if (teleport_on && !(pulse % PULSE_TELEPORT))
    TeleportPulseStuff();
}


/* ******************************************************************
*  general utility stuff (for local use)                            *
****************************************************************** */

/*
 *  new code to calculate time differences, which works on systems
 *  for which tv_usec is unsigned (and thus comparisons for something
 *  being < 0 fail).  Based on code submitted by ss@sirocco.cup.hp.com.
 */

/*
 * code to return the time difference between a and b (a-b).
 * always returns a nonnegative value (floors at 0).
 *
 * Fixed the 'aggregate return' warning.  Now it's not thread-safe.
 *	-gg 6/18/98
 */
struct timeval *timediff(struct timeval a, struct timeval b)
{
  static struct timeval rslt;

  if (a.tv_sec < b.tv_sec)
    return &null_time;
  else if (a.tv_sec == b.tv_sec) {
    if (a.tv_usec < b.tv_usec)
      return &null_time;
    else {
      rslt.tv_sec = 0;
      rslt.tv_usec = a.tv_usec - b.tv_usec;
      return &rslt;
    }
  } else {			/* a->tv_sec > b->tv_sec */
    rslt.tv_sec = a.tv_sec - b.tv_sec;
    if (a.tv_usec < b.tv_usec) {
      rslt.tv_usec = a.tv_usec + 1000000 - b.tv_usec;
      rslt.tv_sec--;
    } else
      rslt.tv_usec = a.tv_usec - b.tv_usec;
    return &rslt;
  }
}

/*
 * add 2 timevals
 *
 * Fixed the 'aggregate return' warning. Not thread-safe now.
 *	-gg 6/18/98
 */
struct timeval *timeadd(struct timeval a, struct timeval b)
{
  static struct timeval rslt;

  rslt.tv_sec = a.tv_sec + b.tv_sec;
  rslt.tv_usec = a.tv_usec + b.tv_usec;

  while (rslt.tv_usec >= 1000000) {
    rslt.tv_usec -= 1000000;
    rslt.tv_sec++;
  }

  return &rslt;
}


void record_usage(void)
{
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (STATE(d) == CON_PLAYING)
      sockets_playing++;
  }

  log("nusage: %-3d sockets connected, %-3d sockets playing",
	  sockets_connected, sockets_playing);

#ifdef RUSAGE
  {
    struct rusage ru;

    getrusage(RUSAGE_SELF, &ru);
    log("rusage: user time: %ld sec, system time: %ld sec, max res size: %ld",
	    ru.ru_utime.tv_sec, ru.ru_stime.tv_sec, ru.ru_maxrss);
  }
#endif

}



/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] =
  {
    (char) IAC,
    (char) WILL,
    (char) TELOPT_ECHO,
    (char) 0,
  };

  SEND_TO_Q(off_string, d);
}


/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] =
  {
    (char) IAC,
    (char) WONT,
    (char) TELOPT_ECHO,
    (char) TELOPT_NAOFFD,
    (char) TELOPT_NAOCRD,
    (char) 0,
  };

  SEND_TO_Q(on_string, d);
}


/* this little function right here is taken from my inline color code
   I wrote some two years ago from the long gone Krimson DIKUMUD... I
   didn't want to rely upon the easy_color patch (and besides, which,
   I still don't know the actual codes it uses)  This is how the colors
   are on my (currently siteless) MUD. */
int is_color(char c) {
  switch (c) {
  case 'x': return 30; break;
  case 'r': return 31; break;
  case 'g': return 32; break;
  case 'y': return 33; break;
  case 'b': return 34; break;
  case 'm': return 35; break;
  case 'c': return 36; break;
  case 'w': return 37; break;
  
  case '0': return 40; break;
  case '1': return 44; break;
  case '2': return 42; break;
  case '3': return 46; break;
  case '4': return 41; break;
  case '5': return 45; break;
  case '6': return 43; break;
  case '7': return 47; break;
  
  case 'f': return  1; break;
  case '&': return -1; break;
  default : return  0; break;
  }
}


char *get_status_text(int perc) {
       if (perc >= 95) return("unscathed");
  else if (perc >= 84) return("slightly scratched");
  else if (perc >= 75) return("scratched");
  else if (perc >= 50) return("beaten-up");
  else if (perc >= 37) return("beaten-up badly");
  else if (perc >= 25) return("bloody");
  else return("near death");
}

char *interpret_colors(const char *str, bool parse) {
  int clr = 37, bg_clr = 40, flash = 0;
  static char cbuf[MAX_STRING_LENGTH];
  char *cp, *tmp;  
  char i[32];

  if (show_color_codes || !strchr(str, '&'))
    return ((char *)str);

  cp = cbuf;
  
  for (;;) {
    if (*str == '&') {
      str++;
      if ((clr = is_color(LOWER(*str))) > 0 && parse) {
        if (IS_UPPER(*str)) sprintf(i, "\x1b[1;");
        else                sprintf(i, "\x1b[0;");
          
        if (clr >= 40) {
          bg_clr = 40;
          str++;
          continue;
        } else if (clr == 1) {
          flash = !flash;
          str++;
          continue;
        }
          
        sprintf(i, "%s%s%d;%dm", i, (flash ? "5;" : ""), bg_clr, clr);
        tmp = i;
      } else if (clr == -1) {
        *(cp++) = '&';
        str++;
        continue;
      } else {
        str++;
        continue;
      }
      while ((*cp = *(tmp++)))
	cp++;
      str++;
    } else if (!(*(cp++) = *(str++)))
      break;
  }
  
  *cp = '\0';
  return (cbuf);
}

/* Initialize a descriptor */
void init_descriptor (struct descriptor_data *newd, int desc)
{
    static int last_desc = 0;	/* last descriptor number */
    
	newd->descriptor = desc;
	newd->connected = CON_QANSI;
//	newd->idle_tics = 0;
	newd->wait = 1;
	newd->output = newd->small_outbuf;
	newd->bufspace = SMALL_BUFSIZE - 1;
	newd->next = descriptor_list;
	newd->login_time = time (0);
	*newd->output = '\0';
  	newd->bufptr = 0;
  	newd->has_prompt = 1;  /* prompt is part of greetings */
  	/*
   	 * This isn't exactly optimal but allows us to make a design choice.
   	 * Do we embed the history in descriptor_data or keep it dynamically
   	 * allocated and allow a user defined history size?
   	 */
  	CREATE(newd->history, char *, HISTORY_SIZE);

	if (++last_desc == 1000)
		last_desc = 1;
	newd->desc_num = last_desc;

}

char *prompt_str(struct char_data *ch) {
  struct char_data *vict = FIGHTING(ch);  
  static char pbuf[MAX_STRING_LENGTH];  
  char *str = GET_PROMPT(ch);
  struct char_data *tank;
  int perc, color;  
  char *cp, *tmp;
  char i[256];
  
  color = ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_COLOR_1 | PRF_COLOR_2)) ? 1 : 0);
  
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AFK)) {
    if (!PRF_FLAGGED(ch, PRF_LOCKED))
      strcpy(pbuf, "&W[&MAFK&W]&w ");
    else
      sprintf(pbuf, "&W[&MAFK&C LOCK&W - &B%s&W]&w Enter password:", GET_NAME(ch));
    return (interpret_colors(pbuf, color));
  }
  
  if (!str || !*str)
    str = "&G-=#+ &YDesperate Visions&X: &cSet your prompt (see &X'&CHELP DISPLAY, PROMPT&X'&c) &G+#=-\n\r&W> &w";

  
    
  if (!strchr(str, '%'))
    return (interpret_colors(str, color));
  
  cp = pbuf;
  
  for (;;) {
    if (*str == '%') {
      switch (*(++str)) {
      case 'h': /* current hitp */
        sprintf(i, "%d", GET_HIT(ch));
        tmp = i;
        break;
      case 'H': /* maximum hitp */
        sprintf(i, "%d", GET_MAX_HIT(ch));
        tmp = i;
        break;
      case 'm': /* current mana */
        sprintf(i, "%d", GET_MANA(ch));
        tmp = i;
        break;
      case 'M': /* maximum mana */
        sprintf(i, "%d", GET_MAX_MANA(ch));
        tmp = i;
        break;
      case 'v': /* current moves */
        sprintf(i, "%d", GET_MOVE(ch));
        tmp = i;
        break;
      case 'V': /* maximum moves */
        sprintf(i, "%d", GET_MAX_MOVE(ch));
        tmp = i;
        break;
      case 'P':
      case 'p': /* percentage of hitp/move/mana */
        str++;
        switch (LOWER(*str)) {
        case 'h':
          perc = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
          break;
        case 'm':
          perc = (100 * GET_MANA(ch)) / GET_MAX_MANA(ch);
          break;
        case 'v':
          perc = (100 * GET_MOVE(ch)) / GET_MAX_MOVE(ch);
          break;
        case 'x':
          perc = (100 * GET_EXP(ch)) / level_exp(GET_CLASS(ch),GET_LEVEL(ch)+1);
          break;
        case 'o':
          if (vict) 
          	perc = (100*GET_HIT(vict)) / GET_MAX_HIT(vict);
          else perc = UNDEFPERC;
          break;
        case 'T':
        case 't': /* tank */
          if (vict && (tank = FIGHTING(vict)) && tank != ch) {
            perc = (100*GET_HIT(tank)) / GET_MAX_HIT(tank);
          } else {
            perc = UNDEFPERC;
        }
        break;
      
        default :
          perc = UNDEFPERC;
          break;
        }
        if (perc != UNDEFPERC)
        	sprintf(i, "%d%%", perc);
        else i[0] = '\0';
        tmp = i;
        break; 
      case 'O':
      case 'o': /* opponent */
        if (vict) {
          perc = (100*GET_HIT(vict)) / GET_MAX_HIT(vict);
          sprintf(i, "%s &X(&r%s&X)&w", PERS(vict, ch), get_status_text(perc));
          tmp = i;
        } else {
          str++;
          continue;
        }
        break;
      case 'x': /* current exp */
        sprintf(i, "%d", GET_EXP(ch));
        tmp = i;
        break;
      case 'X': /* exp to level */
        sprintf(i, "%d", level_exp(GET_CLASS(ch),GET_LEVEL(ch)+1) -
                GET_EXP(ch));
        tmp = i;
        break;
      case 'g': /* gold on hand */
        sprintf(i, "%d", GET_GOLD(ch));
        tmp = i;
        break;
      case 'G': /* gold in bank */
        sprintf(i, "%d", GET_BANK_GOLD(ch));
        tmp = i;
        break;
      case 'T':
      case 't': /* tank */
        if (vict && (tank = FIGHTING(vict)) && tank != ch) {
          perc = (100*GET_HIT(tank)) / GET_MAX_HIT(tank);
          sprintf(i, "%s &X(&r%s&X)&w", PERS(tank, ch),
            get_status_text(perc));
          tmp = i;
        } else {
          str++;
          continue;
        }
        break;
        
      case '_':
        tmp = "\r\n";
        break;
      case '%':
        *(cp++) = '%';
        str++;
        continue;
        break;
      
      default :
        str++;
        continue;
        break;
      }
      
      while ((*cp = *(tmp++)))
        cp++;
      str++;
    } else if (!(*(cp++) = *(str++)))
      break;
  }
  
  *cp = '\0';
  strcat(pbuf, " &w");
  return (interpret_colors(pbuf, color));
}


char *make_prompt(struct descriptor_data *d)
{
  static char prompt[MAX_INPUT_LENGTH];
  int color = 0;
  
  *prompt = '\0';
  if (d->showstr_count) {
    color = (!IS_NPC(d->character) 
      && PRF_FLAGGED(d->character, PRF_COLOR_1 | PRF_COLOR_2) ? 1 : 0);
    sprintf(prompt,
	    "\r\n&W[ Return to continue, (q)uit, (r)efresh, (b)ack, or page number (%d/%d) ]&w",
	    d->showstr_page, d->showstr_count);
    return (interpret_colors(prompt, color));
  } else if (d->str)
      strcpy(prompt, "] ");
    else if (STATE(d) == CON_PLAYING)
      strcpy(prompt, prompt_str(d->character));
  return((char *)&prompt);
}


void write_to_q(const char *txt, struct txt_q *queue, int aliased)
{
  struct txt_block *newt;

  CREATE(newt, struct txt_block, 1);
  CREATE(newt->text, char, strlen(txt) + 1);
  strcpy(newt->text, txt);
  newt->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    newt->next = NULL;
    queue->head = queue->tail = newt;
  } else {
    queue->tail->next = newt;
    queue->tail = newt;
    newt->next = NULL;
  }
}



int get_from_q(struct txt_q *queue, char *dest, int *aliased)
{
  struct txt_block *tmp;

  /* queue empty? */
  if (!queue->head)
    return 0;

  tmp = queue->head;
  strcpy(dest, queue->head->text);
  *aliased = queue->head->aliased;
  queue->head = queue->head->next;

  free(tmp->text);
  free(tmp);

  return 1;
}


/*
 * get_bind_addr: Return a struct in_addr that should be used in our
 * call to bind().  If the user has specified a desired binding
 * address, we try to bind to it; otherwise, we bind to INADDR_ANY.
 * Note that inet_aton() is preferred over inet_addr() so we use it if
 * we can.  If neither is available, we always bind to INADDR_ANY.
 */

struct in_addr *get_bind_addr()
{
  static struct in_addr bind_addr;

  /* Clear the structure */
  memset((char *) &bind_addr, 0, sizeof(bind_addr));

  /* If DLFT_IP is unspecified, use INADDR_ANY */
  if (DFLT_IP == NULL) {
    bind_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* If the parsing fails, use INADDR_ANY */
    if (!parse_ip(DFLT_IP, &bind_addr)) {
      log("SYSERR: DFLT_IP of %s appears to be an invalid IP address",DFLT_IP);
      bind_addr.s_addr = htonl(INADDR_ANY);
    }
  }

  /* Put the address that we've finally decided on into the logs */
  if (bind_addr.s_addr == htonl(INADDR_ANY))
    log("Binding to all IP interfaces on this host.");
  else
    log("Binding only to IP address %s", inet_ntoa(bind_addr));

  return &bind_addr;
}

#ifdef HAVE_INET_ATON

/*
 * inet_aton's interface is the same as parse_ip's: 0 on failure, non-0 if
 * successful
 */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  return inet_aton(addr, inaddr);
}

#elif HAVE_INET_ADDR

/* inet_addr has a different interface, so we emulate inet_aton's */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  long ip;

  if ((ip = inet_addr(addr)) == -1) {
    return 0;
  } else {
    inaddr->s_addr = (unsigned long) ip;
    return 1;
  }
}

#else

/* If you have neither function - sorry, you can't do specific binding. */
int parse_ip(const char *addr, struct in_addr *inaddr)
{
  log("SYSERR: warning: you're trying to set DFLT_IP but your system has no\n"
      "functions to parse IP addresses (how bizarre!)");
  return 0;
}

#endif /* INET_ATON and INET_ADDR */



/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d)
{
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy));
}

char *linewrap(char *str, int max)
{
  char xbuf2[MAX_STRING_LENGTH];
  static char xbuf[MAX_STRING_LENGTH];
  char *tmp;
  int i, lline = 0, curr = 0;
  bool spec_code = FALSE;

  xbuf[0] = xbuf2[0] = '\0';
  i = max - 1;

  if (strlen(str) < i)
    return (str);

  for (tmp = str; *tmp; tmp++) {
    if (*tmp == '\x1B' && !spec_code)
      spec_code = TRUE;
    if (*tmp == ' ') {
      spec_code = FALSE;
      if (lline > i) {
        sprintf(xbuf, "%s\r\n%s ", xbuf, xbuf2);
        lline = 0;
        curr = 0;
        xbuf2[0] = '\0';
      } else {
        sprintf(xbuf, "%s%s ", xbuf, xbuf2);
        lline++;
        curr = 0;
        xbuf2[0] = '\0';
      }
    } else if (*tmp == '\r') {
      spec_code = FALSE;
      if (lline > (i + 1))
        sprintf(xbuf, "%s\r\n%s", xbuf, xbuf2);
      else
        sprintf(xbuf, "%s%s\r", xbuf, xbuf2);
      lline = 0;
      curr = 0;
      xbuf2[0] = '\0';
    } else {
      xbuf2[curr] = *tmp;
      xbuf2[curr + 1] = '\0';
      curr++;
      if (!spec_code)
        lline++;
    }
    if (*tmp == 'm' && spec_code)
      spec_code = FALSE;
  }
  if (lline > i)
    sprintf(xbuf, "%s\r\n%s", xbuf, xbuf2);
  else
    sprintf(xbuf, "%s%s", xbuf, xbuf2);

  // return (str_dup(xbuf));
  return(xbuf);
}

/* Add a new string to a player's output queue */
void write_to_output(char *txt, struct descriptor_data *t)
{
  int size;
  char *tt;
  tt = linewrap(txt,80);
  
  size = strlen(tt);
  
  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    {
//      free(tt);
      return;
    }

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size) {
    strcpy(t->output + t->bufptr, tt);
    t->bufspace -= size;
    t->bufptr += size;
//    free(tt);
    return;
  }
  /*
   * If the text is too big to fit into even a large buffer, chuck the
   * new text and switch to the overflow state.
   */
  if (size + t->bufptr > LARGE_BUFSIZE - 1) {
    t->bufptr = -1;
    buf_overflows++;
//    free(tt);
    return;
  }
  buf_switches++;
  
  /* if the pool has a buffer in it, grab it */
  if (bufpool != NULL) {
    t->large_outbuf = bufpool;
    bufpool = bufpool->next;
  } else {			/* else create a new one */
    CREATE(t->large_outbuf, struct txt_block, 1);
    CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
    buf_largecount++;
  }

  strcpy(t->large_outbuf->text, t->output);	/* copy to big buffer */
  t->output = t->large_outbuf->text;	/* make big buffer primary */
  strcat(t->output, txt);	/* now add new text */

  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);
	  
  /* calculate how much space is left in the buffer */
  t->bufspace = LARGE_BUFSIZE - 1 - t->bufptr;
  
  /* set the pointer for the next write */
  t->bufptr = strlen(t->output);
}



/* ******************************************************************
*  socket handling                                                  *
****************************************************************** */


/* Sets the kernel's send buffer size for the descriptor */
int set_sendbuf(socket_t s)
{
#if defined(SO_SNDBUF) && !defined(CIRCLE_MACINTOSH)
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return -1;
  }

#if 0
  if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *) &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt RCVBUF");
    return -1;
  }
#endif

#endif
  return 0;
}



/* dnsmod */
int new_descriptor(int s) {
/*  socket_t desc; */
  int desc, sockets_connected = 0;
  unsigned int i;
  unsigned long addr;
  struct dns_entry dns;
  char buf[256];
//  static int last_desc = 0;	/* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  struct hostent *from;
  extern char *ANSI;

  int get_host_from_cache(struct dns_entry *dnsd);
  void add_dns_host(struct dns_entry *dnsd, char *hostname);

  /* accept the new connection */
  i = sizeof(peer);
  if ((desc = accept(s, (struct sockaddr *) &peer, &i)) < 0) {
    perror("accept");
    return -1;
  }
  /* keep it from blocking */
  nonblock(desc);

  /* set the send buffer size */
  if (set_sendbuf(desc) < 0) {
    CLOSE_SOCKET(desc);
    return 0;
  }

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next)
    sockets_connected++;

  if (sockets_connected >= max_players) {
    write_to_descriptor(desc, "Sorry, CircleMUD is full right now... please try again later!\r\n");
    close(desc);
    return 0;
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *) newd, 0, sizeof(struct descriptor_data));

  /* find the numeric site address */
  addr = ntohl(peer.sin_addr.s_addr);
  dns.ip[0] = (int) ((addr & 0xFF000000) >> 24);
  dns.ip[1] = (int) ((addr & 0x00FF0000) >> 16);
  dns.ip[2] = (int) ((addr & 0x0000FF00) >> 8);
  dns.ip[3] = (int) ((addr & 0x000000FF));
  dns.name = NULL;
  dns.next = NULL;

  if(!get_host_from_cache(&dns)) { /* cache lookup failed */
    /* find the sitename */
    if (nameserver_is_slow || !(from = gethostbyaddr((char *) &peer.sin_addr,
				      sizeof(peer.sin_addr), AF_INET))) {

      /* resolution failed */
      if (!nameserver_is_slow)
        perror("gethostbyaddr");
        
/*
      sprintf(newd->host, "%03u.%03u.%03u.%03u",
	(int) ((addr & 0xFF000000) >> 24),
	(int) ((addr & 0x00FF0000) >> 16), (int) ((addr & 0x0000FF00) >> 8),
	(int) ((addr & 0x000000FF)));
*/
      strncpy(newd->host, inet_ntoa(peer.sin_addr), HOST_LENGTH);
      *(newd->host + HOST_LENGTH) = '\0';
      sprintf(buf, "DNS lookup failed on %s.", newd->host);
      mudlog(buf, CMP, LVL_GOD, TRUE);

    } else {
      strncpy(newd->host, from->h_name, HOST_LENGTH);
      *(newd->host + HOST_LENGTH) = '\0';
      add_dns_host(&dns, newd->host);
    }
  } else {
    strncpy(newd->host, dns.name, HOST_LENGTH);
  }

  /* determine if the site is banned */
  if (isbanned(newd->host) == BAN_ALL) {
    write_to_descriptor(desc, "Your site is BANNED!\r\n");
    close(desc);
    sprintf(buf2, "Connection attempt denied from [%s]", newd->host);
    mudlog(buf2, CMP, LVL_GOD, TRUE);
    free(newd);
    return 0;
  }
#if 0
  /* Log new connections - probably unnecessary, but you may want it */
  sprintf(buf2, "New connection from [%s]", newd->host);
  mudlog(buf2, CMP, LVL_GOD, FALSE);
#endif

  init_descriptor(newd, desc);

  /* initialize descriptor data */
  /*
  newd->descriptor = desc;
  newd->connected = CON_QANSI; */
/*  newd->idle_tics = 0; */
  /*newd->wait = 1;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->login_time = time(0);
  *newd->output = '\0';
  newd->bufptr = 0;*/
  /*newd->has_prompt = 1; */ /* prompt is part of greetings */
  /*CREATE(newd->history, char *, HISTORY_SIZE);*/

  /*
   * This isn't exactly optimal but allows us to make a design choice.
   * Do we embed the history in descriptor_data or keep it dynamically
   * allocated and allow a user defined history size?
   */
  /*CREATE(newd->history, char *, HISTORY_SIZE); */
/*
  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;
*/
  /* prepend to list */
  newd->next = descriptor_list;
  descriptor_list = newd;
  if (numgreetings)
    SEND_TO_Q(greetings[number(0,numgreetings-1)], newd);
  SEND_TO_Q(ANSI, newd);

  return 0;
}

/*
 * Send all of the output that we've accumulated for a player out to
 * the player's descriptor.
 * FIXME - This will be rewritten before 3.1, this code is dumb.
 */
int process_output(struct descriptor_data *t)
{
  char i[MAX_SOCK_BUF];
  int result;

  /* we may need this \r\n for later -- see below */
  strcpy(i, "\r\n");

  /* now, append the 'real' output */
  strcpy(i + 2, t->output);

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    strcat(i, "**OVERFLOW**\r\n");

  /* add the extra CRLF if the person isn't in compact mode */
  if (STATE(t) == CON_PLAYING && t->character && (IS_NPC(t->character) || !PRF_FLAGGED(t->character, PRF_COMPACT)))
    strcat(i, "\r\n");

  /* add a prompt if playing*/
  // if (STATE(t) == CON_PLAYING)
    strncat(i, make_prompt(t), MAX_PROMPT_LENGTH);

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (t->has_prompt)		/* && !t->connected) */
    result = write_to_descriptor(t->descriptor, i);
  else
    result = write_to_descriptor(t->descriptor, i + 2);

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  return result;
}


/*
 * perform_socket_write: takes a descriptor, a pointer to text, and a
 * text length, and tries once to send that text to the OS.  This is
 * where we stuff all the platform-dependent stuff that used to be
 * ugly #ifdef's in write_to_descriptor().
 *
 * This function must return:
 *
 * -1  If a fatal error was encountered in writing to the descriptor.
 *  0  If a transient failure was encountered (e.g. socket buffer full).
 * >0  To indicate the number of bytes successfully written, possibly
 *     fewer than the number the caller requested be written.
 *
 * Right now there are two versions of this function: one for Windows,
 * and one for all other platforms.
 */

#if defined(CIRCLE_WINDOWS)

ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  ssize_t result;

  result = send(desc, txt, length, 0);

  if (result > 0) {
    /* Write was sucessful */
    return result;
  }

  if (result == 0) {
    /* This should never happen! */
    log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return -1;
  }

  /* result < 0: An error was encountered. */

  /* Transient error? */
      if (WSAGetLastError() == WSAEWOULDBLOCK)
    return 0;

  /* Must be a fatal error. */
  return -1;
}

#else

#if defined(CIRCLE_ACORN)
#define write	socketwrite
#endif

/* perform_socket_write for all Non-Windows platforms */
ssize_t perform_socket_write(socket_t desc, const char *txt, size_t length)
{
  ssize_t result;

  result = write(desc, txt, length);

  if (result > 0) {
    /* Write was successful. */
    return result;
  }

  if (result == 0) {
    /* This should never happen! */
    log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return -1;
  }

  /*
   * result < 0, so an error was encountered - is it transient?
   * Unfortunately, different systems use different constants to
   * indicate this.
   */

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return 0;
#endif

#ifdef EWOULDBLOCK	/* BSD */
      if (errno == EWOULDBLOCK)
    return 0;
#endif

#ifdef EDEADLK		/* Macintosh */
      if (errno == EDEADLK)
    return 0;
#endif

  /* Looks like the error was fatal.  Too bad. */
  return -1;
}

#endif /* CIRCLE_WINDOWS */

    
/*
 * write_to_descriptor takes a descriptor, and text to write to the
 * descriptor.  It keeps calling the system-level write() until all
 * the text has been delivered to the OS, or until an error is
 * encountered.
 *
 * Returns:
 *  0  If all is well and good,
 * -1  If an error was encountered, so that the player should be cut off
 */
int write_to_descriptor(socket_t desc, const char *txt)
{
  size_t total;
  ssize_t bytes_written;

  total = strlen(txt);

  while (total > 0) {
    bytes_written = perform_socket_write(desc, txt, total);

    if (bytes_written < 0) {
      /* Fatal error.  Disconnect the player. */
	perror("Write to socket");
      return -1;
    } else if (bytes_written == 0) {
      /*
       * Temporary failure -- socket buffer full.  For now we'll just
       * cut off the player, but eventually we'll stuff the unsent
       * text into a buffer and retry the write later.  JE 30 June 98.
       */
      log("process_output: socket write would block, about to close");
      return -1;
    } else {
      txt += bytes_written;
      total -= bytes_written;
    }
  }

  return 0;
}


/*
 * Same information about perform_socket_write applies here. I like
 * standards, there are so many of them. -gg 6/30/98
 */
ssize_t perform_socket_read(socket_t desc, char *read_point, size_t space_left)
{
  ssize_t ret;

#if defined(CIRCLE_ACORN)
  ret = recv(desc, read_point, space_left, MSG_DONTWAIT);
#elif defined(CIRCLE_WINDOWS)
  ret = recv(desc, read_point, space_left, 0);
#else
  ret = read(desc, read_point, space_left);
#endif

  /* Read was successful. */
  if (ret > 0)
    return ret;

  /* read() returned 0, meaning we got an EOF. */
  if (ret == 0) {
    log("EOF on socket read (connection broken by peer)");
    return -1;
  }

  /*
   * read returned a value < 0: there was an error
   */

#if defined(CIRCLE_WINDOWS)	/* Windows */
  if (WSAGetLastError() == WSAEWOULDBLOCK)
    return 0;
#else

#ifdef EINTR		/* Interrupted system call - various platforms */
  if (errno == EINTR)
    return 0;
#endif

#ifdef EAGAIN		/* POSIX */
  if (errno == EAGAIN)
    return 0;
#endif

#ifdef EWOULDBLOCK	/* BSD */
  if (errno == EWOULDBLOCK)
    return 0;
#endif /* EWOULDBLOCK */

#ifdef EDEADLK		/* Macintosh */
  if (errno == EDEADLK)
    return 0;
#endif

#endif /* CIRCLE_WINDOWS */

  /* We don't know what happened, cut them off. */
  perror("process_input: about to lose connection");
  return -1;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data *t)
{
  int buf_length, failed_subst;
  ssize_t bytes_read;
  size_t space_left;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH + 8];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      log("process_input: about to close connection: input overflow");
      return -1;
    }

    bytes_read = perform_socket_read(t->descriptor, read_point, space_left);

    if (bytes_read < 0)	/* Error, disconnect them. */
      return -1;
    else if (bytes_read == 0)	/* Just blocking, no problems. */
      return 0;

    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0';	/* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

/*
 * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
 * causing the MUD to hang when it encounters input not terminated by a
 * newline.  This was causing hangs at the Password: prompt, for example.
 * I attempt to compensate by always returning after the _first_ read, instead
 * of looping forever until a read returns -1.  This simulates non-blocking
 * I/O because the result is we never call read unless we know from select()
 * that data is ready (process_input is only called if select indicates that
 * this descriptor is in the read set).  JE 2/23/95.
 */
#if !defined(POSIX_NONBLOCK_BROKEN)
  } while (nl_pos == NULL);
#else
  } while (0);

  if (nl_pos == NULL)
    return 0;
#endif /* POSIX_NONBLOCK_BROKEN */

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b' || *ptr == 127) { /* handle backspacing or delete key */
	if (write_point > tmp) {
	  if (*(--write_point) == '$') {
	    write_point--;
	    space_left += 2;
	  } else
	    space_left++;
	}
      } else if (isascii(*ptr) && isprint(*ptr)) {
	if ((*(write_point++) = *ptr) == '$') {		/* copy one character */
	  *(write_point++) = '$';	/* if it's a $, double it */
	  space_left -= 2;
	} else
	  space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      sprintf(buffer, "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
	return -1;
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if (*tmp == '!' && !(*(tmp + 1)))	/* Redo last command. */
      strcpy(tmp, t->last_input);
    else if (*tmp == '!' && *(tmp + 1)) {
      char *commandln = (tmp + 1);
      int starting_pos = t->history_pos,
	  cnt = (t->history_pos == 0 ? HISTORY_SIZE - 1 : t->history_pos - 1);

      skip_spaces(&commandln);
      for (; cnt != starting_pos; cnt--) {
	if (t->history[cnt] && is_abbrev(commandln, t->history[cnt])) {
	  strcpy(tmp, t->history[cnt]);
	  strcpy(t->last_input, tmp);
	  break;
	}
        if (cnt == 0)	/* At top, loop to bottom. */
	  cnt = HISTORY_SIZE;
      }
    } else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
	strcpy(t->last_input, tmp);
    } else {
      strcpy(t->last_input, tmp);
      if (t->history[t->history_pos])
	free(t->history[t->history_pos]);	/* Clear the old line. */
      t->history[t->history_pos] = str_dup(tmp);	/* Save the new. */
      if (++t->history_pos >= HISTORY_SIZE)	/* Wrap to top. */
	t->history_pos = 0;
    }

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
	nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return 1;
}



/* perform substitution for the '^..^' csh-esque syntax orig is the
 * orig string, i.e. the one being modified.  subst contains the
 * substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst)
{
  char newsub[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }
  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(newsub, orig, (strpos - orig));
  newsub[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(newsub, second, (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(newsub, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(newsub) - 1));

  /* terminate the string in case of an overflow from strncat */
  newsub[MAX_INPUT_LENGTH - 1] = '\0';
  strcpy(subst, newsub);

  return 0;
}



void close_socket(struct descriptor_data *d)
{
  char buf[128];
  struct descriptor_data *temp;

  REMOVE_FROM_LIST(d, descriptor_list, next);
  CLOSE_SOCKET(d->descriptor);
  flush_queues(d);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }
  
   /* Clean up clan edit if necessary */
    switch (d->connected)
    {
      case CON_CLAN_EDIT:
        cleanup_clan_edit(d);
        break;
    }
 

  /*. Kill any OLC stuff .*/
  switch(d->connected)
  { case CON_OEDIT:
    case CON_REDIT:
    case CON_ZEDIT:
    case CON_MEDIT:
    case CON_SEDIT:
      cleanup_olc(d, CLEANUP_ALL);
    default:
      break;
  }

  if (d->character) {
    /*
     * Plug memory leak, from Eric Green.
     */
    if (PLR_FLAGGED(d->character, PLR_MAILING) && d->str) {
      if (*(d->str))
        free(*(d->str));
      free(d->str);
    }
    if (d->connected == CON_PLAYING || STATE(d) == CON_DISCONNECT) {
      save_char(d->character, NOWHERE);
      act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
      sprintf(buf, "Closing link to: %s.", GET_NAME(d->character));
      mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
      d->character->desc = NULL;
    } else {
      sprintf(buf, "Losing player: %s.",
	      GET_NAME(d->character) ? GET_NAME(d->character) : "<null>");
      mudlog(buf, CMP, LVL_IMMORT, TRUE);
      free_char(d->character);
    }
  } else
    mudlog("Losing descriptor without char.", CMP, LVL_IMMORT, TRUE);

  /* JE 2/22/95 -- part of my unending quest to make switch stable */
  if (d->original && d->original->desc)
    d->original->desc = NULL;

  /* Clear the command history. */
  if (d->history) {
    int cnt;
    for (cnt = 0; cnt < HISTORY_SIZE; cnt++)
      if (d->history[cnt])
	free(d->history[cnt]);
    free(d->history);
  }
  if (d->showstr_head)
    free(d->showstr_head);
  if (d->showstr_count)
    free(d->showstr_vector);
  if (d->storage)
    free(d->storage);

  free(d);
}



void check_idle_passwords(void)
{
  struct descriptor_data *d, *next_d;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (STATE(d) != CON_PASSWORD && STATE(d) != CON_GET_NAME 
    && STATE(d) != CON_QANSI)
      continue;
    if (!d->idle_tics) {
      d->idle_tics++;
      continue;
    } else {
      echo_on(d);
      SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
      STATE(d) = CON_CLOSE;
    }
  }
}



/*
 * I tried to universally convert Circle over to POSIX compliance, but
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */

#if defined(CIRCLE_WINDOWS)

void nonblock(socket_t s)
{
  long val;

  val = 1;
  ioctlsocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_AMIGA)

void nonblock(socket_t s)
{
  long val;

  val = 1;
  IoctlSocket(s, FIONBIO, &val);
}

#elif defined(CIRCLE_ACORN)

void nonblock(socket_t s)
{
  int val = 1;

  socket_ioctl(s, FIONBIO, &val);
}

#elif defined(CIRCLE_UNIX) || defined(CIRCLE_OS2) || defined(CIRCLE_MACINTOSH)

#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(socket_t s)
{
  int flags;

  flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

#endif  /* CIRCLE_UNIX || CIRCLE_OS2 || CIRCLE_MACINTOSH */


/* ******************************************************************
*  signal-handling functions (formerly signals.c).  UNIX only.      *
****************************************************************** */

#if defined(CIRCLE_UNIX) || defined(CIRCLE_MACINTOSH)

RETSIGTYPE reread_wizlists(int sig)
{
  mudlog("Signal received - rereading wizlists.", CMP, LVL_IMMORT, TRUE);
  reboot_wizlists();
}


RETSIGTYPE unrestrict_game(int sig)
{
  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)",
	 BRF, LVL_IMMORT, TRUE);
  ban_list = NULL;
  circle_restrict = 0;
  num_invalid = 0;
}

#ifdef CIRCLE_UNIX

/* clean up our zombie kids to avoid defunct processes */
RETSIGTYPE reap(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);

  my_signal(SIGCHLD, reap);
}

RETSIGTYPE checkpointing(int sig)
{
  if (!tics) {
    log("SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)");
    abort();
  } else
    tics = 0;
}

RETSIGTYPE hupsig(int sig)
{
  log("SYSERR: Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  Crash_save_all();
  log("Shutdown...");
  exit(1);			/* perhaps something more elegant should
				 * substituted */
}


/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted for uniformity, because BSD systems
 * do not restart select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 *
 * SunOS Release 4.0.2 (sun386) needs this too, according to Tim Aldric.
 */

#ifndef POSIX
#define my_signal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;

  return oact.sa_handler;
}
#endif				/* POSIX */


void signal_setup(void)
{
#ifndef CIRCLE_MACINTOSH
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes.
   */
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGCHLD, reap);
#endif /* CIRCLE_MACINTOSH */
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
}

#endif				/* CIRCLE_UNIX */
#endif	/* CIRCLE_UNIX || CIRCLE_MACINTOSH */

/* ****************************************************************
*       Public routines for system-to-player-communication        *
**************************************************************** */

void send_to_char(const char *messg, struct char_data *ch)
{
  if (ch->desc && messg)
    SEND_TO_Q(messg, ch->desc);
}


void send_to_all(const char *messg)
{
  struct descriptor_data *i;

  if (messg == NULL)
    return;

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING)
	SEND_TO_Q(messg, i);
}


void send_to_outdoor(const char *messg)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || i->character == NULL)
      continue;
    if (!AWAKE(i->character) || !OUTSIDE(i->character))
      continue;
      SEND_TO_Q(messg, i);
  }
}



void send_to_room(const char *messg, int room)
{
  struct char_data *i;

  if (messg == NULL)
    return;

  for (i = world[room].people; i; i = i->next_in_room) {
    if (i == NULL || i->desc == NULL || STATE(i->desc) != CON_PLAYING)
      continue;
    if (!AWAKE(i))
      continue;
    if (i->desc)
      SEND_TO_Q(messg, i->desc);
  }
}

/* send_to_zone sends string to all players in a zone */
void send_to_zone(const char *str, int zone_rnum)
{
  struct descriptor_data *i;
//  int zone = zone_table[zone_rnum].number;
  
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && world[i->character->in_room].zone == zone_rnum 
      && !PLR_FLAGGED(i->character, PLR_WRITING))
      send_to_char(str, i->character);
  }
  return;
}


const char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL) i = ACTNULL; else i = (expression);


/* higher-level communication: the act() function */
void perform_act(const char *orig, struct char_data *ch, struct obj_data *obj,
		const void *vict_obj, struct char_data *to)
{
  const char *i = NULL;
  char lbuf[MAX_STRING_LENGTH], *buf;


//  log("perform_act: %s", orig);
  if (!IS_NPC(to) && PLR_FLAGGED(to, PLR_MAILING | PLR_WRITING)) return;

  buf = lbuf;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
	i = PERS(ch, to);
	break;
      case 'N':
	CHECK_NULL(vict_obj, PERS((struct char_data *) vict_obj, to));
	break;
      case 'm':
	i = HMHR(ch);
	break;
      case 'M':
	CHECK_NULL(vict_obj, HMHR((struct char_data *) vict_obj));
	break;
      case 's':
	i = HSHR(ch);
	break;
      case 'S':
	CHECK_NULL(vict_obj, HSHR((struct char_data *) vict_obj));
	break;
      case 'e':
	i = HSSH(ch);
	break;
      case 'E':
	CHECK_NULL(vict_obj, HSSH((struct char_data *) vict_obj));
	break;
      case 'o':
	CHECK_NULL(obj, OBJN(obj, to));
	break;
      case 'O':
	CHECK_NULL(vict_obj, OBJN((struct obj_data *) vict_obj, to));
	break;
      case 'p':
	CHECK_NULL(obj, OBJS(obj, to));
	break;
      case 'P':
	CHECK_NULL(vict_obj, OBJS((struct obj_data *) vict_obj, to));
	break;
      case 'a':
	CHECK_NULL(obj, SANA(obj));
	break;
      case 'A':
	CHECK_NULL(vict_obj, SANA((struct obj_data *) vict_obj));
	break;
      case 'T':
	CHECK_NULL(vict_obj, (char *) vict_obj);
	break;
      case 'F':
	CHECK_NULL(vict_obj, fname((char *) vict_obj));
	break;
      case '$':
	i = "$";
	break;
      default:
	log("SYSERR: Illegal $-code to act(): %c", *orig);
	log("SYSERR: %s", orig);
	break;
      }
      while ((*buf = *(i++)))
	buf++;
      orig++;
    } else if (!(*(buf++) = *(orig++)))
      break;
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';

  if (to->desc)
    SEND_TO_Q(CAP(lbuf), to->desc);
  if (MOBTrigger)
    mprog_act_trigger(lbuf, to, ch, obj, (char *) vict_obj);
}


#define SENDOK(ch)	(((ch)->desc || (IS_NPC(ch) && (mob_index[ch->nr].progtypes & ACT_PROG))) \
			&& (to_sleeping || AWAKE(ch)) && \
		    	(IS_NPC(ch) || !PLR_FLAGGED((ch), PLR_WRITING)))

void act(const char *str, int hide_invisible, struct char_data *ch,
	 struct obj_data *obj, const void *vict_obj, int type)
{
  struct char_data *to = NULL;
  int to_sleeping;
  struct descriptor_data *i;

  if (!str || !*str) {
    MOBTrigger = TRUE;
    return;
  }
  
  

  /*
   * Warning: the following TO_SLEEP code is a hack.
   * 
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   * TO_SLEEP only works because its value "happens to be" a single bit;
   * do not change it to something else.  In short, it is a hack.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((to_sleeping = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;

  if (type == TO_CHAR) {
//    log("TO_CHAR: %s", str);
    if (ch && SENDOK(ch)) {
      perform_act(str, ch, obj, vict_obj, ch);
    }
    MOBTrigger = TRUE;
    return;
  }

  if (type == TO_VICT) {
    to = (struct char_data *) vict_obj;
//    log("TO_VICT: %s, SENDOK: %d", str, to ? SENDOK(to) : -1);
    if ((to = (struct char_data *) vict_obj) && SENDOK(to)) {
      perform_act(str, ch, obj, vict_obj, to);
    }
    MOBTrigger = TRUE;
    return;
  }
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

   if (type == TO_GMOTE) {
    for (i = descriptor_list; i; i = i->next) {
      if (!i->connected && i->character && !PRF_FLAGGED(i->character, PRF_NOGOSS) &&

!PLR_FLAGGED(i->character, PLR_WRITING) &&

!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

        send_to_char(CCYEL(i->character, C_NRM), i->character);
        perform_act(str, ch, obj, vict_obj, i->character);
        send_to_char(CCNRM(i->character, C_NRM), i->character);
      }
    }
    return;
  }

//  log("TO_ROOM/NOTVICT: %s", str);
  if (ch && ch->in_room != NOWHERE)
    to = world[ch->in_room].people;
  else if (obj && obj->in_room != NOWHERE)
    to = world[obj->in_room].people;
  else {
    if (!read_obj)
      log("SYSERR: no valid target to act()!");
    return;
  }

  for (; to; to = to->next_in_room) {
    if (!SENDOK(to) || (to == ch))
      continue;
    if (hide_invisible && ch && !CAN_SEE(to, ch))
      continue;
    if (type != TO_ROOM && to == vict_obj)
      continue;
      perform_act(str, ch, obj, vict_obj, to);
  }
}

/*
 * This function is called every 30 seconds from heartbeat().  It checks
 * the four global buffers in CircleMUD to ensure that no one has written
 * past their bounds.  If our check digit is not there (and the position
 * doesn't have a NUL which may result from snprintf) then we gripe that
 * someone has overwritten our buffer.  This could cause a false positive
 * if someone uses the buffer as a non-terminated character array but that
 * is not likely. -gg
 */
#define offset	(MAX_STRING_LENGTH - 1)
void sanity_check(void)
{
  int ok = TRUE;

  /*
   * If any line is false, 'ok' will become false also.
   */
  ok &= (buf[offset] == MAGIC_NUMBER || buf[offset] == '\0');
  ok &= (buf1[offset] == MAGIC_NUMBER || buf1[offset] == '\0');
  ok &= (buf2[offset] == MAGIC_NUMBER || buf2[offset] == '\0');
  ok &= (arg[offset] == MAGIC_NUMBER || arg[offset] == '\0');

  /*
   * This isn't exactly the safest thing to do (referencing known bad memory)
   * but we're doomed to crash eventually, might as well try to get something
   * useful before we go down. -gg
   */
  if (!ok)
    log("SYSERR: *** Buffer overflow! ***\n"
	"buf: %s\nbuf1: %s\nbuf2: %s\narg: %s", buf, buf1, buf2, arg);
#if 0
  log("Statistics: buf=%d buf1=%d buf2=%d arg=%d",
	strlen(buf), strlen(buf1), strlen(buf2), strlen(arg));
#endif
    MOBTrigger = TRUE;
}

void make_who2html(void) {
   extern struct descriptor_data *descriptor_list;
   extern char *class_abbrevs[];
   FILE *opf;
   struct descriptor_data *d;
   struct char_data *ch;

   if ((opf = fopen("~/public_html/who.tmp", "w")) == 0)
     return; /* or log it ? *shrug* */

   fprintf(opf, "<HTML><HEAD><TITLE>Who is on the Mud?</TITLE></HEAD>\n");
   fprintf(opf, "<BODY><H1>Who is playing right now?</H1><HR>\n");

   for(d = descriptor_list; d; d = d->next)
     if(!d->connected)
       {
         if(d->original)
            ch = d->original;
         else if (!(ch = d->character))
            continue;
         if(GET_LEVEL(ch) < LVL_IMMORT || (GET_LEVEL(ch)>=LVL_IMMORT &&
                                         !GET_INVIS_LEV(ch)))
           {
             sprintf(buf, "[%2d %s] %s %s\n <BR>", GET_LEVEL(ch), 
CLASS_ABBR(ch),
                     GET_NAME(ch), GET_TITLE(ch));
             fprintf(opf, buf);
           }
        }
     else 
       fprintf(opf,"Apparently, nobody is logged on at the moment...");

   fprintf(opf, "<HR></BODY></HTML>\n");
   fclose(opf);
   system("mv ~/public_html/mud.tmp ~/public_html/who.html");
}



