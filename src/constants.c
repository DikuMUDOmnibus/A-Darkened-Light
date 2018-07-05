/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"

const char circlemud_version[] = {
"CircleMUD, version 3.00 beta patchlevel 14 - Modified by Mayhem\r\n\0"};


/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */


/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "\n"
};


/* ROOM_x */
const char *room_bits[] = {
  "DARK",		/* 0 */
  "DEATH",
  "!MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "!TRACK",
  "!MAGIC",
  "TUNNEL",
  "PRIVATE",		
  "GODROOM",		/* 10 */
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "GOOD_REGEN",
  "TELEPORT",
  "OWNER_ONLY",
  "BAD_REGEN",		
  "*DELETED*",		/* 20 */
  "!SUMMON",
  "WASTED",
  "\n"
};

const char *room_affections[] = {
  "FOG",
  "\n"
};

/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "\n"
};


/* SECT_ */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "Underwater",
  "In Flight",
  "Swamp",
  "Lava",
  "Road",
  "\n"
};


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] =
{
  "Neutral",
  "Male",
  "Female"
};


/* POS_x */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};


/* PLR_x */
const char *player_bits[] = {
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "!WIZL",
  "!DEL",
  "INVST",
  "CRYO",
  "MEDITATE",
  "NEWBIE",
  "TRUST",
  "\n"
};


/* MOB_x */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "!CHARM",
  "!SUMMN",
  "!SLEEP",
  "!BASH",
  "!BLIND",
  "MOUNTABLE",
  "GHOST",
  "UNDEAD",
  "AGGR_CLAN",
  "PET",
  "ETHEREAL",
  "FASTREGEN",
  "FASTAGGR",
  "!BURN",
  "+BURN",
  "!FREEZE",
  "+FREEZE",
  "!ACID",
  "+ACID", 
  "CANBURN",   /* 32 */
  "CANFREEZE",
  "CANACID",
  "GAZE_PETR",
  "CANTALK",
  "CANT_FLEE",
  "HUNT",
  "*DELETED*",
  "\n"
};


/* PRF_x */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "!TELL",
  "EDITBW",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "!HASS",
  "QUEST",
  "SUMN",
  "!REP",
  "LIGHT",
  "C1",
  "C2",
  "!WIZ",
  "L1",
  "L2",
  "!AUC",
  "!GOS",
  "!GTZ",
  "RMFLG",
  "ALOOT",
  "AGOLD",
  "AASSIST",
  "CLANTALK",
  "EXAMUNIT",
  "SHOWCLAN",
  "ASPLIT",
  "AMAIL",
  "AFK",
  "LOCK",
  "\n"
};


/* AFF_x */
const char *affected_bits[] =
{
  "BLIND",		/* 0 */
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",		/* 10 */
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "!TRACK",
  "TAMED",
  "UNUSED",
  "SNEAK",
  "HIDE",
  "UNUSED",		/* 20 */
  "CHARM",
  "DET-TRAP",
  "HOLD",
  "HASTE",
  "SHIELD",
  "DEATHDANCE",
  "MIRROR",
  "STONESKIN",
  "BLINK",
  "FLY",		/* 30 */
  "WATERBR",
  "PROT_FIRE", 		/* 0 */
  "+1",
  "+2",
  "+3",
  "+4",
  "+5",
  "SILVER",
  "FARSEE",
  "CRIT_HIT",		
  "BURNING",
  "FREEZING",		/* 10 */
  "ACIDED",
  "PROT_COLD",
  "PASSDOOR",
  "ENH_HEAL",
  "ENH_MANA",
  "ENH_MOVE",
  "UNUSED",
  "ANTIMAGIC",
  "BLOODL",
  "BLOCK",
  "FSHIELD",
  "MIRROR-IMAGE",
  "\n"
};


/* CON_x */
const char *connected_types[] = {
  "Playing",		/* 0 */
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",		/* 10 */
  "Get descript.",	
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",	/* 20 */
  "Shop edit",
  "Setting Color",
  "Rolling status",
  "Editing Clans",
  "Choosing Hometown",
  "Confirm Class",
  "Text Editor",
  "Guild Edit",
  "Statistics",
  "Confirm Class",	/* 30 */
  "Disconnecting",
  "\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *where[] = {
  "<used as light>      ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn as shield>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<held>               ",
  "<worn on face>       "
};


/* WEAR_x - for stat */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Worn on face",
  "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "SOULS",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "THROW",
  "GRENADE",
  "BOW",
  "SLING",
  "CROSSBOW",
  "BOLT",
  "ARROW",
  "ROCK",
  "FLIGHT",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "FACE",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
  "GLOW",			/* 0 */
  "HUM",
  "!RENT",
  "!DONATE",
  "!INVIS",
  "INVISIBLE",
  "MAGIC",
  "!DROP",
  "BLESS",
  "!GOOD",
  "!EVIL",			/* 10 */
  "!NEUTRAL",
  "!MAGE",
  "!CLERIC",
  "!THIEF",
  "!WARRIOR",
  "!SELL",
  "!LIVE_GRENADE",
  "!MORTAL",
  "BURIED",
  "!PALADIN",			/* 20 */
  "!RANGER",
  "!WARLOCK",
  "!CYBORG",
  "!NECROMANCER",
  "!DRUID",
  "!ALCHEMIST",
  "!BARBARIAN",
  "!DISARM",
  "BOOMERANG",
  "ENGRAVED",			/* 30 */
  "AUTOENGRAVE",
  "*DELETED*",
  "!SINK",
  "\n"
};


/* APPLY_x */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "\n"
};


/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
const char *drinks[] =
{
  "blood",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "blood",
  "blood",
  "blood",
  "\n"
};


/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "blood",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "blood",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13},
  {0, 1, 0}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "red",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "red",
  "red",
  "red",
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
cpp_extern const struct str_app_type str_app[] = {
  {-5, -4, 0, 0},	/* str = 0 */
  {-5, -4, 3, 1},	/* str = 1 */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},	/* str = 5 */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},	/* str = 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 170, 14},
  {0, 0, 170, 15},	/* str = 15 */
  {0, 1, 195, 16},
  {1, 1, 220, 18},
  {1, 2, 255, 20},	/* str = 18 */
  {3, 7, 640, 40},
  {3, 8, 700, 40},	/* str = 20 */
  {4, 9, 810, 40},
  {4, 10, 970, 40},
  {5, 11, 1130, 40},
  {6, 12, 1440, 40},
  {7, 14, 1750, 40},	/* str = 25 */
  {1, 3, 280, 22},	/* str = 18/0 - 18-50 */
  {2, 3, 305, 24},	/* str = 18/51 - 18-75 */
  {2, 4, 330, 26},	/* str = 18/76 - 18-90 */
  {2, 5, 380, 28},	/* str = 18/91 - 18-99 */
  {3, 6, 480, 30}	/* str = 18/100 */
};



/* [dex] skill apply (thieves only) */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
  {-99, -99, -90, -99, -60},	/* dex = 0 */
  {-90, -90, -60, -90, -50},	/* dex = 1 */
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},	/* dex = 5 */
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},	/* dex = 10 */
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},		/* dex = 15 */
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},		/* dex = 18 */
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},		/* dex = 20 */
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}		/* dex = 25 */
};



/* [dex] apply (all) */
cpp_extern const struct dex_app_type dex_app[] = {
  {-7, -7, 6},		/* dex = 0 */
  {-6, -6, 5},		/* dex = 1 */
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},		/* dex = 5 */
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},		/* dex = 15 */
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},		/* dex = 18 */
  {3, 3, -4},
  {3, 3, -4},		/* dex = 20 */
  {4, 4, -5},
  {4, 4, -5},
  {4, 4, -5},
  {5, 5, -6},
  {5, 5, -6}		/* dex = 25 */
};



/* [con] apply (all) */
cpp_extern const struct con_app_type con_app[] = {
  {-4, 20},		/* con = 0 */
  {-3, 25},		/* con = 1 */
  {-2, 30},
  {-2, 35},
  {-1, 40},
  {-1, 45},		/* con = 5 */
  {-1, 50},
  {0, 55},
  {0, 60},
  {0, 65},
  {0, 70},		/* con = 10 */
  {0, 75},
  {0, 80},
  {0, 85},
  {0, 88},
  {1, 90},		/* con = 15 */
  {2, 95},
  {2, 97},
  {3, 99},		/* con = 18 */
  {3, 99},
  {4, 99},		/* con = 20 */
  {5, 99},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99}		/* con = 25 */
};



/* [int] apply (all) */
cpp_extern const struct int_app_type int_app[] = {
  {3},		/* int = 0 */
  {5},		/* int = 1 */
  {7},
  {8},
  {9},
  {10},		/* int = 5 */
  {11},
  {12},
  {13},
  {15},
  {17},		/* int = 10 */
  {19},
  {22},
  {25},
  {30},
  {35},		/* int = 15 */
  {40},
  {45},
  {50},		/* int = 18 */
  {53},
  {55},		/* int = 20 */
  {56},
  {57},
  {58},
  {59},
  {60}		/* int = 25 */
};


/* [wis] apply (all) */
cpp_extern const struct wis_app_type wis_app[] = {
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {0},  /* wis = 10 */
  {0},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {3},
  {4},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {6},
  {6},
  {7},
  {7},
  {7},
  {7},
  {6}  /* wis = 25 */
};



const char *spell_wear_off_msg[] = {
  "RESERVED DB.C",		/* 0 */
  "You feel less protected.",	/* 1 */
  "!Teleport!",
  "You feel less righteous.",
  "You feel a cloak of blindness disolve.",
  "!Burning Hands!",		/* 5 */
  "!Call Lightning",
  "You feel more self-confident.",
  "You feel your strength return.",
  "!Clone!",
  "!Color Spray!",		/* 10 */
  "!Control Weather!",
  "!Create Food!",
  "!Create Water!",
  "!Cure Blind!",
  "!Cure Critic!",		/* 15 */
  "!Cure Light!",
  "You feel more optimistic.",
  "You feel less aware.",
  "Your eyes stop tingling.",
  "The detect magic wears off.",/* 20 */
  "The detect poison wears off.",
  "!Dispel Evil!",
  "!Earthquake!",
  "!Enchant Weapon!",
  "!Energy Drain!",				/* 25 */
  "!Fireball!",
  "!Harm!",
  "!Heal!",
  "You feel yourself exposed.",
  "!Lightning Bolt!",				/* 30 */
  "!Locate object!",
  "!Magic Missile!",
  "You feel less sick.",
  "You feel less protected.",
  "!Remove Curse!",				/* 35 */
  "The white aura around your body fades.",
  "!Shocking Grasp!",
  "You feel less tired.",
  "You feel weaker.",
  "!Summon!",					/* 40 */
  "!Ventriloquate!",
  "!Word of Recall!",
  "!Remove Poison!",
  "You feel less aware of your suroundings.",
  "!Animate Dead!",				/* 45 */
  "!Dispel Good!",
  "!Group Armor!",
  "!Group Heal!",
  "!Group Recall!",
  "Your night vision seems to fade.",		/* 50 */
  "Your feet seem less boyant.",
  "!Disintegrate!",
  "!Portal!",
  "!Fear!",
  "Minor Identify!",				/* 55 */
  "!Phantasm!",
  "!Recharge!",
  "!Meteor Shower!",
  "You feel more vulnerable.",
  "You feel more vulnerable.",          	/* 60 */
  "!Chain Lightning!",
  "You can move better!",
  "You aren't paralyzed anymore.",
  "!Holy Word!",
  "!Holy Shout!",				/* 65 */
  "Your haste expires!",
  "You feel your life force return to you!",
  "!Death Ripple!",
  "!Death Wave!",
  "You feel your force shield dwindle off!",	/* 70 */
  "!Group Shield!",
  "!Relocate!",
  "!Peace!",
  "You find a soft landing spot.",		
  "You float gently to the ground.",		/* 75 */
  "You feel less insulated.",
  "Your gills disappear and you return to normal.",
  "!Group Fly!",
  "!Group Invisibility!",
  "!Group Protect from Evil!",			/* 80 */
  "!Acid Arrow!",
  "!Flame Arrow!",
  "!Cure Serious!",
  "!Group Waterbreath!",
  "You feel yourself becoming solid again.",	/* 85 */
  "You feel your strength returning.",
  "You feel warm again.",
  "You feel your prayers run out.",
  "You feel your skin become soft again.",
  "You feel your stomach rumble.",		/* 90 */
  "!Feast All!",
  "!BladeBarrier!",
  "!Knock!",
  "!Planeshift!",
  "!Dispel Magic!",				/* 95 */
  "You feel your shell of warm dissipate.",
  "You feel more in tune with your surroundings.",
  "Your images recombine into one.",
  "You feel yourself more sensitive to magic.",
  "You feel yourself solid again.",		/* 100 */
  "!Earth Elemental!",
  "!Water Elemental!",
  "!Air Elemental!",
  "!Fire Elemental!",
  "You don't feel bloodlust anymore.",
  "The fog seems to clear out.",
  "The fire shield slowly fades out.",
  "!UNUSED!"
};



const char *npc_class_types[] = {
  "Normal",
  "Undead",
  "\n"
};



int rev_dir[] =
{
  2,
  3,
  0,
  1,
  5,
  4
};


int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  5,    /* Underwater */
  1,	/* Flying     */
  5,    /* Swamp      */
  5,    /* Lava       */
  1     /* Road       */
  
};

/* Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the day of the Great Gods",
  "the Day of the Sun"
};

/* Not used in sprinttype(). */
const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};

const HOMETOWN hometowns[] = {
  {"None", 3001, HT_NOHOMETOWN},
  {"Heaven", 1204, HT_IMM_ONLY},
  {"Midgaard", 3001, 0},
  {"New Thalos", 5509, 0},
  {"New Sparta", 21100, 0},
  {"Sundhaven", 6601, 0}
};

char *teleport_bits[] = {
	  "ForceLook",
	  "EntryResetsTime",
	  "SetsRandomTime",
	  "Spin(NOT AVAIL)",
	  "HasOBJinInv",
	  "NoOBJinInv",
	  "NoMesgDisplay",
	  "SkipMobs",
	  "SkipGroundObjects",
	  "\n"
	};
	
/* ZON_? bitvector values */
const char *zon_bits[] =
{
  "MAZE",
  "!OWNER",
  "FIXED_OWNER",
  "!LIST",
  "!PK",
  "PRIVATE",
  "PUBLIC",
  "\n"
};

/* CF_x - clan flags*/
const char *clan_flags[] = {
  "GODCLAN",
  "\n"
};
