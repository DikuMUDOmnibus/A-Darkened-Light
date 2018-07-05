/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __DB_H__
#define __DB_H__

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD	0
#define DB_BOOT_MOB	1
#define DB_BOOT_OBJ	2
#define DB_BOOT_ZON	3
#define DB_BOOT_SHP	4
#define DB_BOOT_HLP	5
#define DB_BOOT_CLAN	6
#define DB_BOOT_GRT	7
#define DB_BOOT_GLD	8

#define MAX_GREETINGS	10

#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD	":world:"
#define LIB_TEXT	":text:"
#define LIB_TEXT_HELP	":text:help:"
#define LIB_MISC	":misc:"
#define LIB_ETC		":etc:"
#define LIB_PLRTEXT	":plrtext:"
#define LIB_PLROBJS	":plrobjs:"
#define LIB_HOUSE	":house:"
#define SLASH		":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN)
#define LIB_WORLD	"world/"
#define LIB_TEXT	"text/"
#define LIB_TEXT_HELP	"text/help/"
#define LIB_MISC	"misc/"
#define LIB_ETC		"etc/"
#define LIB_PLRTEXT	"plrtext/"
#define LIB_OBJS	"objs/"
#define LIB_PLROBJS	"plrobjs/"
#define LIB_HOUSE	"house/"
#define SLASH		"/"
#endif

#define SUF_OBJS	"objs"
#define SUF_TEXT	"text"
#define SUF_SAVE	"save"
  
#if defined(CIRCLE_AMIGA)
#define FASTBOOT_FILE   "/.fastboot"    /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "/.killscript"  /* autorun: shut mud down       */
#define PAUSE_FILE      "/pause"        /* autorun: don't restart mud   */
#elif defined(CIRCLE_MACINTOSH)
#define FASTBOOT_FILE	"::.fastboot"	/* autorun: boot without sleep	*/
#define KILLSCRIPT_FILE	"::.killscript"	/* autorun: shut mud down	*/
#define PAUSE_FILE	"::pause"	/* autorun: don't restart mud	*/
#else
#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */
#define SIGN_PID_FILE	"../.signpid"	/* pid of the sign program	*/
#endif
/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files		*/
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode	*/
#define WLD_PREFIX	LIB_WORLD"wld"SLASH	/* room definitions	*/
#define MOB_PREFIX	LIB_WORLD"mob"SLASH	/* monster prototypes	*/
#define OBJ_PREFIX	LIB_WORLD"obj"SLASH	/* object prototypes	*/
#define ZON_PREFIX	LIB_WORLD"zon"SLASH	/* zon defs & command tables */
#define SHP_PREFIX	LIB_WORLD"shp"SLASH	/* shop definitions	*/
#define HLP_PREFIX	LIB_TEXT"help"SLASH	/* for HELP <keyword>	*/
#define GLD_PREFIX	LIB_WORLD"gld"SLASH	/* guild definitions		*/
#define MOB_DIR		LIB_WORLD"prg"SLASH	/* Mob programs			*/
#define CLAN_PREFIX	LIB_TEXT"clan"SLASH	/* For clan info		*/
#define GRT_PREFIX      LIB_TEXT"grt"SLASH	/* Greetings (title screens)    */

#define CREDITS_FILE	LIB_TEXT"credits"/* for the 'credits' command	*/
#define NEWS_FILE	LIB_TEXT"news"	/* for the 'news' command	*/
#define MOTD_FILE	LIB_TEXT"motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE	LIB_TEXT"imotd"	/* messages of the day / immort	*/
#define HELP_PAGE_FILE	LIB_TEXT_HELP"screen" /* for HELP <CR>		*/
#define INFO_FILE	LIB_TEXT"info"		/* for INFO		*/
#define WIZLIST_FILE	LIB_TEXT"wizlist"	/* for WIZLIST		*/
#define IMMLIST_FILE	LIB_TEXT"immlist"	/* for IMMLIST		*/
#define BACKGROUND_FILE	LIB_TEXT"background"/* for the background story	*/
#define POLICIES_FILE	LIB_TEXT"policies" /* player policies/rules	*/
#define HANDBOOK_FILE	LIB_TEXT"handbook" /* handbook for new immorts	*/
#define BLDBOOK_FILE	LIB_TEXT"bldbook"  /* handbook for builders	*/

#define IDEA_FILE	LIB_MISC"ideas"	/* for the 'idea'-command	*/
#define TYPO_FILE	LIB_MISC"typos"	/*         'typo'		*/
#define BUG_FILE	LIB_MISC"bugs"	/*         'bug'		*/
#define MESS_FILE	LIB_MISC"messages" /* damage messages		*/
#define SOCMESS_FILE	LIB_MISC"socials" /* messgs for social acts	*/
#define XNAME_FILE	LIB_MISC"xnames" /* invalid name substrings	*/

#define PLAYER_FILE	LIB_ETC"players" 	/* the player database		*/
#define MAIL_FILE	LIB_ETC"plrmail" 	/* for the mudmail system	*/
#define BAN_FILE	LIB_ETC"badsites" 	/* for the siteban system	*/
#define HCONTROL_FILE	LIB_ETC"hcontrol"  	/* for the house system	*/
#define DNS_FILE	LIB_ETC"dns"		/* for the dns cache   * dnsmod */
#define WORLD_SET_FILE	LIB_ETC"world_set"	/* World settings 		*/
#define HEGEMONY_FILE	LIB_ETC"hegemony"	/* Zone owners			*/
#define MEMBERS_FILE	LIB_ETC"members"	/* For generating clan members lists */

/* externs */
extern const char *teleport_bits[];
extern int teleport_on;

/* public procedures in db.c */
void	boot_db(void);
int	create_entry(char *name);
void	zone_update(void);
int	real_room(int vnum);
char	*fread_string(FILE *fl, char *error);
long	get_id_by_name(char *name);
char	*get_name_by_id(long id);

void	char_to_store(struct char_data *ch, struct char_file_u *st);
void	store_to_char(struct char_file_u *st, struct char_data *ch);
int	load_char(char *name, struct char_file_u *char_element);
void	save_char(struct char_data *ch, sh_int load_room);
void 	save_char_raw(struct char_file_u * chdata, long pos_i);
void	init_char(struct char_data *ch);
struct char_data* create_char(void);
struct char_data *read_mobile(int nr, int type);
int	real_mobile(int vnum);
int	vnum_mobile(char *searchname, struct char_data *ch);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);
void	reboot_wizlists(void);
int 	get_level_from_string(char *str);

struct obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
int	real_object(int vnum);
struct obj_data *read_object(int nr, int type);
int	vnum_object(char *searchname, struct char_data *ch);
int 	real_clan(int vnum);
void 	reload_room_descs(sh_int real_nr, int lock);
void 	unload_room_descs(int zone);
void	unlock_room_descs(sh_int real_nr);

extern int sunlight;	/* What state the sun is at */

#define REAL 0
#define VIRTUAL 1

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   int line;		/* line number this command appears on  */

   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
   */
};



/* zone definition structure. for the 'zone-table'   */
struct zone_data {
   char	*name;		    /* name of this zone                  */
   int	lifespan;           /* how long between resets (minutes)  */
   int	age;                /* current age of this zone (minutes) */
   int	top;                /* upper limit for rooms in this zone */

   int	reset_mode;         /* conditions for reset (see below)   */
   int	number;		    /* virtual number of this zone	  */
   struct reset_com *cmd;   /* command table for reset	          */
   int  pressure;	/* How is the pressure ( Mb ) */
   int  change;		/* How fast and what way does it change. */
   int  sky;		/* How is the sky. */

   char *creator;	/* Creator of zone */
   int  master_mob;	/* Master of zone */
   int	zon_flags;	/* ZON_???? flags */
   int	owner;		/* Clan owning the zone */
   char *builders;          /* builder list for this zone         */
   bool unloaded;	/* are the room descs loaded? 	  */
   bool locked;		/* locked due to editing */
   /*
	*  Reset mode:                              *
	*  0: Don't reset, and don't update age.    *
	*  1: Reset if no PC's are located in zone. *
	*  2: Just reset.                           *
   */
};



/* for queueing zones for update   */
struct reset_q_element {
   int	zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};



struct player_index_element {
   char	*name;
   long id;
};


struct help_index_element {
   char	*keyword;
   char *entry;
   int duplicate;
   int min_level;
};


struct dns_entry { /* dnsmod */
   int ip[4];
   char *name;
   struct dns_entry *next;
};

/* dnsmod - the magic number for dns caching */
#define DNS_HASH_NUM 257

/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};


/* global buffering system */

#ifdef __DB_C__
char	buf[MAX_STRING_LENGTH];
char	buf1[MAX_STRING_LENGTH];
char	buf2[MAX_STRING_LENGTH];
char	arg[MAX_STRING_LENGTH];
#else
extern struct player_special_data dummy_mob;
extern char	buf[MAX_STRING_LENGTH];
extern char	buf1[MAX_STRING_LENGTH];
extern char	buf2[MAX_STRING_LENGTH];
extern char	arg[MAX_STRING_LENGTH];
#endif


/* Externs exported from config.c (widely used strings and world settings) */
#ifndef __CONFIG_C__
extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;
extern char	*NONESTR;

extern int pk_allowed;
extern int pt_allowed;
extern int sleep_allowed;
extern int charm_allowed;
extern int summon_allowed;
extern int roomaffect_allowed;
extern int max_level_diff;
extern int olc_allowed;
extern int clan_olc_allowed;
#endif

#endif
