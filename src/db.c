/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "olc.h"
#include "clan.h"

// #include <sys/time.h>
// #include <sys/types.h>
#include <sys/wait.h>

/* define some of these to invalidate (to make ready to save) */
#undef INVALIDATE_ROOM
#undef INVALIDATE_MOB
#undef INVALIDATE_OBJ

/* define this if you want debug messages on reload and unloading descriptions */
#undef DEBUG_UNLOAD
#define UNLOAD_EXDESC

void mprog_read_programs(FILE * fp, struct index_data * pMobIndex);
char err_buf[MAX_STRING_LENGTH];


/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

struct clan_info *clan_index = NULL; /* (FIDO) Clan global */
int clan_top = 0;			/* (FIDO) Top of clan struct */

struct room_data *world = NULL;	/* array of rooms		 */
int top_of_world = 0;		/* ref to top element of world	 */

struct char_data *character_list = NULL;	/* global linked list of
						 * chars	 */
struct index_data *mob_index;	/* index table for mobile file	 */
struct char_data *mob_proto;	/* prototypes for mobs		 */
int top_of_mobt = 0;		/* top of mobile index table	 */
struct raff_node *raff_list = NULL;

struct obj_data *object_list = NULL;	/* global linked list of objs	 */
struct index_data *obj_index;	/* index table for object file	 */
struct obj_data *obj_proto;	/* prototypes for objs		 */
int top_of_objt = 0;		/* top of object index table	 */

struct zone_data *zone_table;	/* zone table			 */
int top_of_zone_table = 0;	/* top element of zone tab	 */
struct message_list fight_messages[MAX_MESSAGES];	/* fighting messages	 */

struct player_index_element *player_table = NULL;	/* index to plr file	 */
FILE *player_fl = NULL;		/* file desc of player file	 */
int top_of_p_table = 0;		/* ref to top of table		 */
int top_of_p_file = 0;		/* ref of size of p file	 */
long top_idnum = 0;		/* highest idnum in use		 */

int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
sh_int r_mortal_start_room;	/* rnum of mortal start room	 */
sh_int r_immort_start_room;	/* rnum of immort start room	 */
sh_int r_frozen_start_room;	/* rnum of frozen start room	 */
sh_int r_death_start_room;      /* rnum of death  start room     */

char *credits = NULL;		/* game credits			 */
char *news = NULL;		/* mud news			 */
char *motd = NULL;		/* message of the day - mortals */
char *imotd = NULL;		/* message of the day - immorts */
char *help = NULL;		/* help screen			 */
char *info = NULL;		/* info page			 */
char *wizlist = NULL;		/* list of higher gods		 */
char *immlist = NULL;		/* list of peon gods		 */
char *background = NULL;	/* background story		 */
char *handbook = NULL;		/* handbook for new immortals	 */
char *policies = NULL;		/* policies page		 */
char *buildbook = NULL;		/* builders handbook		 */

int  numgreetings = 0;
char *greetings[MAX_GREETINGS];

char *spell_tables[NUM_CLASSES];

struct help_index_element *help_table = 0;	/* the help table	 */
int top_of_helpt = 0;		/* top of help index table	 */

struct dns_entry *dns_cache[DNS_HASH_NUM]; /* for dns caching * dnsmod */

struct time_info_data time_info;/* the infomation about the time    */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	 */
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

int spell_sort_info[MAX_SKILLS+1];
int sunlight;	/* What state the sun is at */

int room_desc_size = 0;
int room_exdesc_size = 0;
int mob_desc_size = 0;
int obj_desc_size = 0;

/* local functions */
void setup_dir(FILE * fl, int room, int dir, int reload);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode, char *filename);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void load_help(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
int is_empty(int zone_nr);
void reset_zone(int zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE * fl);
long asciiflag_conv(char *flag);
void parse_simple_mob(FILE *mob_f, int i, int nr);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE *mob_f, int i, int nr);
void get_one_line(FILE *fl, char *buf);
void save_etext(struct char_data * ch);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(int zone, int cmd_no, const char *message);
void reset_time(void);
void boot_dns(void); /* dnsmod */
void save_dns_cache(void); /* dnsmod */
int get_host_from_cache(struct dns_entry *dnsd); /* dnsmod */
void add_dns_host(struct dns_entry *dnsd, char *hostname); /* dnsmod */

/* external functions */
extern void parse_clan(FILE *fl, int virtual_nr);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void boot_social_messages(void);
void update_obj_file(void);	/* In objsave.c */
void sort_commands(void);
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
int  find_name(char *name);
int  hsort(const void *a, const void *b);
void assign_the_gms(void);
void boot_the_guilds(FILE *gm_f, char *filename, int rec_count);
void load_world_settings(void);
void load_zone_owners(void);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void free_alias(struct alias * a);
void mprog_reset_trigger(struct char_data *mob);
int build_cmd_tree(void);

/* external vars */
extern int no_specials;
extern char *spells[];
extern struct specproc_info mob_procs[];
extern struct specproc_info obj_procs[];
extern struct specproc_info room_procs[];
extern int read_obj;
extern sh_int mortal_start_room;
extern sh_int immort_start_room;
extern sh_int frozen_start_room;
extern sh_int death_start_room;
extern struct descriptor_data *descriptor_list;

#define READ_SIZE 256

void sort_spells(void)
{
  int a, b, tmp;

  /* initialize array */
  for (a = 1; a < MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < MAX_SKILLS - 1; a++)
    for (b = a + 1; b < MAX_SKILLS; b++)
      if (strcmp(spells[spell_sort_info[a]], spells[spell_sort_info[b]]) > 0) {
	tmp = spell_sort_info[a];
	spell_sort_info[a] = spell_sort_info[b];
	spell_sort_info[b] = tmp;
      }
}

void make_spell_lists(void) {
  int i;
  for (i = 0; i < NUM_CLASSES; i++) {
    class_spells_index(i, buf);
    CREATE(spell_tables[i], char, strlen(buf)+1);
    strcpy(spell_tables[i], buf);
  }
}

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/


/* Handling of zipped files (any indexed files may be zipped) */
/* Written by Vladimir >Yilard< Marko			      */
/* You can use any unpacker which is able to dump to stdout   */
#define ZIP_EXTENSION	".bz2"
#define ZIP_UNZIPPER	{ "bzip2", "-d", "-c", fname, NULL }

/* This sleeps for msec miliseconds */
#define MS_SLEEP(msec)	{ struct timeval timeout;		\
			  timeout.tv_sec = 0;			\
			  timeout.tv_usec = msec;		\
			  select(0, NULL, NULL, NULL, &timeout); }

pid_t defunct_pid = -1;

/* This function uses variable `buf' !!! */
FILE *open_db_file(char *fname, char *mode,bool exact_fname)
{
  int pipefd[2], i;
  pid_t res;
  char *unzipper[] = ZIP_UNZIPPER;
  FILE *fl;
  int status;
  
  if (strlen(fname) < strlen(ZIP_EXTENSION) 
    || strcmp(fname + (strlen(fname)-strlen(ZIP_EXTENSION)), ZIP_EXTENSION)) {
    fl = fopen(fname, mode);
    if (exact_fname || fl) return fl;
    strcpy(buf, fname);
    strcat(buf, ZIP_EXTENSION);
    return open_db_file(buf, mode, TRUE);
  }
  // log("Packed db: %s - unpacking...", fname);
  if (pipe(pipefd) < 0) {
    perror("pipe()");
    log("SYSERR: Cannot create pipe to unzipper.");
    exit(1);
  }
  
  if (defunct_pid >= 0) {
    waitpid(defunct_pid, &status, 0);
   /* if (!WIFEXITED(status)) {
      log("SYSERR: Unzipper has not exited sucessfully.");
      exit(1);
    } */
    defunct_pid = -1;
  }
  if ((res = fork()) < 0) {
    perror("fork()");
    log("SYSERR: Cannot fork to unzipper.");
    exit(1);
  }
  if (res == 0) {
    dup2(pipefd[1], 1);
    close(pipefd[0]);
    close(pipefd[1]);
    for (i=3; i<=1000; i++) close(i);
    execvp(unzipper[0], unzipper);
    perror("execvp()");
    log("SYSERR: Unzipper execution failed.");
    exit(1);
  } else {
    defunct_pid = res;
    close(pipefd[1]);
    fl = fdopen(pipefd[0], mode); 
    return fl;
  }
}


/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}

ACMD(do_reboot)
{
  int i;

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BLDBOOK_FILE, &buildbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
  } else if (!str_cmp(arg, "wizlist"))
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
  else if (!str_cmp(arg, "immlist"))
    file_to_string_alloc(IMMLIST_FILE, &immlist);
  else if (!str_cmp(arg, "news"))
    file_to_string_alloc(NEWS_FILE, &news);
  else if (!str_cmp(arg, "credits"))
    file_to_string_alloc(CREDITS_FILE, &credits);
  else if (!str_cmp(arg, "motd"))
    file_to_string_alloc(MOTD_FILE, &motd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string_alloc(IMOTD_FILE, &imotd);
  else if (!str_cmp(arg, "help"))
    file_to_string_alloc(HELP_PAGE_FILE, &help);
  else if (!str_cmp(arg, "info"))
    file_to_string_alloc(INFO_FILE, &info);
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "buildbook"))
    file_to_string_alloc(BLDBOOK_FILE, &buildbook);
  else if (!str_cmp(arg, "background"))
    file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "xhelp")) {
    if (help_table) {
      for (i = 0; i <= top_of_helpt; i++) {
        if (help_table[i].keyword)
	  free(help_table[i].keyword);
        if (help_table[i].entry && !help_table[i].duplicate)
	  free(help_table[i].entry);
      }
      free(help_table);
    }
    top_of_helpt = 0;
    index_boot(DB_BOOT_HLP);
  } else {
    send_to_char("Unknown reload option.\r\n", ch);
    return;
  }

  send_to_char(OK, ch);
}


void boot_world(void)
{
  log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  log("Renumbering rooms.");
  renum_world();

  log("Checking start rooms.");
  check_start_rooms();

  log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

 log("Loading clans and generating index.");
  index_boot(DB_BOOT_CLAN);
  //update_clan_cross_reference();

  log("Renumbering zone table.");
  renum_zone_table();

  if (!no_specials) {
    log("Loadin guild masters.");
    index_boot(DB_BOOT_GLD);
    
    log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }
}

/* body of the booting system */
void boot_db(void)
{
  int i;

  log("Boot db -- BEGIN.");

  log("Resetting the game time:");
  reset_time();

  log("Loading world settings:");
  load_world_settings();
  
#ifdef CMD_TREE_SEARCH
  log("Building command search tree:");
  log("  %d commands. Done.", build_cmd_tree());
#endif

//  log("Initializing spec-proc tables.");
//  count_spec_procs();


  log("Loading introscreens.");
  index_boot(DB_BOOT_GRT);
  
  
  log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  file_to_string_alloc(BLDBOOK_FILE, &buildbook);
  
  boot_world();

  log("Loading zone owners.");
  load_zone_owners();

  log("Loading help entries.");
  index_boot(DB_BOOT_HLP);

//  log("Purging timed-out entries in player file:");
//  purge_player_file();
//  log("Done.");

  log("Generating player index.");
  build_player_index();

  log("Loading fight messages.");
  load_messages();

  log("Loading social messages.");
  boot_social_messages();

  log("Assigning function pointers:");

  if (!no_specials) {
  /*
    log("   Mobiles.");
    assign_mobiles();
  */
    log("   Shopkeepers.");
    assign_the_shopkeepers();
    /*
    log("   Objects.");
    assign_objects();
    log("   Rooms.");
    assign_rooms();
    */
    log("   Guildmasters.");
    assign_the_gms();
  }
  log("   Spells.");
  mag_assign_spells();

  log("Assigning spell and skill levels.");
  init_spell_levels();

  log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  log("Generating lists of spells.");
  make_spell_lists();

  log("Booting mail system.");
  if (!scan_file()) {
    log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  log("Booting dns cache."); /* dnsmod */
  boot_dns();

  if (!no_rent_check) {
    log("Deleting timed-out crash and rent files:");
    update_obj_file();
    log("Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!mini_mud) {
    log("Booting houses.");
    House_boot();
  }

  for (i = 0; i <= top_of_zone_table; i++) {
    log("Resetting %s (rooms %d-%d).", zone_table[i].name,
	(i ? (zone_table[i - 1].top + 1) : 0), zone_table[i].top);
    reset_zone(i);
    zone_table[i].unloaded = FALSE;
    zone_table[i].locked   = FALSE;
  }

  reset_q.head = reset_q.tail = NULL;

  boot_time = time(0);

  MOBTrigger = TRUE;

  log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
#if defined(CIRCLE_MACINTOSH)
  long beginning_of_time = -1561789232;
#else
  long beginning_of_time = 650336715;
#endif

  time_info = *mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    sunlight = SUN_SET;
  else
    sunlight = SUN_DARK;

  log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);
}

void purge_player_file(void)
{
  FILE *fl;
  int size, recs, i, okay, num =0;
  long pointer1, pointer2, timeout;
  struct char_file_u player;
  char local_buf[80], reason[80], *ptr;
  
  if (!(fl = fopen(PLAYER_FILE, "r+b"))) {
    if (errno != ENOENT) {
      perror("SYSERR: fatal error opening playerfile");
      exit(1);
    } else {
      log("No playerfile.");
      return;
    }
  }
  fseek(fl, 0L, SEEK_END);
  size = ftell(fl);
  rewind(fl);
  if (size % sizeof(struct char_file_u))
    fprintf(stderr, "\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!\n");
  recs = size / sizeof(struct char_file_u);
  pointer1 = 0;
  pointer2 = 0;
  okay = 1;
  for (i = 0; i < recs; i++) {
    fseek(fl, pointer1, SEEK_SET);
    fread(&player, sizeof(struct char_file_u), 1, fl);
    *reason = '\0';
    for (ptr = player.name; *ptr; ptr++) {
      if (!isalpha(*ptr) || *ptr == ' ') {
	okay = 0;
	strcpy(reason, "Invalid name");
      }
    if (player.level == 0) {
      okay = 0;
      strcpy(reason, "Never entered game");
    }
    if (player.level < 0 || player.level > LVL_IMPL) {
      okay = 0;
      strcpy(reason, "Invalid level");
    }
    /* now, check for timeouts.  If char is CRYO-rented the timeouts are triple*/

    timeout = 1000;

    if (okay && player.level <= LVL_IMMORT) {
      if (player.level == 1)		timeout = 4;	/* Lev   1 : 4 days */
      else if (player.level <= 4)	timeout = 7;	/* Lev 2-4 : 7 days */
      else if (player.level <= 10)	timeout = 30;	/* Lev 5-10: 30 days */
      else if (player.level <= LVL_IMMORT - 1)
	  timeout = 60;		/* Lev 11-30: 60 days */
      else if (player.level <= LVL_IMMORT)
	  timeout = 90;		/* Lev 31: 90 days */
      };

      if (!(player.char_specials_saved.act[0] & PLR_CRYO)) 
        timeout *= 3;

      timeout *= SECS_PER_REAL_DAY;

      if ((time(0) - player.last_logon) > timeout) {
	okay = 0;
	sprintf(reason, "Level %2d idle for %3ld days", player.level,
		((time(0) - player.last_logon) / SECS_PER_REAL_DAY));
      }
    }
    if (player.char_specials_saved.act[0] & PLR_DELETED) {
      okay = 0;
      sprintf(reason, "Deleted flag set");
    }

    /* Don't delete for *any* of the above reasons if they have NODELETE */
    if (!okay && (player.char_specials_saved.act[0] & PLR_NODELETE)) {
      okay = 2;
      strcat(reason, "; NOT deleted.");
    }
    
    if (okay != 0) {
      if (pointer1 != pointer2) {
        fseek(fl, pointer2, SEEK_SET);
        fwrite(&player, sizeof(struct char_file_u), 1, fl);
        pointer2 += sizeof(struct char_file_u);
      }
      sprintf(local_buf, "* %4d. %-20s %s\n", ++num, player.name, reason);
      log(local_buf);
    }
    pointer1 += sizeof(struct char_file_u);
    
    if (okay == 2) {
      sprintf(local_buf, "* %-20s %s\n", player.name, reason);
      log(local_buf);
    }
  }
  fclose(fl);
}

/* generate index table for the player file */
void build_player_index(void)
{
  int nr = -1, i, clannum;
  long size, recs;
  struct char_file_u dummy;
  int lastpos = 0;

  if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
    if (errno != ENOENT) {
      perror("SYSERR: fatal error opening playerfile");
      exit(1);
    } else {
      log("No playerfile.  Creating a new one.");
      touch(PLAYER_FILE);
      if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
	perror("SYSERR: fatal error opening playerfile");
	exit(1);
      }
    }
  }

  fseek(player_fl, 0L, SEEK_END);
  size = ftell(player_fl);
  rewind(player_fl);
  if (size % sizeof(struct char_file_u))
    log("\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!");
  recs = size / sizeof(struct char_file_u);
  if (recs) {
    log("   %ld players in database.", recs);
    CREATE(player_table, struct player_index_element, recs);
  } else {
    player_table = NULL;
    top_of_p_file = top_of_p_table = -1;
    return;
  }
  lastpos = 0;
  for (; !feof(player_fl);) {
    fread(&dummy, sizeof(struct char_file_u), 1, player_fl);
    if (!feof(player_fl)) {	/* new record */
      nr++;
      CREATE(player_table[nr].name, char, strlen(dummy.name) + 1);
      for (i = 0;
	   (*(player_table[nr].name + i) = LOWER(*(dummy.name + i))); i++);
      player_table[nr].id = dummy.char_specials_saved.idnum;
      top_idnum = MAX(top_idnum, dummy.char_specials_saved.idnum);
      
      
      
      if (dummy.player_specials_saved.clannum != 0) {
        clannum = real_clan(dummy.player_specials_saved.clannum);
        /*
        log("DEBUG: Player %s / Clan %d (%d) (%s)", dummy.name, 
          dummy.player_specials_saved.clannum, clannum,
          CLANNAME(clan_index[clannum])); 
        */
        CLANPLAYERS(clan_index[clannum])++;
        if (dummy.player_specials_saved.clannum < 0) {
          dummy.player_specials_saved.clannum = 0;
          fseek(player_fl, lastpos, SEEK_SET);
          fwrite(&dummy, sizeof(struct char_file_u), 1, player_fl);
          log("SYSERR: Fixed bad clan number (%s)", dummy.name);
        }
      }
      
      lastpos = ftell(player_fl);
    }
  }

  top_of_p_file = top_of_p_table = nr;
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
	goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return total_keywords;

  /* No, they are not evil. -gg 6/24/98 */
ackeof:	
  log("SYSERR: Unexpected end of help file.");
  exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}



void index_boot(int mode)
{
  const char *index_filename, *prefix;
  FILE *index, *db_file;
  int rec_count = 0;

  switch (mode) {
  case DB_BOOT_WLD:
    prefix = WLD_PREFIX;
    break;
  case DB_BOOT_MOB:
    prefix = MOB_PREFIX;
    break;
  case DB_BOOT_OBJ:
    prefix = OBJ_PREFIX;
    break;
  case DB_BOOT_ZON:
    prefix = ZON_PREFIX;
    break;
  case DB_BOOT_SHP:
    prefix = SHP_PREFIX;
    break;
  case DB_BOOT_HLP:
    prefix = HLP_PREFIX;
    break;
  case DB_BOOT_CLAN:
    prefix = CLAN_PREFIX;
    break;
  case DB_BOOT_GRT:
    prefix = GRT_PREFIX;
    break;
  case DB_BOOT_GLD:
     prefix = GLD_PREFIX;
     break;
  default:
    log("SYSERR: Unknown subcommand %d to index_boot!", mode);
    exit(1);
    break;
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  sprintf(buf2, "%s%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    sprintf(buf1, "SYSERR: opening index file '%s'", buf2);
    perror(buf1);
    exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s%s", prefix, buf1);
    if (!(db_file = open_db_file(buf2, "r", TRUE))) {
      perror(buf2);
      log("SYSERR: File '%s' listed in %s/%s not found.", buf2, prefix,
	  index_filename);
      fscanf(index, "%s\n", buf1);
      continue;
    } else {
      if (mode == DB_BOOT_ZON || mode == DB_BOOT_GRT)
	rec_count++;
      else if (mode == DB_BOOT_HLP)
	rec_count += count_alias_records(db_file);
      else
	rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  /* Exit if 0 records, unless this is shops */
  if (!rec_count) {
    if (mode == DB_BOOT_SHP || mode == DB_BOOT_GLD)
      return;
    log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
	index_filename);
    exit(1);
  }

  rec_count++;

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  switch (mode) {
  case DB_BOOT_WLD:
    CREATE(world, struct room_data, rec_count);
    log("   %d rooms, %d bytes.", rec_count, sizeof(struct room_data) * rec_count);
    break;
  case DB_BOOT_MOB:
    CREATE(mob_proto, struct char_data, rec_count);
    CREATE(mob_index, struct index_data, rec_count);
    log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, sizeof(struct index_data) * rec_count, sizeof(struct char_data) * rec_count);
    break;
  case DB_BOOT_OBJ:
    CREATE(obj_proto, struct obj_data, rec_count);
    CREATE(obj_index, struct index_data, rec_count);
    log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, sizeof(struct index_data) * rec_count, sizeof(struct obj_data) * rec_count);
    break;
  case DB_BOOT_ZON:
    CREATE(zone_table, struct zone_data, rec_count);
    log("   %d zones, %d bytes.", rec_count, sizeof(struct zone_data) * rec_count);
    break;
  case DB_BOOT_HLP:
    CREATE(help_table, struct help_index_element, rec_count);
    log("   %d entries, %d bytes.", rec_count, sizeof(struct help_index_element) * rec_count);
    break;
  case DB_BOOT_CLAN:
    CREATE(clan_index, struct clan_info, rec_count);
    clan_top = rec_count - 2;
    break;
  
  }
  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    sprintf(buf2, "%s%s", prefix, buf1);
    if (!(db_file = open_db_file(buf2, "r", TRUE))) {
      perror(buf2);
      exit(1);
    }
    switch (mode) {
    case DB_BOOT_WLD:
    case DB_BOOT_OBJ:
    case DB_BOOT_MOB:
    case DB_BOOT_CLAN:
      discrete_load(db_file, mode, buf2);
      
      break;
    case DB_BOOT_ZON:
      load_zones(db_file, buf2);
      break;
    case DB_BOOT_HLP:
      /*
       * If you think about it, we have a race here.  Although, this is the
       * "point-the-gun-at-your-own-foot" type of race.
       */
      load_help(db_file);
      break;
    case DB_BOOT_SHP:
      boot_the_shops(db_file, buf2, rec_count);
      break;
    case DB_BOOT_GLD:
      boot_the_guilds(db_file, buf2, rec_count);
      break;
    case DB_BOOT_GRT:
      if (numgreetings < MAX_GREETINGS) {
        greetings[numgreetings] = NULL;
        file_to_string_alloc(buf2, &(greetings[numgreetings]));
        numgreetings++;
      } else log("Too many greetings - ignoring.");
      break;
    }
    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }
  fclose(index); /* The index file was not closed - deliberate? */

  /* sort the help index */
  if (mode == DB_BOOT_HLP) {
    qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
    top_of_helpt--;
  }
  /* Write out the size of descs and other malloc'd things */
  switch (mode){
    case DB_BOOT_WLD:
      log("   %d bytes in descs, %d bytes in exdescs.", room_desc_size, room_exdesc_size);
      break;
    case DB_BOOT_MOB:
      log("   %d bytes in descs.", mob_desc_size);
      break;
    case DB_BOOT_OBJ:
      log("   %d bytes in descs.", obj_desc_size);
      break;
  }
}

void discrete_load(FILE * fl, int mode, char *filename)
{
  int nr = -1, last = 0;
  char line[256];

  const char *modes[] = {"world", "mob", "obj", "clan"};

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
	if (nr == -1) {
	  log("SYSERR: %s file %s is empty!", modes[mode], filename);
	} else {
	  log("SYSERR: Format error in %s after %s #%d\n"
	      "...expecting a new %s, but file ended!\n"
	      "(maybe the file is not terminated with '$'?)", filename,
	      modes[mode], nr, modes[mode]);
	}
	exit(1);
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	log("SYSERR: Format error after %s #%d", modes[mode], last);
	exit(1);
      }
      if (nr >= 99999)
	return;
      else
	switch (mode) {
	case DB_BOOT_WLD:
	  parse_room(fl, nr);
	  break;
	case DB_BOOT_MOB:
	  parse_mobile(fl, nr);
	  break;
	case DB_BOOT_OBJ:
	  strcpy(line, parse_object(fl, nr));
	  break;
	case DB_BOOT_CLAN:
	  parse_clan(fl, nr);
	  break;
	}
    } else {
      log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
	  filename, modes[mode], nr);
      log("...offending line: '%s'", line);
      exit(1);
    }
  }
}


long asciiflag_conv(char *flag)
{
  long flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return flags;
}

char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);
  } while (isspace(c));
  return c;
}

/* loads room extra descriptions, dir descs, etc. */
/* if reload == 1, changes only ex_descs, dir_descs */
void parse_room_extra(FILE * fl, int virtual_nr, int room_nr, int reload)
{
  char line[256], flags[128], sp_buf[64];
  struct extra_descr_data *new_descr;
  struct teleport_data *new_tele;
  long raff_buf;
  int t[10];
  
  #ifndef UNLOAD_EXDESC
  if (reload != 0)
    return;
  #endif
  
  for (;;) {
    if (!get_line(fl, line)) {
      log("%s", buf);
      exit(1);
    }
    switch (*line) {
    case 'D':
      setup_dir(fl, room_nr, atoi(line + 1), reload);
      break;
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(fl, buf2);
      new_descr->description = fread_string(fl, buf2);
      new_descr->next = world[room_nr].ex_description;
      world[room_nr].ex_description = new_descr;
      if (reload == 0) {
        if (new_descr->description != NULL)
          room_exdesc_size += strlen(new_descr->description + 1);
        if (new_descr->keyword != NULL)
          room_exdesc_size += strlen(new_descr->keyword + 1);
      }
      break;
    case 'S':			/* end of room */
      return;
    case 'T':
      
      if (!get_line(fl, line) || sscanf(line, " %d %s %d %d", t, flags,
                                        t+2, t+3) != 4) {
        fprintf(stderr, "Format error in room #%d's section T\n", virtual_nr);
        exit(1);
      }
      if (reload != 0) break;
      CREATE(new_tele, struct teleport_data, 1);
      world[room_nr].tele = new_tele;
      world[room_nr].tele->targ = t[0];
      world[room_nr].tele->mask = asciiflag_conv(flags);
      world[room_nr].tele->time = t[2];
      world[room_nr].tele->obj = t[3];
      break;
    case 'P':  /* Do room spec procs */
      if (!get_line(fl, line) || sscanf(line, " %s", sp_buf) != 1) {
        fprintf(stderr, "Format error in room #%d's section P\n", virtual_nr);
        exit(1);
      }
      if (reload != 0) break;
      t[0] = get_spec_proc(room_procs, sp_buf);
      if (t[0] == 0) {
        sprintf(buf,"SYSWARR: Attempt to assign non-existing room spec-proc (Room: %d)", real_room(room_nr));
        log(buf);
      } else
        world[room_nr].func = room_procs[t[0]].sp_pointer;
      break;
      
    case 'A':
      
      if (!get_line(fl, line) || sscanf(line, " %ld", &raff_buf) != 1) {
        fprintf(stderr, "Format error in room #%d's section A\n", virtual_nr);
        exit(1);
      }
      if (reload != 0) break;
      world[room_nr].room_affections = raff_buf;
      world[room_nr].orig_affections = raff_buf;
      break;
      
    default:
      log(buf);
      log("...found `%s'", line); 
      exit(1);
    }
  }
}

/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256], flags[128], f1[128], f2[128], f3[128];
  
  
  
  world[room_nr].tele = NULL;
  sprintf(buf2, "room #%d", virtual_nr);

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
    exit(1);
  }
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      log("SYSERR: Room %d is outside of any zone.", virtual_nr);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);
  
  if (world[room_nr].description)
    room_desc_size += strlen(world[room_nr].description) + 1;

  if (!get_line(fl, line)) {
    log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
	virtual_nr);
    exit(1);
  }

  if (sscanf(line, " %d %s %s %s %s %d ", t, flags,
    f1, f2, f3, t + 2) != 6) {
    log("SYSERR: Format error in roomflags/sector typr of room #%d\n", virtual_nr);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
  world[room_nr].room_flags[0] = asciiflag_conv(flags);
  
  world[room_nr].room_flags[1] = asciiflag_conv(f1);
  world[room_nr].room_flags[2] = asciiflag_conv(f2);
  world[room_nr].room_flags[3] = asciiflag_conv(f3);
 
  world[room_nr].sector_type = t[2];

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0;	/* Zero light sources */
  
  world[room_nr].room_affections = 0;
  world[room_nr].orig_affections = 0;

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;
  
#ifdef INVALIDATE_ROOM
  olc_add_to_save_list(zone_table[real_zone(virtual_nr)].number, OLC_SAVE_ROOM);
#endif

  sprintf(buf, "SYSERR: Format error in room #%d (expecting D/E/S/P/A)", virtual_nr);

  parse_room_extra(fl, virtual_nr, room_nr, 0);
  top_of_world = room_nr++;
}



/* read direction data */
/* if reload == 1 then do not create anything */
void setup_dir(FILE * fl, int room, int dir, int reload)
{
  int t[5];
  char line[256];

  sprintf(buf2, "room #%d, direction D%d", GET_ROOM_VNUM(room), dir);

  if (reload == 0) {
    CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
    world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
    world[room].dir_option[dir]->keyword = fread_string(fl, buf2);
  
    if (world[room].dir_option[dir]->general_description)
      room_exdesc_size += strlen(world[room].dir_option[dir]->general_description) + 1;
    if (world[room].dir_option[dir]->keyword)
      room_exdesc_size += strlen(world[room].dir_option[dir]->keyword) + 1;
  
    if (!get_line(fl, line)) {
      log("SYSERR: Format error, %s", buf2);
      exit(1);
    }
    if (sscanf(line, " %d %d %d ", t, t + 1, t + 2) != 3) {
      log("SYSERR: Format error, %s", buf2);
      exit(1);
    }
    if (t[0] == 1)
      world[room].dir_option[dir]->exit_info = EX_ISDOOR;
    else if (t[0] == 2)
      world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
    else
      world[room].dir_option[dir]->exit_info = 0;

    world[room].dir_option[dir]->key = t[1];
    world[room].dir_option[dir]->to_room = t[2];
  } else {
    fread_string(fl, buf2);
    fread_string(fl, buf2);
    get_line(fl, line);
  }
}


/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
    if ((r_death_start_room = real_room(death_start_room)) < 0) {
    log("SYSERR:  death start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    if (!mini_mud)
      log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  register int room, door;

  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door])
	if (world[room].dir_option[door]->to_room != NOWHERE)
	  world[room].dir_option[door]->to_room =
	    real_room(world[room].dir_option[door]->to_room);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int zone, cmd_no, a, b, c, olda, oldb, oldc;
  char buf[128];

  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = c = 0;
      olda = ZCMD.arg1;
      oldb = ZCMD.arg2;
      oldc = ZCMD.arg3;
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	c = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
	b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      }
      if (a < 0 || b < 0 || c < 0) {
	if (!mini_mud) {
	  sprintf(buf,  "Invalid vnum %d, cmd disabled",
			 (a < 0) ? olda : ((b < 0) ? oldb : oldc));
	  log_zone_error(zone, cmd_no, buf);
	}
	ZCMD.command = '*';
      }
    }
}



void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int j, t[10];
  char line[256];

    mob_proto[i].real_abils.str = 11;
    mob_proto[i].real_abils.intel = 11;
    mob_proto[i].real_abils.wis = 11;
    mob_proto[i].real_abils.dex = 11;
    mob_proto[i].real_abils.con = 11;
    mob_proto[i].real_abils.cha = 11;

  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
	  t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
      log("SYSERR: Format error in mob #%d, first line after S flag\n"
	"...expecting line of form '# # # #d#+# #d#+#'", nr);
      exit(1);
    }

    GET_LEVEL(mob_proto + i) = t[0];
    mob_proto[i].points.hitroll = 20 - t[1];
    mob_proto[i].points.armor = 10 * t[2];

    /* max hit = 0 is a flag that H, M, V is xdy+z */
    mob_proto[i].points.max_hit = 0;
    mob_proto[i].points.hit = t[3];
    mob_proto[i].points.mana = t[4];
    mob_proto[i].points.move = t[5];

    mob_proto[i].points.max_mana = 10;
    mob_proto[i].points.max_move = 50;

    mob_proto[i].mob_specials.damnodice = t[6];
    mob_proto[i].mob_specials.damsizedice = t[7];
    mob_proto[i].points.damroll = t[8];

  if (!get_line(mob_f, line)) {
      log("SYSERR: Format error in mob #%d, second line after S flag\n"
	  "...expecting line of form '# #', but file ended!", nr);
      exit(1);
    }

  if (sscanf(line, " %d %d ", t, t + 1) != 2) {
    log("SYSERR: Format error in mob #%d, second line after S flag\n"
	"...expecting line of form '# #'", nr);
      exit(1);
    }

    GET_GOLD(mob_proto + i) = t[0];
    GET_EXP(mob_proto + i) = t[1];

  if (!get_line(mob_f, line)) {
      log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #', but file ended!", nr);
    exit(1);
  }

  if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 3) {
    log("SYSERR: Format error in last line of mob #%d\n"
	"...expecting line of form '# # #'", nr);
      exit(1);
    }

    mob_proto[i].char_specials.position = t[0];
    mob_proto[i].mob_specials.default_pos = t[1];
    mob_proto[i].player.sex = t[2];

    mob_proto[i].player.chclass = 0;
    mob_proto[i].player.weight = 200;
    mob_proto[i].player.height = 198;

    /*
     * These are player specials! -gg
     */
#if 0
    for (j = 0; j < 3; j++)
      GET_COND(mob_proto + i, j) = -1;
#endif

    /*
     * these are now save applies; base save numbers for MOBs are now from
     * the warrior save table.
     */
    for (j = 0; j < 5; j++)
      GET_SAVE(mob_proto + i, j) = 0;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, int i, int nr)
{
  int num_arg, matched = 0;

  num_arg = atoi(value);

  CASE("BareHandAttack") {
    RANGE(0, 99);
    mob_proto[i].mob_specials.attack_type = num_arg;
  }

  CASE("Str") {
    RANGE(3, 25);
    mob_proto[i].real_abils.str = num_arg;
  }

  CASE("StrAdd") {
    RANGE(0, 100);
    mob_proto[i].real_abils.str_add = num_arg;    
  }

  CASE("Int") {
    RANGE(3, 25);
    mob_proto[i].real_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(3, 25);
    mob_proto[i].real_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(3, 25);
    mob_proto[i].real_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(3, 25);
    mob_proto[i].real_abils.con = num_arg;
  }

  CASE("Cha") {
    RANGE(3, 25);
    mob_proto[i].real_abils.cha = num_arg;
  }
  
  CASE("MaxMove") {
    RANGE(1, 1000);
    mob_proto[i].points.max_move = num_arg;
  }
  
  CASE("SpecProc") {
    num_arg = get_spec_proc(mob_procs, value);
    if (num_arg == 0) {
      log("ESPEC ERROR: Attempt to assign non-existing spec-proc.");
    } else {
      mob_index[i].func = mob_procs[num_arg].sp_pointer;
    }
  }

  if (!matched) {
    log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
	    keyword, nr);
  }    
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
#if 0	/* Need to evaluate interpret_espec()'s NULL handling. */
  }
#else
  } else
    ptr = "";
#endif
  interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[256];

  parse_simple_mob(mob_f, i, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E"))	/* end of the ehanced section */
      return;
    else if (*line == '#') {	/* we've hit the next mob, maybe? */
      log("SYSERR: Unterminated E section in mob #%d", nr);
      exit(1);
    } else
      parse_espec(line, i, nr);
  }

  log("SYESRR: Unexpected end of file reached after mob #%d", nr);
  exit(1);
}


void parse_mobile(FILE * mob_f, int nr)
{
  static int i = 0;
  int j, t[10];
  char line[256], *tmpptr, letter;
  char f1[128], f2[128], f3[128], f4[128], f5[128], f6[128], f7[128], f8[128];

  mob_index[i].vnum = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  /*
   * Mobiles should NEVER use anything in the 'player_specials' structure.
   * The only reason we have every mob in the game share this copy of the
   * structure is to save newbie coders from themselves. -gg 2/25/98
   */
  mob_proto[i].player_specials = &dummy_mob;
  sprintf(buf2, "mob vnum %d", nr);

  /***** String data *****/
  mob_proto[i].player.name = fread_string(mob_f, buf2);
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.description = fread_string(mob_f, buf2);
  mob_proto[i].player.title = NULL;
  mob_proto[i].player.prompt = NULL;
  
  if (mob_proto[i].player.description)
    mob_desc_size += strlen(mob_proto[i].player.description) + 1;

  /* *** Numeric data *** */
  if (!get_line(mob_f, line)) {
    log("SYSERR: Format error after string section of mob #%d\n"
	"...expecting line of form '# # # {S | E}', but file ended!", nr);
    exit(1);
  }
  
  #ifdef CIRCLE_ACORN	/* Ugh. */
  if ((sscanf(line, "%s %s %s %s %s %s %s %s %d %s", 
    f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter) != 10)) {
  #else
  if ((sscanf(line, "%s %s %s %s %s %s %s %s %d %c", 
    f1, f2, f3, f4, f5, f6, f7, f8, t + 2, &letter) != 10)) {
  #endif
    log("SYSERR: Format error after string section of mob #%d\n"
		    "...expecting line of form '# # # {S | E}'\n", nr);
    exit(1);
  }
  MOB_FLAGS(mob_proto + i) = asciiflag_conv(f1);
  MOB2_FLAGS(mob_proto + i) = asciiflag_conv(f2);
  MOB3_FLAGS(mob_proto + i) = asciiflag_conv(f3);
  MOB4_FLAGS(mob_proto + i) = asciiflag_conv(f4);
  SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  AFF_FLAGS(mob_proto + i) = asciiflag_conv(f5);
  AFF2_FLAGS(mob_proto + i) = asciiflag_conv(f6);
  AFF3_FLAGS(mob_proto + i) = asciiflag_conv(f7);
  AFF4_FLAGS(mob_proto + i) = asciiflag_conv(f8);
  GET_ALIGNMENT(mob_proto + i) = t[2];

  switch (UPPER(letter)) {
  case 'S':	/* Simple monsters */
    parse_simple_mob(mob_f, i, nr);
    break;
  case 'E':	/* Circle3 Enhanced monsters */
    parse_enhanced_mob(mob_f, i, nr);
    break;
  /* add new mob types here.. */
  default:
    log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
    exit(1);
  }

  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  letter = fread_letter(mob_f);
  if (letter == '>') {
    ungetc(letter, mob_f);
    (void)  mprog_read_programs(mob_f, &mob_index[i]);
  } else ungetc(letter, mob_f);
  // mob_account(mob_proto + i);
  top_of_mobt = i++;
  
#ifdef INVALIDATE_MOB
  olc_add_to_save_list(zone_table[real_zone(nr)].number, OLC_SAVE_MOB);
#endif
}




/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
  static int i = 0;
  static char line[256], sp_buf[128];
  int t[10], j, retval;
  char *tmpptr;
  char f1[256], f2[256], f3[256], f4[256], f5[256];
  struct extra_descr_data *new_descr;

  obj_index[i].vnum = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;
  

  clear_object(obj_proto + i);
  obj_proto[i].in_room = NOWHERE;
  obj_proto[i].item_number = i;
  obj_proto[i].obj_flags.bottom_level = 0;
  obj_proto[i].obj_flags.top_level = LVL_IMPL;

  sprintf(buf2, "object #%d", nr);

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    log("SYSERR: Null obj name or format error at or near %s", buf2);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
	!str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    *tmpptr = UPPER(*tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);
  
  if (obj_proto[i].description != NULL)
    obj_desc_size += strlen(obj_proto[i].description) + 1;

  /* *** numeric data *** */
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, " %d %s %s %s %s %s", t, f1, f2, f3, f4, f5)) != 6) {
    log("SYSERR: Format error in first numeric line (expecting 6 args, got %d), %s\n", retval, buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.type_flag = t[0];
  obj_proto[i].obj_flags.extra_flags[0] = asciiflag_conv(f1);
 
  obj_proto[i].obj_flags.extra_flags[1] = asciiflag_conv(f2);
  obj_proto[i].obj_flags.extra_flags[2] = asciiflag_conv(f3);
  obj_proto[i].obj_flags.extra_flags[3] = asciiflag_conv(f4);
  
  obj_proto[i].obj_flags.wear_flags = asciiflag_conv(f5);
/* HERE I AM GONNA FIX NEW CLASSES */
/*
  if (obj_proto[i].obj_flags.extra_flags & ITEM_ANTI_CLERIC) 
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_DRUID;
  if (obj_proto[i].obj_flags.extra_flags & ITEM_ANTI_MAGIC_USER) {
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_ALCHEMIST;
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_NECROMANCER;
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_WARLOCK;
  }
  if (obj_proto[i].obj_flags.extra_flags & ITEM_ANTI_WARRIOR) {
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_PALADIN;
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_CYBORG;
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_BARBARIAN;
  }
  if (obj_proto[i].obj_flags.extra_flags & ITEM_ANTI_THIEF) {
    obj_proto[i].obj_flags.extra_flags |= ITEM_ANTI_RANGER;
  } */
  
#ifdef INVALIDATE_OBJ
  olc_add_to_save_list(zone_table[real_zone(nr)].number, OLC_SAVE_OBJ);
#endif

/* END FIX */
  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  
  if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4) {
    log("SYSERR: Format error in second numeric line (expecting 4 args, got %d), %s\n", retval, buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.value[0] = t[0];
  obj_proto[i].obj_flags.value[1] = t[1];
  obj_proto[i].obj_flags.value[2] = t[2];
  obj_proto[i].obj_flags.value[3] = t[3];

  if (!get_line(obj_f, line)) {
    log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
    exit(1);
  }
  if ((retval = sscanf(line, "%d %d %d", t, t + 1, t + 2)) != 3) {
    log("SYSERR: Format error in third numeric line (expecting 3 args, got %d), %s", retval, buf2);
    exit(1);
  }
  obj_proto[i].obj_flags.weight = t[0];
  obj_proto[i].obj_flags.cost = t[1];
  obj_proto[i].obj_flags.cost_per_day = t[2];

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (obj_proto[i].obj_flags.type_flag == ITEM_DRINKCON ||
      obj_proto[i].obj_flags.type_flag == ITEM_FOUNTAIN) {
    if (obj_proto[i].obj_flags.weight < obj_proto[i].obj_flags.value[1])
      obj_proto[i].obj_flags.weight = obj_proto[i].obj_flags.value[1] + 5;
  }

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  strcat(buf2, ", after numeric constants\n"
	 "...expecting 'E', 'A', '$', or next object number");
  j = 0;

  for (;;) {
    if (!get_line(obj_f, line)) {
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
    switch (*line) {
    case 'E':
      CREATE(new_descr, struct extra_descr_data, 1);
      new_descr->keyword = fread_string(obj_f, buf2);
      new_descr->description = fread_string(obj_f, buf2);
      new_descr->next = obj_proto[i].ex_description;
      obj_proto[i].ex_description = new_descr;
      break;
    case 'A':
      if (j >= MAX_OBJ_AFFECT) {
	log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFFECT, buf2);
	exit(1);
      }
      if (!get_line(obj_f, line)) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric constants but file ended!", buf2);
	exit(1);
      }

      if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2) {
	log("SYSERR: Format error in 'A' field, %s\n"
	    "...expecting 2 numeric arguments, got %d\n"
	    "...offending line: '%s'", buf2, retval, line);
	exit(1);
      }
      obj_proto[i].affected[j].location = t[0];
      obj_proto[i].affected[j].modifier = t[1];
      j++;
      break;
    case 'C':
      if (!get_line(obj_f, line)) {
        log("SYSERR: Format error in 'C' field - unexpected end of file");
        exit(0);
      }
      if (sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
        log("SYSERR: Format error in 'C' field, in %s", buf2);
        exit(0);
      }
      COPY_LONG_BITV(obj_proto[i].obj_flags.bitvector, obj_proto[i].obj_flags.bitvector);
/*      for (j = 0; j < LONG_BITV_WIDTH; j++)
        obj_proto[i].obj_flags.bitvector[j] = obj_proto[i].obj_flags.bitvector[j] | t[j];       */
      break;  
    case 'S':
      get_line(obj_f, line);
      sscanf(line, "%s ", sp_buf);
      t[0] = get_spec_proc(obj_procs, sp_buf);
      obj_index[i].func = obj_procs[t[0]].sp_pointer;
      if (t[0] == 0)
        log("SYSWARR: Attempt to assign nonexisting obj spec-proc");
      break;
    case 'L':
      get_line(obj_f, line);
      if (sscanf(line, "%d %d ", t, t + 1) != 2) {
        log("SYSERR: Error reading level restrictions (L) in: %s", buf2);
        exit(1);
      }
      obj_proto[i].obj_flags.bottom_level = t[0];
      obj_proto[i].obj_flags.top_level = t[1];
      break;
    case '$':
    case '#':
      top_of_objt = i++;
      return line;
    default:
      log("SYSERR: Format error in %s", buf2);
      exit(1);
    }
  }
}


#define Z	zone_table[zone]
#define MAX_CMD	500

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static int zone = 0;
  int cmd_no = 0, num_of_cmds = 0, line_num = 0, tmp, error;
  char *ptr, buf[256], zname[256];

  strcpy(zname, zonename);

  CREATE(Z.cmd, struct reset_com, MAX_CMD);
  num_of_cmds = 0;

  line_num += get_line(fl, buf);
  if (sscanf(buf, "#%d", &Z.number) != 1) {
    log("SYSERR: Format error in %s, line %d", zname, line_num);
    exit(1);
  }
  sprintf(buf2, "beginning of zone #%d", Z.number);

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = str_dup(buf);
  
  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
    *ptr = '\0';
  Z.creator = str_dup(buf);

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %d %d ", &Z.master_mob, &Z.zon_flags) != 2) {
    log("Format error in 2-constant line of %s", zname);
    exit(1);
  }

  line_num += get_line(fl, buf);
  if (sscanf(buf, " %d %d %d ", &Z.top, &Z.lifespan, &Z.reset_mode) != 3) {
    log("Format error in 3-constant line of %s", zname);
    exit(1);
  }

  Z.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    Z.pressure += dice(1, 50);
  else
    Z.pressure += dice(1, 80);
  Z.change = 0;
  if (Z.pressure <= 980)
    Z.sky = SKY_LIGHTNING;
  else if (Z.pressure <= 1000)
    Z.sky = SKY_RAINING;
  else if (Z.pressure <= 1020)
    Z.sky = SKY_CLOUDY;
  else
    Z.sky = SKY_CLOUDLESS;

  cmd_no = 0;

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0) {
      log("Format error in %s - premature end of file", zname);
      exit(1);
    }
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;
    num_of_cmds++;
    if (num_of_cmds >= MAX_CMD) {
      log("MAX_CMD in db.c exceeded, please adjust to some higher value.");
      exit(1);
    }

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPD", ZCMD.command) == NULL) {	/* a 3-arg command */
      if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
	error = 1;
    } else {
      if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2,
		 &ZCMD.arg3) != 4)
	error = 1;
    }

    ZCMD.if_flag = tmp;

    if (error) {
      log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
      exit(1);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }
  if (num_of_cmds == 0) {
    log("SYSERR: %s is empty!", zname);
    exit(1);
  }
  RECREATE(Z.cmd, struct reset_com, num_of_cmds);
  Z.builders = NULL;
  line_num += get_line(fl, buf);
  if (!strchr(buf, '$')) {
     if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
       *ptr = '\0';
      if (*buf != '\0')
        Z.builders = str_dup(buf);
  }
    
  
  top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL) {
    log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}

int get_level_from_string(char *str)
{
  int i;
  if (atoi(str) != 0) return atoi(str);
  for (i = 0; i<strlen(str); i++) str[i] = toupper(str[i]);
  if (strcmp(str,"LVL_IMMORT") == 0) 	return 100;
  if (strcmp(str,"LVL_GURU")   == 0) 	return 101;
  if (strcmp(str,"LVL_HALF_GOD") == 0) 	return 102;
  if (strcmp(str,"LVL_MINOR_GOD") == 0) return 103;
  if (strcmp(str,"LVL_GOD") == 0) 	return 104;
  if (strcmp(str,"LVL_GRGOD") == 0) 	return 105;
  if (strcmp(str,"LVL_BUILDER") == 0) 	return 106;
  if (strcmp(str,"LVL_CHBUILD") == 0) 	return 107;
  if (strcmp(str,"LVL_CODER") == 0) 	return 108;
  if (strcmp(str,"LVL_COIMPL") == 0) 	return 109;
  if (strcmp(str,"LVL_IMPL") == 0) 	return 110;
  return 0;
}

void load_help(FILE *fl)
{
#if defined(CIRCLE_MACINTOSH)
  static char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384]; /* ? */
#else
  char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384];
#endif
  char line[READ_SIZE+1], *scan;
  struct help_index_element el;
  char *b;

  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$') {
    /* read in the corresponding help entry */
    strcpy(entry, strcat(key, "\r\n"));
    get_one_line(fl, line);
    while (*line != '#') {
      strcat(entry, strcat(line, "\r\n"));
      get_one_line(fl, line);
    }
    b = &line[2];
    el.min_level = get_level_from_string(b);
    
    /* now, add the entry to the index with each keyword on the keyword line */
    el.duplicate = 0;
    el.entry = str_dup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keyword = str_dup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }

    /* get next keyword line (or $) */
    get_one_line(fl, key);
  }
}


int hsort(const void *a, const void *b)
{
  struct help_index_element *a1, *b1;

  a1 = (struct help_index_element *) a;
  b1 = (struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/



int vnum_mobile(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_mobt; nr++) {
    if (isname(searchname, mob_proto[nr].player.name)) {
      sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
	      mob_index[nr].vnum,
	      mob_proto[nr].player.short_descr);
      send_to_char(buf, ch);
    }
  }

  return (found);
}



int vnum_object(char *searchname, struct char_data * ch)
{
  int nr, found = 0;

  for (nr = 0; nr <= top_of_objt; nr++) {
    if (isname(searchname, obj_proto[nr].name)) {
      sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
	      obj_index[nr].vnum,
	      obj_proto[nr].short_description);
      send_to_char(buf, ch);
    }
  }
  return (found);
}


/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;

  return ch;
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(int nr, int type)
{
  int i;
  struct char_data *mob;

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) < 0) {
      sprintf(buf, "Mobile (V) %d does not exist in database.", nr);
      log(buf);
      return NULL;
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
      mob->points.move;
  } else
    mob->points.max_hit = number(mob->points.hit, mob->points.mana);

  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);
  
  GET_MOB_WAIT(mob) = 0;

  mob_index[i].number++;

  return mob;
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  return obj;
}


/* create a new object from a prototype */
struct obj_data *read_object(int nr, int type)
{
  struct obj_data *obj;
  int i;

  if (nr < 0) {
    log("SYSERR: Trying to create obj with negative (%d) num!", nr);
    return NULL;
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      log("Object (V) %d does not exist in database.", nr);
      return NULL;
    }
  } else
    i = nr;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  obj_index[i].number++;

  return obj;
}

void unload_room_descs(int zone)
{
  int counter, realcounter;
  struct extra_descr_data *t1, *t2;

  if (zone_table[zone].unloaded || zone_table[zone].locked) return;
  
  #ifdef DEBUG_UNLOAD
  sprintf(buf, "Unloading room descs for zone %d.", 
	(int)(zone_table[zone].number));
  log(buf);
  #endif

  zone_table[zone].unloaded = TRUE;
  zone_table[zone].locked = FALSE;

  for (counter = zone_table[zone].number * 100;
       counter <= zone_table[zone].top;
       counter++) {
    realcounter = real_room(counter);
    if (realcounter >= 0 && (world[realcounter].description != NULL)) {
      free(world[realcounter].description);
      world[realcounter].description = NULL;
      #ifdef UNLOAD_EXDESC
      if (world[realcounter].ex_description != NULL)
        for (t1 = world[realcounter].ex_description; t2; t1 = t2) {
          t2 = t1->next;
          free(t1->keyword);
          free(t1->description);
          free(t1);
          
        }
      #ifdef DEBUG_UNLOAD
      log("Unloaded extra description of zone #%d", counter);
      #endif 
      world[realcounter].ex_description = NULL;
      #endif
    }
  }
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;
  char buf[128];

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode) {
	(zone_table[i].age)++;
	if (zone_table[i].lifespan > 5 && 
	  (zone_table[i].lifespan - zone_table[i].age) == 2)
	  send_to_zone("[ <-TICK-> ] Tick counter countdown - Tick-Tock.\r\n", i);
      }

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

	CREATE(update_u, struct reset_q_element, 1);

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }	/* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next) {
    if (is_empty(update_u->zone_to_reset)) {
      unload_room_descs(update_u->zone_to_reset);
    }
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
      is_empty(update_u->zone_to_reset)) {
      send_to_zone("[ <-TICK-> ] Tick counter countdown - Tick-Tock.\r\n", 
update_u->zone_to_reset);
      reset_zone(update_u->zone_to_reset);
      sprintf(buf, "Auto zone reset: %s",
	      zone_table[update_u->zone_to_reset].name);
      mudlog(buf, CMP, LVL_GOD, FALSE);
    } else
      break;

      /* dequeue */
    if (update_u == reset_q.head)
      reset_q.head = reset_q.head->next;
    else {
      for (temp = reset_q.head; temp->next != update_u; temp = temp->next);
	if (!update_u->next)
	  reset_q.tail = temp;
	temp->next = update_u->next;
    }
    free(update_u);
    break;
  }
}

void log_zone_error(int zone, int cmd_no, const char *message)
{
  char buf[256];

  sprintf(buf, "SYSERR: zone file: %s", message);
  mudlog(buf, NRM, LVL_GOD, TRUE);

  sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	  ZCMD.command, zone_table[zone].number, ZCMD.line);
  mudlog(buf, NRM, LVL_GOD, TRUE);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(int zone)
{
  int cmd_no, last_cmd = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

    if (ZCMD.if_flag && !last_cmd)
      continue;

    switch (ZCMD.command) {
    case '*':			/* ignore command */
      last_cmd = 0;
      break;

    case 'M':			/* read a mobile */
      if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
	mob = read_mobile(ZCMD.arg1, REAL);
	char_to_room(mob, ZCMD.arg3);
	mprog_reset_trigger(mob);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'O':			/* read an object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	if (ZCMD.arg3 >= 0) {
	  obj = read_object(ZCMD.arg1, REAL);
	  obj_to_room(obj, ZCMD.arg3);
	  last_cmd = 1;
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  obj->in_room = NOWHERE;
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      break;

    case 'P':			/* object to object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
	  ZONE_ERROR("target obj not found");
	  break;
	}
	obj_to_obj(obj, obj_to);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'G':			/* obj_to_char */
      if (!mob) {
	ZONE_ERROR("attempt to give obj to non-existant mob");
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	obj_to_char(obj, mob);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'E':			/* object to equipment list */
      if (!mob) {
	ZONE_ERROR("trying to equip non-existant mob");
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  equip_char(mob, obj, ZCMD.arg3);
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL) {
        obj_from_room(obj);
        extract_obj(obj);
      }
      last_cmd = 1;
      break;


    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
	ZONE_ERROR("door does not exist");
      } else
	switch (ZCMD.arg3) {
	case 0:
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_CLOSED);
	  break;
	case 1:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		     EX_LOCKED);
	  break;
	case 2:
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_LOCKED);
	  SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
		  EX_CLOSED);
	  break;
	}
      last_cmd = 1;
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
    if (STATE(i) == CON_PLAYING)
      if (world[i->character->in_room].zone == zone_nr)
	return 0;

  return 1;
}





/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/


long get_id_by_name(char *name)
{
  int i;

  one_argument(name, arg);
  for (i = 0; i <= top_of_p_table; i++)
    if (!strcmp((player_table + i)->name, arg))
      return ((player_table + i)->id);

  return -1;
}


char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if ((player_table + i)->id == id)
      return ((player_table + i)->name);

  return NULL;
}


/* Load a char, TRUE if loaded, FALSE if not */
int load_char(char *name, struct char_file_u * char_element)
{
  int player_i;

  if ((player_i = find_name(name)) >= 0) {
    fseek(player_fl, (long) (player_i * sizeof(struct char_file_u)), SEEK_SET);
    fread(char_element, sizeof(struct char_file_u), 1, player_fl);
    return (player_i);
  } else
    return (-1);
}

void save_char_raw(struct char_file_u * chdata, long pos_i)
{
  if (pos_i >= 0) {
    fseek(player_fl, pos_i * sizeof(struct char_file_u), SEEK_SET);
    fwrite(chdata, sizeof(struct char_file_u), 1, player_fl);
  }
}


/*
 * write the vital data of a player to the player file
 *
 * NOTE: load_room should be an *VNUM* now.  It is converted to a vnum here.
 */
void save_char(struct char_data * ch, sh_int load_room)
{
  struct char_file_u st;
  

  if (IS_NPC(ch) || !ch->desc || GET_PFILEPOS(ch) < 0)
    return;
  
  GET_LOADROOM(ch) = load_room;
  
  char_to_store(ch, &st);

  strncpy(st.host, ch->desc->host, HOST_LENGTH);
  st.host[HOST_LENGTH] = '\0';
    
  fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
  fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
}



/* copy data from the file structure to a char struct */
void store_to_char(struct char_file_u * st, struct char_data * ch)
{
  int i;

  /* to save memory, only PC's -- not MOB's -- have player_specials */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  GET_SEX(ch) = st->sex;
  GET_CLASS(ch) = st->chclass;
  GET_LEVEL(ch) = st->level;

  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.title = str_dup(st->title);
  ch->player.prompt = str_dup(st->prompt);
  ch->player.description = str_dup(st->description);

  ch->player.hometown = st->hometown;
  ch->player.time.birth = st->birth;
  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  ch->player.weight = st->weight;
  ch->player.height = st->height;

  ch->real_abils = st->abilities;
  ch->aff_abils = st->abilities;
  ch->points = st->points;
  ch->char_specials.saved = st->char_specials_saved;
  ch->player_specials->saved = st->player_specials_saved;
  
  if (st->poof_in[0])
    POOFIN(ch) = strdup(st->poof_in);
  else POOFIN(ch) = NULL;
  
  if (st->poof_out[0])
    POOFOUT(ch) = strdup(st->poof_out);
  else POOFOUT(ch) = NULL;

  GET_LAST_TELL(ch) = NOBODY;

  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;

  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->points.armor = 100;
  ch->points.hitroll = 0;
  ch->points.damroll = 0;

  if (ch->player.name == NULL)
    CREATE(ch->player.name, char, strlen(st->name) + 1);
  strcpy(ch->player.name, st->name);
  strcpy(ch->player.passwd, st->pwd);

  /* Add all spell effects */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, &st->affected[i]);
  }

  /*
   * If you're not poisioned and you've been away for more than an hour of
   * real time, we'll set your HMV back to full
   */

  if (!AFF_FLAGGED(ch, AFF_POISON) &&
      (((long) (time(0) - st->last_logon)) >= SECS_PER_REAL_HOUR)) {
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
  }
}				/* store_to_char */




/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data * ch, struct char_file_u * st)
{
  int i, j, hit, mana, move;
  struct affected_type *af;
  struct obj_data *char_eq[NUM_WEARS];


  hit = GET_HIT(ch);
  mana = GET_MANA(ch);
  move = GET_MOVE(ch);
  /* Unaffect everything a character can be affected by */

  read_obj = TRUE;
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = NULL;
  }

  for (af = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (af) {
      st->affected[i] = *af;
      st->affected[i].next = 0;
      af = af->next;
    } else {
      st->affected[i].type = 0;	/* Zero signifies not used */
      st->affected[i].duration = 0;
      st->affected[i].modifier = 0;
      st->affected[i].location = 0;
      for (j=0; j<LONG_BITV_WIDTH; j++)
        st->affected[i].bitvector[j] = 0;
      st->affected[i].next = 0;
    }
  }


  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  if ((i >= MAX_AFFECT) && af && af->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  st->birth = ch->player.time.birth;
  st->played = ch->player.time.played;
  st->played += (long) (time(0) - ch->player.time.logon);
  st->last_logon = time(0);

  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  st->hometown = ch->player.hometown;
  st->weight = GET_WEIGHT(ch);
  st->height = GET_HEIGHT(ch);
  st->sex = GET_SEX(ch);
  st->chclass = GET_CLASS(ch);
  st->level = GET_LEVEL(ch);
  st->abilities = ch->real_abils;
  st->points = ch->points;
  st->char_specials_saved = ch->char_specials.saved;
  st->player_specials_saved = ch->player_specials->saved;

  st->points.armor = 100;
  st->points.hitroll = 0;
  st->points.damroll = 0;

  if (GET_TITLE(ch))
    strcpy(st->title, GET_TITLE(ch));
  else
    *st->title = '\0';
    
  if (GET_PROMPT(ch))
    strcpy(st->prompt, GET_PROMPT(ch));
  else
    *st->prompt = '\0';

  if (ch->player.description)
    strcpy(st->description, ch->player.description);
  else
    *st->description = '\0';

  strcpy(st->name, GET_NAME(ch));
  strcpy(st->pwd, GET_PASSWD(ch));
  
  if (POOFIN(ch)) {
    strncpy(st->poof_in, POOFIN(ch), MAX_POOF_LENGTH);
    st->poof_in[MAX_POOF_LENGTH] = '\0';
  } else st->poof_in[0] = '\0';
  
  if (POOFOUT(ch)) {
    strncpy(st->poof_out, POOFOUT(ch), MAX_POOF_LENGTH);
    st->poof_out[MAX_POOF_LENGTH] = '\0';
  } else st->poof_out[0] = '\0';

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, &st->affected[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);
  }
  read_obj = FALSE;
  GET_HIT(ch) = hit;
  GET_MANA(ch) = mana;
  GET_MOVE(ch) = move;
/*   affect_total(ch); unnecessary, I think !?! */
}				/* Char to store */



void save_etext(struct char_data * ch)
{
/* this will be really cool soon */

}


/* create a new entry in the in-memory index table for the player file */
int create_entry(char *name)
{
  int i;

  if (top_of_p_table == -1) {
    CREATE(player_table, struct player_index_element, 1);
    top_of_p_table = 0;
  } else if (!(player_table = (struct player_index_element *)
	       realloc(player_table, sizeof(struct player_index_element) *
		       (++top_of_p_table + 1)))) {
    perror("SYSERR: create entry");
    exit(1);
  }
  CREATE(player_table[top_of_p_table].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (*(player_table[top_of_p_table].name + i) = LOWER(*(name + i)));
       i++);

  return (top_of_p_table);
}



/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/

inline char *find_tilde(char *string)
{
  char *point = string;
  do {
    point = strchr(point, '~');
    if (point != NULL) {
      if (point[1] != '~') 
        return point;
      else
        point = point + 2;
    }
  } while (point != NULL);
  return NULL;
}

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      log("SYSERR: fread_string: format error at or near %s", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = find_tilde(tmp)) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      log("SYSERR: fread_string: string too large (db.c)");
      log(error);
      exit(1);
    } else {
      strcat(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (strlen(buf) > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
  } else
    rslt = NULL;

  return rslt;
}


/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
{
  int i;
  struct alias *a;
  MPROG_ACT_LIST *tmp;

  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    while ((a = GET_ALIASES(ch)) != NULL) {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }
    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    free(ch->player_specials);
    if (IS_NPC(ch))
      log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (GET_NAME(ch))
      free(GET_NAME(ch));
    if (ch->player.title)
      free(ch->player.title);
    if (ch->player.prompt)
      free(ch->player.prompt);
    if (ch->player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description)
      free(ch->player.description);
  } else if ((i = GET_MOB_RNUM(ch)) > -1) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name)
      free(ch->player.name);
    if (ch->player.title && ch->player.title != mob_proto[i].player.title)
      free(ch->player.title);
    if (ch->player.prompt && ch->player.prompt != mob_proto[i].player.prompt)
      free(ch->player.prompt);
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
      free(ch->player.short_descr);
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
      free(ch->player.long_descr);
    if (ch->player.description && ch->player.description != mob_proto[i].player.description)
      free(ch->player.description);
  }
  if (IS_NPC(ch) && ch->mpact) 
    for ( ; tmp ; ch->mpact = tmp) {
      tmp = ch->mpact->next;
      free(ch->mpact);
    }
  
  while (ch->affected)
    affect_remove(ch, ch->affected);

  if (ch->desc)
    ch->desc->character = NULL;

  free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  int nr;
  struct extra_descr_data *thisd, *next_one;

  if ((nr = GET_OBJ_RNUM(obj)) == -1) {
    if (obj->name)
      free(obj->name);
    if (obj->description)
      free(obj->description);
    if (obj->short_description)
      free(obj->short_description);
    if (obj->action_description)
      free(obj->action_description);
    if (obj->ex_description)
      for (thisd = obj->ex_description; thisd; thisd = next_one) {
	next_one = thisd->next;
	if (thisd->keyword)
	  free(thisd->keyword);
	if (thisd->description)
	  free(thisd->description);
	free(thisd);
      }
  } else {
    if (obj->name && obj->name != obj_proto[nr].name)
      free(obj->name);
    if (obj->description && obj->description != obj_proto[nr].description)
      free(obj->description);
    if (obj->short_description && obj->short_description != obj_proto[nr].short_description)
      free(obj->short_description);
    if (obj->action_description && obj->action_description != obj_proto[nr].action_description)
      free(obj->action_description);
    if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
      for (thisd = obj->ex_description; thisd; thisd = next_one) {
	next_one = thisd->next;
	if (thisd->keyword)
	  free(thisd->keyword);
	if (thisd->description)
	  free(thisd->description);
	free(thisd);
      }
  }

  free(obj);
}



/* read contets of a text file, alloc space, point buf to it */
int file_to_string_alloc(const char *name, char **buf)
{
  char temp[MAX_STRING_LENGTH];

  if (*buf)
    free(*buf);

  if (file_to_string(name, temp) < 0) {
    *buf = NULL;
    return -1;
  } else {
    *buf = str_dup(temp);
    return 0;
  }
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE+3];

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    sprintf(tmp, "SYSERR: reading %s", name);
    perror(tmp);
    return (-1);
  }
  do {
    fgets(tmp, READ_SIZE, fl);
    tmp[strlen(tmp) - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");

    if (!feof(fl)) {
      if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
        log("SYSERR: %s: string too big (%d max)", name,
		MAX_STRING_LENGTH);
	*buf = '\0';
	return -1;
      }
      strcat(buf, tmp);
    }
  } while (!feof(fl));

  fclose(fl);

  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  ch->in_room = NOWHERE;
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;

  GET_LEAVE_DIR(ch) = -1;

  GET_LAST_TELL(ch) = NOBODY;
  //check_regen_rates(ch);
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  ch->in_room = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;

  GET_AC(ch) = 100;		/* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
}


void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
  obj->worn_on = NOWHERE;
  obj->obj_flags.bottom_level = 0;
  obj->obj_flags.top_level = LVL_IMPL;
}




/* initialize a new character only if class is set */
void init_char(struct char_data * ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */

  if (top_of_p_table == 0) {
    GET_EXP(ch) = 7000000;
    GET_LEVEL(ch) = LVL_IMPL;
    GET_TOT_LEVEL(ch) = LVL_IMPL * NUM_CLASSES;

    ch->points.max_hit = 500;
    ch->points.max_mana = 500;
    ch->points.max_move = 500;
    
    ch->real_abils.intel = 25;
    ch->real_abils.wis = 25;
    ch->real_abils.dex = 25;
    ch->real_abils.str = 25;
    ch->real_abils.str_add = 100;
    ch->real_abils.con = 25;
    ch->real_abils.cha = 25;
  }
  set_title(ch, NULL);

  ch->player.prompt = NULL;
  ch->player.short_descr = NULL;
  ch->player.long_descr = NULL;
  ch->player.description = NULL;

  ch->player.hometown = 1;

  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  for (i = 0; i < MAX_TONGUE; i++)
    GET_TALK(ch, i) = 0;

  /* make favors for sex */
  if (ch->player.sex == SEX_MALE) {
    ch->player.weight = number(120, 180);
    ch->player.height = number(160, 200);
  } else {
    ch->player.weight = number(100, 160);
    ch->player.height = number(150, 180);
  }

  ch->points.gold = 5000;

  ch->points.max_mana = 100;
  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.hit = GET_MAX_HIT(ch);
  ch->points.max_move = 82;
  ch->points.move = GET_MAX_MOVE(ch);
  ch->points.armor = 100;

  player_table[top_of_p_table].id = GET_IDNUM(ch) = ++top_idnum;

  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
  }
  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);

  GET_LOADROOM(ch) = NOWHERE;
  GET_OLC_ZONE(ch) = -1;
//  dump_char(ch);
}



/* returns the real number of the room with given virtual number */
int real_room(int vnum)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((world + mid)->number == vnum)
      return mid;
    if (bot >= top)
      return NOWHERE;
    if ((world + mid)->number > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

void parse_desc(FILE * fl, int virt_nr)
{
  char line[256];
  char *room_name;
  int real_nr = real_room(virt_nr);

  room_name = fread_string(fl, buf2);
  if (room_name && world[real_nr].name && 
    str_cmp(room_name, world[real_nr].name)) {
    log("SYSERR: Read name does not equal room name.");
    exit(1);
  }
  free(room_name); 
   /* fread_string allocates mem..so free it */
  
  if (world[real_nr].description == NULL)
    world[real_nr].description = fread_string(fl, buf2);
  else {
    sprintf(buf, "SYSERR: Attempting to reload Non-NULL description for room %d.",
	world[real_nr].number);
    log(buf);
    /* exit(1); */
  }
  
  #ifdef UNLOAD_EXDESC
  if (!get_line(fl, line)) {
    sprintf(buf, "SYSERR: Format error when reloading room descriptions for %d.wld\n",
	world[real_nr].zone);
    log(buf);
    exit(1);
  }
  sprintf(buf, "Parse error reloading extra description (room #%d)", world[real_nr].number);
  parse_room_extra(fl, world[real_nr].number, real_nr, 1);
  #else
  
  for (;;) {
    if (!get_line(fl, line)) {
      sprintf(buf, "SYSERR: Format error when reloading room descriptions for %d.wld\n",
		world[real_nr].zone);
      log(buf);
      exit(1);
    }
    if (*line == 'S') /* continue to the end of the record */
      if (*(line + 1) == '\0') /* make sure it IS the end of the record */
        return;
  }
  #endif
}

void reload_room_descs(sh_int real_nr, int lock)
{
  FILE *db_file;
  int nr = -1, last = 0;
  int zone = (int) zone_table[GET_ROOM_ZONE(real_nr)].number;
  char line[256];

  
  if (zone_table[world[real_nr].zone].locked) return; 
  if (lock)
    zone_table[world[real_nr].zone].locked = TRUE;
    
  if (!zone_table[world[real_nr].zone].unloaded) return;

  #ifdef DEBUG_UNLOAD
  sprintf(buf, "Reloading room descs for zone %d.", zone);
  log(buf);
  #endif

  sprintf(buf1, "%d.wld", zone);

  zone_table[world[real_nr].zone].unloaded = FALSE;
  
//  zone_table[world[real_nr].zone].age = 0;

  sprintf(buf2, "%s%s", WLD_PREFIX, buf1);
  if (!(db_file = open_db_file(buf2, "r", FALSE))) {
      perror(buf2);
      return;
      exit(1);
  }

  for (;;) {
    if (!get_line(db_file, line)) {
	fprintf(stderr, "Format error after world #%d\n", nr);
	exit(1);
    }
    if (*line == '$')
      break;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
	fprintf(stderr, "Format error after world #%d\n", last);
	exit(1);
      }
      if (nr >= 99999)
	break;
      else
	parse_desc(db_file, nr);
    } else {
      fprintf(stderr, "Format error in world file near world #%d\n", nr);
      fprintf(stderr, "Offending line: '%s'\n", line);
      exit(1);
    }
  }

  fclose(db_file);
}

/* real number of the zone to unlock room descriptions */
void unlock_room_descs(sh_int real_nr)
{
  zone_table[real_nr].locked = FALSE;
}


/* MOBProg functions */

/* This routine transfers between alpha and numeric forms of the
 * mob_prog bitvector types.  This allows the use of the words in the
 * mob/script files.
 */

int mprog_name_to_type (char *name)
{
    if (!str_cmp(name, "in_file_prog"  ))    return IN_FILE_PROG;
    if (!str_cmp(name, "act_prog"      ))    return ACT_PROG;
    if (!str_cmp(name, "speech_prog"   ))    return SPEECH_PROG;
    if (!str_cmp(name, "rand_prog"     ))    return RAND_PROG;
    if (!str_cmp(name, "fight_prog"    ))    return FIGHT_PROG;
    if (!str_cmp(name, "hitprcnt_prog" ))    return HITPRCNT_PROG;
    if (!str_cmp(name, "death_prog"    ))    return DEATH_PROG;
    if (!str_cmp(name, "entry_prog"    ))    return ENTRY_PROG;
    if (!str_cmp(name, "greet_prog"    ))    return GREET_PROG;
    if (!str_cmp(name, "all_greet_prog"))    return ALL_GREET_PROG;
    if (!str_cmp(name, "give_prog"     ))    return GIVE_PROG;
    if (!str_cmp(name, "bribe_prog"    ))    return BRIBE_PROG;
    if (!str_cmp(name, "leave_prog"    ))    return LEAVE_PROG;
    if (!str_cmp(name, "reset_prog"    ))    return RESET_PROG;
    if (!str_cmp(name, "time_prog"     ))    return TIME_PROG;

    return(ERROR_PROG);
}

  /*
   * Read a number from a file.
   */
  int fread_number(FILE *fp)
  {
      int number;
      bool sign;
      char c;

      do {
          c = getc(fp);
      } while (isspace(c));

      number = 0;

      sign   = FALSE;
      if (c == '+') {
          c = getc(fp);
      } else if (c == '-') {
          sign = TRUE;
          c = getc(fp);
      }


      if (!isdigit(c)) {
          log("Fread_number: bad format.");
          exit(1);
      }

      while (isdigit(c)) {
          number = number * 10 + c - '0';
          c      = getc(fp);
      }

      if (sign)
          number = 0 - number;

      if (c == '|')
          number += fread_number(fp);
      else if (c != ' ')
          ungetc(c, fp);

      return number;
  }

  /*
   * Read to end of line (for comments).
   */
  void fread_to_eol(FILE *fp)
  {
      char c;

      do {
          c = getc(fp);
      } while (c != '\n' && c != '\r');

      do {
          c = getc(fp);
      } while (c == '\n' || c == '\r');

      ungetc(c, fp);
      return;
  }


  /*
   * Read one word (into static buffer).
   */
  char *fread_word(FILE *fp)
  {
      static char word[MAX_INPUT_LENGTH];
      char *pword;
      char cEnd;

      do
      {
          cEnd = getc(fp);
      }
      while (isspace(cEnd));

      if (cEnd == '\'' || cEnd == '"')
      {
          pword   = word;
      }
      else
      {
          word[0] = cEnd;
          pword   = word+1;
          cEnd    = ' ';
      }

      for (; pword < word + MAX_INPUT_LENGTH; pword++)
      {
          *pword = getc(fp);
          if (cEnd == ' ' ? isspace(*pword) || *pword == '~' : *pword == cEnd)
          {
              if (cEnd == ' ' || cEnd == '~')
                  ungetc(*pword, fp);
              *pword = '\0';
              return word;
          }
      }

      log("SYSERR: Fread_word: word too long.");
      exit(1);
      return NULL;
  }


  /* This routine reads in scripts of MOBprograms from a file */

  MPROG_DATA* mprog_file_read(char *f, MPROG_DATA *mprg,
                              struct index_data *pMobIndex)
  {

    char        MOBProgfile[ MAX_INPUT_LENGTH ];
    MPROG_DATA *mprg2;
    FILE       *progfile;
    char        letter;
    bool        done = FALSE;

    sprintf(MOBProgfile, "%s/%s", MOB_DIR, f);
    mprg->filename = strdup(MOBProgfile);
    progfile = fopen(MOBProgfile, "r");
    if (!progfile)
    {
       sprintf(err_buf, "Mob: %d couldnt open mobprog file", pMobIndex->vnum);
       log(err_buf);
       exit(1);
    }

    mprg2 = mprg;
    switch (letter = fread_letter(progfile))
    {
      case '>':
       break;
      case '|':
         log("empty mobprog file.");
         exit(1);
       break;
      default:
         log("in mobprog file syntax error.");
         exit(1);
       break;
    }

    while (!done)
    {
      mprg2->type = mprog_name_to_type(fread_word(progfile));
      switch (mprg2->type)
      {
       case ERROR_PROG:
          log("mobprog file type error");
          exit(1);
        break;
       case IN_FILE_PROG:
          log("mprog file contains a call to file.");
          exit(1);
        break;
       default:
          sprintf(buf2, "Error in file %s", f);
          pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
          {
          char *t1,*t2;
          t1 = fread_string(progfile,buf2);
          t2 = t1;
          skip_spaces(&t1);
          mprg2->arglist       = strdup(t1);
          free(t2);
          }
          mprg2->comlist       = fread_string(progfile,buf2);
          switch (letter = fread_letter(progfile))
          {
            case '>':
               mprg2->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
               mprg2       = mprg2->next;
               mprg2->next = NULL;
             break;
            case '|':
               done = TRUE;
             break;
            default:
               sprintf(err_buf,"in mobprog file %s syntax error.", f);
               log(err_buf);
               exit(1);
             break;
          }
        break;
      }
    }
    fclose(progfile);
    return mprg2;
  }


  struct index_data *get_obj_index (int vnum)
  {
    int nr;
    for(nr = 0; nr <= top_of_objt; nr++) {
      if(obj_index[nr].vnum == vnum) return &obj_index[nr];
    }
    return NULL;
  }

  struct index_data *get_mob_index (int vnum)
  {
    int nr;
    for(nr = 0; nr <= top_of_mobt; nr++) {
      if(mob_index[nr].vnum == vnum) return &mob_index[nr];
    }
    return NULL;
  }


  /* This procedure is responsible for reading any in_file MOBprograms.
   */

  void mprog_read_programs(FILE *fp, struct index_data *pMobIndex)
  {
    MPROG_DATA *mprg;
    char        letter;
    bool        done = FALSE;

    if ((letter = fread_letter(fp)) != '>')
    {
        sprintf(err_buf,"Load_mobiles: vnum %d MOBPROG char", pMobIndex->vnum);
        log(err_buf);
        exit(1);
    }
    pMobIndex->mobprogs = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
    mprg = pMobIndex->mobprogs;

    while (!done)
    {
      mprg->type = mprog_name_to_type(fread_word(fp));
      mprg->filename = NULL;
      switch (mprg->type)
      {
       case ERROR_PROG:
          sprintf(err_buf, "Load_mobiles: vnum %d MOBPROG type.", pMobIndex->vnum);
          log(err_buf);
          exit(1);
        break;
       case IN_FILE_PROG:
          sprintf(buf2, "Mobprog for mob #%d", pMobIndex->vnum);
          mprg = mprog_file_read(fread_word(fp), mprg,pMobIndex);
          fread_to_eol(fp);   /* need to strip off that silly ~*/
          switch (letter = fread_letter(fp))
          {
            case '>':
               mprg->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
               mprg       = mprg->next;
               mprg->next = NULL;
             break;
            case '|':
               mprg->next = NULL;
               fread_to_eol(fp);
               done = TRUE;
             break;
            default:
               sprintf(err_buf, "Load_mobiles: vnum %d bad MOBPROG.", pMobIndex->vnum);
               log(err_buf);
               exit(1);
             break;
          }
        break;
       default:
          sprintf(buf2, "Mobprog for mob #%d", pMobIndex->vnum);
          pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
          {
          char *t1,*t2;
          t1 = fread_string(fp, buf2);
          t2 = t1;
          skip_spaces(&t1);
          mprg->arglist        = strdup(t1);
          free(t2);
          }
          mprg->comlist        = fread_string(fp, buf2);
          switch (letter = fread_letter(fp))
          {
            case '>':
               mprg->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
               mprg       = mprg->next;
               mprg->next = NULL;
             break;
            case '|':
               mprg->next = NULL;
               fread_to_eol(fp);
               done = TRUE;
             break;
            default:
               sprintf(err_buf, "Load_mobiles: vnum %d bad MOBPROG (%c).", 
	       pMobIndex->vnum, letter);
               log(err_buf);
               exit(1);
             break;
          }
        break;
      }
    }

    return;
}


/* returns the real number of the monster with given virtual number */
int real_mobile(int vnum)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

int real_clan(int vnum)
{
  int bot, top, mid;

  bot = 0;
  top = clan_top;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if (CLANNUM(clan_index[mid]) == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if (CLANNUM(clan_index[mid]) > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}


/* dnsmod */
void boot_dns(void)
{
  int i = 0;
  char line[256], name[256];
  FILE *fl;
  struct dns_entry *dns;

  memset((char *) dns_cache, 0, sizeof(struct dns_entry *) * DNS_HASH_NUM);

  if(!(fl = fopen(DNS_FILE, "r"))) {
    log("No DNS cache!");
    return;
  }

  do {
    i = 0;
    get_line(fl, line);
    if(*line != '~') {
      CREATE(dns, struct dns_entry, 1);
      dns->name = NULL;
      dns->next = NULL;
      sscanf(line, "%d.%d.%d.%d %s", dns->ip, dns->ip + 1,
	dns->ip + 2, dns->ip + 3, name);
      dns->name = str_dup(name);
      i = (dns->ip[0] + dns->ip[1] + dns->ip[2]) % DNS_HASH_NUM;
      dns->next = dns_cache[i];
      dns_cache[i] = dns;
    }
  } while (!feof(fl) && *line != '~');
  fclose(fl);
}


/* dnsmod */
void save_dns_cache(void)
{
  int i;
  FILE *fl;
  struct dns_entry *dns;

  if(!(fl = fopen(DNS_FILE, "w"))) {
    log("SYSERR: Can't open dns cache file for write!");
    return;
  }

  for(i = 0; i < DNS_HASH_NUM; i++) {
    if(dns_cache[i]) {
      for(dns = dns_cache[i]; dns; dns = dns->next)
	fprintf(fl, "%d.%d.%d.%d %s\n", dns->ip[0], dns->ip[1],
	  dns->ip[2], dns->ip[3], dns->name);
    }
  }
  fprintf(fl, "~\n");
  fclose(fl);
}


/* dnsmod */
int get_host_from_cache(struct dns_entry *dnsd)
{
  int i;
  struct dns_entry *d;
  char buf[256];

  i = (dnsd->ip[0] + dnsd->ip[1] + dnsd->ip[2]) % DNS_HASH_NUM;
  if(dns_cache[i]) {
    for(d = dns_cache[i]; d; d = d->next) {
      if(dnsd->ip[0] == d->ip[0] && dnsd->ip[1] == d->ip[1] &&
	dnsd->ip[2] == d->ip[2]) {
	if(d->ip[3] == -1) {
	  sprintf(buf, "%d.%s", dnsd->ip[3], d->name);
	  dnsd->name = str_dup(buf);
	  return TRUE;
	} else if(dnsd->ip[3] == d->ip[3]) {
	  dnsd->name = str_dup(d->name);
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}


/* dnsmod */
void add_dns_host(struct dns_entry *dnsd, char *hostname)
{
  int i;
  struct dns_entry *d;

  i = (dnsd->ip[0] + dnsd->ip[1] + dnsd->ip[2]) % DNS_HASH_NUM;
  CREATE(d, struct dns_entry, 1);
  d->ip[0] = dnsd->ip[0];
  d->ip[1] = dnsd->ip[1];
  d->ip[2] = dnsd->ip[2];
  d->ip[3] = dnsd->ip[3];
  d->name = str_dup(hostname);
  d->next = dns_cache[i];
  dns_cache[i] = d;
  save_dns_cache();
}




/* returns the real number of the object with given virtual number */
int real_object(int vnum)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->vnum == vnum)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((obj_index + mid)->vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

void vwear_obj(int type, struct char_data * ch)
{
  int nr, found =0;
  for (nr = 0; nr <= top_of_objt; nr++) {
    if (GET_OBJ_TYPE(&obj_proto[nr]) == type) {
    sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
     obj_index[nr].vnum, obj_proto[nr].short_description);
     send_to_char(buf, ch);
    }
  }
}

