/* 
************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "interpreter.h"
#include "constants.h"
#include "clan.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;

extern struct index_data *obj_index;
extern sh_int r_mortal_start_room;
extern int top_of_world;
extern char *spells[];

extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;

extern struct default_mobile_stats *mob_defaults;
extern struct apply_mod_defaults *apmd;
extern HOMETOWN hometowns[];
extern int max_npc_corpse_time, max_pc_corpse_time;

void clearMemory(struct char_data * ch);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_savingthrow(struct char_data * ch, int type);
void name_to_drinkcon(struct obj_data * obj, int type);
void name_from_drinkcon(struct obj_data * obj);
void weather_change(void);

/* Strings for arcane spells */
char *arc[5][2][11] = {
 { /* Ten Thousands */
  {"ba", "bo", "di", "fo", "gu", "hy", "ja", "ke", "li", "mu", "\n"},
  {"ny", "cu", "qe", "ri", "so", "tu", "vy", "wa", "xe", "zi", "\n"},
 },
 { /* Thousands */
 {"b", "c", "d", "f", "g", "h", "j", "k", "l", "m", "\n"},
 {"n", "p", "q", "r", "s", "t", "v", "w", "x", "z", "\n"},
 },
 { /* Hundreds */ 
 {"ab", "ae", "ai", "ao", "au", "ay", "ea", "ec", "ei", "eo", "\n"},
 {"oi", "of", "ou", "oy", "ac", "ec", "ed", "ug", "uz", "uq", "\n"},
 },
 { /* Tens */
 {"z", "c", "x", "f", "w", "h", "v", "k", "s", "m", "\n"},
 {"n", "b", "q", "d", "k", "t", "j", "g", "r", "p", "\n"},
 },
 { /* Ones */
 {"aq", "as", "av", "au", "an", "ef", "eg", "et", "en", "ep", "\n"},
 {"re", "ru", "us", "ut", "at", "ur", "yr", "yr", "er", "ea", "\n"},
 }
};

#define CAN_SUMMON(from, to)	((!ROOM_FLAGGED(from, ROOM_NO_SUMMON) && \
      !ROOM_FLAGGED(to, ROOM_NO_SUMMON | ROOM_TUNNEL | ROOM_GODROOM) | \
      ROOM_DEATH | ROOM_PRIVATE) || \
      GET_LEVEL(ch) >= LVL_GOD)


/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
  level = MAX(MIN(level, LVL_IMPL), 1);

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ASPELL(spell_enchant_weapon)
{
  int i;

  if (ch == NULL || obj == NULL)
    return;

  if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
      !OBJ_FLAGGED(obj, ITEM_MAGIC)) {

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
	return;

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 1 + (level >= 18);

    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 1 + (level >= 20);

    if (IS_GOOD(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
    } else if (IS_EVIL(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}


ASPELL(spell_recall)
{
  int load_room;
  if (victim == NULL || IS_NPC(victim))
    return;

  if (ROOM_FLAGGED(victim->in_room, ROOM_NO_SUMMON) && GET_LEVEL(victim) < LVL_GOD) {
        act("Your spell is broken by divine forces.",
        FALSE, ch, 0, victim, TO_CHAR);
        return;
    }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  if (GET_HOME(ch) < 0 || GET_HOME(ch) > NUM_HOMETOWNS) {
        log("SYSERR: spell_recall: Invalid hometown for %s", GET_NAME(ch));
        load_room = r_mortal_start_room;
      } else {
	int index = GET_HOME(ch);
        load_room = real_room(hometowns[index].magic_room);
      }
  char_to_room(victim, load_room);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}


ASPELL(spell_teleport)
{
  int to_room;

  if (victim == NULL || IS_NPC(victim))
    return;

  do {
    to_room = number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_TUNNEL));

  act("$n slowly fades out of existence and is gone.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
  if (ch == NULL || victim == NULL)
    return;

  level = GET_LEVEL(ch);
  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  /* (FIDO) Changed from pk_allowed to summon_allowed */
  
  if (GET_LEVEL(ch) < LVL_IMMORT) {
    if (ENEMY_CLAN(ch, victim) && !summon_allowed) {
      send_to_char("You fail, because Gods disallow summoning enemies!\r\n", ch);
      return;
    }
    else if (MOB_FLAGGED(victim, MOB_AGGRESSIVE) && level < LVL_GOD) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!CAN_SUMMON(victim->in_room, ch->in_room)) {
        act("You failed summoning $N because you spell is broken by divine forces.",
        FALSE, ch, 0, victim, TO_CHAR);
        return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      sprintf(buf, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      GET_NAME(ch), world[ch->in_room].name,
	      (ch->player.sex == SEX_MALE) ? "He" : "She");
      send_to_char(buf, victim);

      sprintf(buf, "You failed because %s has summon protection on.\r\n",
	      GET_NAME(victim));
      send_to_char(buf, ch);

      sprintf(buf, "%s failed summoning %s to %s.",
	      GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      mudlog(buf, BRF, LVL_IMMORT, TRUE);
      return;
    }
  

    if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
      (IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL))) {
      send_to_char(SUMMON_FAIL, ch);
      return;
    }
  }

  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, ch->in_room);

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
}



ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  /*
   * FIXME: This is broken.  The spell parser routines took the argument
   * the player gave to the spell and located an object with that keyword.
   * Since we're passed the object and not the keyword we can only guess
   * at what the player originally meant to search for. -gg
   */
  strcpy(name, fname(obj->name));
  j = level / 2;

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname(name, i->name))
      continue;

    if (i->carried_by)
      sprintf(buf, "%s is being carried by %s.\r\n",
	      i->short_description, PERS(i->carried_by, ch));
    else if (i->in_room != NOWHERE)
      sprintf(buf, "%s is in %s.\r\n", i->short_description,
	      world[i->in_room].name);
    else if (i->in_obj)
      sprintf(buf, "%s is in %s.\r\n", i->short_description,
	      i->in_obj->short_description);
    else if (i->worn_by)
      sprintf(buf, "%s is being worn by %s.\r\n",
	      i->short_description, PERS(i->worn_by, ch));
    else
      sprintf(buf, "%s's location is uncertain.\r\n",
	      i->short_description);

    CAP(buf);
    send_to_char(buf, ch);
    j--;
  }

  if (j == level / 2)
    send_to_char("You sense nothing.\r\n", ch);
}



ASPELL(spell_charm)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if (victim == ch)
    send_to_char("You like yourself even better!\r\n", ch);
  else if (!IS_NPC(victim) && (!ENEMY_CLAN(ch, victim) || !charm_allowed))
    {
      send_to_char("You fail!\r\n", ch);
    }
  else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
    send_to_char("Your victim is protected by sanctuary!\r\n", ch);
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char("Your victim resists!\r\n", ch);
  else if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char("You can't have any followers of your own!\r\n", ch);
  else if (AFF_FLAGGED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char("You fail.\r\n", ch);
  /* player charming another player - no legal reason for this */
  /* (FIDO) Changed from pk_allowed to charm_allowed */
  else if (!charm_allowed && !IS_NPC(victim))
    send_to_char("You fail - shouldn't be doing it anyway.\r\n", ch);
  else if (circle_follow(victim, ch))
    send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);
  else if (mag_savingthrow(victim, SAVING_PARA))
    send_to_char("Your victim resists!\r\n", ch);
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    af.type = SPELL_CHARM;

    if (GET_INT(victim))
      af.duration = 24 * 18 / GET_INT(victim);
    else
      af.duration = 24 * 18;

    af.modifier = 0;
    af.location = 0;
    af.bitvector[0] = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim)) {
      REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
    }
  }
}



ASPELL(spell_identify)
{
  int i;
  int found;

  if (obj) {
    send_to_char("You feel informed:\r\n", ch);
    sprintf(buf, "Object '%s', Item type: ", obj->short_description);
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    if (obj->obj_flags.bitvector) {
      send_to_char("Item will give you following abilities:  ", ch);
      sprintbit(obj->obj_flags.bitvector[0], affected_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
	    GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
    send_to_char(buf, ch);

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

      if (GET_OBJ_VAL(obj, 1) >= 1)
	sprintf(buf + strlen(buf), " %s", spells[GET_OBJ_VAL(obj, 1)]);
      if (GET_OBJ_VAL(obj, 2) >= 1)
	sprintf(buf + strlen(buf), " %s", spells[GET_OBJ_VAL(obj, 2)]);
      if (GET_OBJ_VAL(obj, 3) >= 1)
	sprintf(buf + strlen(buf), " %s", spells[GET_OBJ_VAL(obj, 3)]);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
      sprintf(buf + strlen(buf), " %s\r\n", spells[GET_OBJ_VAL(obj, 3)]);
      sprintf(buf + strlen(buf), "It has %d maximum charge%s and %d remaining.\r\n",
	      GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
	      GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_THROW:
    case ITEM_ROCK:
    case ITEM_BOLT:
    case ITEM_ARROW:
      sprintf(buf+strlen(buf), "Damage dice: %dD%d\r\n",
         GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_GRENADE:
      sprintf(buf+strlen(buf), "Timer: %d Damage dice: %dD%d\r\n",
          GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_BOW:
    case ITEM_CROSSBOW:
    case ITEM_SLING:
      sprintf(buf+strlen(buf), "Range: %d\r\n", GET_OBJ_VAL(obj, 1));
      send_to_char(buf, ch);
      break;
    case ITEM_WEAPON:
      sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
	      GET_OBJ_VAL(obj, 2));
      sprintf(buf + strlen(buf), " for an average per-round damage of %.1f.\r\n",
	      (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
      send_to_char(buf, ch);
      break;
    case ITEM_ARMOR:
      sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
	  (obj->affected[i].modifier != 0)) {
	if (!found) {
	  send_to_char("Can affect you as :\r\n", ch);
	  found = TRUE;
	}
	sprinttype(obj->affected[i].location, apply_types, buf2);
	sprintf(buf, "   Affects: %s By %d\r\n", buf2, obj->affected[i].modifier);
	send_to_char(buf, ch);
      }
    }
  } else if (victim) {		/* victim */
    sprintf(buf, "Name: %s\r\n", GET_NAME(victim));
    send_to_char(buf, ch);
    if (!IS_NPC(victim)) {
      sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
	      GET_NAME(victim), age(victim)->year, age(victim)->month,
	      age(victim)->day, age(victim)->hours);
      send_to_char(buf, ch);
    }
    sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
	    GET_HEIGHT(victim), GET_WEIGHT(victim));
    sprintf(buf + strlen(buf), "Level: %d, Hits: %d, Mana: %d\r\n",
	    GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
    sprintf(buf + strlen(buf), "AC: %d, Hitroll: %d, Damroll: %d\r\n",
	    GET_AC(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
    sprintf(buf + strlen(buf), "Str: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
	GET_STR(victim), GET_ADD(victim), GET_INT(victim),
	GET_WIS(victim), GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
    send_to_char(buf, ch);

  }
}


/*
ACMD(do_enchant_weapon)
{
  struct obj_data *obj;
  int level;
  int i;

  if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh?!?\r\n",ch);
    return;
}

  if (GET_POWER18(ch) == !POWER_ENCHANT) {
  send_to_char("You do not know how to enchant weapons yet!\r\n",ch);
   return;
} else
  if (GET_POWER18(ch) == POWER_ENCHANT) {

  if (ch == NULL || obj == NULL)
    return;

  if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
      !OBJ_FLAGGED(obj, ITEM_MAGIC)) {

  if (GET_OBJ_TYPE(obj) == !ITEM_WEAPON) {
    send_to_char("A weapon stupid!\r\n",ch);
   return;
}
    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
	return;

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 1 + (level >= 18);

    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 1 + (level >= 20);

    if (IS_GOOD(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
    } else if (IS_EVIL(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
   }
  }
 }
}
*/
ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (AFF_FLAGGED(victim, AFF_POISON))
        send_to_char("You can sense poison in your blood.\r\n", ch);
      else
        send_to_char("You feel healthy.\r\n", ch);
    } else {
      if (AFF_FLAGGED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char("You sense that it should not be consumed.\r\n", ch);
    }
  }
}

ASPELL(spell_disintegrate)
{
   struct obj_data *foolz_objs;
   int save, i;

   if (ch == NULL)
	return;
   if (obj) {
    /* Used on my mud 
    if (GET_OBJ_EXTRA(obj) == ITEM_IMMORT && GET_LEVEL(ch) < LVL_IMMORT) {
	send_to_char("Your mortal magic fails.\r\n", ch);
	return;
    }
    */
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
	save = 19;
	break;
    case ITEM_SCROLL:
	save = 20;
	break;
    case ITEM_STAFF:
    /*
    case ITEM_ROD:
    */
    case ITEM_WAND:
	save = 19;
	break;
    case ITEM_WEAPON:
	save = 18;
	break;
    case ITEM_MISSILE:
	save = 20;
	break;
    case ITEM_ARMOR:
	save = 16;
	break;
    case ITEM_WORN:
	save = 18;
	break;
    /*
    case ITEM_SPELLBOOK:
	save = 15;
	break;
    case ITEM_PORTAL:
	save = 13;
	break;
    */
    default:
	save = 19;
	break;
    }
    /*  Save modified by affect on weapons..this is kinda based
     *  on +5 or so being high
     */
    if (GET_OBJ_EXTRA(obj) == ITEM_MAGIC && GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
      for (i = 0; i < MAX_OBJ_AFFECT; i++) {
	  if (obj->affected[i].location == APPLY_DAMROLL)
		save -= obj->affected[i].modifier;
	}
    }
    /* A bonus for ac affecting items also */
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
	if (obj->affected[i].location == APPLY_AC)
   	   save -= obj->affected[i].modifier / 10;
    }
    if (number(1, 20) < save) {
	act("$n disintegrates $p.", FALSE, ch, obj, 0, TO_NOTVICT);
	act("You disintegrate $p.", FALSE, ch, obj, 0, TO_CHAR);
	extract_obj(obj);
	return;
    }
    else {
	act("You fail to disintegrate $p.", FALSE, ch, obj, 0, TO_CHAR);
	return;
    }
   }
   if (victim) {

    if (GET_LEVEL(victim) >= LVL_IMMORT) {
	send_to_char("Nice try..\r\n", ch);
        return;
    }

    /* Note this is extremely powerful, therefore im giving it 2 saves */
    if (mag_savingthrow(victim, 1)) {
	act("You resist $n's attempt to disintegrate you.", FALSE, ch, 0, 0, TO_VICT);
	if (IS_NPC(victim))
	    hit(victim, ch, TYPE_UNDEFINED);
	return;
    }
    if (mag_savingthrow(victim, 1)) {
       	act("You resist $n's attempt to disintegrate you.", FALSE, ch, 0, 0, TO_VICT);
	if (IS_NPC(victim))
            hit(victim, ch, TYPE_UNDEFINED);
	return;
    }
    /*
    if (victim->desc) {
	close_socket(victim->desc);
	victim->desc = NULL;
    }
    */	
    /* Note this is for disintegrating all items the vict carries also..
       may wish to comment this off if you feel it too powerful */
    for (i = 0; i < NUM_WEARS; i++) 
	if (GET_EQ(victim, i))
	  unequip_char(victim, i);
    while (victim->carrying) {
	foolz_objs = victim->carrying;
	extract_obj(foolz_objs);
    }
				
    extract_char(victim);
    act("$n disintegrates $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You disintegrate $N!", FALSE, ch, 0, victim, TO_CHAR);

    return;
  }
  else
    return;
}

#define PORTAL 31

ASPELL(spell_portal)
{
  /* create a magic portal */
  struct obj_data *tmp_obj, *tmp_obj2;
  struct extra_descr_data *ed;
  struct room_data *rp, *nrp;
  struct char_data *tmp_ch = (struct char_data *) victim;
  char buf[512];

  assert(ch);
  assert((level >= 0) && (level <= LVL_IMPL));


  /*
    check target room for legality.
   */
  rp = &world[ch->in_room];
  tmp_obj = read_object(PORTAL, VIRTUAL);
  if (!rp || !tmp_obj || GET_LEVEL(victim)-GET_LEVEL(ch) > 8) {
    send_to_char("The magic fails\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (IS_SET(rp->room_flags[0], ROOM_NOMAGIC)) {
    send_to_char("Eldritch wizardry obstructs thee.\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (IS_SET(rp->room_flags[0], ROOM_TUNNEL)) {
    send_to_char("There is no room in here to summon!\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (!(nrp = &world[tmp_ch->in_room])) {
    char str[180];
    sprintf(str, "%s not in any room", GET_NAME(tmp_ch));
    log(str);
    send_to_char("The magic cannot locate the target\n", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (ROOM_FLAGGED(tmp_ch->in_room, ROOM_NOMAGIC)) {
    send_to_char("Your target is protected against your magic.\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }
  
  if (!CAN_SUMMON(tmp_ch->in_room, ch->in_room)) {
      send_to_char("Your magic is broken by the divine forces.\r\n", ch);
      extract_obj(tmp_obj);
      return;
  }

  sprintf(buf, "Through the mists of the portal, you can faintly see %s", nrp->name);

  CREATE(ed , struct extra_descr_data, 1);
  ed->next = tmp_obj->ex_description;
  tmp_obj->ex_description = ed;
  CREATE(ed->keyword, char, strlen(tmp_obj->name) + 1);
  strcpy(ed->keyword, tmp_obj->name);
  ed->description = str_dup(buf);

  tmp_obj->obj_flags.value[0] = level/5;
  tmp_obj->obj_flags.value[1] = tmp_ch->in_room;

  obj_to_room(tmp_obj,ch->in_room);

  act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_ROOM);
  act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_CHAR);

/* Portal at other side */
   rp = &world[ch->in_room];
   tmp_obj2 = read_object(PORTAL, VIRTUAL);
   if (!rp || !tmp_obj2) {
     send_to_char("The magic fails\n\r", ch);
     extract_obj(tmp_obj2);
     return;
   }
  sprintf(buf, 
"Through the mists of the portal, you can faintly see %s", rp->name);

  CREATE(ed , struct extra_descr_data, 1);
  ed->next = tmp_obj2->ex_description;
  tmp_obj2->ex_description = ed;
  CREATE(ed->keyword, char, strlen(tmp_obj2->name) + 1);
  strcpy(ed->keyword, tmp_obj2->name);
  ed->description = str_dup(buf);

  tmp_obj2->obj_flags.value[0] = level/5;
  tmp_obj2->obj_flags.value[1] = ch->in_room;

  obj_to_room(tmp_obj2,tmp_ch->in_room);

  act("$p suddenly appears.",TRUE,tmp_ch,tmp_obj2,0,TO_ROOM);
  act("$p suddenly appears.",TRUE,tmp_ch,tmp_obj2,0,TO_CHAR);

}

ASPELL(spell_fear)
{
  struct char_data *target = (struct char_data *) victim;
  struct char_data *next_target;
  int rooms_to_flee = 0;

  ACMD(do_flee);

  if (ch == NULL)
        return;

  send_to_char("You radiate an aura of fear into the room!\r\n", ch);
  act("$n is surrounded by an aura of fear!", TRUE, ch, 0, 0, TO_ROOM);

  for (target = world[ch->in_room].people; target; target = next_target) {
        next_target = target->next_in_room;

        if (target == NULL) 
	       return;
		
	if (target == ch)
	       continue;
		
	if (GET_LEVEL(target) >= LVL_IMMORT)
	       continue;
		
	if (mag_savingthrow(target, 1)) {
	       sprintf(buf, "%s is unaffected by the fear!\r\n", GET_NAME(target));
	       act(buf, TRUE, ch, 0, 0, TO_ROOM);
	       send_to_char("Your victim is not afraid of the likes of you!\r\n", ch);
	       if (IS_NPC(target))
		 hit(target, ch, TYPE_UNDEFINED);
	       }
               else {
		 for(rooms_to_flee = level / 10; rooms_to_flee > 0; rooms_to_flee--) {
		    send_to_char("You flee in terror!\r\n", target);
		    do_flee(target, "", 0, 0);
		 }
	       }
	}
}

ASPELL(spell_minor_identify)
{
 char *min_id[] = {
  "%sIt would bring %s if you sold it!\r\n",            /* NONE */
  "your physique!\r\n",                                 /* STR */
  "your speed!\r\n",                                    /* DEX */
  "your thoughts!\r\n",                                 /* INT */
  "your knowledge of things!\r\n",                      /* WIS */
  "your endurance!\r\n",                                /* CON */
  "your appearance!\r\n",                               /* CHA */
  "!CLASS!\r\n",                                        /* CLASS */
  "!LEVEL!\r\n",                                        /* LEVEL */
  "the time!\r\n",                                      /* AGE */
  "your burden of life!\r\n",                           /* WEIGHT */
  "your vertical stance!\r\n",                          /* HEIGHT */
  "your magical aura!\r\n",                             /* MANA */
  "your physical resistance!\r\n",                      /* HIT */
  "your ability to move!\r\n",                          /* MOVE */
  "your wealth!\r\n",                                   /* GOLD */
  "your conscious perception!\r\n",                     /* EXP */
  "your armor!\r\n",                                    /* AC */
  "the way you hit!\r\n",                               /* HITROLL */
  "the damage you give!\r\n",                           /* DAMROLL */
  "your ability to withstand paralysis!\r\n",           /* PARA */
  "your ability to withstand rod attacks!\r\n",         /* ROD */
  "your ability to withstand petrification!\r\n",       /* PETRI */
  "your ability to withstand breath Attacks!\r\n",      /* BREATH */
  "your ability to withstand spells!\r\n",              /* SPELL */
  "!RACES!\r\n"                                         /* RACE */
  "\n"};

 int cost = GET_OBJ_COST(obj), i, x, mes_get = 0;
 bool found = FALSE, sent = FALSE;
 
 if (cost == 0) cost = number(1, 1000);

 if (!obj->affected[0].modifier) {
  sprintf(buf, "%s cannot help you in any special way.\r\nBut it might \
    bring %s if you sold it.\r\n", obj->short_description, money_desc(cost));
  send_to_char(buf, ch);
  return;
 }
 sprintf(buf, "%s can help you in the following way:\r\n", obj->short_description);
 for (i = 0; i < MAX_OBJ_AFFECT; i++)
   if (obj->affected[i].modifier) {
     if (number(0, 20) > GET_INT(ch) && GET_LEVEL(ch) < LVL_IMMORT)
     continue;
 
  switch(obj->affected[i].location) {
   case APPLY_NONE:
/*   case APPLY_RACE: */
   case APPLY_CLASS:
   case APPLY_LEVEL:
    if (!found) {
     sprintf(buf, min_id[0], buf, money_desc(cost));
     found = TRUE;
     sent = TRUE;
    }
   break;
   default:
    mes_get = obj->affected[i].location;
    x = number(0, 1);
    sprintf(buf, "%s%s%s", buf, (x ? "it might do something about ":
        "It could do something about "), min_id[mes_get]);
    sent = TRUE;
   break;
  }
 }


 if (!sent) {
  sprintf(buf, "It seems to you that %s cannot help you in any special way.\r\n", obj->short_description);
  send_to_char(buf, ch);
  return;
 }

 send_to_char(buf, ch);

}

ASPELL(spell_recharge)
{
 int restored_charges = 0, explode = 0;

 if (ch == NULL || obj == NULL)
   return;

 /* This is on my mud, comment off on yours */
/* if (GET_OBJ_EXTRA(obj) == ITEM_NO_RECHARGE) {
   send_to_char("This item cannot be recharged.\r\n", ch);
   return;
 }*/
	
 if (GET_OBJ_TYPE(obj) == ITEM_WAND) {
   if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
      send_to_char("You attempt to recharge the wand.\r\n", ch);
      restored_charges = number(1, 5);
      GET_OBJ_VAL(obj, 2) += restored_charges;
      if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1)) {
	send_to_char("The wand is overcharged and explodes!\r\n", ch);
	sprintf(buf, "%s overcharges %s and it explodes!\r\n", 
	        GET_NAME(ch), obj->name);
	act(buf, TRUE, 0, 0, 0, TO_NOTVICT);
	explode = dice(GET_OBJ_VAL(obj, 2), 2);
	GET_HIT(ch) -= explode;
	update_pos(ch);
	extract_obj(obj);
	return;
      }
      else {
	sprintf(buf, "You restore %d charges to the wand.\r\n", restored_charges);
	send_to_char(buf, ch);
	return;
      }
   }
   else {
     send_to_char("That item is already at full charges!\r\n", ch);
     return;
   }
  }
  else if (GET_OBJ_TYPE(obj) == ITEM_STAFF) {
    if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1)) {
	send_to_char("You attempt to recharge the staff.\r\n", ch);
                      restored_charges = number(1, 3);
	GET_OBJ_VAL(obj, 2) += restored_charges;
	if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1)) {
	  send_to_char("The staff is overcharged and explodes!\r\n", ch);
	  sprintf(buf, "%s overcharges %s and it explodes!\r\n", 
			GET_NAME(ch), obj->name);
	  act(buf, TRUE, 0, 0, 0, TO_NOTVICT);
	  explode = dice(GET_OBJ_VAL(obj, 2), 3);
	  GET_HIT(ch) -= explode;
	  update_pos(ch);
	  extract_obj(obj);
	  return;
	}
	else {
	  sprintf(buf, "You restore %d charges to the staff.\r\n", restored_charges);
	  send_to_char(buf, ch);
	  return;
	}
     }
     else {
	send_to_char("That item is already at full charges!\r\n", ch);
	return;
     }
   }
}

void fchar_init_flags_room(struct char_data *ch, int *numtargets)
{
  struct char_data *vict;

  for (vict = world[ch->in_room].people; (vict); vict = vict->next_in_room) {
     if (vict == ch || (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) ||
       (!IS_NPC(ch) && !IS_NPC(vict) && !pk_allowed))
       SET_BIT(INTERNAL(vict), INT_MARK);
     else {
       REMOVE_BIT(INTERNAL(vict), INT_MARK);
       (*numtargets)++;
     }
   }
}

struct char_data *fchar_next(struct char_data *ch, int *numtargets)
{
  int i ;
  struct char_data *vict;
  
  i = number(0, *numtargets-1);
  if (*numtargets <= 0) return NULL;
  for (vict = world[ch->in_room].people; (vict); ) {
    if (i == 0) {
      SET_BIT(INTERNAL(vict), INT_MARK);
      (*numtargets)--;
      return vict;
    }
    do {
      vict = vict->next_in_room;
    } while (vict && IS_SET(INTERNAL(vict), INT_MARK));
  }
  return NULL;
}

ASPELL(spell_chain_lightning)
{
   int dam;
   struct char_data *v[4] = {NULL, NULL, NULL, NULL};
   
   

struct char_data* FindNext() {
   int inx,j,k, numch;
   struct char_data *vict;
   
   numch = 0;
   for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
     for (j = 0; j<=3; j++) if (v[j] == vict) {numch--;break;}
     if (vict != ch && !(!pk_allowed && !IS_NPC(vict) && !IS_NPC(ch))
     && (IS_NPC(vict) || GET_LEVEL(ch) < LVL_IMMORT)) numch++;
   }
   if (!numch) return NULL;
    
   inx = number(1, numch);
   k = 0;
   for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
     for (j = 0; j<=3; j++) if (v[j] == vict) {k--;break;}
     if (vict != ch && !(!pk_allowed && !IS_NPC(vict) && !IS_NPC(ch))
     && (IS_NPC(vict) || GET_LEVEL(ch) < LVL_IMMORT)) k++;
     if (k == inx) return vict;
   }
   return NULL;
}
   
   
   if (ch == NULL)
	return;	

   if (victim) {

    if (GET_LEVEL(victim) >= LVL_IMMORT) {
	send_to_char("You fool...\r\n", ch);
        return;
    }

    if (!IS_NPC(ch) && !IS_NPC(victim) && !pk_allowed) {
        send_to_char("You rather shouldn't do this...\r\n", ch);
        return;    
    }
    
    v[0] = victim;
    act("Your chain lightning strikes $N with full power!", FALSE, ch, 0, v[0], TO_CHAR);
    act("$N's chain lightning strikes you with full power!", FALSE, v[0], 0, ch, TO_CHAR);
    act("$n's chain lightning strikes $N with full power!", FALSE, ch, 0, v[0], TO_NOTVICT);
    dam = dice(7, 8+ GET_LEVEL(ch) / 9) + 8;
    if (mag_savingthrow(v[0], 1)) dam = dam / 2;
    damage(ch, v[0], dam, TYPE_UNDEFINED); 

    v[1] = FindNext();
    if (!v[1]) return;
    act("Your chain lightning strikes $N with half power!", FALSE, ch, 0, v[1], TO_CHAR);
    act("$N's chain lightning strikes you with half power!", FALSE, v[1], 0, ch, TO_CHAR);
    act("$n's chain lightning strikes $N with half power!", FALSE, ch, 0, v[1], TO_NOTVICT);
    dam = dam / 2;
    if (mag_savingthrow(v[1], 1)) dam = dam / 2;
    damage(ch, v[1], dam, TYPE_UNDEFINED); 
    
    v[2] = FindNext();
    if (!v[2]) return;
    act("Your chain lightning strikes $N with quarter power!", FALSE, ch, 0, v[2], TO_CHAR);
    act("$N's chain lightning strikes you with quarter power!", FALSE, v[2], 0, ch, TO_CHAR);
    act("$n's chain lightning strikes $N with quarter power!", FALSE, ch, 0, v[2], TO_NOTVICT);
    dam = dam / 2;
    if (mag_savingthrow(v[2], 1)) dam = dam / 2;
    damage(ch, v[2], dam, TYPE_UNDEFINED); 
  }  
  else
    return;
}

ASPELL(spell_dispel_magic)
{
  int i, found;
  
  found = FALSE;
  for (i = 1; i<=MAX_SPELLS; i++) 
    if (affected_by_spell(victim, i)) {
      affect_from_char(victim, i);
      found = TRUE;
    }
  if (found) {
    if (victim != ch)
      act("$N breaks the magic affecting you.", FALSE, victim, 0, ch, TO_CHAR);
    act("$N breaks the magic filling the air.", FALSE, ch, 0, victim, TO_NOTVICT);
    send_to_char("All magic has been removed.\r\n", ch);
  }  else send_to_char(NOEFFECT, ch);
  return;
} 

ASPELL(spell_peace)
{
   struct char_data *temp;

   act("$n tries to stop the fight.", TRUE, ch, 0, 0, TO_ROOM);
   act("You try to stop the fight.", FALSE, ch, 0, 0, TO_CHAR);

   if (IS_EVIL(ch)) {
	return;
	}
   for (temp = world[ch->in_room].people; temp; temp = temp->next_in_room)
       if (FIGHTING(temp)) {
	  stop_fighting(temp);
	  if (IS_NPC(temp)) {
		clearMemory(temp);
	        }
          if (ch != temp) {
                act("$n stops fighting.", TRUE, temp, 0, 0, TO_ROOM);
                act("You stop fighting.", TRUE, temp, 0, 0, TO_CHAR);
                }
          }
   return;
}

ASPELL(spell_relocate)
{
  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }
  if (MOB_FLAGGED(victim, MOB_NOSUMMON))
     {
	send_to_char(SUMMON_FAIL, ch);
        return;
     }
  if (!pk_allowed) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and a rift travels\r\n"
	  "through time and space toward $N, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely close it.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      sprintf(buf, "%s just tried to relocate to you.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to relocate to you.\r\n",
	      GET_NAME(ch),
	      (ch->player.sex == SEX_MALE) ? "He" : "She");
      send_to_char(buf, victim);

      sprintf(buf, "You failed because %s has summon protection on.\r\n",
	      GET_NAME(victim));
      send_to_char(buf, ch);

      sprintf(buf, "%s failed relocating to %s at %s.",
	      GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      mudlog(buf, BRF, LVL_IMMORT, TRUE);
      return;
    }
  }
  
  act("$n opens a portal and steps through it.", TRUE, ch, 0, 0, TO_ROOM);
  act("You open a portal and step through.", FALSE, ch, 0, 0, TO_CHAR);
  char_from_room(ch);
  char_to_room(ch, victim->in_room);
  act("A portal opens and $n steps out.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}

ASPELL(spell_arcane_word)
{
int roomno, tmp, tenthousands, thousands, hundreds, tens, ones;

  if (ch == NULL)
    return;
  
  roomno = world[ch->in_room].number;
  
  ones = roomno % 10;            /* 12345 = 5    */
  tmp = roomno / 10;             /* 12345 = 1234 */
  tens = tmp % 10; 	         /* 1234  = 4    */ 
  tmp = tmp / 10;                /* 1234  = 123  */
  hundreds = tmp % 10;           /* 123   = 3    */
  tmp = tmp / 10;                /* 123   = 12   */
  thousands = tmp % 10;          /* 12    = 2    */
  if (tmp > 0) 
   tenthousands = tmp / 10;       /* 12    = 1    */
  else
   tenthousands = 0;

  sprintf(buf, "You feel a strange sensation in your body, as the wheel of time stops.\r\n");
  send_to_char(buf, ch);
  sprintf(buf, "Strange sounds can be heard through the mists of time.\r\n");
  send_to_char(buf, ch);
  sprintf(buf, "An old man tells you : 'The Arcane name for this place is %s%s%s%s%s'.\r\n", 
  arc[0][number(0,1)][tenthousands], arc[1][number(0,1)][thousands], 
  arc[2][number(0,1)][hundreds], arc[3][number(0,1)][tens], 
  arc[4][number(0,1)][ones]); 
  send_to_char(buf, ch);
}

ASPELL(spell_arcane_portal)
{
char *arc_name = strarg;
char *tmp = 0;
int arc_found = 0;
int nr, icount;
sh_int location;

int dicechance = 0;
extern int top_of_world;

 location = 0;
  
 if (ch == NULL)
  return;
  
/* It takes *SOME* kind of Intelligence to portal!! */

 if (GET_INT(ch) < 7) {
  send_to_char("Sorry, you're too stupid to portal anywhere", ch);
  return;
 }
 
 if (arc_name == NULL) {
  send_to_char("Portal to where?", ch);
  return;
 }

 for (nr = 0; nr < 5;nr++) 
 {
  arc_found = FALSE;
  icount = 0;
  for (icount = 0; icount < 9;icount++) 
  {
   if (!arc_found) 
   {
    tmp = arc[nr][0][icount];
    if(!strncmp(arc_name, tmp, strlen(tmp))) 
    {
     arc_found = TRUE;
     arc_name = arc_name + strlen(tmp);
     location = location * 10 + icount;
    }
    if (!arc_found) 
    {
     tmp = arc[nr][1][icount];
     if(!strncmp(arc_name, tmp, strlen(tmp))) 
     {
      arc_found = TRUE;
      arc_name = arc_name + strlen(tmp);
      location = location * 10 + icount;
     }
    }
   }
  }
  /* here we have tried all 20 chars, and that means.. If still !arc_found = total fail */
  /* They still can be saved by their Intelligence, the higher intelligence, then less */ 
  if (!arc_found) {
   dicechance = GET_INT(ch) - number((GET_INT(ch) - 7), GET_INT(ch));
   switch(dicechance) { 
    case 0 : /* Full Score Nothing happens! */
            send_to_char("You feel strangely relieved, like something terrible just was avoided.\r\n",ch);
            return;
    case 1 : /* Minor Mishap! Give a lame text or push the character somewhere */
            send_to_char("Your concentration is lost.\r\n", ch);
            return;
    case 7 : /* TOTAL Fail!! Let's send the bugger off to somewhere <g> */ 
            send_to_char("You feel a disturbance in the magic aura, and your portal fails.\r\n", ch);
            do {
            location = number(0, top_of_world);
            } while (ROOM_FLAGGED(location, ROOM_TUNNEL | ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM));

            act("$n fails a portal terribly, and is sent off to somewhere.", FALSE, ch, 0, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, location);
            act("A portal opens, and $n tumbles out of it, clearly confused.", FALSE, ch, 0, 0, TO_ROOM);
            look_at_room(ch, 0);
            return;
    default : /* Hum, just do something text based */
            send_to_char("Your portal fails.\r\n",ch);
            return;
   } 
  }
 } /* End decode roomno */
 /* At this point all have been successful! now, let's make magic */
/* The portal spell takes a persons int and wisdom, and uses them for a saving throw */
/* Beware it *IS* possible to die from this spell!!! (should happen once out of 1000 or so */

   
  if (ROOM_FLAGGED(location, ROOM_TUNNEL | ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM)) {
   act("You feel a disturbance in the magic aura, and your portal fails.", FALSE, 0, 0, 0, TO_CHAR);
   do {
   location = number(0, top_of_world);
   } while (ROOM_FLAGGED(location, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM));
   act("$n fails a portal terribly, and is sent off to somewhere.", FALSE, ch, 0, 0, TO_ROOM);
   char_from_room(ch);
   char_to_room(ch, location);
   act("A portal opens, and $n tumbles into the room, clearly confused.", FALSE, ch, 0, 0, TO_ROOM);
   look_at_room(ch, 0);
   return;
 }

 act("$n opens a portal, steps into it and vanishes.", FALSE, ch, 0, 0, TO_ROOM);

 char_from_room(ch);
 char_to_room(ch, real_room(location));

 act("A portal opens, and $n steps out of it.", FALSE, ch, 0, 0, TO_ROOM);
 look_at_room(ch, 0);
} 

ASPELL(spell_control_weather)
{
  int i;
  if (!OUTSIDE(ch)) {
    send_to_char("You are unable to concentrate enough to take control over nature.\r\n", ch);
    return;
  }
  if (GET_INT(ch) < number(1, 19) || !strarg || !*strarg) {
    send_to_char("You fail.\r\n", ch);
    return;
  }
  i = GET_ROOM_ZONE(ch->in_room);
  if (!str_cmp(strarg, "better")) {
    zone_table[i].pressure += GET_INT(ch) + dice(2, 10);
    weather_change(); 
  } else if (!str_cmp(strarg, "worse")) {
    zone_table[i].pressure -= GET_INT(ch) + dice(2, 10);
    weather_change(); 
  } else {
    send_to_char("You feel yourself very powerful.\r\n", ch);
    return;
  }
}

#define MOB_CORPSE_HOST 11
ASPELL(spell_corpse_host)
{
  struct affected_type af;
  struct char_data *mob;
  int hitp;

  if ((obj == NULL) || !IS_CORPSE(obj)) {
    send_to_char("Thats not what you need to be concentrating on.\r\n", ch);
    return;
  }

  mob = read_mobile(MOB_CORPSE_HOST, VIRTUAL);
  char_to_room(mob, ch->in_room);
  hitp = GET_OBJ_VAL(obj, 2) > 0 ? GET_OBJ_VAL(obj, 2) : dice(3,5) ;
 
 
  mob->player.name = str_dup("Ju-Ju Zombie\r\n");
  mob->player.long_descr = str_dup("a Ju-Ju Zombie is standing here.\r\n");
  mob->player.short_descr = str_dup("a Ju-Ju Zombie");
    

  GET_LEVEL(mob)    = GET_LEVEL(ch);
 
  GET_MAX_MANA(mob) = GET_MAX_MANA(ch);
  GET_MANA(mob) = GET_MANA(ch);
 
  GET_MAX_HIT(mob)  = GET_OBJ_TIMER(obj) * hitp / max_pc_corpse_time;
  GET_HIT(mob)      = MAX(1,  GET_MAX_HIT(mob) / 2);
  
  GET_INT(mob)      = GET_INT(ch);
  GET_WIS(mob)      = GET_WIS(ch);
  GET_STR(mob)      = 18;
  GET_CON(mob)      = 15;
  GET_CHA(mob)      = 3;
  GET_DEX(mob)      = 8;
  SET_BIT(MOB_FLAGS(mob), MOB_UNDEAD);
  /*GET_RACE(mob)     = GET_RACE(ch);*/ /* If you want I can show you how to make this same race as mob was */
  
  extract_obj(obj);

  af.type = SPELL_CORPSE_HOST;
  af.duration = (GET_INT(ch) + GET_CON(ch) + GET_WIS(ch)) / 3; 
  af.modifier = 0;
  af.location = 0;
  affect_to_char(mob, &af);
  affect_to_char(ch, &af);

  ch->desc->original = ch;
  ch->desc->character = mob;

  mob->desc = ch->desc;
  ch->desc = NULL;   
}

