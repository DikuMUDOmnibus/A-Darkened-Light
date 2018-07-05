/* ************************************************************************
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __SPELLS_H__
#define __SPELLS_H__

#define DEFAULT_STAFF_LVL	12
#define DEFAULT_WAND_LVL	12

#define CAST_UNDEFINED	-1
#define CAST_SPELL	0
#define CAST_POTION	1
#define CAST_WAND	2
#define CAST_STAFF	3
#define CAST_SCROLL	4
#define CAST_BREATH	5

#define MAG_DAMAGE	(1 << 0)
#define MAG_AFFECTS	(1 << 1)
#define MAG_UNAFFECTS	(1 << 2)
#define MAG_POINTS	(1 << 3)
#define MAG_ALTER_OBJS	(1 << 4)
#define MAG_GROUPS	(1 << 5)
#define MAG_MASSES	(1 << 6)
#define MAG_AREAS	(1 << 7)
#define MAG_SUMMONS	(1 << 8)
#define MAG_CREATIONS	(1 << 9)
#define MAG_MANUAL	(1 << 10)
#define MAG_ROOM	(1 << 11)


#define TYPE_UNDEFINED               -1
#define SPELL_RESERVED_DBC            0  /* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR                   1 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_TELEPORT                2 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLESS                   3 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BLINDNESS               4 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_BURNING_HANDS           5 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CALL_LIGHTNING          6 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHARM                   7 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CHILL_TOUCH             8 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CLONE                   9 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_COLOR_SPRAY            10 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CONTROL_WEATHER        11 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_FOOD            12 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CREATE_WATER           13 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_BLIND             14 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_CRITIC            15 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURE_LIGHT             16 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_CURSE                  17 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_ALIGN           18 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_INVIS           19 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_MAGIC           20 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DETECT_POISON          21 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_EVIL            22 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_EARTHQUAKE             23 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENCHANT_WEAPON         24 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ENERGY_DRAIN           25 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_FIREBALL               26 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HARM                   27 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_HEAL                   28 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INVISIBLE              29 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LIGHTNING_BOLT         30 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_LOCATE_OBJECT          31 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_MAGIC_MISSILE          32 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_POISON                 33 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_PROT_FROM_EVIL         34 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_CURSE           35 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SANCTUARY              36 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SHOCKING_GRASP         37 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SLEEP                  38 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_STRENGTH               39 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SUMMON                 40 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_VENTRILOQUATE          41 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WORD_OF_RECALL         42 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_REMOVE_POISON          43 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_SENSE_LIFE             44 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_ANIMATE_DEAD	     45 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_DISPEL_GOOD	     46 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_ARMOR	     47 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_HEAL	     48 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_GROUP_RECALL	     49 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_INFRAVISION	     50 /* Reserved Skill[] DO NOT CHANGE */
#define SPELL_WATERWALK		     51 /* Reserved Skill[] DO NOT CHANGE */
/* Insert new spells, up to MAX_SPELLS */
#define SPELL_DISINTEGRATE	     52
#define SPELL_PORTAL		     53
#define SPELL_FEAR		     54
#define SPELL_MINOR_IDENTIFY         55
#define SPELL_PHANTASM	             56
#define SPELL_RECHARGE		     57
#define SPELL_METEOR_SHOWER	     58
#define SPELL_STONESKIN		     59
#define SPELL_STEELSKIN              60
#define SPELL_CHAIN_LIGHTNING        61
#define SPELL_HOLD_PERSON	     62
#define SPELL_PARALYZE               63
#define SPELL_HOLY_WORD		     64
#define SPELL_HOLY_SHOUT             65
#define SPELL_HASTE		     66
#define SPELL_DEATHDANCE	     67
#define SPELL_DEATH_RIPPLE	     68
#define SPELL_DEATH_WAVE	     69
#define SPELL_SHIELD		     70
#define SPELL_GROUP_SHIELD	     71



#define SPELL_RELOCATE		     72
#define SPELL_PEACE		     73
#define SPELL_FLY		     74
#define SPELL_LEVITATE               75
#define SPELL_PROT_FIRE              76
#define SPELL_WATERBREATH	     77
#define SPELL_GROUP_FLY		     78
#define SPELL_GROUP_INVIS            79
#define SPELL_GROUP_PROT_EVIL        80
#define SPELL_ACID_ARROW	     81
#define SPELL_FLAME_ARROW	     82
#define SPELL_CURE_SERIOUS	     83
#define SPELL_GROUP_WATBREATH        84
#define SPELL_WRAITHFORM	     85
#define SPELL_WEAKEN		     86
#define SPELL_CONE_OF_COLD	     87
#define SPELL_AID		     88
#define SPELL_BARKSKIN               89
#define SPELL_FEAST                  90
#define SPELL_FEAST_ALL              91
#define SPELL_BLADEBARRIER           92
#define SPELL_KNOCK	             93
#define SPELL_PLANESHIFT	     94
#define SPELL_DISPEL_MAGIC           95
#define SPELL_PROT_COLD		     96
#define SPELL_BLINK		     97
#define SPELL_MIRROR_IMAGE	     98
#define SPELL_ANTI_MAGIC	     99
#define SPELL_DISRUPTING_RAY	    100
#define SPELL_EARTH_ELEMENTAL	    101
#define SPELL_WATER_ELEMENTAL	    102
#define SPELL_AIR_ELEMENTAL	    103
#define SPELL_FIRE_ELEMENTAL	    104
#define SPELL_BLOODLUST		    105
#define SPELL_WALL_OF_FOG           106
#define SPELL_FIRE_SHIELD	    107
#define SPELL_ARCANE_WORD	    108
#define SPELL_ARCANE_PORTAL	    109
#define SPELL_LIFE_TRANSFER	    110
#define SPELL_MANA_TRANSFER	    111
#define SPELL_CORPSE_HOST	    112
#define NUM_SPELLS		    113

/* Insert new spells here, up to MAX_SPELLS */
#define MAX_SPELLS		    130

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB              131 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_BASH                  132 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_HIDE                  133 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_KICK                  134 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PICK_LOCK             135 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_PUNCH                 136 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_RESCUE                137 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_SNEAK                 138 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_STEAL                 139 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_TRACK		    140 /* Reserved Skill[] DO NOT CHANGE */
#define SKILL_MOUNT		    141 /* Mounting (DAK) */
#define SKILL_RIDING		    142 /* Riding (DAK) */
#define SKILL_TAME		    143 /* Ability to tame (DAK) */
/* New skills may be added here up to MAX_SKILLS (200) */

#define SKILL_THROW             144 /* new */
#define SKILL_BOW               145
#define SKILL_SLING             146
#define SKILL_CROSSBOW          147
#define SKILL_SECOND_ATTACK	148 /* Second attack (DAK) */
#define SKILL_THIRD_ATTACK	149 /* Third attack (DAK)  */
#define SKILL_DISARM		150

#define SKILL_SPRING            151 /* Ability to create springs */
#define SKILL_FORAGE            152 /* Forage for food */
#define SKILL_FISSION		153 /* Ability to turn object into energy */
#define SKILL_EXAMINE		154 /* Ability of using examination unit */
#define SKILL_HAND_TO_HAND	155 /* Hand 2 hand combat                */
#define SKILL_UNARMED_COMBAT	156 /* Unarmed combat */
#define SKILL_TRAP_AWARE	157 /* Trap aware */
#define SKILL_PARRY		158 /* Can parry attacks */
#define SKILL_RETREAT		159 /* The art of retreat */
#define SKILL_PATHFINDING	160 /* The skill of pathfinding */

#define NUM_SKILLS		30

/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */
#define START_NON_PLAYER_SPELLS 201

#define SPELL_IDENTIFY               201
#define SPELL_FIRE_BREATH            202
#define SPELL_GAS_BREATH             203
#define SPELL_FROST_BREATH           204
#define SPELL_ACID_BREATH            205
#define SPELL_LIGHTNING_BREATH       206
#define SPELL_MANA_BOOST             207
#define SPELL_REGENERATE             208
#define SPELL_BURN		     209
#define SPELL_FREEZE		     210
#define SPELL_ACID		     211
#define SPELL_ENH_HEAL		     212
#define SPELL_ENH_MANA		     213
#define SPELL_ENH_MOVE		     214
#define SPELL_CRIT_HIT		     215

#define NUM_NON_PLAYER_SPELLS   15

#define TOP_SPELL_DEFINE	     299
/* NEW NPC/OBJECT SPELLS can be inserted here up to 299 */

#define TOP_SPELLS		((TOP_SPELL_DEFINE)+1)

/* WEAPON ATTACK TYPES */

#define TYPE_HIT                     300
#define TYPE_STING                   301
#define TYPE_WHIP                    302
#define TYPE_SLASH                   303
#define TYPE_BITE                    304
#define TYPE_BLUDGEON                305
#define TYPE_CRUSH                   306
#define TYPE_POUND                   307
#define TYPE_CLAW                    308
#define TYPE_MAUL                    309
#define TYPE_THRASH                  310
#define TYPE_PIERCE                  311
#define TYPE_BLAST		     312
#define TYPE_PUNCH		     313
#define TYPE_STAB		     314

/* new attack types can be added here - up to TYPE_SUFFERING */

#define NUM_ATTACK_TYPES	15

#define TYPE_SUFFERING		     399



#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4


#define TAR_IGNORE        1
#define TAR_CHAR_ROOM     2
#define TAR_CHAR_WORLD    4
#define TAR_FIGHT_SELF    8
#define TAR_FIGHT_VICT   16
#define TAR_SELF_ONLY    32 /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF     64 /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV     128
#define TAR_OBJ_ROOM    256
#define TAR_OBJ_WORLD   512
#define TAR_OBJ_EQUIP  1024

struct spell_info_type {
   byte min_position;	/* Position for caster	 */
   int mana_min;	/* Min amount of mana used by a spell (highest lev) */
   int mana_max;	/* Max amount of mana used by a spell (lowest lev) */
   int mana_change;	/* Change in mana used by spell from lev to lev */

   int min_level[NUM_CLASSES];
   int routines;
   byte violent;
   int targets;         /* See below for use with TAR_XXX  */
};

/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.

*/

#define SPELL_TYPE_SPELL   0
#define SPELL_TYPE_POTION  1
#define SPELL_TYPE_WAND    2
#define SPELL_TYPE_STAFF   3
#define SPELL_TYPE_SCROLL  4


/* Attacktypes with grammar */

struct attack_hit_type {
   const char	*singular;
   const char	*plural;
};


#define ASPELL(spellname) \
void	spellname(int level, struct char_data *ch, \
		  struct char_data *victim, struct obj_data *obj, char *strarg)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict, tar_str);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
// ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_disintegrate);
ASPELL(spell_portal);
ASPELL(spell_fear);
ASPELL(spell_minor_identify);
ASPELL(spell_chain_lightning);
ASPELL(spell_recharge);
ASPELL(spell_dispel_magic);
ASPELL(spell_peace);
ASPELL(spell_relocate);
ASPELL(spell_arcane_word);
ASPELL(spell_arcane_portal);
ASPELL(spell_control_weather);
ASPELL(spell_corpse_host);

/* basic magic calling functions */

int find_skill_num(char *name);

int mag_damage(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

void mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(int level, struct char_data *ch, int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
 int spellnum, int savetype);

void mag_points(int level, struct char_data *ch, struct char_data *victim,
 int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
  int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
  int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

void mag_room(int level, struct char_data * ch, int spellnum);

int mag_manacost(struct char_data * ch, int spellnum);

int	call_magic(struct char_data *caster, struct char_data *cvict,
  struct obj_data *ovict, char *svict, int spellnum, int level, int casttype);

void	mag_objectmagic(struct char_data *ch, struct obj_data *obj,
			char *argument);

int	cast_spell(struct char_data *ch, struct char_data *tch,
  struct obj_data *tobj, char *tar_str, int spellnum);

void class_spells_index(int class, char *str);

/* other prototypes */
void spell_level(int spell, int chclass, int level);
void init_spell_levels(void);
const char *skill_name(int num);

#endif
