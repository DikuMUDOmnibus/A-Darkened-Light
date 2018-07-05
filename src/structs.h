/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

/* preamble *************************************************************/

#define NOWHERE    -1    /* nil reference for room-database	*/
#define NOTHING	   -1    /* nil reference for objects		*/
#define NOBODY	   -1    /* nil reference for mobiles		*/

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)

/* quests ***************************************************************/
#define	MAX_QPOINTS	100
#define MAX_QPOINT_ADD	5

/* MAX LEGEND LEVELS ****/
#define MAX_LEGEND_LEVELS 6
#define MAX_PSTATUS_LEVELS 6

/* misc editor defines **************************************************/


/* format modes for format_text */
#define FORMAT_INDENT		(1 << 0)

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x03000D /* Major/Minor/Patchlevel - MMmmPP */

/* room-related defines *************************************************/

/*Parts */
#define OBJ_VNUM_SEVERED_HEAD     29910
#define OBJ_VNUM_TORN_HEART       29911
#define OBJ_VNUM_SLICED_LARM      29912
#define OBJ_VNUM_SLICED_RARM      29913
#define OBJ_VNUM_SLICED_LLEG      29914
#define OBJ_VNUM_SLICED_RLEG      29915
#define OBJ_VNUM_SOULBLADE        30000
#define OBJ_VNUM_PORTAL           30001
#define OBJ_VNUM_EGG              30002
#define OBJ_VNUM_EMPTY_EGG        30003
#define OBJ_VNUM_SPILLED_ENTRAILS 30004
#define OBJ_VNUM_QUIVERING_BRAIN  30005
#define OBJ_VNUM_SQUIDGY_EYEBALL  30006
#define OBJ_VNUM_SPILT_BLOOD      30007
#define OBJ_VNUM_VOODOO_DOLL      30010
#define OBJ_VNUM_RIPPED_FACE      30012
#define OBJ_VNUM_TORN_WINDPIPE    30013
#define OBJ_VNUM_CRACKED_HEAD     30014
#define OBJ_VNUM_SLICED_EAR       30025
#define OBJ_VNUM_SLICED_NOSE      30026
#define OBJ_VNUM_KNOCKED_TOOTH    30027
#define OBJ_VNUM_TORN_TONGUE      30028
#define OBJ_VNUM_SEVERED_HAND     30029
#define OBJ_VNUM_SEVERED_FOOT     30030
#define OBJ_VNUM_SEVERED_THUMB    30031
#define OBJ_VNUM_SEVERED_INDEX    30032
#define OBJ_VNUM_SEVERED_MIDDLE   30033
#define OBJ_VNUM_SEVERED_RING     30034
#define OBJ_VNUM_SEVERED_LITTLE   30035
#define OBJ_VNUM_SEVERED_TOE      30036

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5

/* Zone flags */
#define ZON_MAZE	(1 << 0)
#define ZON_NO_OWNER	(1 << 1)
#define ZON_FIXED_OWNER	(1 << 2)
#define ZON_NO_LIST	(1 << 3)
#define ZON_NO_PK	(1 << 4)
#define ZON_PRIVATE	(1 << 5)
#define ZON_PUBLIC	(1 << 6)

#define NUM_ZON_FLAGS		7



/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		(1 << 0)   /* Dark			*/
#define ROOM_DEATH		(1 << 1)   /* Death trap		*/
#define ROOM_NOMOB		(1 << 2)   /* MOBs not allowed		*/
#define ROOM_INDOORS		(1 << 3)   /* Indoors			*/
#define ROOM_PEACEFUL		(1 << 4)   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF		(1 << 5)   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK		(1 << 6)   /* Track won't go through	*/
#define ROOM_NOMAGIC		(1 << 7)   /* Magic not allowed		*/
#define ROOM_TUNNEL		(1 << 8)   /* room for only 1 pers	*/
#define ROOM_PRIVATE		(1 << 9)   /* Can't teleport in		*/
#define ROOM_GODROOM		(1 << 10)  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		(1 << 11)  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH	(1 << 12)  /* (R) House needs saving	*/
#define ROOM_ATRIUM		(1 << 13)  /* (R) The door to a house	*/
#define ROOM_OLC		(1 << 14)  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK		(1 << 15)  /* (R) breath-first srch mrk	*/
#define ROOM_GOOD_REGEN		(1 << 16)  /* Good Regen Room 		*/
#define ROOM_TELEPORT		(1 << 17)  /* Teleportation room 	*/
#define ROOM_OWNER_ONLY		(1 << 18)  /* Only owning clan may enter*/
#define ROOM_BAD_REGEN		(1 << 19)  /* Bad Regen Room		*/
#define ROOM_DELETED		(1 << 20)  /* Room was deleted		*/
#define ROOM_NO_SUMMON		(1 << 21)  /* No summon/tele/portal room*/
#define ROOM_WASTED		(1 << 22)  /* Wasted room 		*/
#define ROOM_FOG                (1 << 23)

#define NUM_ROOM_FLAGS 		24

/* Room affections */
#define RAFF_FOG        (1 << 0)

#define NUM_ROOM_AFF	1

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		(1 << 3)   /* Lock can't be picked	*/


/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_UNDERWATER	     8		   /* Underwater		*/
#define SECT_FLYING	     9		   /* Wheee!			*/
#define SECT_SWAMP           10            /* Swamp     		*/
#define SECT_LAVA	     11		   /* Lava 			*/
#define SECT_ROAD	     12		   /* Road			*/

#define NUM_ROOM_SECTORS	13

/* char and mob-related defines *****************************************/


/* PC classes */
#define CLASS_UNDEFINED	  -1
#define CLASS_MAGIC_USER  0
#define CLASS_CLERIC      1
#define CLASS_THIEF       2
#define CLASS_WARRIOR     3
#define CLASS_PALADIN     4
#define CLASS_RANGER      5
#define CLASS_WARLOCK     6
#define CLASS_CYBORG      7
#define CLASS_NECROMANCER 8
#define CLASS_DRUID       9
#define CLASS_ALCHEMIST   10
#define CLASS_BARBARIAN   11

#define NUM_CLASSES	  12  /* This must be the number of classes!! */

/* NPC classes (currently unused - feel free to implement!) */
#define CLASS_OTHER       0
#define CLASS_UNDEAD      1
#define CLASS_HUMANOID    2
#define CLASS_ANIMAL      3
#define CLASS_DRAGON      4
#define CLASS_GIANT       5
#define CLASS_GHOST       6


/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

#define NUM_GENDERS		3


/* Spirit world info */
#define SPIRIT_MIN  14100  
#define SPIRIT_MAX  14150


/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/
#define POS_EXPAND     9        /* expanded             */
#define POS_CLAWS     10

#define NUM_POSITIONS		11

#define HIT_INCAP	-3
#define HIT_MORTALLYW	-6
#define HIT_DEAD	-11

/* NOTE: Arrangement of PLR, PRF, MOB, AFF... flags is critical */
/* They must fill whole type used for storage!!!!! (YIL) */


/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER	(1 << 0)   /* Player is a player-killer		*/
#define PLR_THIEF	(1 << 1)   /* Player is a player-thief		*/
#define PLR_FROZEN	(1 << 2)   /* Player is frozen			*/
#define PLR_DONTSET     (1 << 3)   /* Don't EVER set (ISNPC bit)	*/
#define PLR_WRITING	(1 << 4)   /* Player writing (board/mail/olc)	*/
#define PLR_MAILING	(1 << 5)   /* Player is writing mail		*/
#define PLR_CRASH	(1 << 6)   /* Player needs to be crash-saved	*/
#define PLR_SITEOK	(1 << 7)   /* Player has been site-cleared	*/
#define PLR_NOSHOUT	(1 << 8)   /* Player not allowed to shout/goss	*/
#define PLR_NOTITLE	(1 << 9)   /* Player not allowed to set title	*/
#define PLR_DELETED	(1 << 10)  /* Player deleted - space reusable	*/
#define PLR_LOADROOM	(1 << 11)  /* Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST	(1 << 12)  /* Player shouldn't be on wizlist	*/
#define PLR_NODELETE	(1 << 13)  /* Player shouldn't be deleted	*/
#define PLR_INVSTART	(1 << 14)  /* Player should enter game wizinvis	*/
#define PLR_CRYO	(1 << 15)  /* Player is cryo-saved (purge prog)	*/
#define PLR_MEDITATE	(1 << 16)  /* Player is meditating		*/
#define PLR_NEWBIE	(1 << 17)  /* Player is newbie after being killed */
#define PLR_TRUST	(1 << 18)  /* Trusted builder 			*/
#define PLR_EXPAND      (1 << 19)  /* EXPANDED PLAYERS       */
#define PLR_DEAD        (1 << 20) /* PLAYER DEATH AND DYING */
#define PLR_CLAWS       (1 << 21)
#define PLR_FANGS       (1 << 22)
#define PLR_DRAGONFORM  (1 << 23) /* player is dragonformed */
#define PLR_RAGE        (1 << 24) /* player is mortal */ 
#define PLR_VAMPED      (1 << 25) /* player is now vamped */
#define PLR_POWERUP     (1 << 26) /* saiyan is powered up */
#define PLR_POWERUP2    (1 << 27) /* saiyan is now super saiyan */
#define PLR_POWERUP3    (1 << 28) /* saiyan is now accendent saiyan */
#define PLR_POWERUP4    (1 << 29) /* saiyan is now ssj3 */
#define PLR_POWERUP5    (1 << 30) /* saiyan is now ssj4 */
#define PLR_POWERUP6    (1 << 31) /* saiyan is now ssj5 */

#define PLR2_NOFACE       (1 << 0)
#define PLR2_NOWPIPE      (1 << 1)
#define PLR2_NOLARM       (1 << 2)



/* Mobile flags: used by char_data.char_specials.act[0] */
#define MOB_SPEC         (1 << 0)  /* Mob has a callable spec-proc	*/
#define MOB_SENTINEL     (1 << 1)  /* Mob should not move		*/
#define MOB_SCAVENGER    (1 << 2)  /* Mob picks up stuff on the ground	*/
#define MOB_ISNPC        (1 << 3)  /* (R) Automatically set on all Mobs	*/
#define MOB_AWARE	 (1 << 4)  /* Mob can't be backstabbed		*/
#define MOB_AGGRESSIVE   (1 << 5)  /* Mob hits players in the room	*/
#define MOB_STAY_ZONE    (1 << 6)  /* Mob shouldn't wander out of zone	*/
#define MOB_WIMPY        (1 << 7)  /* Mob flees if severely injured	*/
#define MOB_AGGR_EVIL	 (1 << 8)  /* auto attack evil PC's		*/
#define MOB_AGGR_GOOD	 (1 << 9)  /* auto attack good PC's		*/
#define MOB_AGGR_NEUTRAL (1 << 10) /* auto attack neutral PC's		*/
#define MOB_MEMORY	 (1 << 11) /* remember attackers if attacked	*/
#define MOB_HELPER	 (1 << 12) /* attack PCs fighting other NPCs	*/
#define MOB_NOCHARM	 (1 << 13) /* Mob can't be charmed		*/
#define MOB_NOSUMMON	 (1 << 14) /* Mob can't be summoned		*/
#define MOB_NOSLEEP	 (1 << 15) /* Mob can't be slept		*/
#define MOB_NOBASH	 (1 << 16) /* Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND	 (1 << 17) /* Mob can't be blinded		*/
#define MOB_MOUNTABLE	 (1 << 18) /* Is the mob mountable? (DAK)       */
#define MOB_GHOST        (1 << 19) /* Mob is a ghost (no corpse)        */
#define MOB_UNDEAD	 (1 << 20) /* Mob is undead, holy spells apply  */
#define MOB_AGGR_CLAN	 (1 << 21) /* Auto attack enemy clans		*/
#define MOB_PET		 (1 << 22) /* Mob is a pet		        */
#define MOB_ETHEREAL	 (1 << 23) /* Mob is invisible			*/
#define MOB_FASTREGEN    (1 << 24) /* Mob regens hp faster than normal  */
#define MOB_FAST_AGGR	 (1 << 25) /* Fast aggresivity			*/
#define MOB_NOBURN       (1 << 26) /* Mob can't be burned               */
#define MOB_MOREBURN     (1 << 27) /* Mob more susceptable to burning   */
#define MOB_NOFREEZE     (1 << 28) /* Mob can't be frozen		*/
#define MOB_MOREFREEZE   (1 << 29) /* Mob more susceptable to freezing  */
#define MOB_NOACID       (1 << 30) /* Mob can't be acided 		*/
#define MOB_MOREACID     (1 << 31) /* Mob more susceptable to acid      */

/* Mobile2 flags: used by char_data.char_specials.act[1] */
#define MOB2_CANBURN      (1 << 1) /* Mob's touch causes burning        */
#define MOB2_CANFREEZE    (1 << 2) /* Mob's touch causes freezing       */
#define MOB2_CANACID      (1 << 3) /* Mob's touch causes acidburning    */
#define MOB2_GAZEPETRIFY  (1 << 4) /* Mob's gaze can petrify		*/
#define MOB2_CANTALK	  (1 << 5) /* Mob can speak			*/
#define MOB2_CANT_FLEE    (1 << 6) /* Prevents attackers from fleeing   */
#define MOB2_HUNT	  (1 << 7) /* Mob will hunt fleeed players      */
#define MOB2_DELETED	  (1 << 8) /* Mob record was deleted            */
#define MOB2_NOLARM       (1 << 9)
#define MOB2_NOFACE       (1 << 10)

#define NUM_MOB_FLAGS		43

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF       (1 << 0)  /* Room descs won't normally be shown	*/
#define PRF_COMPACT     (1 << 1)  /* No extra CRLF pair before prompts	*/
#define PRF_DEAF	(1 << 2)  /* Can't hear shouts			*/
#define PRF_NOTELL	(1 << 3)  /* Can't receive tells		*/
#define PRF_EDIT_BW	(1 << 4)  /* Show color codes in editor		*/
#define PRF_FREE1	(1 << 5)  /* free				*/
#define PRF_FREE2	(1 << 6)  /* free				*/
#define PRF_AUTOEXIT	(1 << 7)  /* Display exits in a room		*/
#define PRF_NOHASSLE	(1 << 8)  /* Aggr mobs won't attack		*/
#define PRF_QUEST	(1 << 9)  /* On quest				*/
#define PRF_SUMMONABLE	(1 << 10) /* Can be summoned			*/
#define PRF_NOREPEAT	(1 << 11) /* No repetition of comm commands	*/
#define PRF_HOLYLIGHT	(1 << 12) /* Can see in dark			*/
#define PRF_COLOR_1	(1 << 13) /* Color (low bit)			*/
#define PRF_COLOR_2	(1 << 14) /* Color (high bit)			*/
#define PRF_NOWIZ	(1 << 15) /* Can't hear wizline			*/
#define PRF_LOG1	(1 << 16) /* On-line System Log (low bit)	*/
#define PRF_LOG2	(1 << 17) /* On-line System Log (high bit)	*/
#define PRF_NOAUCT	(1 << 18) /* Can't hear auction channel		*/
#define PRF_NOGOSS	(1 << 19) /* Can't hear gossip channel		*/
#define PRF_NOGRATZ	(1 << 20) /* Can't hear grats channel		*/
#define PRF_ROOMFLAGS	(1 << 21) /* Can see room flags (ROOM_x)	*/
#define PRF_AUTOLOOT    (1 << 22) /* Autolooting corpses                */
#define PRF_AUTOGOLD    (1 << 23) /* Automatic gold looting             */
#define PRF_AUTOASSIST  (1 << 24) /* Automatic assist             	*/
#define PRF_CLANTALK    (1 << 25) /* Can hear clan talk                 */
#define PRF_EXAMUNIT    (1 << 26) /* whether is exam unit on            */
#define PRF_SHOWCLAN	(1 << 27) /* Whether clans will be displayed	*/
#define PRF_AUTOSPLIT	(1 << 28) /* Automatic gold splitting		*/
#define PRF_AUTOMAIL	(1 << 29) /* Auto mail receiving		*/
#define PRF_AFK		(1 << 30) /* Away from keyboard			*/
#define PRF_LOCKED	(1 << 31) /* Console is locked and waiting for password */


/* Affect bits: used in char_data.char_specials.saved.affected_by[0] */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND             (1 << 0)	   /* (R) Char is blind		*/
#define AFF_INVISIBLE         (1 << 1)	   /* Char is invisible		*/
#define AFF_DETECT_ALIGN      (1 << 2)	   /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      (1 << 3)	   /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      (1 << 4)	   /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        (1 << 5)	   /* Char can sense hidden life*/
#define AFF_WATERWALK	      (1 << 6)	   /* Char can walk on water	*/
#define AFF_SANCTUARY         (1 << 7)	   /* Char protected by sanct.	*/
#define AFF_GROUP             (1 << 8)	   /* (R) Char is grouped	*/
#define AFF_CURSE             (1 << 9)	   /* Char is cursed		*/
#define AFF_INFRAVISION       (1 << 10)	   /* Char can see in dark	*/
#define AFF_POISON            (1 << 11)	   /* (R) Char is poisoned	*/
#define AFF_PROTECT_EVIL      (1 << 12)	   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      (1 << 13)	   /* Char protected from good  */
#define AFF_SLEEP             (1 << 14)	   /* (R) Char magically asleep	*/
#define AFF_NOTRACK	      (1 << 15)	   /* Char can't be tracked	*/
#define AFF_TAMED   	      (1 << 16)	   /* Char has been tamed (DAK) */
#define AFF_UNUSED17	      (1 << 17)	   /* Room for future expansion	*/
#define AFF_SNEAK             (1 << 18)	   /* Char can move quietly	*/
#define AFF_HIDE              (1 << 19)	   /* Char is hidden		*/
#define AFF_UNUSED20	      (1 << 20)	   /* Room for future expansion	*/
#define AFF_CHARM             (1 << 21)	   /* Char is charmed		*/
#define AFF_UNUSED22	      (1 << 22)    /*                           */
#define AFF_HOLD              (1 << 23)    /* Char cannot flee or move  */
#define AFF_HASTE	      (1 << 24)    /* Char moves faster		*/
#define AFF_SHIELD	      (1 << 25)	   /* Halves damage from missiles*/
#define AFF_DEATHDANCE	      (1 << 26)
#define AFF_MIRRORIMAGE       (1 << 27)
#define AFF_STONESKIN	      (1 << 28)
#define AFF_BLINK             (1 << 29)
#define AFF_FLYING            (1 << 30)    /* Char is Flying            */
#define AFF_WATERBREATH       (1 << 31)    /* Char can breath water     */
#define AFF_MIRROR_IMAGE      (1 << 32) 


/* Affect2 bits: used in char_data.char_specials.saved.affected_by[1] */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF2_PROT_FIRE	      (1 << 0)    /* Char protected from fire  */
#define AFF2_PLUSONE          (1 << 1)    /* Char needs +1 to damage   */
#define AFF2_PLUSTWO          (1 << 2)    /* Char needs +2 to damage   */
#define AFF2_PLUSTHREE        (1 << 3)    /* Char needs +3 to damage   */
#define AFF2_PLUSFOUR         (1 << 4)    /* Char needs +4 to damage   */
#define AFF2_PLUSFIVE         (1 << 5)    /* Char needs +5 to damage   */
#define AFF2_SILVER           (1 << 6)    /* Char needs silver to dam  */
#define AFF2_FARSEE           (1 << 7)    /* Char affected by Farsee   */
#define AFF2_CRIT_HIT         (1 << 8)    /* Char was crit hit         */
#define AFF2_BURNING          (1 << 9)    /* Char is on fire           */
#define AFF2_FREEZING         (1 << 10)   /* Char is freezing          */
#define AFF2_ACIDED           (1 << 11)   /* Char is covered with acid */
#define AFF2_PROT_COLD	      (1 << 12)   /* Char is prot. from cold   */
#define AFF2_PASSDOOR         (1 << 13)   /* Char walks through doors  */
#define AFF2_ENH_HEAL	      (1 << 14)     /* Char affected by Enh Heal */
#define AFF2_ENH_MANA         (1 << 15)     /* Char affected by Enh Mana */
#define AFF2_ENH_MOVE         (1 << 16)     /* Char affected by Enh Move */
#define AFF2_UNUSED17	      (1 << 17)	    /* Reserved */
#define AFF2_ANTI_MAGIC	      (1 << 18)	  /* Character is less sensitive to magic */
#define AFF2_BLOODLUST        (1 << 19)   /* Blood sucking */
#define AFF2_BLOCK            (1 << 20)   /* (R) Mob is blocked */
#define AFF2_FIRE_SHIELD      (1 << 21)   /* Fire shield */

#define NUM_AFF_FLAGS		54

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0		/* Playing - Nominal state	*/
#define CON_CLOSE	 1		/* Disconnecting		*/
#define CON_GET_NAME	 2		/* By what name ..?		*/
#define CON_NAME_CNFRM	 3		/* Did I get that right, x?	*/
#define CON_PASSWORD	 4		/* Password:			*/
#define CON_NEWPASSWD	 5		/* Give me a password for x	*/
#define CON_CNFPASSWD	 6		/* Please retype password:	*/
#define CON_QSEX	 7		/* Sex?				*/
#define CON_QCLASS	 8		/* Class?			*/
#define CON_RMOTD	 9		/* PRESS RETURN after MOTD	*/
#define CON_MENU	 10		/* Your choice: (main menu)	*/
#define CON_EXDESC	 11		/* Enter a new description:	*/
#define CON_CHPWD_GETOLD 12		/* Changing passwd: get old	*/
#define CON_CHPWD_GETNEW 13		/* Changing passwd: get new	*/
#define CON_CHPWD_VRFY   14		/* Verify new password		*/
#define CON_DELCNF1	 15		/* Delete confirmation 1	*/
#define CON_DELCNF2	 16		/* Delete confirmation 2	*/
#define CON_OEDIT	 17		/*. OLC mode - object edit     .*/
#define CON_REDIT	 18		/*. OLC mode - room edit       .*/
#define CON_ZEDIT	 19		/*. OLC mode - zone info edit  .*/
#define CON_MEDIT	 20		/*. OLC mode - mobile edit     .*/
#define CON_SEDIT	 21		/*. OLC mode - shop edit       .*/
#define CON_QANSI	 22		/* Ask for ANSI support		*/
#define CON_QROLLSTATS	 23
#define CON_CLAN_EDIT	 24
#define CON_QHOMETOWN	 25
#define CON_QCONFIRMCLASS	26
#define CON_TEXTED	 27
#define	CON_GEDIT	 28
#define CON_STATISTICS	 29
#define CON_QCONFIRMCLASS2	30
#define CON_DISCONNECT	 31		/* In-game disconnection	*/


/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT      0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_SHIELD    11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD     16
#define WEAR_HOLD      17
#define WEAR_FACE      18	

#define NUM_WEARS      19	/* This must be the # of eq positions!! */


/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_MISSILE    7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_THROW     24               /* Item can be thrown as weapon */
#define ITEM_GRENADE   25               /* Item is a grenade            */
#define ITEM_BOW       26               /* shoots arrows                */
#define ITEM_SLING     27               /* shoots rocks                 */
#define ITEM_CROSSBOW  28               /* shoots bolts                 */
#define ITEM_BOLT      29
#define ITEM_ARROW     30
#define ITEM_ROCK      31
#define ITEM_FLIGHT    32		/* enables to fly		*/

#define NUM_ITEM_TYPES		32

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE		(1 << 0)  /* Item can be takes		*/
#define ITEM_WEAR_FINGER	(1 << 1)  /* Can be worn on finger	*/
#define ITEM_WEAR_NECK		(1 << 2)  /* Can be worn around neck 	*/
#define ITEM_WEAR_BODY		(1 << 3)  /* Can be worn on body 	*/
#define ITEM_WEAR_HEAD		(1 << 4)  /* Can be worn on head 	*/
#define ITEM_WEAR_LEGS		(1 << 5)  /* Can be worn on legs	*/
#define ITEM_WEAR_FEET		(1 << 6)  /* Can be worn on feet	*/
#define ITEM_WEAR_HANDS		(1 << 7)  /* Can be worn on hands	*/
#define ITEM_WEAR_ARMS		(1 << 8)  /* Can be worn on arms	*/
#define ITEM_WEAR_SHIELD	(1 << 9)  /* Can be used as a shield	*/
#define ITEM_WEAR_ABOUT		(1 << 10) /* Can be worn about body 	*/
#define ITEM_WEAR_WAIST 	(1 << 11) /* Can be worn around waist 	*/
#define ITEM_WEAR_WRIST		(1 << 12) /* Can be worn on wrist 	*/
#define ITEM_WEAR_WIELD		(1 << 13) /* Can be wielded		*/
#define ITEM_WEAR_HOLD		(1 << 14) /* Can be held		*/
#define ITEM_WEAR_FACE		(1 << 15) /* Can be held		*/

#define NUM_ITEM_WEARS 		16

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW          (1 << 0)	/* Item is glowing		*/
#define ITEM_HUM           (1 << 1)	/* Item is humming		*/
#define ITEM_NORENT        (1 << 2)	/* Item cannot be rented	*/
#define ITEM_NODONATE      (1 << 3)	/* Item cannot be donated	*/
#define ITEM_NOINVIS	   (1 << 4)	/* Item cannot be made invis	*/
#define ITEM_INVISIBLE     (1 << 5)	/* Item is invisible		*/
#define ITEM_MAGIC         (1 << 6)	/* Item is magical		*/
#define ITEM_NODROP        (1 << 7)	/* Item is cursed: can't drop	*/
#define ITEM_BLESS         (1 << 8)	/* Item is blessed		*/
#define ITEM_ANTI_GOOD     (1 << 9)	/* Not usable by good people	*/
#define ITEM_ANTI_EVIL     (1 << 10)	/* Not usable by evil people	*/
#define ITEM_ANTI_NEUTRAL  (1 << 11)	/* Not usable by neutral people	*/
#define ITEM_ANTI_MAGIC_USER (1 << 12)	/* Not usable by mages		*/
#define ITEM_ANTI_CLERIC   (1 << 13)	/* Not usable by clerics	*/
#define ITEM_ANTI_THIEF	   (1 << 14)	/* Not usable by thieves	*/
#define ITEM_ANTI_WARRIOR  (1 << 15)	/* Not usable by warriors	*/
#define ITEM_NOSELL	   (1 << 16)	/* Shopkeepers won't touch it	*/
#define ITEM_LIVE_GRENADE  (1 << 17)    /* grenade's pin has been pulled */
#define ITEM_ANTI_MORT     (1 << 18)    /* Not usable by mortals (unused)*/
#define ITEM_BURIED	   (1 << 19)   	/* Buried item			*/
#define ITEM_ANTI_PALADIN  (1 << 20)
#define ITEM_ANTI_RANGER   (1 << 21)
#define ITEM_ANTI_WARLOCK  (1 << 22)
#define ITEM_ANTI_CYBORG   (1 << 23)
#define ITEM_ANTI_NECROMANCER (1 << 24)
#define ITEM_ANTI_DRUID    (1 << 25)
#define ITEM_ANTI_ALCHEMIST (1 << 26)
#define ITEM_ANTI_BARBARIAN (1 << 27)
#define ITEM_NO_DISARM	   (1 << 28)	/* Cannot disarm (weapons only)*/
#define ITEM_BOOMERANG	   (1 << 29)	/* Item returns (rocks, etc.)*/
#define ITEM_ENGRAVED	   (1 << 30)
#define ITEM_AUTOENGRAVE   (1 << 31)

#define ITEM2_DELETED	   (1 << 0)
#define ITEM2_NOSINK	   (1 << 1)

#define NUM_ITEM_FLAGS		34

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to constitution	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_HITROLL          18	/* Apply to hitroll		*/
#define APPLY_DAMROLL          19	/* Apply to damage roll		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/

#define NUM_APPLIES		25

/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15

#define NUM_LIQ_TYPES 		16

/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2
#define VAMPTHIRST   3

/* Sun state for sunlight */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3


/* Sky conditions for weather */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3

/* Hometowns (hometown table is in constants.c!!!) */
#define NUM_HOMETOWNS	 6

#define DEFAULT_HOMETOWN	2
#define GOD_HOMETOWN		1

#define HT_IMM_ONLY	 1
#define HT_CLAN_ONLY	 2
#define HT_NOHOMETOWN	 4

/* DEMON XP */
#define MAX_DEMON_XP   20000000
#define MAX_DEMON_LOSS 100000
/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5
#define RENT_DEATH	6

/* char_data.internal_flags (INT_XXX) ************************************/
#define INT_MARK	(1 << 0)


/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1.
 */
 
 #define MAX_MORT_LEVEL	100
 
 #define LVL_IMMORT	101
 #define LVL_GURU	102
 
 #define LVL_HALF_GOD	103
 #define LVL_MINOR_GOD	104
 #define LVL_GOD	105
 #define LVL_GRGOD	106
 #define LVL_BUILDER	107
 #define LVL_CHBUILD	108
 #define LVL_CODER	109
 #define LVL_COIMPL	110
 #define LVL_IMPL	111

/* Level of the 'freeze' command */
#define LVL_FREEZE	LVL_GRGOD

/* Maximum level allowing do_newbie */
#define MAX_NEWBIE_LEVEL	3

/* Maximum difference between levels allowing the playerkilling */
#define MAX_LEVEL_DIFF	10

#define NUM_OF_DIRS	6	/* number of directions in a room (nsewud) */
#define MAGIC_NUMBER	(0x06)	/* Arbitrary number that won't be in a string */

#define OPT_USEC	10000	/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE      (12 RL_SEC)
#define PULSE_MOBILE    (8 RL_SEC)
#define PULSE_VIOLENCE  (((2 RL_SEC) * 3) / 2)
#define PULSE_TRAINS  	(12 RL_SEC)
#define PULSE_REGEN     (2 RL_SEC)

#define LAVA_DAMAGE	15
#define UNWAT_DAMAGE	10
#define CRIT_DAMAGE	5

/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (12 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       256          /* Max length of prompt        */
#define GARBAGE_SPACE		32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE		1024        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE	   (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

/*
 * --- WARNING ---
 * If you are using a BSD-derived UNIX with MD5 passwords, you _must_
 * make MAX_PWD_LENGTH larger.  A length of 20 should be good. If
 * you leave it at the default value of 10, then any character with
 * a name longer than about 5 characters will be able to log in with
 * _any_ password.  This has not (yet) been changed to ensure pfile
 * compatibility for those unaffected.
 */
#define HISTORY_SIZE		5	/* Keep last 5 commands. */
#define MAX_STRING_LENGTH	8192
#define MAX_INPUT_LENGTH	256	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512	/* Max size of *raw* input */
#define MAX_MESSAGES		60
#define MAX_NAME_LENGTH		20  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_PWD_LENGTH		10  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TITLE_LENGTH	80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define HOST_LENGTH		30  /* Used in char_file_u *DO*NOT*CHANGE* */
#define EXDSCR_LENGTH		240 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_TONGUE		3   /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_SKILLS		200 /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFFECT		32  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_POOF_LENGTH		80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_AFK_LENGTH		80  /* Used in char_file_u *DO*NOT*CHANGE* */
#define MAX_OBJ_AFFECT		6   /* Used in obj_file_elem *DO*NOT*CHANGE* */
#define	MAX_ALIAS_LENGTH	128

/* Other defines */
#define OBJ_SPRING_VNUM		50

#define LEARNED_LEVEL		0	// % known which is considered "learned" 
#define MAX_PER_PRAC_SPELL	1	// max percent gain in spell per practice 
#define MIN_PER_PRAC_SPELL	2	// min percent gain in spell per practice 
#define MAX_PER_PRAC_SKILL	3	// max percent gain in skill per practice 
#define MIN_PER_PRAC_SKILL	4	// min percent gain in skill per practice 
#define PRAC_TYPE		5	// should it say 'spell' or 'skill'?	

/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char			bool;
#endif

#ifndef CIRCLE_WINDOWS
typedef char			byte;
#endif

typedef sh_int	room_vnum;	/* A room's vnum type */
typedef sh_int	obj_vnum;	/* An object's vnum type */
typedef sh_int	mob_vnum;	/* A mob's vnum type */

typedef sh_int	room_rnum;	/* A room's real (internal) number type */
typedef sh_int	obj_rnum;	/* An object's real (internal) num type */
typedef sh_int	mob_rnum;	/* A mobile's real (internal) num type */

#define LONG_BITV_WIDTH		(4)
typedef long	long_bitv[LONG_BITV_WIDTH];

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};


/* object-related structures ******************************************/


/* object flags; used in obj_data */
struct obj_flag_data {
   int	value[4];	/* Values of the item (see list)    */
   byte type_flag;	/* Type of item			    */
   int	wear_flags;	/* Where you can wear it	    */
   long_bitv	extra_flags;	/* If it hums, glows, etc.	    */
   int	weight;		/* Weigt what else                  */
   int	cost;		/* Value when sold (gp.)            */
   int	cost_per_day;	/* Cost to keep pr. real day        */
   int	timer;		/* Timer for object                 */
   long_bitv	bitvector;	/* To set chars bits                */
   ubyte bottom_level;
   ubyte top_level;
};


/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
   byte location;      /* Which ability to change (APPLY_XXX) */
   sbyte modifier;     /* How much it changes by              */
};


/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			*/
   room_rnum in_room;		/* In what room -1 when conta/carr	*/

   struct obj_flag_data obj_flags;/* Object information               */
   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;	  /* Worn by?			      */
   sh_int worn_on;		  /* Worn where?		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */
   long engraved_id;		  /* Used when ITEM_ENGRAVED set      */
};
/* ======================================================================= */


/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent files		   */
struct obj_file_elem {
   obj_vnum item_number;

   int	value[4];
   int	extra_flags;
   int	weight;
   int	timer;
   long_bitv	bitvector;
   struct obj_affected_type affected[MAX_OBJ_AFFECT];
   long engraved_id;
};

/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
struct rent_info {
   int	time;
   int	rentcode;
   int	net_cost_per_diem;
   int	gold;
   int	account;
   int	nitems;
   int	alias_offset;
   int	item_offset;
   int	spare2;
   int	spare3;
   int	spare4;
   int	spare5;
   int	spare6;
   int	spare7;
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
   char	*general_description;       /* When look DIR.			*/

   char	*keyword;		/* for open/close			*/

   sh_int exit_info;		/* Exit info				*/
   obj_vnum key;		/* Key's number (-1 for no key)		*/
   room_rnum to_room;		/* Where direction leads (NOWHERE)	*/
};

struct teleport_data {
	   int time;
	   room_vnum targ;
	   long mask;
	   int cnt;
	   obj_vnum obj;
	};

struct raff_node {
	room_rnum room;        /* location in the world[] array of the room */
	int      timer;       /* how many ticks this affection lasts */
	long     affection;   /* which affection does this room have */
	int      spell;       /* the spell number */

	struct raff_node *next; /* link to the next node */
};

/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		/* Rooms number	(vnum)		      */
   sh_int zone;                 /* Room zone (for resetting)          */
   byte sector_type;            /* sector type (move/hide)            */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   struct teleport_data *tele;	/* teleport info */
   long_bitv room_flags;		/* DEATH,DARK ... etc                 */

   byte light;                  /* Number of lightsources in room     */
   SPECIAL(*func);
   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */
   long room_affections;    	/* Bitvector for spells/skills 	      */
   long orig_affections;	
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	id;
   char *name;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


 /* MOBProgram foo */
 struct mob_prog_act_list {
   struct mob_prog_act_list *next;
   char *buf;
   char *pos;
   struct char_data *ch;
   struct obj_data *obj;
   void *vo;
   int progtype;
 };
 typedef struct mob_prog_act_list MPROG_ACT_LIST;

 struct mob_prog_data {
   struct mob_prog_data *next;
   int type;
   char *arglist;
   char *comlist;
   char *filename;
 };

 typedef struct mob_prog_data MPROG_DATA;

 extern bool MOBTrigger;

 #define ERROR_PROG		-1
 #define NO_PROG		-2
 #define IN_FILE_PROG		0
 #define ACT_PROG		(1 << 0)
 #define SPEECH_PROG		(1 << 1)
 #define RAND_PROG          	(1 << 2)
 #define FIGHT_PROG         	(1 << 3)
 #define DEATH_PROG        	(1 << 4)
 #define HITPRCNT_PROG     	(1 << 5)
 #define ENTRY_PROG        	(1 << 6)
 #define GREET_PROG       	(1 << 7)
 #define ALL_GREET_PROG   	(1 << 8)
 #define GIVE_PROG        	(1 << 9)
 #define BRIBE_PROG      	(1 << 10)
 #define LEAVE_PROG	 	(1 << 11)
 #define RESET_PROG	 	(1 << 12)
 #define TIME_PROG	 	(1 << 13)

 #define NUM_MPROG_TYPES		15

 /* end of MOBProg foo */



/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;    /* This represents the characters age                */
   time_t logon;    /* Time of the last logon (used to calculate played) */
   int	played;     /* This is the total accumulated time played in secs */
};


/* general player-related info, usually PC's and NPC's */
struct char_player_data {
   char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
   char	*name;	       /* PC / NPC s name (kill ...  )         */
   char	*short_descr;  /* for NPC 'actions'                    */
   char	*long_descr;   /* for 'look'			       */
   char	*description;  /* Extra descriptions                   */
   char	*title;        /* PC / NPC's title                     */
   char *prompt;       /* PC's prompt */
   byte sex;           /* PC / NPC's sex                       */
   byte chclass;       /* PC / NPC's class		       */
   byte level;         /* PC / NPC's level                     */
   byte	hometown;      /* PC s Hometown (zone)                 */
   struct time_data time;  /* PC's AGE in days                 */
   ubyte weight;       /* PC / NPC's weight                    */
   ubyte height;       /* PC / NPC's height                    */
};


/* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_ability_data {
   sbyte str;
   sbyte str_add;      /* 000 - 100 if strength 18             */
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
};


/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
struct char_point_data {
   sh_int mana;
   sh_int max_mana;     /* Max move for PC/NPC			   */
   sh_int hit;
   sh_int max_hit;      /* Max hit for PC/NPC                      */
   sh_int move;
   sh_int max_move;     /* Max move for PC/NPC                     */

   sh_int armor;        /* Internal -100..100, external -10..10 AC */
   int	gold;           /* Money carried                           */
   int	bank_gold;	/* Gold the char has in a bank account	   */
   int	exp;            /* The experience of the player            */
   int  demonxp;        /* The demonxp of a player      */
   int  celerity;
   int  brennum;  
   int  afilase;
   int  tutula;
   int  ninjitsu;
   int  samurai;
   int  budokai;
   int  ki;
   int  mitus;
   int  xian;
   int  mulian;
   int  hybrid;
   int  pkillp; 
   sbyte hitroll;       /* Any bonus or penalty to the hit roll    */
   sbyte damroll;       /* Any bonus or penalty to the damage roll */
};


/* 
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
   int	alignment;		/* +-1000 for alignments                */
   long	idnum;			/* player's idnum; -1 for mobiles	*/
   long_bitv	act;			/* act flag for NPC's; player flag for PC's */

   long_bitv	affected_by;		/* Bitvector for spells/skills affected by */
   sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)		*/
};


/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
   struct char_data *fighting;	/* Opponent				*/
   struct char_data *hunting;	/* Char hunted by this char		*/
   struct event *event_fight;
   struct char_data *riding;	/* Who are they riding? (DAK) */
   struct char_data *ridden_by; /* Who is riding them? (DAK) */
   byte position;		/* Standing, fighting, sleeping, etc.	*/

   int	carry_weight;		/* Carried weight			*/
   byte carry_items;		/* Number of items carried		*/
   int	timer;			/* Timer for update			*/
   byte leavedir;		/* Last leaving direction (used for blocking) */
   struct char_special_data_saved saved; /* constants saved in plrfile	*/
};


/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
   byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
   byte PADDING0;		/* used to be spells_to_learn		*/
   bool talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
   int	wimp_level;		/* Below this # of hit points, flee!	*/
   byte freeze_level;		/* Level of god who froze char, if any	*/
   sh_int invis_level;		/* level of invisibility		*/
   room_vnum load_room;		/* Which room to place char in		*/
   long_bitv	pref;		/* preference flags for PC's.		*/
   ubyte bad_pws;		/* number of bad password attemps	*/
   sbyte conditions[3];         /* Drunk, full, thirsty			*/

   /* spares below for future expansion.  You can change the names from
      'sparen' to something meaningful, but don't change the order.  */

   ubyte qpoints;               /* Number of questpoints */
   ubyte spare1;
   ubyte spare2;
   ubyte spare3;
   ubyte spare4;
   ubyte spare5;
   int spells_to_learn;		/* How many can you learn yet this level*/
   int olc_zone;
   int spare8;
   int clannum;			/* Index number of clan you belong to (FIDO) */
   int spare10;	
   int clanrank;                /* Rank within clan */
   int rip_cnt;		
   int kill_cnt;
   int dt_cnt;
   int spare14;
   int multi_flags;
   int total_level;
   int abilities;
   int temp_gold;
   int llevels;
   int vamp_power1;
   int powerl;
   int no_lleg;
   int no_rleg;
   int no_rarm;
   int pkillp;
   int player_kills;
   int no_larm;
   int no_wpipe;
   int no_face;
   int spare31;
   int spare32;
   int spare33;
   int spare34;
   int spare35;   
   int spare36;
   int spare37;
   int spare38;
   int spare39;
   int spare40;
   int spare41;
   int spare42;
   int spare43;
   int spare44;
   int spare45;
   int spare46;
   int spare47;
   int spare48;
   int spare49;
   int spare50;
   int spare51;
   int spare52;
   int spare53;
   int spare54;
   int spare55;
   int spare56;
   int spare57;
   int spare58;
   int spare59;
   int spare60;
   int spare61;
   int spare62;
   int spare63;
   int spare64;
   int spare65;
   int spare66;
   int spare67;
   int spare68;
   int spare69;
   int spare70;
   int spare71;
   int spare72;
   int spare73;
   int spare74;
   int spare75;
   int spare76;
   int spare77;
   int spare78;
   int spare79;
   int spare80;
   int spare81;
   int spare82;
   int spare83;
   int spare84;
   int spare85;
   int spare86;
   int spare87;
   int spare88;
   int spare89;
   int spare90;
   int spare91;
   int spare92;
   int spare93;
   int spare94;
   int spare95;
   int spare96;
   int spare97;
   int spare98;
   int spare99;
   int spare100;
   long	spare101;
   long	spare102;
   long	spare103;
   long	spare104;
   long	spare105;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
   struct player_special_data_saved saved;

   char	*poofin;		/* Description on arrival of a god.     */
   char	*poofout;		/* Description upon a god's exit.       */
   struct alias *aliases;	/* Character's aliases			*/
   long last_tell;		/* idnum of last tell from		*/
   void *last_olc_targ;		/* olc control				*/
   sh_int last_olc_mode;	/* olc control				*/
   char *afk_message;		/* message displayed when AFK		*/
};


/* Specials used by NPCs, not PCs */
struct mob_special_data {
   byte last_direction;     /* The last direction the monster went     */
   sh_int attack_type;        /* The Attack Type Bitvector for NPC's     */
   byte default_pos;        /* Default position for NPC                */
   memory_rec *memory;	    /* List of attackers to remember	       */
   byte damnodice;          /* The number of damage dice's	       */
   byte damsizedice;        /* The size of the damage dice's           */
   int wait_state;	    /* Wait state for bashed mobs	       */
};


/* An affect structure.  Used in char_file_u *DO*NOT*CHANGE* */
struct affected_type {
   sh_int type;          /* The type of spell that caused this      */
   sh_int duration;      /* For how long its effects will last      */
   sbyte modifier;       /* This is added to apropriate ability     */
   byte location;        /* Tells which ability to change(APPLY_XXX)*/
   long_bitv	bitvector;       /* Tells which bits to set (AFF_XXX)       */
   struct affected_type *next;
};


/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
   int pfilepos;			 /* playerfile pos		  */
   sh_int nr;                            /* Mob's rnum			  */
   room_rnum in_room;                    /* Location (real room number)	  */
   room_rnum was_in_room;		 /* location for linkdead people  */

   struct char_player_data player;       /* Normal data                   */
   struct char_ability_data real_abils;	 /* Abilities without modifiers   */
   struct char_ability_data aff_abils;	 /* Abils with spells/stones/etc  */
   struct char_point_data points;        /* Points                        */
   struct char_special_data char_specials;	/* PC/NPC specials	  */
   struct player_special_data *player_specials; /* PC specials		  */
   struct mob_special_data mob_specials;	/* NPC specials		  */

   struct affected_type *affected;       /* affected by what spells       */
   struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

   struct obj_data *carrying;            /* Head of list                  */
   struct descriptor_data *desc;         /* NULL for mobiles              */

   struct char_data *next_in_room;     /* For room->people - list         */
   struct char_data *next;             /* For either monster or ppl-list  */
   struct char_data *next_fighting;    /* For fighting list               */

   struct event *points_event[3];
   struct follow_type *followers;        /* List of chars followers       */
   struct char_data *master;             /* Who is char following?        */
   MPROG_ACT_LIST *mpact;
   int mpactnum;
   byte internal_flags;
};
/* ====================================================================== */


/* ==================== File Structure for Player ======================= */
/*             BEWARE: Changing it will ruin the playerfile		  */
struct char_file_u {
   /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[EXDSCR_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   char prompt[MAX_INPUT_LENGTH+1];
   byte sex;
   byte chclass;
   byte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int	played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;

   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	/* host of last logon */
   char	poof_in[MAX_POOF_LENGTH+1];
   char poof_out[MAX_POOF_LENGTH+1];
   char afk[MAX_AFK_LENGTH+1];
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};


struct descriptor_data {
   socket_t descriptor;		/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int	wait;			/* wait for how many loops		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;		/* added for handling abort buffers     */
   size_t max_str;		/*		-			*/
   long	mail_to;		/* name for mail system			*/
   sh_int mail_vnum;
   struct obj_data *obj_mail;
   int	has_prompt;		/* is user at a prompt?  		*/
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct olc_data *olc;	     /*. OLC info - defined in olc.h   .*/
   char   *storage;
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};

struct fight_event_data {
  struct char_data *ch, *vict;   /* attacker and victim for fight_event */
};

struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};


struct dex_skill_type {
   sh_int p_pocket;
   sh_int p_locks;
   sh_int traps;
   sh_int sneak;
   sh_int hide;
};


struct dex_app_type {
   sh_int reaction;
   sh_int miss_att;
   sh_int defensive;
};


struct str_app_type {
   sh_int tohit;    /* To Hit (THAC0) Bonus/Penalty        */
   sh_int todam;    /* Damage Bonus/Penalty                */
   sh_int carry_w;  /* Maximum weight that can be carrried */
   sh_int wield_w;  /* Maximum weight that can be wielded  */
};


struct wis_app_type {
   byte bonus;       /* how many practices player gains per lev */
};



struct int_app_type {
   byte learn;       /* how many % a player learns a spell/skill */
};


struct con_app_type {
   sh_int hitp;
   sh_int shock;
};


struct title_type {
   char	*title_m;
   char	*title_f;
   int	exp;
};


/* element in monster and object index-tables   */
struct index_data {
   int	vnum;		/* virtual number of this mob/obj		*/
   int	number;     /* number of existing units of this mob/obj	*/
   int  progtypes;  /* program types for MOBProg              */
   MPROG_DATA *mobprogs; /* programs for MOBProg              */
   SPECIAL(*func);
};

typedef struct {
  char *homename;
  int magic_room;
  int flags;
} HOMETOWN;

struct specproc_info {
  char *name;
  SPECIAL((*sp_pointer));
  sh_int minimum_level;
};


#endif // __STRUCTS_H__
