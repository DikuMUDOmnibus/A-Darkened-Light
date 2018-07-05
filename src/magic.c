/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "clan.h"

/* local functions */
int mag_materials(struct char_data * ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, struct char_data * ch, struct char_data * tch, int spellnum, int savetype);
int mag_savingthrow(struct char_data * ch, int type);
void affect_update(void);

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *obj_index;

extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;
extern char *spell_wear_off_msg[];
byte saving_throws(int class_num, int type, int level); /* class.c */
extern struct default_mobile_stats *mob_defaults;
extern struct apply_mod_defaults *apmd;

void clearMemory(struct char_data * ch);

void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
extern struct spell_info_type spell_info[];

/*
 * Saving throws are now in class.c (bpl13)
 */

int mag_savingthrow(struct char_data * ch, int type)
{
  int save;

  /* negative apply_saving_throw values make saving throws better! */

  if (IS_NPC(ch)) /* NPCs use warrior tables according to some book */
    save = saving_throws(CLASS_WARRIOR, type, (int) GET_LEVEL(ch));
  else
    save = (GET_LEVEL(ch) < LVL_IMMORT) ? 
       saving_throws((int) GET_CLASS(ch), type, (int) GET_LEVEL(ch)) : 0;

  save += GET_SAVE(ch, type);

  /* throwing a 0 is always a failure */
  if (MAX(1, save) < number(0, 99))
    return TRUE;
  else
    return FALSE;
}


/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  struct affected_type *af, *next;
  struct char_data *i;
  extern struct raff_node *raff_list;
  struct raff_node *raff, *next_raff, *temp;

  for (i = character_list; i; i = i->next)
    for (af = i->affected; af; af = next) {
      next = af->next;
      if (af->duration >= 1)
	af->duration--;
      else if (af->duration == -1)	/* No action */
	af->duration = -1;	/* GODs only! unlimited */
      else {
	if ((af->type > 0) && (af->type <= MAX_SPELLS))
	  if (!af->next || (af->next->type != af->type) ||
	      (af->next->duration > 0))
	    if (*spell_wear_off_msg[af->type]) {
	      send_to_char(spell_wear_off_msg[af->type], i);
	      send_to_char("\r\n", i);
	    }
	affect_remove(i, af);
      }
    }
  /* update the room affections */
  for (raff = raff_list; raff; raff = next_raff) {
    next_raff = raff->next;
    raff->timer--;
    if (raff->timer <= 0) {
      /* this room affection has expired */
      send_to_room(spell_wear_off_msg[raff->spell],
      raff->room);
      send_to_room("\r\n", raff->room);

      /* remove the affection */
      REMOVE_BIT(world[(int)raff->room].room_affections,
      raff->affection);
      /*
      SET_BIT(world[(int)raff->room].room_affections, 
        world[(int)raff->room].orig_affections);
      */
      REMOVE_FROM_LIST(raff, raff_list, next)
      free(raff);
    }
  }
}



/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data * ch, int item0, int item1, int item2,
		      int extract, int verbose)
{
  struct obj_data *tobj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (number(0, 2)) {
      case 0:
	send_to_char("A wart sprouts on your nose.\r\n", ch);
	break;
      case 1:
	send_to_char("Your hair falls out in clumps.\r\n", ch);
	break;
      case 2:
	send_to_char("A huge corn develops on your big toe.\r\n", ch);
	break;
      }
    }
    return (FALSE);
  }
  if (extract) {
    if (item0 < 0) {
      obj_from_char(obj0);
      extract_obj(obj0);
    }
    if (item1 < 0) {
      obj_from_char(obj1);
      extract_obj(obj1);
    }
    if (item2 < 0) {
      obj_from_char(obj2);
      extract_obj(obj2);
    }
  }
  if (verbose) {
    send_to_char("A puff of smoke rises from your pack.\r\n", ch);
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}




/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype)
{
  int dam = 0;

  if (victim == NULL || ch == NULL)
    return 0;

  switch (spellnum) {
    /* Mostly mages */
  case SPELL_MAGIC_MISSILE:
  case SPELL_CHILL_TOUCH:	/* chill touch also has an affect */
    if (IS_WIZARD_TYPE(ch))
      dam = dice(1, 8) + 1;
    else
      dam = dice(1, 6) + 1;
    break;
  case SPELL_BURNING_HANDS:
    if (IS_WIZARD_TYPE(ch))
      dam = dice(3, 8) + 3;
    else
      dam = dice(3, 6) + 3;
    break;
  case SPELL_SHOCKING_GRASP:
    if (IS_WIZARD_TYPE(ch))
      dam = dice(5, 8) + 5;
    else
      dam = dice(5, 6) + 5;
    break;
  case SPELL_LIGHTNING_BOLT:
    if (IS_WIZARD_TYPE(ch))
      dam = dice(7, 8) + 7;
    else
      dam = dice(7, 6) + 7;
    break;
  case SPELL_COLOR_SPRAY:
    if (IS_WIZARD_TYPE(ch))
      dam = dice(9, 8) + 9;
    else
      dam = dice(9, 6) + 9;
    break;
  case SPELL_FIREBALL:
    if (IS_WIZARD_TYPE(ch))
      dam = dice(11, 8) + 11;
    else
      dam = dice(11, 6) + 11;
    break;

    /* Mostly clerics */
  case SPELL_DISPEL_EVIL:
    dam = dice(6, 8) + 6;
    if (IS_EVIL(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_GOOD(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      dam = 0;
      return 0;
    }
    break;
  case SPELL_DISPEL_GOOD:
    dam = dice(6, 8) + 6;
    if (IS_GOOD(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_EVIL(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      dam = 0;
      return 0;
    }
    break;


  case SPELL_CALL_LIGHTNING:
    if (zone_table[GET_ROOM_ZONE(victim->in_room)].sky == SKY_LIGHTNING)
      dam = dice(10, 8) + 10;
    else
      dam = dice(7, 8) + 7;
    break;

  case SPELL_HARM:
    dam = dice(8, 8) + 8;
    break;

  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2)
      dam = 100;
    else
      dam = dice(1, 10);
    break;
  case SPELL_HOLY_SHOUT:
  case SPELL_HOLY_WORD:
    if (MOB_FLAGGED(victim, MOB_UNDEAD) || GET_CLASS(victim) == CLASS_NECROMANCER) {
      if (IS_CLERIC_TYPE(ch))
        dam = dice(8, 10) + 6;
      else
        dam = dice(6, 8) + 2;
      if (number(0, 10)+GET_LEVEL(victim) < level)
        GET_POS(victim) = POS_SITTING;
      }
    else {
      dam = 0;
    }
    break;
    
    /* Area spells */
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    break;

  case SPELL_METEOR_SHOWER:
    if (SECT(ch->in_room) != SECT_INSIDE)
      dam = dice(level / 3, 7);
    else
      dam = 0;
    break;
    
  case SPELL_DEATH_RIPPLE:
    if (GET_CLASS(victim) != CLASS_NECROMANCER && GET_CLASS(victim) != CLASS_CYBORG) {
      if (GET_CLASS(ch) == CLASS_NECROMANCER) dam = dice(3, 9);
      else dam = dice(2, 7);
    } else dam = 0;
    break;
    
  case SPELL_DEATH_WAVE:
    if (GET_CLASS(victim) != CLASS_NECROMANCER && GET_CLASS(victim) != CLASS_CYBORG) {
      if (GET_CLASS(ch) == CLASS_NECROMANCER) dam = dice(2, 9) + level;
      else dam = dice(3, 7) + level/2;
    } else dam = 0;
    break;
  
  /* Dragon "spells".  I've made them all the same as a fireball spell */
  case SPELL_FIRE_BREATH:
  case SPELL_GAS_BREATH:
  case SPELL_FROST_BREATH:
  case SPELL_ACID_BREATH:
  case SPELL_LIGHTNING_BREATH:
    dam = dice(11, 8) + 11;
    break;

  case SPELL_ACID_ARROW:
    dam = dice(level / 3, 4);
    break;

  case SPELL_FLAME_ARROW:
    dam = dice(level / 2, 4);
    break;

  case SPELL_CONE_OF_COLD:
    dam = dice(MIN(20, level), 4);
    break;
    
  case SPELL_BLADEBARRIER:
    dam = dice(8, level / 3 + 1);
    break;
    
  } /* switch(spellnum) */


  /* divide damage by two if victim makes his saving throw */
  if (mag_savingthrow(victim, savetype))
    dam /= 2;
    
  if (AFF2_FLAGGED(victim, AFF2_ANTI_MAGIC)) {
    dam /= 3;
  }  

  /* and finally, inflict the damage */
  if (!CAN_MURDER(ch, victim) || (GET_LEVEL(victim) >= LVL_IMMORT)) {
      act("The Mark of Neutrality protects $N.", FALSE, ch, 0, victim, TO_CHAR);
      act("The Mark of Neutrality protects you.", FALSE, ch, 0, victim, TO_VICT);
  } else
    return damage(ch, victim, dam, spellnum);
  return 0;
}


/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
*/

#define MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, struct char_data * ch, struct char_data * victim,
		      int spellnum, int savetype)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;


  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector[0] = 0;
    af[i].bitvector[1] = 0;
    af[i].bitvector[2] = 0;
    af[i].bitvector[3] = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  switch (spellnum) {

  case SPELL_BURNING_HANDS:
    if (number(1, 30) <= level && !AFF2_FLAGGED(victim, AFF2_PROT_FIRE)
      && !mag_savingthrow(victim, savetype) && !MOB_FLAGGED(victim, MOB_NOBURN)) {
      af[0].duration = 3;
      af[0].bitvector[1] = AFF2_BURNING;
      accum_duration = TRUE;
      to_vict = "You start burning.";
      to_room = "$n starts burning.";
    }
    break;
   

  case SPELL_CHILL_TOUCH:
    af[0].location = APPLY_STR;
    if (mag_savingthrow(victim, savetype))
      af[0].duration = 1;
    else
      af[0].duration = 4;
    af[0].modifier = -1;
    accum_duration = TRUE;
    to_vict = "You feel your strength wither!";
    break;

  case SPELL_AID:
    af[0].location = APPLY_HITROLL;
    af[0].bitvector[1] = AFF2_PROT_COLD | AFF2_PROT_FIRE;
    af[0].modifier = 2 + ((ch == victim));
    af[0].duration = 5 + level / 6;
    break;

  case SPELL_ARMOR:
    af[0].location = APPLY_AC;
    af[0].modifier = -20;
    af[0].duration = 24;
    accum_duration = TRUE;
    to_vict = "You feel someone protecting you.";
    break;
    
  case SPELL_STONESKIN:
    af[0].location = APPLY_AC;
    af[0].modifier = -25;
    af[0].duration = 24;
    af[0].bitvector[0] = AFF_STONESKIN;
    accum_duration = TRUE;
    to_vict = "You feel your skin turn into granite.";
    break;
    
  case SPELL_STEELSKIN:
    af[0].location = APPLY_AC;
    af[0].modifier = -20 - GET_CHA(ch);
    af[0].duration = 24;
    
    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -8;
    af[1].duration = 24;
    
    accum_duration = TRUE;
    to_vict = "You feel your skin as tough as steel.";
    break;

  case SPELL_BLESS:
    af[0].location = APPLY_HITROLL;
    af[0].modifier = 2 + ((ch == victim)*3);
    af[0].duration = 4 + level / 6;

    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -6;
    af[1].duration = 6;

    accum_duration = TRUE;
    to_vict = "You feel righteous.";
    break;

  case SPELL_WEAKEN:
    af[0].location = APPLY_STR;
    af[0].modifier = -2;
    af[0].duration = 4 + level / 8;
    accum_duration = TRUE;
    break;
    
  case SPELL_ANTI_MAGIC:
    if (AFF2_FLAGGED(victim, AFF2_ANTI_MAGIC)) {
      send_to_char(NOEFFECT, ch);
      return;
    }
    
    af[0].location = APPLY_AC;
    af[0].modifier = 20;
    af[0].duration = 12;
    af[0].bitvector[1] = AFF2_ANTI_MAGIC;

    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -25;
    af[1].duration = 12;
    
//    af[2].bitvector[1] = AFF2_ANTI_MAGIC;
//    af[2].duration = 12;

    accum_duration = FALSE;
    to_vict = "Shiny aura surrounds you. You feel protected against magic.";
    break;

  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim,MOB_NOBLIND) || mag_savingthrow(victim, savetype)) {
      send_to_char("You fail.\r\n", ch);
      return;
    }
    af[0].location = APPLY_HITROLL;
    af[0].modifier = -4;
    af[0].duration = 2;
    af[0].bitvector[0] = AFF_BLIND;

    af[1].location = APPLY_AC;
    af[1].modifier = 40;
    af[1].duration = 2;
    af[1].bitvector[0] = AFF_BLIND;

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_CURSE:
    if (mag_savingthrow(victim, savetype)) {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].location = APPLY_HITROLL;
    af[0].duration = 1 + (level / 2);
    af[0].modifier = -1;
    af[0].bitvector[0] = AFF_CURSE;

    af[1].location = APPLY_DAMROLL;
    af[1].duration = 1 + (level / 2);
    af[1].modifier = -1;
    af[1].bitvector[0] = AFF_CURSE;

    accum_duration = TRUE;
    accum_affect = TRUE;
    to_room = "$n briefly glows red!";
    to_vict = "You feel very uncomfortable.";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration = 12 + level;
    af[0].bitvector[0] = AFF_DETECT_ALIGN;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_DETECT_INVIS:
    af[0].duration = 12 + level;
    af[0].bitvector[0] = AFF_DETECT_INVIS;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration = 12 + level;
    af[0].bitvector[0] = AFF_DETECT_MAGIC;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_INFRAVISION:
    af[0].duration = 12 + level;
    af[0].bitvector[0] = AFF_INFRAVISION;
    accum_duration = TRUE;
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_INVISIBLE:
    if (!victim)
      victim = ch;

    af[0].duration = 12 + (level / 4);
    af[0].modifier = -40;
    af[0].location = APPLY_AC;
    af[0].bitvector[0] = AFF_INVISIBLE;
    accum_duration = TRUE;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_POISON:
    if (mag_savingthrow(victim, savetype)) {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].location = APPLY_STR;
    af[0].duration = level;
    af[0].modifier = -2;
    af[0].bitvector[0] = AFF_POISON;
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;
    
  case SPELL_HOLD_PERSON:
    if (mag_savingthrow(victim, savetype)) {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].duration = 6;
    af[0].bitvector[0] = AFF_HOLD;
    to_vict = "You are held.";
    to_room = "$n is held!";
    break;
    
  case SPELL_PARALYZE:
    if (mag_savingthrow(victim, savetype)) {
      send_to_char(NOEFFECT, ch);
      return;
    }
    af[0].location = APPLY_HITROLL;
    af[0].modifier = -3;
    af[0].duration = 4;
    af[0].bitvector[0] = AFF_HOLD;
    to_vict = "You are paralyzed.";
    to_room = "$n is paralyzed!";
    break;

  case SPELL_PROT_FROM_EVIL:
    af[0].duration = 24;
    af[0].bitvector[0] = AFF_PROTECT_EVIL;
    accum_duration = TRUE;
    to_vict = "You feel invulnerable!";
    break;

  case SPELL_SANCTUARY:
    af[0].duration = 4;
    af[0].bitvector[0] = AFF_SANCTUARY;

    accum_duration = TRUE;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    break;

  case SPELL_SLEEP:   /* (FIDO) Changed this from pk_allowed to sleep_allowed */
    if (IS_NPC(ch) && MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;
    if (ENEMY_CLAN(ch, victim) && !sleep_allowed)
      return;
    if (mag_savingthrow(victim, savetype))
      return;

    af[0].duration = 4 + (level / 4);
    af[0].bitvector[0] = AFF_SLEEP;

    if (GET_POS(victim) > POS_SLEEPING) {
      act("You feel very sleepy...  Zzzz......", FALSE, victim, 0, 0, TO_CHAR);
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    break;

  case SPELL_STRENGTH:
    af[0].location = APPLY_STR;
    af[0].duration = (level / 2) + 4;
    af[0].modifier = 1 + (level > 18);
    accum_duration = TRUE;
    accum_affect = TRUE;
    to_vict = "You feel stronger!";
    break;

  case SPELL_SENSE_LIFE:
    to_vict = "Your feel your awareness improve.";
    af[0].duration = level;
    af[0].bitvector[0] = AFF_SENSE_LIFE;
    accum_duration = TRUE;
    break;

  case SPELL_WATERWALK:
    af[0].duration = 24;
    af[0].bitvector[0] = AFF_WATERWALK;
    accum_duration = TRUE;
    to_vict = "You feel webbing between your toes.";
    break;
    
  case SPELL_HASTE:
    af[0].duration = 2;
    af[0].bitvector[0] = AFF_HASTE;
    accum_duration = FALSE;
    to_vict = "You feel yourself moving much faster.";
    break;
    
  case SPELL_SHIELD:
    af[0].duration = 12;
    af[0].bitvector[0] = AFF_SHIELD;
    accum_duration = TRUE;
    to_vict = "A pulsing shield has appeared around you.";
    to_room = "A pulsing shield suddenly surrounds $n.";
    break;
    
  case SPELL_DEATHDANCE:
    af[0].duration = 24;
    af[0].bitvector[0] = AFF_DEATHDANCE;
    to_vict = "You feel your life take on a whole new meaning....";
    to_room = "A wave of death dances forth from $n";
    break;
    
  case SPELL_BLOODLUST:
    af[0].duration = 24;
    af[0].bitvector[1] = AFF2_BLOODLUST;
    af[0].modifier = -5;
    af[0].location = APPLY_AC;
    to_vict = "Blood, blood, BLOOD.....";
    to_room = "$n turns into vampire.";
    break;
 

  case SPELL_FLY:
    af[0].duration = 2 + level / 4;
    af[0].modifier = -10;
    af[0].location = APPLY_AC;
    af[0].bitvector[0] = AFF_FLYING;
    accum_duration = FALSE;
    to_vict = "You take off into the air.";
    to_room = "$n takes off into the air.";
    break;
    
  case SPELL_LEVITATE:
    af[0].duration = 8 + level / 4;
    af[0].bitvector[0] = AFF_WATERWALK;
    af[0].modifier = -5;
    af[0].location = APPLY_AC;
    accum_duration = TRUE;
    to_vict = "You start to float off the ground.";
    to_room = "$n begins to float off the ground.";
    break;

  case SPELL_PROT_FIRE:
    af[0].duration = 10 + level;
    af[0].bitvector[1] = AFF2_PROT_FIRE;
    accum_duration = FALSE;
    to_vict = "You feel a shell of insulation form around your body.";
    break;

  case SPELL_PROT_COLD:
    af[0].duration = 10 + level;
    af[0].bitvector[1] = AFF2_PROT_COLD;
    accum_duration = FALSE;
    to_vict = "You feel a shell of warmth form around your body.";
    break;

  case SPELL_WATERBREATH:
    af[0].duration = 24;
    af[0].bitvector[0] = AFF_WATERBREATH;
    accum_duration = FALSE;
    to_vict = "It feels as if you just sprouted some gills.";
    break;

  case SPELL_CONE_OF_COLD:
    if (number(1, 30) <= level && !AFF2_FLAGGED(victim, AFF2_PROT_FIRE)
      && !MOB_FLAGGED(victim, MOB_NOFREEZE) && !mag_savingthrow(victim, savetype)) {
      af[0].duration = 1;
      af[0].bitvector[1] = AFF2_FREEZING;
      accum_duration = TRUE;
      to_vict = "You are consumed with coldness.";
      to_room = "$n starts shivering.";
    }
    break;

  case SPELL_ACID_ARROW:
    if (number(1, 30) <= level && !MOB_FLAGGED(victim, MOB_NOACID)
      && !mag_savingthrow(victim, savetype)) {
      af[0].duration = 3;
      af[0].bitvector[1] = AFF2_ACIDED;
      accum_duration = TRUE;
      to_vict = "The acid arrow drenches you.";
      to_room = "$n is drenched by an acid arrow.";
    }
    break;
    

  case SPELL_FLAME_ARROW:
    if (number(1, 30) <= level && !AFF2_FLAGGED(victim, AFF2_PROT_FIRE)
      && !MOB_FLAGGED(victim, MOB_NOBURN) && !mag_savingthrow(victim, savetype)) {
      af[0].duration = 3;
      af[0].bitvector[1] = AFF2_BURNING;
      accum_duration = TRUE;
      to_vict = "The flame arrow sets you on fire.";
      to_room = "$n starts burning.";
    }
    break;

  case SPELL_BARKSKIN:
    af[0].location = APPLY_AC;
    af[0].modifier = -20 - (IS_CLERIC_TYPE(ch) * 10);
    af[0].duration = level / 5;
    to_vict = "You feel your skin hardening.";
    break;

  case SPELL_MIRROR_IMAGE:
    af[0].duration = 6;
    af[0].bitvector[0] = AFF_MIRRORIMAGE;
    af[0].location = APPLY_AC;
    af[0].modifier = -10;
    to_vict = "Your image breaks up!";
    to_room = "$n breaks up into many images!!!";
    break;

  case SPELL_BLINK:
    af[0].duration = 3;
    af[0].bitvector[0] = AFF_BLINK;
    to_vict = "You don't feel any different.";
    to_room = "You see $n shift a few feet away.";
    break;

  case SPELL_WRAITHFORM:
    af[0].duration = 8;
    af[0].bitvector[1] = AFF2_PASSDOOR;
    to_vict = "You turn translucent!";
    to_room = "$n turns translucent!";
    break;

  case SPELL_FIRE_BREATH:
    if (number(1, 30) <= level && !AFF2_FLAGGED(victim, AFF2_PROT_FIRE)
      && !MOB_FLAGGED(victim, MOB_NOBURN) && !mag_savingthrow(victim, savetype)) { 
      af[0].duration = 1;
      af[0].bitvector[1] = AFF2_BURNING;
      accum_duration = TRUE;
      to_vict = "You are engulfed in flames.";
      to_room = "$n is engulfed in flames.";
    }
    break;

  case SPELL_BURN:
    af[0].duration = 1;
    af[0].bitvector[1] = AFF2_BURNING;
    accum_duration = TRUE;
    to_vict = "You are engulfed in flames.";
    to_room = "$n is engulfed in flames.";
    break;

  case SPELL_FREEZE:
    af[0].duration = 1;
    af[0].bitvector[1] = AFF2_FREEZING;
    accum_duration = TRUE;
    to_vict = "You are consumed with coldness.";
    to_room = "$n starts shivering.";
    break;

  case SPELL_ACID:
    af[0].duration = 1;
    af[0].bitvector[1] = AFF2_ACIDED;
    accum_duration = TRUE;
    to_vict = "You are drenched in acid.";
    to_room = "$n drops some acid.";
    break;

  case SPELL_CRIT_HIT:
    af[0].duration = 2;
    af[0].bitvector[1] = AFF2_CRIT_HIT;
    accum_duration = FALSE;
    to_vict = "You bleed from a fatal wound.";
    to_room = "$n bleeds from a fatal wound.";
    break;

  case SPELL_ENH_HEAL:
    af[0].duration = 24;
    af[0].bitvector[1] = AFF2_ENH_HEAL;
    to_vict = "You feel much healthier.";
    break;

  case SPELL_ENH_MANA:
    af[0].duration = 24;
    af[0].bitvector[1] = AFF2_ENH_MANA;
    to_vict = "You feel much more intuned to the flow of mana.";
    break;

  case SPELL_ENH_MOVE:
    af[0].duration = 24;
    af[0].bitvector[1] = AFF2_ENH_MOVE;
    to_vict = "You feel much more restful.";
    break;
    
  case SPELL_DISRUPTING_RAY:
    af[0].location = APPLY_AC;
    af[0].modifier = number(7, 12);
    af[0].duration = 4;
    accum_duration = FALSE;
    accum_affect = TRUE;
    to_vict = "You feel yourself less sturdy.";
    to_room = "$n's looks suddenly not very well armored.";
    break;
    
  case SPELL_FIRE_SHIELD:
    af[0].duration = 4;
    af[0].bitvector[1] = AFF2_FIRE_SHIELD;
    to_vict = "You are surrounded by red glow.";
    to_room = "$n is suddenly surrounded by the red glow.";
    break;
  }

  if (spell_info[spellnum].violent && 
      (!CAN_MURDER(ch, victim) || GET_LEVEL(victim) >= LVL_IMMORT)) {
      act("The Mark of Neutrality protects $N.", FALSE, ch, 0, victim, TO_CHAR);
      act("The Mark of Neutrality protects you.", FALSE, ch, 0, victim, TO_VICT);
      return;
  }
  
  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF_FLAGGED(victim, af[i].bitvector[0]) ||
          AFF2_FLAGGED(victim, af[i].bitvector[1]) ||
          AFF3_FLAGGED(victim, af[i].bitvector[2]) ||
          AFF4_FLAGGED(victim, af[i].bitvector[3])) {
	send_to_char(NOEFFECT, ch);
	return;
      }

  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    send_to_char(NOEFFECT, ch);
    return;
  }

  if (AFF2_FLAGGED(victim, AFF2_ANTI_MAGIC)) {
      for (i = 0; i< MAX_SPELL_AFFECTS; i++) {
        af[i].modifier /= 3;
        af[i].duration /= 3;
      }
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector[0] || af[i].bitvector[1] ||
        af[i].bitvector[2] || af[i].bitvector[3] || 
        (af[i].location != APPLY_NONE))
      affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */

void perform_mag_groups(int level, struct char_data * ch,
			struct char_data * tch, int spellnum, int savetype)
{
  switch (spellnum) {
    case SPELL_GROUP_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL, savetype);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
    break;
  case SPELL_GROUP_SHIELD:
    mag_affects(level, ch, tch, SPELL_SHIELD, savetype);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL, 0);
    break;
  case SPELL_GROUP_FLY:
    mag_affects(level, ch, tch, SPELL_FLY, savetype);
    break;
  case SPELL_GROUP_INVIS:
    mag_affects(level, ch, tch, SPELL_INVISIBLE, savetype);
    break;
  case SPELL_GROUP_PROT_EVIL:
    mag_affects(level, ch, tch, SPELL_PROT_FROM_EVIL, savetype);
    break;
  case SPELL_GROUP_WATBREATH:
    mag_affects(level, ch, tch, SPELL_WATERBREATH, savetype);
    break;
  }
}


/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */

void mag_groups(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (tch->in_room != ch->in_room)
      continue;
    if (!AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum, savetype);
  }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
    perform_mag_groups(level, ch, k, spellnum, savetype);
  perform_mag_groups(level, ch, ch, spellnum, savetype);
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented as of Circle 3.0.
 */

void mag_masses(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *tch_next;

  for (tch = world[ch->in_room].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    switch (spellnum) {
    }
  }
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
*/

void mag_areas(int level, struct char_data * ch, int spellnum, int savetype)
{
  struct char_data *tch, *next_tch;
  const char *to_char = NULL, *to_room = NULL;

  if (ch == NULL)
    return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum) {
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room = "$n gracefully gestures and the earth begins to shake violently!";
    break;
  case SPELL_HOLY_SHOUT:
    to_char = "The world shook as you issue holy shout!";
    to_room = "$n opens the holy bible and said the holy word!";
    break;
    
  case SPELL_METEOR_SHOWER:
    to_char = "You call the iron meteors with your magic!";
    to_room = "$n glows and the rain of iron meteors comes suddenly!";
    break;
    
  case SPELL_DEATH_RIPPLE:
    to_char = "Your lips utters the hymn of death!";
    to_room = "$n utters the word of DEATH!";
    break;
    
  case SPELL_DEATH_WAVE:
    to_char = "You sing the hymn of death!";
    to_room = "$n shouts the words of DEATH! Deadly wave destroys all life!";
  break;
    
  case SPELL_FIRE_BREATH:
    to_char = "You snort and fire shoots out of your nostrils!";
    to_room = "$n snorts and a gout of fire shoots out of $s nostrils at you!";
    break;
    
  case SPELL_GAS_BREATH:
    to_char = "You burp and a noxious gas rolls rapidly out of your nostrils!";
    to_room = "$n rumbles and a noxious gas rolls out of $s nostrils!";
    break;
  case SPELL_FROST_BREATH:
    to_char = "You shiver as a shaft of frost leaps from your mouth!";
    to_room = "$n shivers as a shaft of frost leaps from $s mouth!";
    break;
  case SPELL_ACID_BREATH:
    to_char = "Your indigestion acts up and a wash of acid leaps from your mouth!";
    to_room = "$n looks pained as a wash of acid leaps from $s mouth!";
    break;
  case SPELL_LIGHTNING_BREATH:
    to_char = "You open your mouth and bolts of lightning shoot out!";
    to_room = "$n opens $s mouth and bolts of lightning shoot out!";
    break;
    
  case SPELL_CONE_OF_COLD:
    to_room ="$n launches a deadly cone of super-cooled air!";
    to_char ="You aim your cone of cold.";
    break;
    
  case SPELL_BLADEBARRIER:
    to_room ="$n laughs and attempts to put up a BLADE BARRIER!";
    to_char ="You concentrate on swords.  Swords. SWORDS!!!";
    break;
  }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);
  

  for (tch = world[ch->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     * players can only hit players in CRIMEOK rooms 4) players can only hit
     * charmed mobs in CRIMEOK rooms
     */

    if (tch == ch)
      continue;
    if (!IS_NPC(tch) && GET_LEVEL(tch) >= LVL_IMMORT)
      continue;
    /* (FIDO) Changed this from pk_allowed to roomaffect_allowed */
    /* The whole idea being, that you can specify whether room affect spells */
    /* WILL hurt other players or not.  You could even set it up to change */
    /* from room to room. */
    if (!roomaffect_allowed && !IS_NPC(ch) && !IS_NPC(tch))
      continue;
    if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM) &&
      tch->master == ch)
      continue;
    if (!CAN_MURDER(ch, tch))
      continue;
      
    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    mag_damage(level, ch, tch, spellnum, 1);
  }
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {
  "\r\n",
  "$n makes a strange magical gesture; you feel a strong breeze!",
  "$n animates a corpse!",
  "$N appears from a cloud of thick blue smoke!",
  "$N appears from a cloud of thick green smoke!",
  "$N appears from a cloud of thick red smoke!",
  "$N disappears in a thick black cloud!"
  "As $n makes a strange magical gesture, you feel a strong breeze.",
  "As $n makes a strange magical gesture, you feel a searing heat.",
  "As $n makes a strange magical gesture, you feel a sudden chill.",
  "As $n makes a strange magical gesture, you feel the dust swirl.",
  "$n magically divides!",
  "$n animates a corpse!"
};

/*
 * Keep the \r\n because these use send_to_char.
 */
const char *mag_summon_fail_msgs[] = {
  "\r\n",
  "There are no such creatures.\r\n",
  "Uh oh...\r\n",
  "Oh dear.\r\n",
  "Oh shit!\r\n",
  "The elements resist!\r\n",
  "You failed.\r\n",
  "There is no corpse!\r\n"
};

/* These mobiles do not exist. */
#define MOB_MONSUM_I		130
#define MOB_MONSUM_II		140
#define MOB_MONSUM_III		150
#define MOB_GATE_I		160
#define MOB_GATE_II		170
#define MOB_GATE_III		180

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE	20
#define MOB_CLONE		10
#define MOB_ZOMBIE		11
#define MOB_AERIALSERVANT	19
#define MOB_PHANTASM		21
#define MOB_EARTH_ELEM		22
#define MOB_WATER_ELEM		23
#define MOB_AIR_ELEM		24
#define MOB_FIRE_ELEM		25


void mag_summons(int level, struct char_data * ch, struct obj_data * obj,
		      int spellnum, int savetype)
{
  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int	pfail = 0, msg = 0, fmsg = 0, mob_num = 0,
	num = 1, handle_corpse = FALSE, i;

  if (ch == NULL)
    return;

  switch (spellnum) {
  case SPELL_CLONE:
    msg = 10;
    fmsg = number(2, 6);	/* Random fail message. */
    mob_num = MOB_CLONE;
    pfail = 50;	/* 50% failure, should be based on something later. */
    break;

  case SPELL_ANIMATE_DEAD:
    if (obj == NULL || !IS_CORPSE(obj)) {
      act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    handle_corpse = TRUE;
    msg = 11;
    fmsg = number(2, 6);	/* Random fail message. */
    mob_num = MOB_ZOMBIE;
    pfail = 10;	/* 10% failure, should vary in the future. */
    break;
    
  case SPELL_PHANTASM:
    handle_corpse = FALSE;
    msg = 3;
    fmsg = number(2, 6);	/* Random fail message. */
    mob_num = MOB_PHANTASM;
    pfail = 25;	
    break;
    
  case SPELL_EARTH_ELEMENTAL:
    handle_corpse = FALSE;
    msg = 10;
    fmsg = number(2, 6);	/* Random fail message. */
    mob_num = MOB_EARTH_ELEM;
    pfail = 25;	
    break;
  case SPELL_WATER_ELEMENTAL:
    handle_corpse = FALSE;
    msg = 9;
    fmsg = number(2, 6);	/* Random fail message. */
    mob_num = MOB_WATER_ELEM;
    pfail = 25;	
    break;
  case SPELL_AIR_ELEMENTAL:
    handle_corpse = FALSE;
    msg = 7;
    fmsg = number(2, 6);	/* Random fail message. */
    mob_num = MOB_AIR_ELEM;
    pfail = 25;	
    break;
  case SPELL_FIRE_ELEMENTAL:
    handle_corpse = FALSE;
    msg = 8;
    fmsg = number(2, 6);	/* Random fail message. */
    mob_num = MOB_FIRE_ELEM;
    pfail = 25;	
    break;
  default:
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    send_to_char("You are too giddy to have any followers!\r\n", ch);
    return;
  }
  if (number(0, 101) < pfail) {
    send_to_char(mag_summon_fail_msgs[fmsg], ch);
    return;
  }
  for (i = 0; i < num; i++) {
    if (!(mob = read_mobile(mob_num, VIRTUAL))) {
      send_to_char("You don't quite remember how to make that creature.\r\n", ch);
      return;
    }
    char_to_room(mob, ch->in_room);
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
    SET_BIT(MOB_FLAGS(mob), MOB_PET);
    if (spellnum == SPELL_CLONE) {	/* Don't mess up the proto with strcpy. */
      mob->player.name = str_dup(GET_NAME(ch));
      mob->player.short_descr = str_dup(GET_NAME(ch));
    }
    act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
    add_follower(mob, ch);
  }
  if (handle_corpse) {
    for (tobj = obj->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }
}


void mag_points(int level, struct char_data * ch, struct char_data * victim,
		     int spellnum, int savetype)
{
  int hit = 0;
  int move = 0;
  int mana = 0;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_LIGHT:
    hit = dice(1, 8) + 1 + (level / 4);
    send_to_char("You feel better.\r\n", victim);
    break;
  case SPELL_CURE_SERIOUS:
    hit = dice(3, 8) + 3 + level / 2;
    send_to_char("You feel much better!\r\n", victim);    
    break;
  case SPELL_CURE_CRITIC:
    hit = dice(3, 8) + 3 + (level / 4);
    send_to_char("You feel a lot better!\r\n", victim);
    break;
  case SPELL_HEAL:
    hit = 100 + dice(3, 8);
    send_to_char("A warm feeling floods your body.\r\n", victim);
    break;
  case SPELL_MANA_BOOST:
    mana = 100 + dice(3, 8) + level;
    send_to_char("Magical fluid has surrounded you. You feel your powers return.\r\n", victim);
    break;
  case SPELL_REGENERATE:
    move = 100 + dice(3, 8) + level;
    send_to_char("Your vitality has been magically regenerated.\r\n", victim);
    break;
  case SPELL_DEATHDANCE:
    if (GET_LEVEL(victim) < LVL_IMMORT) {
      hit = 0 - (GET_MAX_HIT(victim) / 2);
      move = 0 - (GET_MAX_MOVE(victim) / 4);
    }
    break;
  case SPELL_LIFE_TRANSFER:
    hit = MIN(dice(1, 6) + GET_LEVEL(ch), GET_HIT(ch) - 1);
    GET_HIT(ch) -= hit;
    hit = MAX(0, hit - dice(1, 3));
    send_to_char("You feel woozy as your life essence is drained away.\r\n", ch);
    send_to_char("Your life essence returns.\r\n", victim);
    break;
  case SPELL_MANA_TRANSFER:
    mana = MIN(dice(1, 6) + GET_LEVEL(ch), GET_MANA(ch) - 1);
    GET_MANA(ch) -= mana;
    mana = MAX(0, mana - dice(1, 3));
    send_to_char("Your head aches as your magic flows away.\r\n", ch);
    send_to_char("You feel a strong flow of magical energy.\r\n", victim);
    break;
  }
  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + hit);
  GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
  GET_MANA(victim) = MIN(GET_MAX_MANA(victim), GET_MANA(victim) + mana);
  //alter_hit(victim, -hit);
  //alter_move(victim, -move);
  update_pos(victim);
}


void mag_unaffects(int level, struct char_data * ch, struct char_data * victim,
		        int spellnum, int type)
{
  int spell = 0;
  const char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_BLIND:
  case SPELL_HEAL:
    spell = SPELL_BLINDNESS;
    to_vict = "Your vision returns!";
    to_room = "There's a momentary gleam in $n's eyes.";
    break;
  case SPELL_REMOVE_POISON:
    spell = SPELL_POISON;
    to_vict = "A warm feeling runs through your body!";
    to_room = "$n looks better.";
    break;
  case SPELL_REMOVE_CURSE:
    spell = SPELL_CURSE;
    to_vict = "You don't feel so unlucky.";
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  if (!affected_by_spell(victim, spell) && spellnum != SPELL_HEAL) {
    send_to_char(NOEFFECT, ch);
    return;
  }

  affect_from_char(victim, spell);
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
  //check_regen_rates(victim);
}


void mag_alter_objs(int level, struct char_data * ch, struct obj_data * obj,
		         int spellnum, int savetype)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum) {
    case SPELL_BLESS:
      if (!IS_OBJ_STAT(obj, ITEM_BLESS) &&
	  (GET_OBJ_WEIGHT(obj) <= 5 * level)) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly.";
      }
      break;
    case SPELL_CURSE:
      if (!IS_OBJ_STAT(obj, ITEM_NODROP)) {
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	  GET_OBJ_VAL(obj, 2)--;
	to_char = "$p briefly glows red.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!IS_OBJ_STAT(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
        SET_BIT(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3)) {
      GET_OBJ_VAL(obj, 3) = 1;
      to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
        REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          GET_OBJ_VAL(obj, 2)++;
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_REMOVE_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3)) {
        GET_OBJ_VAL(obj, 3) = 0;
        to_char = "$p steams briefly.";
      }
      break;
  }

  if (to_char == NULL)
    send_to_char(NOEFFECT, ch);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);

}



void mag_creations(int level, struct char_data * ch, int spellnum)
{
  struct obj_data *tobj;
  int z;

  if (ch == NULL)
    return;
  level = MAX(MIN(level, LVL_IMPL), 1);

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10;
    break;
  default:
    send_to_char("Spell unimplemented, it would seem.\r\n", ch);
    return;
  }

  if (!(tobj = read_object(z, VIRTUAL))) {
    send_to_char("I seem to have goofed.\r\n", ch);
    log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    return;
  }
  obj_to_char(tobj, ch);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
}

void mag_room(int level, struct char_data * ch, int spellnum)
{
	long aff; /* what affection */
	int ticks; /* how many ticks this spell lasts */
	char *to_char = NULL;
	char *to_room = NULL;
	struct raff_node *raff;

	extern struct raff_node *raff_list;

	aff = ticks =0;

	if (ch == NULL)
		return;
	level = MAX(MIN(level, LVL_IMPL), 1);

	switch (spellnum) {
	case SPELL_WALL_OF_FOG:
		to_char = "You create a fog out of nowhere.";
		to_room = "$n creates a fog out of nowhere.";
		aff = RAFF_FOG;
		ticks = 4; /* this spell lasts one ticks */
		break;

	/* add more room spells here */

	default:
   	  sprintf(buf, "SYSERR: unknown spellnum %d " \
		"passed to mag_unaffects", spellnum);
	  log(buf);
	  break;
	}

	if ((aff & ROOM_AFFECTIONS(ch->in_room)) == aff) {
          send_to_char(NOEFFECT, ch);
          return;
        }

	/* create, initialize, and link a room-affection node */
	CREATE(raff, struct raff_node, 1);
	raff->room = ch->in_room;
	raff->timer = ticks;
	raff->affection = aff;
	raff->spell = spellnum;
	raff->next = raff_list;
	raff_list = raff;

        
	/* set the affection */
	SET_BIT(ROOM_AFFECTIONS(raff->room), aff);

	if (to_char == NULL)
		send_to_char(NOEFFECT, ch);
	else
		act(to_char, TRUE, ch, 0, 0, TO_CHAR);

	if (to_room != NULL)
		act(to_room, TRUE, ch, 0, 0, TO_ROOM);
	else if (to_char != NULL)
		act(to_char, TRUE, ch, 0, 0, TO_ROOM);
}
