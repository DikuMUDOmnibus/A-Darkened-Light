/*
************************************************************************ *
File: limits.c Part of CircleMUD * * Usage: limits & gain funcs for HMV,
exp, hunger/thirst, idle time * * * * All rights reserved.  See
license.doc for complete information.  * * * * Copyright (C) 1993, 94 by
the Trustees of the Johns Hopkins University * * CircleMUD is based on
DikuMUD, Copyright (C) 1990, 1991.  *
************************************************************************
*/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
/*
#define READ_TITLE(ch) (GET_SEX(ch) == SEX_MALE ?   \
	titles[(int)GET_CLASS(ch)][(int)GET_LEVEL(ch)].title_m :  \
	titles[(int)GET_CLASS(ch)][(int)GET_LEVEL(ch)].title_f)
*/

extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct room_data *world;
extern struct index_data *obj_index;
extern int max_exp_gain;
extern int max_exp_loss;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int free_rent;

/* local functions */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void check_autowiz(struct char_data * ch, int old_level);

void Crash_rentsave(struct char_data *ch, int cost);
int level_exp(int chclass, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
void update_char_objects(struct char_data * ch);	/* handler.c */

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (age < 15)
    return (p0);		/* < 15   */
  else if (age <= 29)
    return (int) (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (age <= 44)
    return (int) (p2 +(((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (age <= 59)
    return (int) (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (age <= 79)
    return (int) (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);		/* >= 80 */
}

extern int has_boat(struct char_data *ch);

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */


void gain_demonxp(struct char_data * ch, int gain)
{
 // int is_altered = FALSE;
 // int num_levels = 0;
 // char buf[128];
  int old_level;
  
  

  if (IS_NPC(ch)) {
    GET_DEMONXP(ch) += gain;
    return;
  }
  old_level = GET_LEVEL(ch);
  if (gain > 0) {
    gain = MIN(MAX_DEMON_XP, gain);	/* put a cap on the max gain per kill */
    GET_DEMONXP(ch) += gain;

  } else if (gain < 0) {
    gain = MAX(-MAX_DEMON_LOSS, gain);	/* Cap max exp lost per death */
    GET_DEMONXP(ch) += gain;
    if (GET_DEMONXP(ch) < 0)
      GET_DEMONXP(ch) = 0;
  }
}
/*
void gain_demonxp_regardless(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;
  int old_level = GET_LEVEL(ch);
  
  GET_DEMONXP(ch) += gain;
  if (GET_DEMONXP(ch) < 0)
    GET_DEMONXP(ch) = 0;

  if (!IS_NPC(ch)) {
 
       GET_LEVEL(ch) += 1;
      GET_TOT_LEVEL(ch) += 1; 
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      if (num_levels == 1)
        send_to_char("You rise a level!\r\n", ch);
      else {
	sprintf(buf, "You rise %d levels!\r\n", num_levels);
	send_to_char(buf, ch);
      }
      set_title(ch, NULL);
      if (GET_LEVEL(ch) >= LVL_IMMORT)
        check_autowiz(ch, old_level);
    }
  }
}
*/
/* manapoint gain pr. game hour */
int mana_gain(struct char_data * ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_SITTING:
      if (PLR_FLAGGED(ch, PLR_MEDITATE))
        gain *= 2;
      else
        gain += (gain / 4);	/* Divide by 4 */
      break;
    }

    if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC
      || GET_CLASS(ch) == CLASS_ALCHEMIST || GET_CLASS(ch) == CLASS_NECROMANCER
      || GET_CLASS(ch) == CLASS_WARLOCK || GET_CLASS(ch) == CLASS_DRUID)
      gain *= 2;

 //   if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
  //   gain /= 4;
  }
  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  
    
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_GOOD_REGEN))
    gain += (gain * 2);
    
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_BAD_REGEN))
    gain -= (gain / 2);
    
  
  if (HARMED(ch))
    gain /= 4;
  
 
  return (gain);
  
}

/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data * ch)
/* Hitpoint gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    gain = GET_LEVEL(ch);
    if (MOB_FLAGGED(ch, MOB_FASTREGEN))
	gain *= 3;
    /* Neat and fast */
  } else {

    gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);

    /* Class/Level calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }

    if (GET_CLASS(ch) == CLASS_MAGIC_USER || GET_CLASS(ch) == CLASS_CLERIC
      || GET_CLASS(ch) == CLASS_ALCHEMIST || GET_CLASS(ch) == CLASS_NECROMANCER
      || GET_CLASS(ch) == CLASS_WARLOCK || GET_CLASS(ch) == CLASS_DRUID)
      gain /= 2;
      
   // if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
   //   gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

 // if (!IS_NPC(ch) && ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) 
//== 0)))
 //   gain /= 4;

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_GOOD_REGEN))
    gain += (gain * 2);
    
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_BAD_REGEN))
    gain -= (gain / 2);
  
     
    
  if (GET_LEVEL(ch) < 1) {
     gain += (gain * 2);
}
  if (HARMED(ch))
    gain /= 4;

  return (gain);
}



int move_gain(struct char_data * ch)
/* move gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    gain = GET_LEVEL(ch);
    /* Neat and fast */
  } else {
    gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);

    /* Class/Level calculations */

    /* Skill/Spell calculations */


    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain / 2);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain / 4);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain / 8);	/* Divide by 8 */
      break;
    }
//  if (!IS_NPC(ch) && ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) 
//== 0)))
 //   gain /= 4;
  
  }
  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_GOOD_REGEN))
    gain += (gain * 2);
    
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_BAD_REGEN))
    gain -= (gain / 2);
    
  if (HARMED(ch))
    gain /= 4;

  if (GET_LEVEL(ch) < 1) {
   gain += (gain * 2);
}
     
  return (gain);
}



void set_title(struct char_data * ch, char *title)
{
  if (title == NULL) {
    if (GET_SEX(ch) == SEX_FEMALE)
      title = title_female(GET_CLASS(ch), GET_LEVEL(ch));
    else
      title = title_male(GET_CLASS(ch), GET_LEVEL(ch));
  }
  if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  GET_TITLE(ch) = str_dup(title);
}


void check_autowiz(struct char_data * ch, int old_level)
{
  void relist(void);
/*
#ifndef CIRCLE_UNIX
  return;
#else
  char buf[100];
  

  if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
    mudlog("Initiating autowiz.", CMP, LVL_IMMORT, FALSE);
    system(buf);
  } else
  
#endif */  /* CIRCLE_UNIX */

  if (GET_LEVEL(ch) >= LVL_IMMORT || old_level >= LVL_IMMORT) {
    relist(); 
    reboot_wizlists();
  }
}



void gain_exp(struct char_data * ch, int gain)
{
 // int is_altered = FALSE;
 // int num_levels = 0;
 // char buf[128];
  int old_level;
  
  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain*2;
    return;
  }
  old_level = GET_LEVEL(ch);
  if (gain > 0) {
    gain = MIN(max_exp_gain, gain);	/* put a cap on the max gain per kill */
    GET_EXP(ch) += gain;
 //   while (GET_LEVEL(ch) <= MAX_MORT_LEVEL &&
//	GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
 //     if (GET_LEVEL(ch) >= MAX_MORT_LEVEL && GET_LEVEL(ch) < LVL_IMMORT) 
 //       { if (!IS_NPC(ch) && (QPOINTS(ch) >= MAX_QPOINTS))
  //        GET_LEVEL(ch) = LVL_IMMORT;
   //       else return; }
    //  else 
//        GET_LEVEL(ch) += 1;
//    GET_TOT_LEVEL(ch) += 1; 
//     num_levels++;
//      advance_level(ch);
 //     is_altered = TRUE;
 //   }
//
//    if (is_altered) {
//      if (num_levels == 1)
//        send_to_char("You rise a level!\r\n", ch);
//      else {
//	sprintf(buf, "You rise %d levels!\r\n", num_levels);
//	send_to_char(buf, ch);
 //     }
//      set_title(ch, NULL);
//      if (GET_LEVEL(ch) >= LVL_IMMORT)
//        check_autowiz(ch, old_level);
   
  } else if (gain < 0) {
    gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
}


void gain_exp_regardless(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;
  int old_level = GET_LEVEL(ch);
  
  GET_EXP(ch) += gain*2;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMPL &&
	GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
      if (GET_LEVEL(ch) >= MAX_MORT_LEVEL && GET_LEVEL(ch) < LVL_IMMORT) 
        GET_LEVEL(ch) = LVL_IMMORT;
      else 
       GET_LEVEL(ch) += 1;
      GET_TOT_LEVEL(ch) += 1; 
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      if (num_levels == 1)
        send_to_char("You rise a level!\r\n", ch);
      else {
	sprintf(buf, "You rise %d levels!\r\n", num_levels);
	send_to_char(buf, ch);
      }
      set_title(ch, NULL);
      if (GET_LEVEL(ch) >= LVL_IMMORT)
        check_autowiz(ch, old_level);
    }
  }
}


void gain_condition(struct char_data * ch, int condition, int value)
{
  bool intoxicated;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;
  if ((condition != DRUNK) && (value > 0) && (GET_COND(ch, condition) == value)) {
    //check_regen_rates(ch);
  }
  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(100, GET_COND(ch, condition));

  if (GET_CLASS(ch) == CLASS_THIEF) {
    if (GET_COND(ch, THIRST) == 0) {
      send_to_char("You go mad for the lack of blood.\r\n"
                   "Sharp claws extend from your fingers.\r\n"
                   "You fangs extend to unextendable lengths.\r\n",ch);
      GET_HIT(ch) -= 1;
      return;
    }
  }

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case FULL:
    if (GET_CLASS(ch) == CLASS_WARRIOR) {
    send_to_char("You require the consumption of souls.\r\n", ch);
    return;
}
  case THIRST:
   if (GET_CLASS(ch) == CLASS_THIEF) {
    send_to_char("A thirst for blood ails you.\r\n", ch);
    GET_HIT(ch) -= 1;
}
   return;
  case DRUNK:
    if (intoxicated)
      send_to_char("You are now sober.\r\n", ch);
    return;
  case VAMPTHIRST:
    if (GET_CLASS(ch) == CLASS_THIEF)
      send_to_char("A thirst for blood ails you.\r\n", ch);
      return;
  default:
    break;
  }
}


void check_idling(struct char_data * ch)
{
  if (++(ch->char_specials.timer) > 8) {
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch)) {
        stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
     act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
     save_char(ch, NOWHERE);
     Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->char_specials.timer > 48) {
      if (ch->in_room != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc) {
      close_socket(ch->desc);
      ch->desc = NULL;
      }
      if (free_rent)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);
      sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
      mudlog(buf, CMP, LVL_GOD, TRUE);
      extract_char(ch);
    }
  }
}

void parts_update(void)
{
  struct char_data *i, *next_char;
//  struct obj_data *j, *next_thing, *jj, *next_thing2;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

 if (GET_POS(i) == POS_FIGHTING) {
    return;
} else

      if (PLR2_FLAGGED(i, PLR2_NOFACE) && number(1, 10)) {
         send_to_char("Skin magically begins forming over the meaty tissue on your face.\r\n",i);
         REMOVE_BIT(PLR2_FLAGS(i), PLR2_NOFACE);
         return;
         } 

            if (PLR2_FLAGGED(i, PLR2_NOLARM) && number(5, 10)) {
         send_to_char("Your left arm grows from the nub of your fleshwound.\r\n",i);
         REMOVE_BIT(PLR2_FLAGS(i), PLR2_NOLARM);
         return;
         }
   
 }
}


void blood_update(void)
{
  struct char_data *i, *next_char;
//  struct obj_data *j, *next_thing, *jj, *next_thing2;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;


    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);
 }
}
void darkfog_update(void)
{
 struct char_data *ch;
  
    if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_FOG)) {
     send_to_char("The fog has cleared out of the room.\r\n",ch);

     REMOVE_BIT(ROOM_FLAGS(ch->in_room), ROOM_FOG);
     return;
}
}  

/* Update PCs, NPCs, and objects */
void point_update(void)
{
  // struct affected_type *raff;
  //struct room_data *room;
  //struct char_data *ch;
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;
	
//    gain_condition(i, FULL, -1);
//    gain_condition(i, DRUNK, -1);
//    gain_condition(i, THIRST, -1);

      
     	
    if (GET_POS(i) >= POS_DEAD) {
      if (!AFF_FLAGGED(i, AFF_DEATHDANCE)) {
        GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
        GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
        GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
        //REMOVE_BIT(PLR2_FLAGS(i), PLR2_FACE);
      }
      if (AFF_FLAGGED(i, AFF_POISON))
	if (damage(i, i, 2, SPELL_POISON) == -1)
	  continue;	/* Oops, they died. -gg 6/24/98 */

      if (AFF2_FLAGGED(i, AFF2_BURNING) &&
	SECT(i->in_room) == SECT_UNDERWATER) {
        REMOVE_BIT(AFF2_FLAGS(i), AFF2_BURNING);
      }
      if (AFF2_FLAGGED(i, AFF2_ACIDED) &&
	SECT(i->in_room) == SECT_UNDERWATER) {
        REMOVE_BIT(AFF2_FLAGS(i), AFF2_ACIDED);
      }
      if (AFF2_FLAGGED(i, AFF2_FREEZING) &&
	SECT(i->in_room) == SECT_LAVA) {
        REMOVE_BIT(AFF2_FLAGS(i), AFF2_FREEZING);
      }
     //if (ROOM_FLAGGED(ch->in_room, ROOM_FOG)) {
     //REMOVE_BIT(ROOM_FLAGS(room), ROOM_FOG);       
    // }
      if (AFF2_FLAGGED(i, AFF2_BURNING) && !(AFF2_FLAGGED(i, AFF2_PROT_FIRE))) {
        if (!MOB_FLAGGED(i, MOB_MOREBURN))
          damage(i, i, LAVA_DAMAGE, SPELL_BURN);
        else
          damage(i, i, LAVA_DAMAGE * 2, SPELL_BURN);
      }
     
      if (AFF2_FLAGGED(i, AFF2_FREEZING) && !(AFF2_FLAGGED(i, AFF2_PROT_COLD))) {
        if (!MOB_FLAGGED(i, MOB_MOREFREEZE))
          damage(i, i, LAVA_DAMAGE, SPELL_FREEZE);
        else
          damage(i, i, LAVA_DAMAGE * 2, SPELL_FREEZE);
      }
      
      if (AFF2_FLAGGED(i, AFF2_ACIDED) && !(AFF_FLAGGED(i, AFF_STONESKIN))) {
        if (!MOB_FLAGGED(i, MOB_MOREACID))
          damage(i, i, LAVA_DAMAGE, SPELL_ACID);
        else
          damage(i, i, LAVA_DAMAGE * 2, SPELL_ACID);
      }
     
      if (i->in_room != NOWHERE && (SECT(i->in_room) == SECT_UNDERWATER) &&
	     !AFF_FLAGGED(i, AFF_WATERBREATH)) {
	send_to_char("You're drowning!!\r\n", i);
        GET_HIT(i) -= UNWAT_DAMAGE;
          //alter_hit(i, UNWAT_DAMAGE);
      }
      if (AFF2_FLAGGED(i, AFF2_CRIT_HIT)) {
	send_to_char("You're bleeding critically!!!\r\n", i);
	GET_HIT(i) -= CRIT_DAMAGE;
//          alter_hit(i, CRIT_DAMAGE);
      }	
	
      if (GET_POS(i) <= POS_STUNNED)
	update_pos(i);
    } else if (GET_POS(i) == POS_INCAP) {
      if (damage(i, i, 1, TYPE_SUFFERING) == -1);
        continue;
    } else if (GET_POS(i) == POS_MORTALLYW) {
      if (damage(i, i, 2, TYPE_SUFFERING) == -1);
        continue;
    }
    if (!IS_NPC(i)) {
      update_char_objects(i);
      if (GET_LEVEL(i) < idle_max_level)
	check_idling(i);
    }
    
    if (!IS_NPC(i) && !AFF_FLAGGED(i, AFF_WATERBREATH) &&
      !AFF_FLAGGED(i, AFF_WATERWALK) && GET_LEVEL(i) < LVL_IMMORT)
	if (SECT(i->in_room) == SECT_WATER_NOSWIM && !has_boat(i)) {
	  act("$n thrashes about in the water straining to stay afloat.", FALSE,
		i, 0, 0, TO_ROOM);
	  send_to_char("You are drowning!\r\n", i);
	  damage(i, i, GET_MAX_HIT(i) / 5, TYPE_SUFFERING); /* TYPE_DROWNING ? */
	}
    
  }

  /* objects */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */

    /* If this is a corpse */
    if (j && IS_CORPSE(j)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (j && !GET_OBJ_TIMER(j)) {

	if (j->carried_by)
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
	else if ((j->in_room != NOWHERE) && (world[j->in_room].people)) {
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[j->in_room].people, j, 0, TO_CHAR);
         //REMOVE_BIT(ROOM_FLAGS(ch->in_room), ROOM_FOG);
	}
	for (jj = j->contains; jj; jj = next_thing2) {
	  next_thing2 = jj->next_content;	/* Next in inventory */
	  obj_from_obj(jj);

	  if (j->in_obj)
	    obj_to_obj(jj, j->in_obj);
	  else if (j->carried_by)
	    obj_to_room(jj, j->carried_by->in_room);
	  else if (j->in_room != NOWHERE)
	    obj_to_room(jj, j->in_room);
	  else
	    core_dump();
	}
	extract_obj(j);
      }
    }
    /* if it's a portal */
    else if (j && GET_OBJ_VNUM(j) == 31)
    {
      if (GET_OBJ_VAL(j,0) > 0)
        GET_OBJ_VAL(j,0)--;
      if (GET_OBJ_VAL(j,0) == 1)
      {
        if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
	  act("$p starts to fade!", 
  	    FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("$p starts to fade!", 
	    FALSE, world[j->in_room].people, j, 0, TO_CHAR);
        }
      }
      if (GET_OBJ_VAL(j,0) == 0)
      {
        if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
    	  act("$p vanishes in a cloud of smoke!", 
	    FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("$p vanishes in a cloud of smoke!", 
	    FALSE, world[j->in_room].people, j, 0, TO_CHAR);
        }
        extract_obj(j);
      }
    }
   else if (GET_OBJ_RNUM(j) == real_object(OBJ_SPRING_VNUM))
    {
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;
      if (GET_OBJ_TIMER(j) == 1)
      {
    if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
    act("$p starts to slow down.", FALSE, world[j->in_room].people, j, 0, TO_ROOM);
    act("$p starts to slow down.", FALSE, world[j->in_room].people, j, 0, TO_CHAR);
        }
      }
      if (GET_OBJ_TIMER(j) == 0)
      {
        if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
    	  act("$p has stopped flowing!", FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("$p has stopped flowing!", FALSE, world[j->in_room].people, j, 0, TO_CHAR);
        }
        extract_obj(j);
      }
    }
    else if (GET_OBJ_RNUM(j) == real_object(60))
    {
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;
      if (GET_OBJ_TIMER(j) == 0)
      {
        if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
    	  act("A worker come in and repairs $p!", FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("A worker come in and repairs $p!", FALSE, world[j->in_room].people, j, 0, TO_CHAR);
        }
        extract_obj(j);
      }
    }
    else if (GET_OBJ_RNUM(j) >= real_object(61) && GET_OBJ_RNUM(j) <= real_object(65))
    {
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;
      if (GET_OBJ_TIMER(j) == 0)
      {
        
        if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
    	  act("The winds and weather blot out remarks of digging!", FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("The winds and weather blot out remarks of digging!", FALSE, world[j->in_room].people, j, 0, TO_CHAR);
        } 
        extract_obj(j);
      }
    }
    else if (GET_OBJ_RNUM(j) == real_object(70))
    {
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;
      if (GET_OBJ_TIMER(j) == 0)
      {
        if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
    	  act("A worker come in and repairs the floor!", FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	  act("A worker come in and repairs the floor!", FALSE, world[j->in_room].people, j, 0, TO_CHAR);
        }
        extract_obj(j);
      }
    }
    else if (GET_OBJ_RNUM(j) >= real_object(71) && GET_OBJ_RNUM(j) <= real_object(71))
    {
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;
      if (GET_OBJ_TIMER(j) == 0)
      {
        extract_obj(j);
      }
    }
   
  }
}

void set_afk(struct char_data * ch, char *afk_message, int result)
{
  if (!*afk_message)
    sprintf(buf1, "%s", AFK_DEFAULT);
  else
    sprintf(buf1, "%s", afk_message);

  if (strlen(buf1) > MAX_AFK_LENGTH) {
    send_to_char("Afk message too long, using default.\r\n", ch);
    sprintf(buf1, "%s", AFK_DEFAULT);
  }

  if (GET_AFK(ch))
    free(GET_AFK(ch));
  GET_AFK(ch) = NULL;
  GET_AFK(ch) = str_dup(buf1);

  if (result) {
    sprintf(buf, "$n has just gone &mAFK&w: %s", GET_AFK(ch));
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    sprintf(buf, "You have just gone AFK: %s", GET_AFK(ch));
    act(buf, FALSE, ch, 0, 0, TO_CHAR);
  } else {
    act("$n is back from AFK!", FALSE, ch, 0, 0, TO_ROOM);
    act("You are back from AFK!", FALSE, ch, 0, 0, TO_CHAR);
  }
}
