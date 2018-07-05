/* 
************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */

#define __CLASS_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "handler.h"

ACMD(do_help);

/* Names first */

const char *class_abbrevs[] = {
  "*~.Black Mage.~*",
  "<<<  Saiyan  >>>",
  "<-~  Vampire ~->",
  "><~  Demon   ~><",
  "=-=  None    =-=",
  "Ranger",
  "Blue Mage",
  "Cyborg",
  "White Mage",
  "Druid",
  "Alchemist",
  "Barbarian",
  "\n"
};

extern struct wis_app_type wis_app[];
extern struct con_app_type con_app[];
extern int siteok_everyone;

/* local functions */
int parse_class(char arg);
long find_class_bitvector(char arg);
byte saving_throws(int class_num, int type, int level);
int thaco(int class_num, int level);
void roll_real_abils(struct char_data * ch);
void do_start(struct char_data * ch);
int backstab_mult(int level);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(int chclass, int level);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);
extern void send_to_all(const char *messg);

const char *pc_class_types[] = {
  "Black Mage",
  "Saiyan",
  "Vampire",
  "Demon",
  "None",
  "Ranger",
  "Warlock",
  "Cyborg",
  "White Mage",
  "Druid",
  "Alchemist",
  "Barbarian",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
/*
const char *class_menu =

"\r\n"
"&CSelect a class:&w\r\n"
"  [&YS&w]aiyan\r\n"
"  [&YV&w]ampire\r\n"
"  [&YD&w]emon\r\n"
"  [&YB&w]lack Mage\r\n"
//"  [&YP&w]aladin     &R+&w\r\n"
//"  [&YR&w]anger      &R+&w\r\n"
//"  W[&YA&w]rlock     &R+&w\r\n"
//"  C[&YY&w]borg      &R+&w\r\n"
//"  [&YBL&w]ecromancer &R+&w\r\n"
//"  [&YD&w]ruid       &R+&w\r\n"
//"  A[&Yl&w]chemist   &R+&w\r\n"
//"  [&YB&w]arbarian   &R+&w\r\n"
"\r\n"
"  &R+&w = New classes, are finished, but you must obtain full\r\n"
"      level 100 to upgrade to a new class, remeber there is 1350 levels\r\n"
"      play it safe, there are 12 classes.&w\r\n";
*/
/* Class dependent exp modifiers */





void help_class(struct char_data *ch, int class)
{
  do_help(ch, (char *)pc_class_types[class], 0, 0);
}


/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'm': return CLASS_MAGIC_USER;
  case 's': return CLASS_CLERIC;
  case 'd': return CLASS_WARRIOR;
  case 'v': return CLASS_THIEF;
/*  case 'p': return CLASS_PALADIN;
  case 'r': return CLASS_RANGER;
  case 'a': return CLASS_WARLOCK;
  case 'y': return CLASS_CYBORG;
  case 'k': return CLASS_NECROMANCER;
  case 's': return CLASS_DRUID;
  case 'l': return CLASS_ALCHEMIST;
  case 'b': return CLASS_BARBARIAN; */
  default:  return CLASS_UNDEFINED;
  }
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
    case 'm': return (1 << CLASS_MAGIC_USER);
    case 's': return (1 << CLASS_CLERIC);
    case 'v': return (1 << CLASS_THIEF);
    case 'd': return (1 << CLASS_WARRIOR);
/*    case 'p': return (1 << CLASS_PALADIN);
    case 'r': return (1 << CLASS_RANGER);
    case 'a': return (1 << CLASS_WARLOCK);
    case 'y': return (1 << CLASS_CYBORG);
    case 'k': return (1 << CLASS_NECROMANCER);
    case 's': return (1 << CLASS_DRUID);
    case 'l': return (1 << CLASS_ALCHEMIST);
    case 'b': return (1 << CLASS_BARBARIAN); */
    default:  return 0;
  }
}

int cmp_class(char *class_str)
{
  int i;
  for (i = 0; i < NUM_CLASSES; i++) {
    if (!strn_cmp(class_str, pc_class_types[i], strlen(class_str))) return i;
  }  
  return(-1);
}

/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/*
#define LEARNED_LEVEL		0	% known which is considered "learned" 
#define MAX_PER_PRAC_SPELL	1	max percent gain in spell per practice 
#define MIN_PER_PRAC_SPELL	2	min percent gain in spell per practice 
#define MAX_PER_PRAC_SKILL	3	max percent gain in skill per practice 
#define MIN_PER_PRAC_SKILL	4	min percent gain in skill per practice 
#define PRAC_TYPE		5	should it say 'spell' or 'skill'?	
*/

int prac_params[6][NUM_CLASSES] = {
/* MAG    CLE	 THE	WAR    PAL    RAN    WLO    CYB    NEC   DRU   ALC   BAR */
  {95,	  95,	 85,	80,    80,    85,    85,    80,    85,   95,   95,   80   },		/* learned level */
  {100,	  100,	 30,	30,    35,    35,    65,    25,    100,  100,  100,  25   },		/* max per prac spell */
  {25,	  25,	 0,	0,     0,     0,     15,    0,     20,   25,   35,   0    },		/* min per pac spell */
  {5,	  4,	 12,	12,    11,    12,    9,	    14,    8,    4,    2,    13   },		/* max per prac skill */
  {0,	  0,	 0,	0,     0,     0,     0,	    1,     0,    0,    0,    0    },		/* min per pac skill */
  {SPELL, SPELL, SKILL,	SKILL, SKILL, SKILL, SPELL, SKILL, SPELL,SPELL,SPELL,SKILL}		/* prac name */
};


int train_params[6][NUM_CLASSES] = {
/* MAG    CLE    THE    WAR    PAL    RAN    WLO    CYB    NEC   DRU   ALC   BAR */
  {100,   100,   100,   100,   100,   100,   100,   100,   100,   95,   95,   80 },           
  {100,   100,   30,    30,    35,    35,    65,    25,    100,  100,  100,  25   },            
  {25,    25,    50,    50,     50,     50,     15,    50,     20,   25,   35,   50    },            
  {25,     50,     12,    12,    11,    12,    9,     14,    8,    4,    2,    13   },            
  {100,     50,     50,     50,     50,     50,     50,     1,     50,    50,    50,    50    },            
  {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50}             
};

/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 */
int guild_info[][3] = {

/* Midgaard */
  {CLASS_MAGIC_USER,	3017,	SCMD_SOUTH},
  {CLASS_CLERIC,	3004,	SCMD_NORTH},
  {CLASS_THIEF,		3027,	SCMD_EAST},
  {CLASS_WARRIOR,	3021,	SCMD_EAST},
 
  {CLASS_WARLOCK,	3156,	SCMD_WEST},
  {CLASS_PALADIN,	3159,	SCMD_SOUTH},
  {CLASS_RANGER,	3085,	SCMD_UP},

  {CLASS_CYBORG,	3162,	SCMD_SOUTH},
/* Brass Dragon */
  {-999 /* all */ ,	5065,	SCMD_WEST},

/* this must go last -- add new guards above! */
{-1, -1, -1}};

/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */
  
byte saving_throws(int class_num, int type, int level)
{

  if (level > MAX_MORT_LEVEL) return 0;
  switch (class_num) {
  case CLASS_MAGIC_USER:
  case CLASS_WARLOCK:
  case CLASS_ALCHEMIST:
    switch (type) {
    case SAVING_PARA:	/* Paralyzation */
      switch (level) {
        case  0: return 90;
        case  1: return 70;
        case  2: return 69;
        case  3: return 68;
        case  4: return 67;
        case  5: return 66;
        case  6: return 65;
        case  7: return 63;
        case  8: return 61;
        case  9: return 60;
        case 10: return 59;
        case 11: return 57;
        case 12: return 55;
        case 13: return 54;
        case 14: return 53;
        case 15: return 53;
        case 16: return 52;
        case 17: return 51;
        case 18: return 50;
        case 19: return 48;
        case 20: return 46;
        case 21: return 45;
        case 22: return 44;
        case 23: return 42;
        case 24: return 40;
        case 25: return 38;
        case 26: return 36;
        case 27: return 34;
        case 28: return 32;
        case 29: return 30;
        case 30: return 28;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for mage paralyzation saving throw.");
        }
      case SAVING_ROD:	/* Rods */
        switch (level) {
        case  0: return 90;
        case  1: return 55;
        case  2: return 53;
        case  3: return 51;
        case  4: return 49;
        case  5: return 47;
        case  6: return 45;
        case  7: return 43;
        case  8: return 41;
        case  9: return 40;
        case 10: return 39;
        case 11: return 37;
        case 12: return 35;
        case 13: return 33;
        case 14: return 31;
        case 15: return 30;
        case 16: return 29;
        case 17: return 27;
        case 18: return 25;
        case 19: return 23;
        case 20: return 21;
        case 21: return 20;
        case 22: return 19;
        case 23: return 17;
        case 24: return 15;
        case 25: return 14;
        case 26: return 13;
        case 27: return 12;
        case 28: return 11;
        case 29: return 10;
        case 30: return  9;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for mage rod saving throw.");
        }
      case SAVING_PETRI:	/* Petrification */
        switch (level) {
        case  0: return 90;
        case  1: return 65;
        case  2: return 63;
        case  3: return 61;
        case  4: return 59;
        case  5: return 57;
        case  6: return 55;
        case  7: return 53;
        case  8: return 51;
        case  9: return 50;
        case 10: return 49;
        case 11: return 47;
        case 12: return 45;
        case 13: return 43;
        case 14: return 41;
        case 15: return 40;
        case 16: return 39;
        case 17: return 37;
        case 18: return 35;
        case 19: return 33;
        case 20: return 31;
        case 21: return 30;
        case 22: return 29;
        case 23: return 27;
        case 24: return 25;
        case 25: return 23;
        case 26: return 21;
        case 27: return 19;
        case 28: return 17;
        case 29: return 15;
        case 30: return 13;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for mage petrification saving throw.");
        }
      case SAVING_BREATH:	/* Breath weapons */
        switch (level) {
        case  0: return 90;
        case  1: return 75;
        case  2: return 73;
        case  3: return 71;
        case  4: return 69;
        case  5: return 67;
        case  6: return 65;
        case  7: return 63;
        case  8: return 61;
        case  9: return 60;
        case 10: return 59;
        case 11: return 57;
        case 12: return 55;
        case 13: return 53;
        case 14: return 51;
        case 15: return 50;
        case 16: return 49;
        case 17: return 47;
        case 18: return 45;
        case 19: return 43;
        case 20: return 41;
        case 21: return 40;
        case 22: return 39;
        case 23: return 37;
        case 24: return 35;
        case 25: return 33;
        case 26: return 31;
        case 27: return 29;
        case 28: return 27;
        case 29: return 25;
        case 30: return 23;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for mage breath saving throw.");
        }
      case SAVING_SPELL:	/* Generic spells */
        switch (level) {
        case  0: return 90;
        case  1: return 60;
        case  2: return 58;
        case  3: return 56;
        case  4: return 54;
        case  5: return 52;
        case  6: return 50;
        case  7: return 48;
        case  8: return 46;
        case  9: return 45;
        case 10: return 44;
        case 11: return 42;
        case 12: return 40;
        case 13: return 38;
        case 14: return 36;
        case 15: return 35;
        case 16: return 34;
        case 17: return 32;
        case 18: return 30;
        case 19: return 28;
        case 20: return 26;
        case 21: return 25;
        case 22: return 24;
        case 23: return 22;
        case 24: return 20;
        case 25: return 18;
        case 26: return 16;
        case 27: return 14;
        case 28: return 12;
        case 29: return 10;
        case 30: return  8;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for mage spell saving throw.");
        }
      default:
        log("SYSERR: Invalid saving throw type.");
      }
      break;
    case CLASS_CLERIC:
    case CLASS_DRUID:
    case CLASS_PALADIN:
      switch (type) {
      case SAVING_PARA:	/* Paralyzation */
        switch (level) {
        case  0: return 90;
        case  1: return 60;
        case  2: return 59;
        case  3: return 48;
        case  4: return 46;
        case  5: return 45;
        case  6: return 43;
        case  7: return 40;
        case  8: return 37;
        case  9: return 35;
        case 10: return 34;
        case 11: return 33;
        case 12: return 31;
        case 13: return 30;
        case 14: return 29;
        case 15: return 27;
        case 16: return 26;
        case 17: return 25;
        case 18: return 24;
        case 19: return 23;
        case 20: return 22;
        case 21: return 21;
        case 22: return 20;
        case 23: return 18;
        case 24: return 15;
        case 25: return 14;
        case 26: return 12;
        case 27: return 10;
        case 28: return  9;
        case 29: return  8;
        case 30: return  7;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for cleric paralyzation saving throw.");
        }
      case SAVING_ROD:	/* Rods */
        switch (level) {
        case  0: return 90;
        case  1: return 70;
        case  2: return 69;
        case  3: return 68;
        case  4: return 66;
        case  5: return 65;
        case  6: return 63;
        case  7: return 60;
        case  8: return 57;
        case  9: return 55;
        case 10: return 54;
        case 11: return 53;
        case 12: return 51;
        case 13: return 50;
        case 14: return 49;
        case 15: return 47;
        case 16: return 46;
        case 17: return 45;
        case 18: return 44;
        case 19: return 43;
        case 20: return 42;
        case 21: return 41;
        case 22: return 40;
        case 23: return 38;
        case 24: return 35;
        case 25: return 34;
        case 26: return 32;
        case 27: return 30;
        case 28: return 29;
        case 29: return 28;
        case 30: return 27;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for cleric rod saving throw.");
        }
      case SAVING_PETRI:	/* Petrification */
        switch (level) {
        case  0: return 90;
        case  1: return 65;
        case  2: return 64;
        case  3: return 63;
        case  4: return 61;
        case  5: return 60;
        case  6: return 58;
        case  7: return 55;
        case  8: return 53;
        case  9: return 50;
        case 10: return 49;
        case 11: return 48;
        case 12: return 46;
        case 13: return 45;
        case 14: return 44;
        case 15: return 43;
        case 16: return 41;
        case 17: return 40;
        case 18: return 39;
        case 19: return 38;
        case 20: return 37;
        case 21: return 36;
        case 22: return 35;
        case 23: return 33;
        case 24: return 31;
        case 25: return 29;
        case 26: return 27;
        case 27: return 25;
        case 28: return 24;
        case 29: return 23;
        case 30: return 22;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for cleric petrification saving throw.");
        }
      case SAVING_BREATH:	/* Breath weapons */
        switch (level) {
        case  0: return 90;
        case  1: return 80;
        case  2: return 79;
        case  3: return 78;
        case  4: return 76;
        case  5: return 75;
        case  6: return 73;
        case  7: return 70;
        case  8: return 67;
        case  9: return 65;
        case 10: return 64;
        case 11: return 63;
        case 12: return 61;
        case 13: return 60;
        case 14: return 59;
        case 15: return 57;
        case 16: return 56;
        case 17: return 55;
        case 18: return 54;
        case 19: return 53;
        case 20: return 52;
        case 21: return 51;
        case 22: return 50;
        case 23: return 48;
        case 24: return 45;
        case 25: return 44;
        case 26: return 42;
        case 27: return 40;
        case 28: return 39;
        case 29: return 38;
        case 30: return 37;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for cleric breath saving throw.");
        }
      case SAVING_SPELL:	/* Generic spells */
        switch (level) {
        case  0: return 90;
        case  1: return 75;
        case  2: return 74;
        case  3: return 73;
        case  4: return 71;
        case  5: return 70;
        case  6: return 68;
        case  7: return 65;
        case  8: return 63;
        case  9: return 60;
        case 10: return 59;
        case 11: return 58;
        case 12: return 56;
        case 13: return 55;
        case 14: return 54;
        case 15: return 53;
        case 16: return 51;
        case 17: return 50;
        case 18: return 49;
        case 19: return 48;
        case 20: return 47;
        case 21: return 46;
        case 22: return 45;
        case 23: return 43;
        case 24: return 41;
        case 25: return 39;
        case 26: return 37;
        case 27: return 35;
        case 28: return 34;
        case 29: return 33;
        case 30: return 32;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for cleric spell saving throw.");
        }
      default:
        log("SYSERR: Invalid saving throw type.");
      }
      break;
    case CLASS_THIEF:
    case CLASS_NECROMANCER:
    case CLASS_RANGER:
      switch (type) {
      case SAVING_PARA:	/* Paralyzation */
        switch (level) {
        case  0: return 90;
        case  1: return 65;
        case  2: return 64;
        case  3: return 63;
        case  4: return 62;
        case  5: return 61;
        case  6: return 60;
        case  7: return 59;
        case  8: return 58;
        case  9: return 57;
        case 10: return 56;
        case 11: return 55;
        case 12: return 54;
        case 13: return 53;
        case 14: return 52;
        case 15: return 51;
        case 16: return 50;
        case 17: return 49;
        case 18: return 48;
        case 19: return 47;
        case 20: return 46;
        case 21: return 45;
        case 22: return 44;
        case 23: return 43;
        case 24: return 42;
        case 25: return 41;
        case 26: return 40;
        case 27: return 39;
        case 28: return 38;
        case 29: return 37;
        case 30: return 36;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for thief paralyzation saving throw.");
        }
      case SAVING_ROD:	/* Rods */
        switch (level) {
        case  0: return 90;
        case  1: return 70;
        case  2: return 68;
        case  3: return 66;
        case  4: return 64;
        case  5: return 62;
        case  6: return 60;
        case  7: return 58;
        case  8: return 56;
        case  9: return 54;
        case 10: return 52;
        case 11: return 50;
        case 12: return 48;
        case 13: return 46;
        case 14: return 44;
        case 15: return 42;
        case 16: return 40;
        case 17: return 38;
        case 18: return 36;
        case 19: return 34;
        case 20: return 32;
        case 21: return 30;
        case 22: return 28;
        case 23: return 26;
        case 24: return 24;
        case 25: return 22;
        case 26: return 20;
        case 27: return 18;
        case 28: return 16;
        case 29: return 14;
        case 30: return 13;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for thief rod saving throw.");
        }
      case SAVING_PETRI:	/* Petrification */
        switch (level) {
        case  0: return 90;
        case  1: return 60;
        case  2: return 59;
        case  3: return 58;
        case  4: return 58;
        case  5: return 56;
        case  6: return 55;
        case  7: return 54;
        case  8: return 53;
        case  9: return 52;
        case 10: return 51;
        case 11: return 50;
        case 12: return 49;
        case 13: return 48;
        case 14: return 47;
        case 15: return 46;
        case 16: return 45;
        case 17: return 44;
        case 18: return 43;
        case 19: return 42;
        case 20: return 41;
        case 21: return 40;
        case 22: return 39;
        case 23: return 38;
        case 24: return 37;
        case 25: return 36;
        case 26: return 35;
        case 27: return 34;
        case 28: return 33;
        case 29: return 32;
        case 30: return 31;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for thief petrification saving throw.");
        }
      case SAVING_BREATH:	/* Breath weapons */
        switch (level) {
        case  0: return 90;
        case  1: return 80;
        case  2: return 79;
        case  3: return 78;
        case  4: return 77;
        case  5: return 76;
        case  6: return 75;
        case  7: return 74;
        case  8: return 73;
        case  9: return 72;
        case 10: return 71;
        case 11: return 70;
        case 12: return 69;
        case 13: return 68;
        case 14: return 67;
        case 15: return 66;
        case 16: return 65;
        case 17: return 64;
        case 18: return 63;
        case 19: return 62;
        case 20: return 61;
        case 21: return 60;
        case 22: return 59;
        case 23: return 58;
        case 24: return 57;
        case 25: return 56;
        case 26: return 55;
        case 27: return 54;
        case 28: return 53;
        case 29: return 52;
        case 30: return 51;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for thief breath saving throw.");
        }
      case SAVING_SPELL:	/* Generic spells */
        switch (level) {
        case  0: return 90;
        case  1: return 75;
        case  2: return 73;
        case  3: return 71;
        case  4: return 69;
        case  5: return 67;
        case  6: return 65;
        case  7: return 63;
        case  8: return 61;
        case  9: return 59;
        case 10: return 57;
        case 11: return 55;
        case 12: return 53;
        case 13: return 51;
        case 14: return 49;
        case 15: return 47;
        case 16: return 45;
        case 17: return 43;
        case 18: return 41;
        case 19: return 39;
        case 20: return 37;
        case 21: return 35;
        case 22: return 33;
        case 23: return 31;
        case 24: return 29;
        case 25: return 27;
        case 26: return 25;
        case 27: return 23;
        case 28: return 21;
        case 29: return 19;
        case 30: return 17;
        case 31: return  0;
        case 32: return  0;
        case 33: return  0;
        case 34: return  0;
        case 35: return  0;
        case 36: return  0;
        case 37: return  0;
        case 38: return  0;
        case 39: return  0;
        case 40: return  0;
        default:
  	log("SYSERR: Missing level for thief spell saving throw.");
        }
      default:
        log("SYSERR: Invalid saving throw type.");
      }
      break;
    case CLASS_WARRIOR:
    case CLASS_CYBORG:
    case CLASS_BARBARIAN:
      switch (type) {
      case SAVING_PARA:	/* Paralyzation */
        switch (level) {
        case  0: return 90;
        case  1: return 70;
        case  2: return 68;
        case  3: return 67;
        case  4: return 65;
        case  5: return 62;
        case  6: return 58;
        case  7: return 55;
        case  8: return 53;
        case  9: return 52;
        case 10: return 50;
        case 11: return 47;
        case 12: return 43;
        case 13: return 40;
        case 14: return 38;
        case 15: return 37;
        case 16: return 35;
        case 17: return 32;
        case 18: return 28;
        case 19: return 25;
        case 20: return 24;
        case 21: return 23;
        case 22: return 22;
        case 23: return 20;
        case 24: return 19;
        case 25: return 17;
        case 26: return 16;
        case 27: return 15;
        case 28: return 14;
        case 29: return 13;
        case 30: return 12;
        case 31: return 11;
        case 32: return 10;
        case 33: return  9;
        case 34: return  8;
        case 35: return  7;
        case 36: return  6;
        case 37: return  5;
        case 38: return  4;
        case 39: return  3;
        case 40: return  2;
        default:
  	log("SYSERR: Missing level for warrior paralyzation saving throw.");
        }
      case SAVING_ROD:	/* Rods */
        switch (level) {
        case  0: return 90;
        case  1: return 80;
        case  2: return 78;
        case  3: return 77;
        case  4: return 75;
        case  5: return 72;
        case  6: return 68;
        case  7: return 65;
        case  8: return 63;
        case  9: return 62;
        case 10: return 60;
        case 11: return 57;
        case 12: return 53;
        case 13: return 50;
        case 14: return 48;
        case 15: return 47;
        case 16: return 45;
        case 17: return 42;
        case 18: return 38;
        case 19: return 35;
        case 20: return 34;
        case 21: return 33;
        case 22: return 32;
        case 23: return 30;
        case 24: return 29;
        case 25: return 27;
        case 26: return 26;
        case 27: return 25;
        case 28: return 24;
        case 29: return 23;
        case 30: return 22;
        case 31: return 20;
        case 32: return 18;
        case 33: return 16;
        case 34: return 14;
        case 35: return 12;
        case 36: return 10;
        case 37: return  8;
        case 38: return  6;
        case 39: return  5;
        case 40: return  4;
        default:
  	log("SYSERR: Missing level for warrior rod saving throw.");
        }
      case SAVING_PETRI:	/* Petrification */
        switch (level) {
        case  0: return 90;
        case  1: return 75;
        case  2: return 73;
        case  3: return 72;
        case  4: return 70;
        case  5: return 67;
        case  6: return 63;
        case  7: return 60;
        case  8: return 58;
        case  9: return 57;
        case 10: return 55;
        case 11: return 52;
        case 12: return 48;
        case 13: return 45;
        case 14: return 43;
        case 15: return 42;
        case 16: return 40;
        case 17: return 37;
        case 18: return 33;
        case 19: return 30;
        case 20: return 29;
        case 21: return 28;
        case 22: return 26;
        case 23: return 25;
        case 24: return 24;
        case 25: return 23;
        case 26: return 21;
        case 27: return 20;
        case 28: return 19;
        case 29: return 18;
        case 30: return 17;
        case 31: return 16;
        case 32: return 15;
        case 33: return 14;
        case 34: return 13;
        case 35: return 12;
        case 36: return 11;
        case 37: return 10;
        case 38: return  9;
        case 39: return  8;
        case 40: return  7;
        default:
  	log("SYSERR: Missing level for warrior petrification saving throw.");
        }
      case SAVING_BREATH:	/* Breath weapons */
        switch (level) {
        case  0: return 90;
        case  1: return 85;
        case  2: return 83;
        case  3: return 82;
        case  4: return 80;
        case  5: return 75;
        case  6: return 70;
        case  7: return 65;
        case  8: return 63;
        case  9: return 62;
        case 10: return 60;
        case 11: return 55;
        case 12: return 50;
        case 13: return 45;
        case 14: return 43;
        case 15: return 42;
        case 16: return 40;
        case 17: return 37;
        case 18: return 33;
        case 19: return 30;
        case 20: return 29;
        case 21: return 28;
        case 22: return 26;
        case 23: return 25;
        case 24: return 24;
        case 25: return 23;
        case 26: return 21;
        case 27: return 20;
        case 28: return 19;
        case 29: return 18;
        case 30: return 17;
        case 31: return 16;
        case 32: return 15;
        case 33: return 14;
        case 34: return 13;
        case 35: return 12;
        case 36: return 11;
        case 37: return 10;
        case 38: return  9;
        case 39: return  8;
        case 40: return  7;
        default:
  	log("SYSERR: Missing level for warrior breath saving throw.");
        }
      case SAVING_SPELL:	/* Generic spells */
        switch (level) {
        case  0: return 90;
        case  1: return 85;
        case  2: return 83;
        case  3: return 82;
        case  4: return 80;
        case  5: return 77;
        case  6: return 73;
        case  7: return 70;
        case  8: return 68;
        case  9: return 67;
        case 10: return 65;
        case 11: return 62;
        case 12: return 58;
        case 13: return 55;
        case 14: return 53;
        case 15: return 52;
        case 16: return 50;
        case 17: return 47;
        case 18: return 43;
        case 19: return 40;
        case 20: return 39;
        case 21: return 38;
        case 22: return 36;
        case 23: return 35;
        case 24: return 34;
        case 25: return 33;
        case 26: return 31;
        case 27: return 30;
        case 28: return 29;
        case 29: return 28;
        case 30: return 27;
        case 31: return 25;
        case 32: return 23;
        case 33: return 21;
        case 34: return 19;
        case 35: return 17;
        case 36: return 15;
        case 37: return 13;
        case 38: return 11;
        case 39: return  9;
        case 40: return  7;
        default:
  	log("SYSERR: Missing level for warrior spell saving throw.");
        }
      default:
        log("SYSERR: Invalid saving throw type.");
      }
    default:
      log("SYSERR: Invalid class saving throw.");
    }
  
    /* Should not get here unless something is wrong. */
    return 100;
  }
  
  /* THAC0 for classes and levels.  (To Hit Armor Class 0) */
  int thaco(int class_num, int level)
  {
    if (level >= LVL_IMMORT) return 1;
    switch (class_num) {
    case CLASS_MAGIC_USER:
    case CLASS_ALCHEMIST:
    case CLASS_NECROMANCER:
      switch (level) {
    case 0: return 100;
case 1: return 20;
case 2: return 20;
case 3: return 20;
case 4: return 20;
case 5: return 20;
case 6: return 19;
case 7: return 19;
case 8: return 19;
case 9: return 19;
case 10: return 19;
case 11: return 18;
case 12: return 18;
case 13: return 18;
case 14: return 18;
case 15: return 18;
case 16: return 17;
case 17: return 17;
case 18: return 17;
case 19: return 17;
case 20: return 17;
case 21: return 16;
case 22: return 16;
case 23: return 16;
case 24: return 16;
case 25: return 16;
case 26: return 15;
case 27: return 15;
case 28: return 15;
case 29: return 15;
case 30: return 15;
case 31: return 14;
case 32: return 14;
case 33: return 14;
case 34: return 14;
case 35: return 14;
case 36: return 13;
case 37: return 13;
case 38: return 13;
case 39: return 13;
case 40: return 13;
case 41: return 12;
case 42: return 12;
case 43: return 12;
case 44: return 12;
case 45: return 12;
case 46: return 11;
case 47: return 11;
case 48: return 11;
case 49: return 11;
case 50: return 11;
case 51: return 10;
case 52: return 10;
case 53: return 10;
case 54: return 10;
case 55: return 10;
case 56: return 9;
case 57: return 9;
case 58: return 9;
case 59: return 9;
case 60: return 9;
case 61: return 8;
case 62: return 8;
case 63: return 8;
case 64: return 8;
case 65: return 8;
case 66: return 7;
case 67: return 7;
case 68: return 7;
case 69: return 7;
case 70: return 7;
case 71: return 6;
case 72: return 6;
case 73: return 6;
case 74: return 6;
case 75: return 6;
case 76: return 5;
case 77: return 5;
case 78: return 5;
case 79: return 5;
case 80: return 5;
case 81: return 4;
case 82: return 4;
case 83: return 4;
case 84: return 4;
case 85: return 4;
case 86: return 3;
case 87: return 3;
case 88: return 3;
case 89: return 3;
case 90: return 3;
case 91: return 2;
case 92: return 2;
case 93: return 2;
case 94: return 2;
case 95: return 2;
case 96: return 1;
case 97: return 1;
case 98: return 1;
case 99: return 1;
case 100: return 1;
case 101: return 1;
      default:
        log("SYSERR: Missing level for mage thac0.");
      }
    case CLASS_CLERIC:
    case CLASS_CYBORG:
    case CLASS_RANGER:
      switch (level) {
case 0: return 100;
case 1: return 20;
case 2: return 20;
case 3: return 20;
case 4: return 20;
case 5: return 20;
case 6: return 19;
case 7: return 19;
case 8: return 19;
case 9: return 19;
case 10: return 19;
case 11: return 18;
case 12: return 18;
case 13: return 18;
case 14: return 18;
case 15: return 18;
case 16: return 17;
case 17: return 17;
case 18: return 17;
case 19: return 17;
case 20: return 17;
case 21: return 16;
case 22: return 16;
case 23: return 16;
case 24: return 16;
case 25: return 16;
case 26: return 15;
case 27: return 15;
case 28: return 15;
case 29: return 15;
case 30: return 15;
case 31: return 14;
case 32: return 14;
case 33: return 14;
case 34: return 14;
case 35: return 14;
case 36: return 13;
case 37: return 13;
case 38: return 13;
case 39: return 13;
case 40: return 13;
case 41: return 12;
case 42: return 12;
case 43: return 12;
case 44: return 12;
case 45: return 12;
case 46: return 11;
case 47: return 11;
case 48: return 11;
case 49: return 11;
case 50: return 11;
case 51: return 10;
case 52: return 10;
case 53: return 10;
case 54: return 10;
case 55: return 10;
case 56: return 9;
case 57: return 9;
case 58: return 9;
case 59: return 9;
case 60: return 9;
case 61: return 8;
case 62: return 8;
case 63: return 8;
case 64: return 8;
case 65: return 8;
case 66: return 7;
case 67: return 7;
case 68: return 7;
case 69: return 7;
case 70: return 7;
case 71: return 6;
case 72: return 6;
case 73: return 6;
case 74: return 6;
case 75: return 6;
case 76: return 5;
case 77: return 5;
case 78: return 5;
case 79: return 5;
case 80: return 5;
case 81: return 4;
case 82: return 4;
case 83: return 4;
case 84: return 4;
case 85: return 4;
case 86: return 3;
case 87: return 3;
case 88: return 3;
case 89: return 3;
case 90: return 3;
case 91: return 2;
case 92: return 2;
case 93: return 2;
case 94: return 2;
case 95: return 2;
case 96: return 1;
case 97: return 1;
case 98: return 1;
case 99: return 1;
case 100: return 1;
case 101: return 1;
      default:
        log("SYSERR: Missing level for cleric thac0.");
      }
    case CLASS_THIEF:
    case CLASS_DRUID:
      switch (level) {
     case 0: return 100;
case 1: return 20;
case 2: return 20;
case 3: return 20;
case 4: return 20;
case 5: return 20;
case 6: return 19;
case 7: return 19;
case 8: return 19;
case 9: return 19;
case 10: return 19;
case 11: return 18;
case 12: return 18;
case 13: return 18;
case 14: return 18;
case 15: return 18;
case 16: return 17;
case 17: return 17;
case 18: return 17;
case 19: return 17;
case 20: return 17;
case 21: return 16;
case 22: return 16;
case 23: return 16;
case 24: return 16;
case 25: return 16;
case 26: return 15;
case 27: return 15;
case 28: return 15;
case 29: return 15;
case 30: return 15;
case 31: return 14;
case 32: return 14;
case 33: return 14;
case 34: return 14;
case 35: return 14;
case 36: return 13;
case 37: return 13;
case 38: return 13;
case 39: return 13;
case 40: return 13;
case 41: return 12;
case 42: return 12;
case 43: return 12;
case 44: return 12;
case 45: return 12;
case 46: return 11;
case 47: return 11;
case 48: return 11;
case 49: return 11;
case 50: return 11;
case 51: return 10;
case 52: return 10;
case 53: return 10;
case 54: return 10;
case 55: return 10;
case 56: return 9;
case 57: return 9;
case 58: return 9;
case 59: return 9;
case 60: return 9;
case 61: return 8;
case 62: return 8;
case 63: return 8;
case 64: return 8;
case 65: return 8;
case 66: return 7;
case 67: return 7;
case 68: return 7;
case 69: return 7;
case 70: return 7;
case 71: return 6;
case 72: return 6;
case 73: return 6;
case 74: return 6;
case 75: return 6;
case 76: return 5;
case 77: return 5;
case 78: return 5;
case 79: return 5;
case 80: return 5;
case 81: return 4;
case 82: return 4;
case 83: return 4;
case 84: return 4;
case 85: return 4;
case 86: return 3;
case 87: return 3;
case 88: return 3;
case 89: return 3;
case 90: return 3;
case 91: return 2;
case 92: return 2;
case 93: return 2;
case 94: return 2;
case 95: return 2;
case 96: return 1;
case 97: return 1;
case 98: return 1;
case 99: return 1;
case 100: return 1;
case 101: return 1;
      default:
        log("SYSERR: Missing level for thief thac0.");
      }
    case CLASS_WARRIOR:
    case CLASS_BARBARIAN:
      switch (level) {
 case 0: return 100;
case 1: return 20;
case 2: return 20;
case 3: return 20;
case 4: return 20;
case 5: return 20;
case 6: return 19;
case 7: return 19;
case 8: return 19;
case 9: return 19;
case 10: return 19;
case 11: return 18;
case 12: return 18;
case 13: return 18;
case 14: return 18;
case 15: return 18;
case 16: return 17;
case 17: return 17;
case 18: return 17;
case 19: return 17;
case 20: return 17;
case 21: return 16;
case 22: return 16;
case 23: return 16;
case 24: return 16;
case 25: return 16;
case 26: return 15;
case 27: return 15;
case 28: return 15;
case 29: return 15;
case 30: return 15;
case 31: return 14;
case 32: return 14;
case 33: return 14;
case 34: return 14;
case 35: return 14;
case 36: return 13;
case 37: return 13;
case 38: return 13;
case 39: return 13;
case 40: return 13;
case 41: return 12;
case 42: return 12;
case 43: return 12;
case 44: return 12;
case 45: return 12;
case 46: return 11;
case 47: return 11;
case 48: return 11;
case 49: return 11;
case 50: return 11;
case 51: return 10;
case 52: return 10;
case 53: return 10;
case 54: return 10;
case 55: return 10;
case 56: return 9;
case 57: return 9;
case 58: return 9;
case 59: return 9;
case 60: return 9;
case 61: return 8;
case 62: return 8;
case 63: return 8;
case 64: return 8;
case 65: return 8;
case 66: return 7;
case 67: return 7;
case 68: return 7;
case 69: return 7;
case 70: return 7;
case 71: return 6;
case 72: return 6;
case 73: return 6;
case 74: return 6;
case 75: return 6;
case 76: return 5;
case 77: return 5;
case 78: return 5;
case 79: return 5;
case 80: return 5;
case 81: return 4;
case 82: return 4;
case 83: return 4;
case 84: return 4;
case 85: return 4;
case 86: return 3;
case 87: return 3;
case 88: return 3;
case 89: return 3;
case 90: return 3;
case 91: return 2;
case 92: return 2;
case 93: return 2;
case 94: return 2;
case 95: return 2;
case 96: return 1;
case 97: return 1;
case 98: return 1;
case 99: return 1;
case 100: return 1;
case 101: return 1;
      default:
        log("SYSERR: Missing level for warrior thac0.");
      }
    case CLASS_PALADIN:
    case CLASS_WARLOCK:
      switch (level) {
case 0: return 100;
case 1: return 20;
case 2: return 20;
case 3: return 20;
case 4: return 20;
case 5: return 20;
case 6: return 19;
case 7: return 19;
case 8: return 19;
case 9: return 19;
case 10: return 19;
case 11: return 18;
case 12: return 18;
case 13: return 18;
case 14: return 18;
case 15: return 18;
case 16: return 17;
case 17: return 17;
case 18: return 17;
case 19: return 17;
case 20: return 17;
case 21: return 16;
case 22: return 16;
case 23: return 16;
case 24: return 16;
case 25: return 16;
case 26: return 15;
case 27: return 15;
case 28: return 15;
case 29: return 15;
case 30: return 15;
case 31: return 14;
case 32: return 14;
case 33: return 14;
case 34: return 14;
case 35: return 14;
case 36: return 13;
case 37: return 13;
case 38: return 13;
case 39: return 13;
case 40: return 13;
case 41: return 12;
case 42: return 12;
case 43: return 12;
case 44: return 12;
case 45: return 12;
case 46: return 11;
case 47: return 11;
case 48: return 11;
case 49: return 11;
case 50: return 11;
case 51: return 10;
case 52: return 10;
case 53: return 10;
case 54: return 10;
case 55: return 10;
case 56: return 9;
case 57: return 9;
case 58: return 9;
case 59: return 9;
case 60: return 9;
case 61: return 8;
case 62: return 8;
case 63: return 8;
case 64: return 8;
case 65: return 8;
case 66: return 7;
case 67: return 7;
case 68: return 7;
case 69: return 7;
case 70: return 7;
case 71: return 6;
case 72: return 6;
case 73: return 6;
case 74: return 6;
case 75: return 6;
case 76: return 5;
case 77: return 5;
case 78: return 5;
case 79: return 5;
case 80: return 5;
case 81: return 4;
case 82: return 4;
case 83: return 4;
case 84: return 4;
case 85: return 4;
case 86: return 3;
case 87: return 3;
case 88: return 3;
case 89: return 3;
case 90: return 3;
case 91: return 2;
case 92: return 2;
case 93: return 2;
case 94: return 2;
case 95: return 2;
case 96: return 1;
case 97: return 1;
case 98: return 1;
case 99: return 1;
case 100: return 1;
case 101: return 1;
      default:
        log("SYSERR: Missing level for warlock and paladin thac0.");
      }

    default:
      log("SYSERR: Unknown class in thac0 chart.");
    }
  
    /* Will not get there unless something is wrong. */
    return 100;
  }



/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data * ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch)) {
  case CLASS_MAGIC_USER:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_CLERIC:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_THIEF:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_WARRIOR:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = number(0, 100);
    break;
  case CLASS_PALADIN:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_RANGER:
    ch->real_abils.dex = table[0];
    ch->real_abils.con = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_WARLOCK:
    ch->real_abils.str = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.dex = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = number(0, 100);
    break;
    break;
  case CLASS_CYBORG:
    ch->real_abils.con = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_NECROMANCER:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.con = table[3];
    ch->real_abils.dex = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_DRUID:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_ALCHEMIST:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_BARBARIAN:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = number(0, 100);
    break;
  }
  ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
/*
void eq_char(struct char_data *ch, obj_vnum vnum, int where)
{
  struct obj_data *obj;
  obj = read_object(vnum, VIRTUAL);
  equip_char(ch, obj, where);
}

void initial_equip(struct char_data *ch)
{
  
  switch (ch->player.class) {
    case CLASS_WARRIOR:
      eq_char(ch, 3022, WEAR_WIELD);
      eq_char(ch, 3042, WEAR_HOLD);
      eq_char(ch, 3086, WEAR_ARMS);
      break;
  
  }

}
*/



void do_start(struct char_data * ch)
{
  ACMD(do_newbie);
//if (GET_CLASS(ch) == CLASS_WARRIOR) {
  GET_LEGEND_LEVELS(ch) = 0;

 
 GET_LEVEL(ch) = 1;
  GET_TOT_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;
  GET_CLASS(ch) = CLASS_PALADIN;

  set_title(ch, NULL);
  /* roll_real_abils(ch); */
  ch->points.max_hit = 100;
  ch->points.max_mana = 100;
  ch->points.max_move = 100;
  ch->points.demonxp = 100;
  
  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    break;

  case CLASS_CLERIC:
    break;

  case CLASS_THIEF:
    SET_SKILL(ch, SKILL_SNEAK, 10);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_STEAL, 15);
    SET_SKILL(ch, SKILL_BACKSTAB, 10);
    SET_SKILL(ch, SKILL_PICK_LOCK, 10);
    SET_SKILL(ch, SKILL_TRACK, 10);
    break;

  case CLASS_WARRIOR:
    break;
    
  case CLASS_PALADIN:
    break;
    
  case CLASS_RANGER:
    SET_SKILL(ch, SKILL_FORAGE, 50);
    SET_SKILL(ch, SKILL_BOW, 30);
    break;
    
  case CLASS_WARLOCK:
    break;
    
  case CLASS_CYBORG:
    SET_BIT(PRF_FLAGS(ch), PRF_EXAMUNIT);
    break;
  }

  advance_level(ch);
  GET_LEGEND_LEVELS(ch) = 1;
  GET_PLAYER_KILLS(ch) = 1;
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  if (GET_CLASS(ch) == CLASS_THIEF) {
    GET_COND(ch, THIRST) = 100;
    GET_COND(ch, FULL) = 100;
  }
  else {
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, FULL) = -1;
  }
  GET_COND(ch, DRUNK) = 0;
  SET_BIT(PLR_FLAGS(ch), PLR_NEWBIE);
  do_newbie(ch, "self", 0, 0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  if (siteok_everyone)
    SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data * ch)
{
  int check_clan_level(struct char_data *ch);

  int add_hp = 0, add_mana = 0, add_move = 0, i;

  add_hp = con_app[GET_CON(ch)].hitp;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    add_hp = 0;
    add_mana = 0;
   add_mana = 0;
    add_move = 0;
    break;

  case CLASS_CLERIC:
    add_hp = 0;
    add_mana = 0;
    add_mana = 0;
    add_move = 0;
    break;

  case CLASS_THIEF:
    add_hp = 0;
    add_mana = 0;
    add_move = 0;
    break;

  case CLASS_WARRIOR:
    add_hp = 0;
    add_mana = 0;
    add_move = 0;
    break;
    
  case CLASS_PALADIN:
    add_hp += number(9, 13);
    add_mana = number((int) (0.7 * GET_LEVEL(ch)), (int) (1.25 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana,7);
    add_move = number(1, 3);
    break;
    
  case CLASS_RANGER:
    add_hp += number(6, 11);
    add_mana = number(0, 3);
    add_move = number(2, 5);
    break;  
    
  case CLASS_WARLOCK:
    add_hp += number(5, 9);
    add_mana = number(GET_LEVEL(ch), (int) (1.25 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(1, 3);
    break;  
    
  case CLASS_CYBORG:
    add_hp += number(9, 14);
    add_mana = number(0, 3);
    add_move = number(2, 5);
    break;  
    
  case CLASS_NECROMANCER:
    add_hp += number(6, 10);
    add_mana = number(GET_LEVEL(ch), (int) (1.25 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(1, 2);
    break;  
    
  case CLASS_DRUID:
    add_hp += number(5, 8);
    add_mana = number(GET_LEVEL(ch), (int) (1.4 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(2, 3);
    break;  
    
  case CLASS_ALCHEMIST:
    add_hp += number(3, 8);
    add_mana = number((int) (1.2 * GET_LEVEL(ch)), (int) (1.8 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 25);
    add_move = number(1, 2);
    break;  
    
  case CLASS_BARBARIAN:
    add_hp += number(9, 14);
    add_mana = number(0, 3);
    add_move = number(3, 6);
    break;  
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC || 
      GET_CLASS(ch) == CLASS_WARLOCK || GET_CLASS(ch) == CLASS_NECROMANCER ||
      GET_CLASS(ch) == CLASS_DRUID || GET_CLASS(ch) == CLASS_ALCHEMIST)
    GET_PRACTICES(ch) += MAX(3, 2+wis_app[GET_WIS(ch)].bonus);
  else
    GET_PRACTICES(ch) += MIN(3, 2+MAX(1, wis_app[GET_WIS(ch)].bonus));

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    GET_HOMETOWN(ch) = GOD_HOMETOWN;
  } else {
    if (GET_HOMETOWN(ch) == GOD_HOMETOWN)
      GET_HOMETOWN(ch) = DEFAULT_HOMETOWN;
    REMOVE_BIT(PRF_FLAGS(ch), PRF_NOHASSLE | PRF_HOLYLIGHT | PRF_ROOMFLAGS);
  }
  if (GET_LEVEL(ch) > MAX_NEWBIE_LEVEL) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_NEWBIE);
  }
  check_clan_level(ch);
  save_char(ch, NOWHERE);
  sprintf(buf, "[ <-Info-> ] %s has advanced to level (%d) with a total level of (%d)!\r\n", 
GET_NAME(ch), GET_LEVEL(ch), GET_TOT_LEVEL(ch));
  send_to_all(buf);
  sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0)
    return 1;	  /* level 0 */
  else if (level <= 7)
    return 2;	  /* level 1 - 7 */
  else if (level <= 13)
    return 3;	  /* level 8 - 13 */
  else if (level <= 20)
    return 4;	  /* level 14 - 20 */
  else if (level <= 28)
    return 5;	  /* level 21 - 28 */
  else if (level < LVL_IMMORT)
    return 6;	  /* all remaining mortal levels */
  else
    return 20;	  /* immortals */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 * (Yil) added: ITEM_ANTI_MORT not usable by mortals.
 */

int invalid_class(struct char_data *ch, struct obj_data *obj) {
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_MORT) && !IS_IMMORT(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch)) || 
      (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch)) || 
      (IS_OBJ_STAT(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_PALADIN) && IS_PALADIN(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch)) || 
      (IS_OBJ_STAT(obj, ITEM_ANTI_WARLOCK) && IS_WARLOCK(ch)) || 
      (IS_OBJ_STAT(obj, ITEM_ANTI_CYBORG) && IS_CYBORG(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_NECROMANCER) && IS_NECROMANCER(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_DRUID) && IS_DRUID(ch)) || 
      (IS_OBJ_STAT(obj, ITEM_ANTI_ALCHEMIST) && IS_ALCHEMIST(ch)) || 
      (IS_OBJ_STAT(obj, ITEM_ANTI_BARBARIAN) && IS_BARBARIAN(ch)))
	return 1;
  else
	return 0;
}

/* Invalid level */
int invalid_level(struct char_data *ch, struct obj_data *obj) 
{
  if (GET_OBJ_BOTTOM_LEV(obj) > GET_LEVEL(ch) ||
      GET_OBJ_TOP_LEV(obj) < GET_LEVEL(ch) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_MORT) && GET_LEVEL(ch) < LVL_IMMORT)) 
  return 1;
  else return 0;
}


/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void) {
  int i, j;
  /* These variables are not used in this version of VisionMUD */
  /*
  int cls_mage = (1 << CLASS_MAGIC_USER);
  int cls_cleric = (1 << CLASS_CLERIC);
  int cls_thief = (1 << CLASS_THIEF);
  int cls_warrior = (1 << CLASS_WARRIOR);
  int cls_paladin = (1 << CLASS_PALADIN);
  int cls_ranger = (1 << CLASS_RANGER);
  int cls_warlock = (1 << CLASS_WARLOCK);
  int cls_cyborg = (1 << CLASS_CYBORG);
  int cls_necromancer = (1 << CLASS_NECROMANCER);
  int cls_druid = (1 << CLASS_DRUID);
  int cls_alchemist = (1 << CLASS_ALCHEMIST);
  int cls_barbarian = (1 << CLASS_BARBARIAN);
  */
  /* Assign a spell/skill to a a whole group of classes (0 is all)
     For instance, { SKILL_SECOND_ATTACK, cls_mage | cls_cleric, 14 },
     will give mages and clerics the SECOND_ATTACK skill at level 14.
     More convenient than individual spell_level()s.  Use 0 to give
     a skill to all the classes.
       -dkoepke */
  int base_skl[][3] = {
    { SKILL_MOUNT , 0, 1 },
    { SKILL_RIDING, 0, 1 },
    
    { -1, -1 } /* THIS MUST END THE LIST */
  };
  
  /* give all the base_skl[]'s */
  for (j = 0; base_skl[j][0] != -1; j++)
    for (i = 0; i < NUM_CLASSES; i++)
      if (!base_skl[j][1] || IS_SET(base_skl[j][1], (1 << i)))
        spell_level(base_skl[j][0], i, base_skl[j][2]);

  /* in my base patch, cls_mage, etc. are unused and that leads to */
  /* annyoing warnings, so here I'll use them... */
  /*
  j = (cls_mage-cls_mage)+(cls_cleric-cls_cleric)+(cls_thief-cls_thief)+
      (cls_warrior-cls_warrior)+(cls_warlock-cls_warlock)+
      (cls_paladin-cls_paladin)+(cls_ranger-cls_ranger)+
      (cls_cyborg-cls_cyborg);
  */
  /* MAGES */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_INVISIBLE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ARMOR, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_TRAP_AWARE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_STRENGTH, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_THROW, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SLEEP, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_TAME, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CURSE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_POISON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_FIREBALL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_MANA_TRANSFER, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CHARM, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_HOLD_PERSON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_RECHARGE, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_CROSSBOW, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_PORTAL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_GROUP_SHIELD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_FIRE_ELEMENTAL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_METEOR_SHOWER, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CLONE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CORPSE_HOST, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_FIRE_SHIELD, CLASS_MAGIC_USER, 1);
spell_level(SKILL_UNARMED_COMBAT, CLASS_MAGIC_USER, 1);

  /* CLERICS */
  spell_level(SPELL_CURE_LIGHT, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ARMOR, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CREATE_FOOD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CREATE_WATER, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_TRAP_AWARE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DETECT_ALIGN, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CURE_BLIND, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_BLESS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_HOLY_WORD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_PROT_FIRE, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_SLING, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_HOLD_PERSON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_POISON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_GROUP_ARMOR, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CURE_CRITIC, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_TAME, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SUMMON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_REMOVE_POISON, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_PROT_COLD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 1);
  spell_level(SPELL_EARTHQUAKE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_LIFE_TRANSFER, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_BOW, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DISPEL_EVIL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DISPEL_GOOD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SANCTUARY, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_HEAL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_DISARM, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_SENSE_LIFE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_HARM, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_GROUP_SHIELD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_GROUP_HEAL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ARCANE_WORD, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_FEAR, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_AIR_ELEMENTAL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_HOLY_SHOUT, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_ARCANE_PORTAL, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_REMOVE_CURSE, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_SECOND_ATTACK, CLASS_MAGIC_USER, 1);
  spell_level(SKILL_THIRD_ATTACK, CLASS_MAGIC_USER, 1);
  


  /* THIEVES */
  spell_level(SKILL_SNEAK, CLASS_THIEF, 1);
  spell_level(SKILL_HAND_TO_HAND, CLASS_THIEF, 1);
  spell_level(SKILL_THROW, CLASS_THIEF, 2);
  spell_level(SKILL_PICK_LOCK, CLASS_THIEF, 2);
  spell_level(SKILL_TRAP_AWARE, CLASS_THIEF, 2);
  spell_level(SKILL_BACKSTAB, CLASS_THIEF, 3);
  spell_level(SKILL_TAME, CLASS_THIEF, 3);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_THIEF, 3);
  spell_level(SKILL_TRAP_AWARE, CLASS_THIEF, 4);
  spell_level(SKILL_STEAL, CLASS_THIEF, 4);
  spell_level(SKILL_HIDE, CLASS_THIEF, 5);
  spell_level(SPELL_HOLD_PERSON, CLASS_THIEF, 5);
  spell_level(SKILL_TRACK, CLASS_THIEF, 6);
  spell_level(SKILL_SECOND_ATTACK, CLASS_THIEF, 8);
  spell_level(SKILL_RETREAT, CLASS_THIEF, 10);
  spell_level(SPELL_CURE_CRITIC, CLASS_THIEF, 12);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_THIEF, 13);
  spell_level(SKILL_BOW, CLASS_THIEF, 14);
  spell_level(SPELL_STONESKIN, CLASS_THIEF, 15);
  spell_level(SKILL_CROSSBOW, CLASS_THIEF, 16);
  spell_level(SPELL_ACID_ARROW, CLASS_THIEF, 17);
  spell_level(SPELL_BLINK, CLASS_THIEF, 17);
  spell_level(SPELL_LEVITATE, CLASS_THIEF, 18);
  spell_level(SPELL_HASTE, CLASS_THIEF, 19);
  spell_level(SKILL_PATHFINDING, CLASS_THIEF, 20);
  spell_level(SKILL_THIRD_ATTACK, CLASS_THIEF, 22);
  spell_level(SPELL_EARTH_ELEMENTAL, CLASS_THIEF, 28);
  spell_level(SPELL_GROUP_WATBREATH, CLASS_THIEF, 32);


  /* WARRIORS */
  spell_level(SKILL_HAND_TO_HAND, CLASS_WARRIOR, 1);
  spell_level(SKILL_KICK, CLASS_WARRIOR, 1);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_WARRIOR, 2);
  spell_level(SKILL_RESCUE, CLASS_WARRIOR, 3);
  spell_level(SPELL_MAGIC_MISSILE, CLASS_WARRIOR, 4);
  spell_level(SKILL_THROW, CLASS_WARRIOR, 4);
  spell_level(SKILL_TRAP_AWARE, CLASS_WARRIOR, 5);
  spell_level(SKILL_DISARM, CLASS_WARRIOR, 6);
  spell_level(SKILL_RETREAT, CLASS_WARRIOR, 7);
  spell_level(SKILL_TAME, CLASS_WARRIOR, 7);
  spell_level(SKILL_SLING, CLASS_WARRIOR, 8);
  spell_level(SKILL_TRACK, CLASS_WARRIOR, 9);
  spell_level(SPELL_CHILL_TOUCH, CLASS_WARRIOR, 9);
  spell_level(SKILL_SECOND_ATTACK, CLASS_WARRIOR, 10);
  spell_level(SPELL_SENSE_LIFE, CLASS_WARRIOR, 10);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_WARRIOR, 10);
  spell_level(SKILL_CROSSBOW, CLASS_WARRIOR, 11);
  spell_level(SKILL_BASH, CLASS_WARRIOR, 12);
  spell_level(SPELL_CURE_BLIND, CLASS_WARRIOR, 13);
  spell_level(SKILL_BOW, CLASS_WARRIOR, 14);
  spell_level(SKILL_SPRING, CLASS_WARRIOR, 15);
  spell_level(SKILL_THIRD_ATTACK, CLASS_WARRIOR, 16);
  spell_level(SPELL_FLAME_ARROW, CLASS_WARRIOR, 17);
  spell_level(SPELL_DISPEL_EVIL, CLASS_WARRIOR, 18);
  spell_level(SPELL_SUMMON, CLASS_WARRIOR, 22);
  spell_level(SPELL_DISRUPTING_RAY, CLASS_WARRIOR, 23);
  spell_level(SPELL_FIRE_ELEMENTAL, CLASS_WARRIOR, 29);
  spell_level(SPELL_WATERWALK, CLASS_WARRIOR, 32);
  
  /* RANGERS */
  spell_level(SKILL_FORAGE, CLASS_RANGER, 1);
  spell_level(SKILL_BOW, CLASS_RANGER, 1);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_RANGER, 2);
  spell_level(SKILL_TRAP_AWARE, CLASS_RANGER, 2);
  spell_level(SKILL_SPRING, CLASS_RANGER, 3);
  spell_level(SPELL_STONESKIN, CLASS_RANGER, 4);
  spell_level(SKILL_THROW, CLASS_RANGER, 5);
  spell_level(SKILL_TRACK, CLASS_RANGER, 6);
  spell_level(SPELL_CURE_LIGHT, CLASS_RANGER, 6);
  spell_level(SKILL_CROSSBOW, CLASS_RANGER, 7);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_RANGER, 7);
  spell_level(SKILL_PATHFINDING, CLASS_RANGER, 8);
  spell_level(SKILL_TAME, CLASS_RANGER, 8);
  spell_level(SKILL_SLING, CLASS_RANGER, 9);
  spell_level(SKILL_PARRY, CLASS_RANGER, 11);
  spell_level(SKILL_HIDE, CLASS_RANGER, 12);
  spell_level(SPELL_STEELSKIN, CLASS_RANGER, 13);
  spell_level(SKILL_SECOND_ATTACK, CLASS_RANGER, 14);
  spell_level(SPELL_BURNING_HANDS, CLASS_RANGER, 15);
  spell_level(SPELL_HASTE, CLASS_RANGER, 15);
  spell_level(SKILL_DISARM, CLASS_RANGER, 16);
  spell_level(SKILL_RETREAT, CLASS_RANGER, 17);
  spell_level(SPELL_DISPEL_MAGIC, CLASS_RANGER, 20);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_RANGER, 24);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_RANGER, 26);
  spell_level(SPELL_WATER_ELEMENTAL, CLASS_RANGER, 30);
  spell_level(SPELL_RECHARGE, CLASS_RANGER, 33);
  spell_level(SPELL_FIRE_SHIELD, CLASS_RANGER, 38);
  
  /* WARLOCKS */
  
  spell_level(SPELL_BURNING_HANDS, CLASS_WARLOCK, 1);
  spell_level(SKILL_THROW, CLASS_WARLOCK, 2);
  spell_level(SPELL_DEATH_WAVE, CLASS_WARLOCK, 3);
  spell_level(SPELL_DETECT_POISON, CLASS_WARLOCK, 4);
  spell_level(SPELL_ARMOR, CLASS_WARLOCK, 5);
  spell_level(SPELL_MAGIC_MISSILE, CLASS_WARLOCK, 5);
  spell_level(SPELL_DETECT_INVIS, CLASS_WARLOCK, 6);
  spell_level(SPELL_INFRAVISION, CLASS_WARLOCK, 6);
  spell_level(SKILL_TRAP_AWARE, CLASS_WARLOCK, 7);
  spell_level(SPELL_INVISIBLE, CLASS_WARLOCK, 8);
  spell_level(SKILL_CROSSBOW, CLASS_WARLOCK, 9);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_WARLOCK, 11);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_WARLOCK, 12);
  spell_level(SKILL_THROW, CLASS_WARLOCK, 13);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_WARLOCK, 15);
  spell_level(SPELL_CURSE, CLASS_WARLOCK, 16);
  spell_level(SPELL_PHANTASM, CLASS_WARLOCK, 17); 
  spell_level(SKILL_SECOND_ATTACK, CLASS_WARLOCK, 18);  
  spell_level(SPELL_GROUP_ARMOR, CLASS_WARLOCK, 20);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_WARLOCK, 21);  
  spell_level(SKILL_SPRING, CLASS_WARLOCK, 21);  
  spell_level(SKILL_DISARM, CLASS_WARLOCK, 22);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_WARLOCK, 22);
  spell_level(SPELL_FEAR, CLASS_WARLOCK, 23);
  spell_level(SPELL_HARM, CLASS_WARLOCK, 24);
  spell_level(SKILL_BASH, CLASS_WARLOCK, 25);
  spell_level(SPELL_EARTH_ELEMENTAL, CLASS_WARLOCK, 25);
  spell_level(SPELL_GROUP_FLY, CLASS_WARLOCK, 27);
  spell_level(SPELL_BLOODLUST, CLASS_WARLOCK, 30);
  spell_level(SKILL_THIRD_ATTACK, CLASS_WARLOCK, 32);
  spell_level(SPELL_DEATHDANCE, CLASS_WARLOCK, 33);
  
  
  /* PALADINS */
  spell_level(SPELL_CURE_LIGHT, CLASS_PALADIN, 1);
  spell_level(SPELL_ARMOR, CLASS_PALADIN, 1);
  spell_level(SKILL_SLING, CLASS_PALADIN, 2);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_PALADIN, 2);
  spell_level(SKILL_TRAP_AWARE, CLASS_PALADIN, 3);
  spell_level(SPELL_CREATE_WATER, CLASS_PALADIN, 3);
  spell_level(SKILL_CROSSBOW, CLASS_PALADIN, 4);
  spell_level(SKILL_RESCUE, CLASS_PALADIN, 5);
  spell_level(SKILL_THROW, CLASS_PALADIN, 5);
  spell_level(SKILL_DISARM, CLASS_PALADIN, 6);
  spell_level(SPELL_HOLY_WORD, CLASS_PALADIN, 7);
  spell_level(SKILL_PARRY, CLASS_PALADIN, 7);
  spell_level(SPELL_CREATE_FOOD, CLASS_PALADIN, 8);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_PALADIN, 9);
  spell_level(SPELL_CURE_BLIND, CLASS_PALADIN, 10);
  spell_level(SPELL_CURE_CRITIC, CLASS_PALADIN, 11);
  spell_level(SKILL_DISARM, CLASS_PALADIN, 11);
  spell_level(SPELL_BLESS, CLASS_PALADIN, 12);
  spell_level(SKILL_SECOND_ATTACK, CLASS_PALADIN, 12);
  spell_level(SPELL_INFRAVISION, CLASS_PALADIN, 13);
  spell_level(SKILL_BASH, CLASS_PALADIN, 13);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_PALADIN, 14);
  spell_level(SKILL_BOW, CLASS_PALADIN, 14);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_PALADIN, 15);
  spell_level(SPELL_HOLY_SHOUT, CLASS_PALADIN, 16);
  spell_level(SPELL_DISPEL_GOOD, CLASS_PALADIN, 16);
  spell_level(SPELL_SHIELD, CLASS_PALADIN, 17);
  spell_level(SPELL_INVISIBLE, CLASS_PALADIN, 21);
  spell_level(SPELL_SUMMON, CLASS_PALADIN, 22);
  spell_level(SPELL_SANCTUARY, CLASS_PALADIN, 23);
  spell_level(SPELL_WATERWALK, CLASS_PALADIN, 24);
  spell_level(SPELL_REMOVE_CURSE, CLASS_PALADIN, 26);
  spell_level(SPELL_AIR_ELEMENTAL, CLASS_PALADIN, 27);
  spell_level(SKILL_SECOND_ATTACK, CLASS_PALADIN, 29);
  spell_level(SPELL_PORTAL, CLASS_PALADIN, 31);
  
  /* CYBORGS */
  spell_level(SKILL_EXAMINE, CLASS_CYBORG, 1);
  spell_level(SKILL_KICK, CLASS_CYBORG, 1);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_CYBORG, 2);
  spell_level(SKILL_TRAP_AWARE, CLASS_CYBORG, 3);
  spell_level(SPELL_CURE_LIGHT, CLASS_CYBORG, 3);
  spell_level(SPELL_BURNING_HANDS, CLASS_CYBORG, 5);
  spell_level(SPELL_STEELSKIN, CLASS_CYBORG, 7);
  spell_level(SKILL_CROSSBOW, CLASS_CYBORG, 8);
  spell_level(SPELL_ACID_ARROW, CLASS_CYBORG, 8);
  spell_level(SKILL_TRACK, CLASS_CYBORG, 9);
  spell_level(SPELL_SHIELD, CLASS_CYBORG, 9);
  spell_level(SKILL_HIDE, CLASS_CYBORG, 10);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_CYBORG, 11);
  spell_level(SPELL_SLEEP, CLASS_CYBORG, 12);
  spell_level(SKILL_THROW, CLASS_CYBORG, 14);
  spell_level(SKILL_BASH, CLASS_CYBORG, 14);
  spell_level(SKILL_SECOND_ATTACK, CLASS_CYBORG, 15);
  spell_level(SPELL_WATERWALK, CLASS_CYBORG, 15);
  spell_level(SKILL_BOW, CLASS_CYBORG, 16);
  spell_level(SPELL_LEVITATE, CLASS_CYBORG, 17);
  spell_level(SKILL_DISARM, CLASS_CYBORG, 18);
  spell_level(SPELL_FLY, CLASS_CYBORG, 19);
  spell_level(SPELL_PEACE, CLASS_CYBORG, 19);
  spell_level(SKILL_THIRD_ATTACK, CLASS_CYBORG, 20);
  spell_level(SPELL_WATERBREATH, CLASS_CYBORG, 21);
  spell_level(SPELL_DEATHDANCE, CLASS_CYBORG, 22);
  spell_level(SPELL_GROUP_SHIELD, CLASS_CYBORG, 23);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_CYBORG, 23);
  spell_level(SKILL_TAME, CLASS_CYBORG, 25);
  spell_level(SPELL_METEOR_SHOWER, CLASS_CYBORG, 26);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CYBORG, 28);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_CYBORG, 29);
  spell_level(SPELL_FIRE_ELEMENTAL, CLASS_CYBORG, 29);
  spell_level(SPELL_FIREBALL, CLASS_CYBORG, 30);
  spell_level(SPELL_RECHARGE, CLASS_CYBORG, 34);
  
  /* NECROMANCERS */
  spell_level(SPELL_DEATH_RIPPLE, CLASS_NECROMANCER, 1);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_NECROMANCER, 1);
  spell_level(SKILL_THROW, CLASS_NECROMANCER, 2);
  spell_level(SKILL_TRAP_AWARE, CLASS_NECROMANCER, 2);
  spell_level(SPELL_STONESKIN, CLASS_NECROMANCER, 3);
  spell_level(SKILL_SLING, CLASS_NECROMANCER, 4);
  spell_level(SPELL_HOLD_PERSON, CLASS_NECROMANCER, 5);
  spell_level(SPELL_BLOODLUST, CLASS_NECROMANCER, 6);
  spell_level(SPELL_DEATH_WAVE, CLASS_NECROMANCER, 7);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_NECROMANCER, 8);
  spell_level(SPELL_HASTE, CLASS_NECROMANCER, 8);
  spell_level(SKILL_TRACK, CLASS_NECROMANCER, 9);
  spell_level(SPELL_PROT_COLD, CLASS_NECROMANCER, 9);
  spell_level(SKILL_CROSSBOW, CLASS_NECROMANCER, 10);
  spell_level(SPELL_CURE_CRITIC, CLASS_NECROMANCER, 11);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_NECROMANCER, 11);
  spell_level(SKILL_SECOND_ATTACK, CLASS_NECROMANCER, 12);
  spell_level(SPELL_DEATHDANCE, CLASS_NECROMANCER, 12);
  spell_level(SPELL_FLAME_ARROW, CLASS_NECROMANCER, 13);
  spell_level(SPELL_POISON, CLASS_NECROMANCER, 14);
  spell_level(SPELL_SLEEP, CLASS_NECROMANCER, 14);
  spell_level(SKILL_RETREAT, CLASS_NECROMANCER, 15);
  spell_level(SPELL_PHANTASM, CLASS_NECROMANCER, 16);
  spell_level(SKILL_BOW, CLASS_NECROMANCER, 17);
  spell_level(SKILL_BASH, CLASS_NECROMANCER, 18);
  spell_level(SKILL_BACKSTAB, CLASS_NECROMANCER, 19);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_NECROMANCER, 20);
  spell_level(SPELL_EARTH_ELEMENTAL, CLASS_NECROMANCER, 21);
  spell_level(SPELL_BLADEBARRIER, CLASS_NECROMANCER, 22);
  spell_level(SPELL_PARALYZE, CLASS_NECROMANCER, 24);
  spell_level(SPELL_CURSE, CLASS_NECROMANCER, 25);
  spell_level(SPELL_CLONE, CLASS_NECROMANCER, 26);
  spell_level(SPELL_REMOVE_CURSE, CLASS_NECROMANCER, 27);
  spell_level(SKILL_THIRD_ATTACK, CLASS_NECROMANCER, 29);
  spell_level(SPELL_ARCANE_WORD, CLASS_NECROMANCER, 30);
  spell_level(SPELL_ARCANE_PORTAL, CLASS_NECROMANCER, 30);
  spell_level(SPELL_CORPSE_HOST, CLASS_NECROMANCER, 31);
  
  
  /* DRUIDS */
  spell_level(SPELL_STONESKIN, CLASS_DRUID, 1);
  spell_level(SKILL_FORAGE, CLASS_DRUID, 2);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_DRUID, 2);
  spell_level(SKILL_THROW, CLASS_DRUID, 3);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_DRUID, 3);
  spell_level(SPELL_INVISIBLE, CLASS_DRUID, 4);
  spell_level(SKILL_TRAP_AWARE, CLASS_DRUID, 4);
  spell_level(SPELL_PROT_FIRE, CLASS_DRUID, 5);
  spell_level(SPELL_LEVITATE, CLASS_DRUID, 5);
  spell_level(SPELL_SHIELD, CLASS_DRUID, 6);
  spell_level(SKILL_TAME, CLASS_DRUID, 6);
  spell_level(SPELL_AID, CLASS_DRUID, 7);
  spell_level(SPELL_CONE_OF_COLD, CLASS_DRUID, 8);
  spell_level(SPELL_BARKSKIN, CLASS_DRUID, 9);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_DRUID, 9);
  spell_level(SKILL_BOW, CLASS_DRUID, 10);
  spell_level(SPELL_PROT_COLD, CLASS_DRUID, 10);
  spell_level(SPELL_HOLD_PERSON, CLASS_DRUID, 11);
  spell_level(SPELL_MIRROR_IMAGE, CLASS_DRUID, 12);
  spell_level(SPELL_WATERBREATH, CLASS_DRUID, 12);
  spell_level(SPELL_REMOVE_POISON, CLASS_DRUID, 13);
  spell_level(SPELL_STRENGTH, CLASS_DRUID, 14);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_DRUID, 15);
  spell_level(SPELL_FLAME_ARROW, CLASS_DRUID, 15);
  spell_level(SPELL_FLY, CLASS_DRUID, 16);
  spell_level(SPELL_SLEEP, CLASS_DRUID, 17);
  spell_level(SPELL_LIFE_TRANSFER, CLASS_DRUID, 18);
  spell_level(SPELL_SUMMON, CLASS_DRUID, 19);
  spell_level(SKILL_SECOND_ATTACK, CLASS_DRUID, 20);
  spell_level(SPELL_WALL_OF_FOG, CLASS_DRUID, 20);
  spell_level(SKILL_CROSSBOW, CLASS_DRUID, 21);
  spell_level(SPELL_ANTI_MAGIC, CLASS_DRUID, 22);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_DRUID, 23);
  spell_level(SPELL_WATER_ELEMENTAL, CLASS_DRUID, 24);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_DRUID, 26);
  spell_level(SPELL_SANCTUARY, CLASS_DRUID, 27);
  spell_level(SPELL_PROT_COLD, CLASS_DRUID, 29);
  spell_level(SPELL_ARCANE_WORD, CLASS_DRUID, 30);
  spell_level(SPELL_PEACE, CLASS_DRUID, 31);
  spell_level(SPELL_ARCANE_PORTAL, CLASS_DRUID, 33);
  
  
  
  
  /* ALCHEMISTS */
  spell_level(SPELL_SHIELD, CLASS_ALCHEMIST, 1);
  spell_level(SPELL_ARMOR, CLASS_ALCHEMIST, 2);
  spell_level(SKILL_SLING, CLASS_ALCHEMIST, 3);
  spell_level(SPELL_BLINDNESS, CLASS_ALCHEMIST, 4);
  spell_level(SPELL_CURE_BLIND, CLASS_ALCHEMIST, 4);
  spell_level(SKILL_TRAP_AWARE, CLASS_ALCHEMIST, 5);
  spell_level(SPELL_POISON, CLASS_ALCHEMIST, 5);
  spell_level(SKILL_THROW, CLASS_ALCHEMIST, 6);
  spell_level(SPELL_PROT_FIRE, CLASS_ALCHEMIST, 7);
  spell_level(SPELL_PROT_COLD, CLASS_ALCHEMIST, 7);
  spell_level(SPELL_ACID_ARROW, CLASS_ALCHEMIST, 8);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_ALCHEMIST, 8);
  spell_level(SPELL_DETECT_ALIGN, CLASS_ALCHEMIST, 9);
  spell_level(SPELL_INFRAVISION, CLASS_ALCHEMIST, 9);
  spell_level(SKILL_TAME, CLASS_ALCHEMIST, 10);
  spell_level(SPELL_FLAME_ARROW, CLASS_ALCHEMIST, 10);
  spell_level(SPELL_AID, CLASS_ALCHEMIST, 11);
  spell_level(SPELL_BARKSKIN, CLASS_ALCHEMIST, 12);
  spell_level(SKILL_BOW, CLASS_ALCHEMIST, 13);
  spell_level(SPELL_DETECT_INVIS, CLASS_ALCHEMIST, 13);
  spell_level(SPELL_METEOR_SHOWER, CLASS_ALCHEMIST, 15);
  spell_level(SKILL_BOW, CLASS_ALCHEMIST, 16);
  spell_level(SPELL_DETECT_MAGIC, CLASS_ALCHEMIST, 17);
  spell_level(SPELL_WATERWALK, CLASS_ALCHEMIST, 18);
  spell_level(SPELL_BLINK, CLASS_ALCHEMIST, 18);
  spell_level(SKILL_CROSSBOW, CLASS_ALCHEMIST, 19);
  spell_level(SPELL_AID, CLASS_ALCHEMIST, 20);
  spell_level(SPELL_CHAIN_LIGHTNING, CLASS_ALCHEMIST, 20);
  spell_level(SPELL_DISRUPTING_RAY, CLASS_ALCHEMIST, 21);
  spell_level(SPELL_ANTI_MAGIC, CLASS_ALCHEMIST, 22);
  spell_level(SPELL_HARM, CLASS_ALCHEMIST, 23);
  spell_level(SPELL_GROUP_RECALL, CLASS_ALCHEMIST, 23);
  spell_level(SPELL_INVISIBLE, CLASS_ALCHEMIST, 24);
  spell_level(SPELL_GROUP_HEAL, CLASS_ALCHEMIST, 24);
  spell_level(SPELL_AIR_ELEMENTAL, CLASS_ALCHEMIST, 24);
  spell_level(SPELL_PARALYZE, CLASS_ALCHEMIST, 25);
  spell_level(SPELL_RECHARGE, CLASS_ALCHEMIST, 25);
  spell_level(SPELL_CURSE, CLASS_ALCHEMIST, 26);
  spell_level(SPELL_MANA_TRANSFER, CLASS_ALCHEMIST, 27);
  spell_level(SPELL_PORTAL, CLASS_ALCHEMIST, 28);
  spell_level(SPELL_FIRE_SHIELD, CLASS_ALCHEMIST, 32);
  spell_level(SPELL_LIFE_TRANSFER, CLASS_ALCHEMIST, 34);
  
  
  /* BARBARIANS */
  spell_level(SKILL_KICK, CLASS_BARBARIAN, 1);
  spell_level(SKILL_HAND_TO_HAND, CLASS_BARBARIAN, 1);
  spell_level(SKILL_THROW, CLASS_BARBARIAN, 2);
  spell_level(SPELL_MAGIC_MISSILE, CLASS_BARBARIAN, 3);
  spell_level(SKILL_BOW, CLASS_BARBARIAN, 4);
  spell_level(SKILL_TRAP_AWARE, CLASS_BARBARIAN, 4);
  spell_level(SKILL_SNEAK, CLASS_BARBARIAN, 5);
  spell_level(SKILL_UNARMED_COMBAT, CLASS_BARBARIAN, 6);
  spell_level(SKILL_DISARM, CLASS_BARBARIAN, 7);
  spell_level(SKILL_SLING, CLASS_BARBARIAN, 7);
  spell_level(SPELL_BURNING_HANDS, CLASS_BARBARIAN, 8);
  spell_level(SKILL_PARRY, CLASS_BARBARIAN, 8);
  spell_level(SKILL_BASH, CLASS_BARBARIAN, 9);
  spell_level(SKILL_SECOND_ATTACK, CLASS_BARBARIAN, 10);
  spell_level(SKILL_CROSSBOW, CLASS_BARBARIAN, 10);
  spell_level(SPELL_PROT_COLD, CLASS_BARBARIAN, 11);
  spell_level(SKILL_HIDE, CLASS_BARBARIAN, 12);
  spell_level(SKILL_STEAL, CLASS_BARBARIAN, 13);
  spell_level(SKILL_BACKSTAB, CLASS_BARBARIAN, 13);
  spell_level(SPELL_STEELSKIN, CLASS_BARBARIAN, 14);
  spell_level(SPELL_MINOR_IDENTIFY, CLASS_BARBARIAN, 15);
  spell_level(SPELL_PARALYZE, CLASS_BARBARIAN, 17);
  spell_level(SPELL_ACID_ARROW, CLASS_BARBARIAN, 18);
  spell_level(SPELL_CONE_OF_COLD, CLASS_BARBARIAN, 19);
  spell_level(SKILL_THIRD_ATTACK, CLASS_BARBARIAN, 20);
  spell_level(SPELL_WEAKEN, CLASS_BARBARIAN, 21);
  spell_level(SPELL_CONE_OF_COLD, CLASS_BARBARIAN, 23);
  spell_level(SPELL_DISRUPTING_RAY, CLASS_BARBARIAN, 24);
  spell_level(SPELL_PROT_FIRE, CLASS_BARBARIAN, 15);
  spell_level(SPELL_BLINDNESS, CLASS_BARBARIAN, 20);
  spell_level(SKILL_RETREAT, CLASS_BARBARIAN, 22);
  spell_level(SPELL_LEVITATE, CLASS_BARBARIAN, 26);
  spell_level(SPELL_BLADEBARRIER, CLASS_BARBARIAN, 29);
  spell_level(SPELL_WATER_ELEMENTAL, CLASS_BARBARIAN, 32);
  spell_level(SPELL_CHARM, CLASS_BARBARIAN, 34);
}


/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  90000000
#define EXP_MAX_MORT	20000000

/*  
int exp_mod(int level)
{
  if (level <= 10) return 10;
  else if (level <= 20) return 40;
  else if (level <= 30) return 80;
  else if (level <= 55) return 90;
  else if (level <= 60) return 85;
  else if (level <= 65) return 84;
  else if (level <= 70) return 83;
  else if (level <= 75) return 82;
  else if (level <= 80) return 81;
  else return 8.1;
}

int class_mod(int class) 
{
  switch (class) {
    case CLASS_MAGIC_USER:	return 15;
    case CLASS_CLERIC:		return 13;
    case CLASS_THIEF:		return 13;
    case CLASS_WARRIOR:		return 18;
    case CLASS_PALADIN:		return 18;
    case CLASS_RANGER:		return 15;
    case CLASS_WARLOCK:		return 17;
    case CLASS_CYBORG:		return 18;
    case CLASS_NECROMANCER:	return 15;
    case CLASS_ALCHEMIST:	return 13;
    case CLASS_DRUID:		return 13;
    case CLASS_BARBARIAN:	return 18;
    default: 
      log("SYSERR: Requesting exp modifier for invalid class!");
      return 20;
      break;
  }
}
*/
  
/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
  if (level > LVL_IMPL+1 || level < 0) {
   
    sprintf(buf, "SYSERR: Requesting exp for invalid level! (%d)", level);
    log(buf);
    return 0;
    }
  /*
   * Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist.
   */
   if (level > LVL_IMMORT) {
     return EXP_MAX - ((LVL_IMPL-level) * 1000);
   }
  // return level * 20 * exp_mod(level) * class_mod(class);
  /* Exp required for normal mortals is below */

  switch (chclass) {

    case CLASS_MAGIC_USER:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
    

    case CLASS_THIEF:
    switch (level) {
	      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;

    case CLASS_WARRIOR:
    switch (level) {
          case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;	
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
  
  
  case CLASS_PALADIN:
    switch (level) {
         case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;	
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
  

  case CLASS_RANGER:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;      
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;	
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
    
  case CLASS_WARLOCK:
    switch (level) {
      case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000; 
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;

  case CLASS_CYBORG:
    switch (level) {
           case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;	
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
  

  case CLASS_NECROMANCER:
    switch (level) {
          case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;	
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
    
  case CLASS_DRUID:
    switch (level) {
            case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;		
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
    
  case CLASS_ALCHEMIST:
    switch (level) {
         case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
   
  case CLASS_BARBARIAN:
    switch (level) {
            case  0: return 0;
      case  1: return 1;
      case  2: return 2000;
      case  3: return 4000;
      case  4: return 8000;
      case  5: return 16000;
      case  6: return 32000;
      case  7: return 64000;
      case  8: return 125000;
      case  9: return 250000;
      case 10: return 500000;
      case 11: return 750000;
      case 12: return 1000000;
      case 13: return 1250000;
      case 14: return 1500000;
      case 15: return 1850000;
      case 16: return 2200000;
      case 17: return 2550000;
      case 18: return 2900000;
      case 19: return 3250000;
      case 20: return 3600000;
      case 21: return 3900000;
      case 22: return 4200000;
      case 23: return 4500000;
      case 24: return 4800000;
      case 25: return 5150000;
      case 26: return 5500000;
      case 27: return 5950000;
      case 28: return 6400000;
      case 29: return 6850000;
      case 30: return 7400000;
      case 31: return 8400000;
      case 32: return 9400000;
      case 33: return 10400000;
      case 34: return 11400000;
      case 35: return 12400000;
      case 36: return 13400000;
      case 37: return 14400000;
      case 38: return 15400000;
      case 39: return 16400000;
      case 40: return 17400000;
      case 41: return 18400000;
      case 42: return 19400000;
      case 43: return 20400000;
      case 44: return 21400000;
      case 45: return 22200000;
      case 46: return 22400000;
      case 47: return 23400000;
      case 48: return 24400000;
      case 49: return 25400000;
      case 50: return 26400000;
      case 51: return 27400000;
      case 52: return 28400000;
      case 53: return 29400000;
      case 54: return 30400000;
      case 55: return 31400000;
      case 56: return 32400000;
      case 57: return 33400000;
      case 58: return 34400000;
      case 59: return 35400000;
      case 60: return 36400000;
      case 61: return 37400000;
      case 62: return 38400000;
      case 63: return 39400000;
      case 64: return 40400000;
      case 65: return 41400000;
      case 66: return 42400000;
      case 67: return 43400000;
      case 68: return 44400000;
      case 69: return 45400000;
      case 70: return 46400000;
      case 71: return 47400000;
      case 72: return 48400000;
      case 73: return 49400000;
      case 74: return 50400000;
      case 75: return 51400000;
      case 76: return 52400000;
      case 77: return 53400000;
      case 78: return 54400000;
      case 79: return 55400000;
      case 80: return 56400000;
      case 81: return 57400000;
      case 82: return 58400000;
      case 83: return 59400000;
      case 84: return 60400000;
      case 85: return 61400000;
      case 86: return 62400000;
      case 87: return 63400000;
      case 88: return 64400000;
      case 89: return 65400000;
      case 90: return 66400000;
      case 91: return 67400000;
      case 92: return 68400000;
      case 93: return 69400000;
      case 94: return 70400000;
      case 95: return 71400000;
      case 96: return 72400000;
      case 97: return 73400000;
      case 98: return 74400000;
      case 99: return 75400000;
      case 100: return 76400000;	
      /* add new levels here */
    }
    if (level > MAX_MORT_LEVEL && level <= LVL_IMMORT) return EXP_MAX_MORT;
    break;
  }
    
  /*
   * This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete -- so, complete them!
   */
  log("SYSERR: XP tables not set up correctly in class.c!");
  return 123456;
}


/* 
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Man";
  if (level == LVL_IMPL)
    return "the Implementor";

  switch (chclass) {

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic"; 
      case  2: return "the Spell Student"; 
      case  3: return "the Scholar of Magic"; 
      case  4: return "the Delver in Spells"; 
      case  5: return "the Medium of Magic"; 
      case  6: return "the Scribe of Magic"; 
      case  7: return "the Seer"; 
      case  8: return "the Sage"; 
      case  9: return "the Illusionist"; 
      case 10: return "the Abjurer"; 
      case 11: return "the Invoker"; 
      case 12: return "the Enchanter"; 
      case 13: return "the Conjurer"; 
      case 14: return "the Magician"; 
      case 15: return "the Creator"; 
      case 16: return "the Savant"; 
      case 17: return "the Magus"; 
      case 18: return "the Wizard"; 
      case 19: return "the Warlock"; 
      case 20: return "the Sorcerer"; 
      case 21: return "the Necromancer"; 
      case 22: return "the Thaumaturge"; 
      case 23: return "the Student of the Occult"; 
      case 24: return "the Disciple of the Uncanny"; 
      case 25: return "the Minor Elemental"; 
      case 26: return "the Greater Elemental"; 
      case 27: return "the Crafter of Magics"; 
      case 28: return "the Shaman"; 
      case 29: return "the Keeper of Talismans"; 
      case 30: return "the Archmage"; 
      case LVL_IMMORT: return "the Immortal Warlock"; 
      case LVL_GOD: return "the Avatar of Magic"; 
      case LVL_GRGOD: return "the God of Magic"; 
      case LVL_IMPL: return "the Implementor"; 
      default: return "the Mage"; 
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Saiyan"; 
      case  2: return "the Saiyan"; 
      case  3: return "the Saiyan"; 
      case  4: return "the Saiyan"; 
      case  5: return "the Saiyan"; 
      case  6: return "the Saiyan"; 
      case  7: return "the Saiyan"; 
      case  8: return "the Saiyan"; 
      case  9: return "the Saiyan"; 
      case 10: return "the Saiyan"; 
      case 11: return "the Saiyan"; 
      case 12: return "the Saiyan"; 
      case 13: return "the Saiyan"; 
      case 14: return "the Saiyan"; 
      case 15: return "the Saiyan"; 
      case 16: return "the Saiyan"; 
      case 17: return "the Saiyan"; 
      case 18: return "the Saiyan"; 
      case 19: return "the Saiyan"; 
      case 20: return "the Saiyan"; 
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Cardinal"; 
      case LVL_GOD: return "the Inquisitor"; 
      case LVL_GRGOD: return "the God of good and evil"; 
      case LVL_IMPL: return "the Implementor"; 
      default: return "the saiyan"; 
    }
    break;
    

    case CLASS_THIEF:
    switch (level) {
      case  1: return "the Pilferer"; 
      case  2: return "the Footpad"; 
      case  3: return "the Filcher";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincher";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcher";
      case  9: return "the Sharper";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magsman";
      case 13: return "the Highwayman";
      case 14: return "the Burglar";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Killer";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assasin";
      case LVL_GOD: return "the Demi God of thieves";
      case LVL_GRGOD: return "the God of thieves and tradesmen";
      case LVL_IMPL: return "the Implementor";
      default: return "the Vampire";
    }
    break;

    case CLASS_WARRIOR:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentry";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordsman";
      case  9: return "the Fencer";
      case 10: return "the Combatant";
      case 11: return "the Hero";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckler";
      case 14: return "the Mercenary";
      case 15: return "the Swordmaster";
      case 16: return "the Lieutenant";
      case 17: return "the Champion";
      case 18: return "the Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of war";
      case LVL_IMPL: return "the Implementor";
      default: return "the Warrior";
    }
    break;
    
    case CLASS_PALADIN:
    switch(level) {
      case  1: return "the Paladin Swordpupil";
      case  2: return "the Paladin Recruit";
      case  3: return "the Paladin Sentry";
      case  4: return "the Paladin Fighter";
      case  5: return "the Paladin Soldier";
      case  6: return "the Paladin Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordsman";
      case  9: return "the Fencer";
      case 10: return "the Combatant";
      case 11: return "the Hero";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckler";
      case 14: return "the Mercenary";
      case 15: return "the Swordmaster";
      case 16: return "the Lieutenant";
      case 17: return "the Champion";
      case 18: return "the Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Warlord";
      case LVL_GOD: return "the Extirpator";
      case LVL_GRGOD: return "the God of war";
      case LVL_IMPL: return "the Implementor";
      default: return "the Paladin";
    }
    break;
    
    case CLASS_RANGER:
    switch(level) {
      case LVL_IMPL: return "the Implementor";
      default: return "the Ranger";
    }
    break;
    
    case CLASS_WARLOCK:
    switch(level) {
      case LVL_IMPL: return "the Implementor";
      default: return "the Warlock";
    }
    break;
    
    case CLASS_CYBORG:
    switch(level) {
      case LVL_IMPL: return "the Implementor";
      default: return "the Cyborg";
    }
    break;
    
    case CLASS_NECROMANCER:
    switch(level) {
      case LVL_IMPL: return "the Implementor";
      default: return "the Necromancer";
    }
    break;
    
    case CLASS_DRUID:
    switch(level) {
      case LVL_IMPL: return "the Implementor";
      default: return "the Druid";
    }
    break;
    
    case CLASS_ALCHEMIST:
    switch(level) {
      case LVL_IMPL: return "the Implementor";
      default: return "the Alchemist";
    }
    break;
    
    case CLASS_BARBARIAN:
    switch(level) {
      case LVL_IMPL: return "the Implementor";
      default: return "the Barbarian";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}


/* 
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL)
    return "the Woman";
  if (level == LVL_IMPL)
    return "the Implementress";

  switch (chclass) {

    case CLASS_MAGIC_USER:
    switch (level) {
      case  1: return "the Apprentice of Magic";
      case  2: return "the Spell Student";
      case  3: return "the Scholar of Magic";
      case  4: return "the Delveress in Spells";
      case  5: return "the Medium of Magic";
      case  6: return "the Scribess of Magic";
      case  7: return "the Seeress";
      case  8: return "the Sage";
      case  9: return "the Illusionist";
      case 10: return "the Abjuress";
      case 11: return "the Invoker";
      case 12: return "the Enchantress";
      case 13: return "the Conjuress";
      case 14: return "the Witch";
      case 15: return "the Creator";
      case 16: return "the Savant";
      case 17: return "the Craftess";
      case 18: return "the Wizard";
      case 19: return "the War Witch";
      case 20: return "the Sorceress";
      case 21: return "the Necromancress";
      case 22: return "the Thaumaturgess";
      case 23: return "the Student of the Occult";
      case 24: return "the Disciple of the Uncanny";
      case 25: return "the Minor Elementress";
      case 26: return "the Greater Elementress";
      case 27: return "the Crafter of Magics";
      case 28: return "Shaman";
      case 29: return "the Keeper of Talismans";
      case 30: return "Archwitch";
      case LVL_IMMORT: return "the Immortal Enchantress";
      case LVL_GOD: return "the Empress of Magic";
      case LVL_GRGOD: return "the Goddess of Magic";
      case LVL_IMPL: return "the Implementress";
      default: return "the Witch";
    }
    break;

    case CLASS_CLERIC:
    switch (level) {
      case  1: return "the Believer";
      case  2: return "the Attendant";
      case  3: return "the Acolyte";
      case  4: return "the Novice";
      case  5: return "the Missionary";
      case  6: return "the Adept";
      case  7: return "the Deaconess";
      case  8: return "the Vicaress";
      case  9: return "the Priestess";
      case 10: return "the Lady Minister";
      case 11: return "the Canon";
      case 12: return "the Levitess";
      case 13: return "the Curess";
      case 14: return "the Nunne";
      case 15: return "the Healess";
      case 16: return "the Chaplain";
      case 17: return "the Expositress";
      case 18: return "the Bishop";
      case 19: return "the Arch Lady of the Church";
      case 20: return "the Matriarch";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Priestess";
      case LVL_GOD: return "the Inquisitress";
      case LVL_GRGOD: return "the Goddess of good and evil";
      case LVL_IMPL: return "the Implementress";
      default: return "the Cleric";
    }
    break;

    case CLASS_THIEF:
    switch (level) {
      case  1: return "the Pilferess";
      case  2: return "the Footpad";
      case  3: return "the Filcheress";
      case  4: return "the Pick-Pocket";
      case  5: return "the Sneak";
      case  6: return "the Pincheress";
      case  7: return "the Cut-Purse";
      case  8: return "the Snatcheress";
      case  9: return "the Sharpress";
      case 10: return "the Rogue";
      case 11: return "the Robber";
      case 12: return "the Magswoman";
      case 13: return "the Highwaywoman";
      case 14: return "the Burglaress";
      case 15: return "the Thief";
      case 16: return "the Knifer";
      case 17: return "the Quick-Blade";
      case 18: return "the Murderess";
      case 19: return "the Brigand";
      case 20: return "the Cut-Throat";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Assasin";
      case LVL_GOD: return "the Demi Goddess of thieves";
      case LVL_GRGOD: return "the Goddess of thieves and tradesmen";
      case LVL_IMPL: return "the Implementress";
      default: return "the Thief";
    }
    break;

    case CLASS_WARRIOR:
    switch(level) {
      case  1: return "the Swordpupil";
      case  2: return "the Recruit";
      case  3: return "the Sentress";
      case  4: return "the Fighter";
      case  5: return "the Soldier";
      case  6: return "the Warrior";
      case  7: return "the Veteran";
      case  8: return "the Swordswoman";
      case  9: return "the Fenceress";
      case 10: return "the Combatess";
      case 11: return "the Heroine";
      case 12: return "the Myrmidon";
      case 13: return "the Swashbuckleress";
      case 14: return "the Mercenaress";
      case 15: return "the Swordmistress";
      case 16: return "the Lieutenant";
      case 17: return "the Lady Champion";
      case 18: return "the Lady Dragoon";
      case 19: return "the Cavalier";
      case 20: return "the Lady Knight";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Lady of War";
      case LVL_GOD: return "the Queen of Destruction";
      case LVL_GRGOD: return "the Goddess of war";
      case LVL_IMPL: return "the Implementress";
      default: return "the Warrior";
    }
    break;
    
    case CLASS_NECROMANCER:
    switch(level) {
      case  1: return "the Skeleton";
      case  2: return "the Bat";
      case  3: return "the Zombie";
      case  4: return "the Mutant Zombie";
      case  5: return "the Mummy";
      case  6: return "the Royal Mummy";
      case  7: return "the Unholy Believer";
      case  8: return "the Unholy Prayer";
      case  9: return "the Unholy Cultist";
      case 10: return "the Unholy Fanatic";
      case 11: return "the Unholy Priest";
      case 12: return "the Unholy Bishop";
      case 13: return "the Vampire";
      case 14: return "the Vampire Lord";
      case 15: return "the Lich";
      case 16: return "the Power Lich";
      case 17: return "the Bone Dragon";
      /* no one ever thought up these titles 21-30 */
      case LVL_IMMORT: return "the Immortal Necromancer";
      case LVL_GOD: return "the Necromancer God";
      case LVL_IMPL: return "the Implementress";
      default: return "the Necromancer";
    }
    break;
    
    case CLASS_PALADIN:
    switch(level) {
      case LVL_IMPL: return "the Implementress";
      default: return "the Paladin Lady";
    }
    break;
    
    case CLASS_RANGER:
    switch(level) {
      case LVL_IMPL: return "the Implementress";
      default: return "the Ranger Woman";
    }
    break;
    
    case CLASS_WARLOCK:
    switch(level) {
      case LVL_IMPL: return "the Implementress";
      default: return "the Warlock Lady";
    }
    break;
    
    case CLASS_CYBORG:
    switch(level) {
      case LVL_IMPL: return "the Implementress";
      default: return "the Female Cyborg";
    }
    break;
    
    case CLASS_DRUID:
    switch(level) {
      case LVL_IMPL: return "the Implementress";
      default: return "the Druid";
    }
    break;
    
    case CLASS_ALCHEMIST:
    switch(level) {
      case LVL_IMPL: return "the Implementress";
      default: return "the Alchemist";
    }
    break;
    
    case CLASS_BARBARIAN:
    switch(level) {
      case LVL_IMPL: return "the Implementress";
      default: return "the Barbarian";
    }
    break;
  }

  /* Default title for classes which do not have titles defined */
  return "the Classless";
}

