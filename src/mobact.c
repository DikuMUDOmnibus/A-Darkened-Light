/* ************************************************************************
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "clan.h"
#include "olc.h"

/* external structs */
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct str_app_type str_app[];
extern int no_specials;
extern struct time_info_data time_info;

ACMD(do_get);

void mprog_random_trigger(struct char_data * mob);
void mprog_time_trigger(struct char_data *mob);
void mprog_process_queue(struct char_data *mob);
void* mprog_wordlist_check(char *arg, struct char_data * mob,
	struct char_data * actor, struct obj_data * obj, void *vo, int type);
void hunt_victim(struct char_data *ch);
extern int is_empty(int zone_nr);
extern struct zone_data *zone_table;


/* local functions */
void mobile_activity(void);
void clearMemory(struct char_data * ch);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)
#define GET_MOB_ZONE_VNUM(ch) 	(GET_MOB_VNUM(ch) / 100 * 100)
// #define ENEMY_CLAN(ch, vict) (ZON_OWNER(&zone_table[real_zone(GET_MOB_ZONE_VNUM(ch))]) != PLAYERCLAN(vict))

void mobile_activity(void)
{
  register struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  int door, found, max;
  memory_rec *names;
  static int last_hour = -1;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (!IS_MOB(ch))
      continue;

    /* Examine call for special procedure */
    if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials) {
      if (mob_index[GET_MOB_RNUM(ch)].func == NULL) {
	log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
		GET_NAME(ch), GET_MOB_VNUM(ch));
	REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
      } else {
	/* XXX: Need to see if they can handle NULL instead of "". */
	if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, ""))
	  continue;		/* go to next char */
      }
    }

    /* If the mob has no specproc, do the default actions */
    if (FIGHTING(ch) || !AWAKE(ch))
      continue;

    

    /* Scavenger (picking up objects) */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !FIGHTING(ch) && AWAKE(ch))
      if (world[ch->in_room].contents && !number(0, 10)) {
	max = 1;
	best_obj = NULL;
	for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
	  if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
	    best_obj = obj;
	    max = GET_OBJ_COST(obj);
	  }
	if (best_obj != NULL) {
	  obj_from_room(best_obj);
	  obj_to_char(best_obj, ch);
	  act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
	}
      }

    /* Mob Movement */
    if (!MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
        !(IS_AFFECTED(ch, AFF_CHARM) && ch->master && ch->in_room == ch->master->in_room) &&
	((door = number(0, 18)) < NUM_OF_DIRS) && CAN_GO(ch, door) &&
	!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH) &&
	(!MOB_FLAGGED(ch, MOB_STAY_ZONE) ||
	 (world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone))) {
      perform_move(ch, door, 1);
    }

   /* MOB Prog foo */
   if (IS_NPC(ch) && (mob_index[ch->nr].progtypes > 0)) {
     if(ch->mpactnum > 0) 
       mprog_process_queue(ch);

     if(!is_empty(world[ch->in_room].zone)) mprog_random_trigger(ch);
     if (time_info.hours != last_hour)
       mprog_time_trigger(ch);
   }

    /* Aggressive Mobs */
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN | MOB_AGGR_CLAN)) {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room) {
      /*
        if (!IS_NPC(vict)) {
          sprintf(buf, "CAN_SEE: %d, IMM_CAN_SEE: %d, INVIS_OK: %d, LIGHT_OK: %d", CAN_SEE(ch, vict),
          IMM_CAN_SEE(ch, vict), INVIS_OK(ch, vict), LIGHT_OK(ch));
          log(buf);
        }
      */
	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;
	if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
	  continue;
	/*
	sprintf(buf,"AGGR: Flags: %d, Playerclan: %d, zoneowner: %d",
	  ZON_FLAGS(&zone_table[real_zone(GET_MOB_VNUM(ch) /100 * 100)]),
	  PLAYERCLAN(vict), ZON_OWNER(&zone_table[real_zone(GET_MOB_VNUM(ch) / 100 * 100)]));
	log(buf);
 	*/
	if (
	    !MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN | MOB_AGGR_CLAN) ||
	    (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
	    (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
	    (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)) ||
	    (MOB_FLAGGED(ch, MOB_AGGR_CLAN) && PLAYERCLAN(vict) != 0 &&
	    ENEMY_CLAN(ch, vict))) {
	  hit(ch, vict, TYPE_UNDEFINED);
	  found = TRUE;
	}
      }
    }

    /* Hunting Mobs */
    if (HUNTING(ch))
	hunt_victim(ch);

    /* Mob Memory */
    if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room) {
	if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
	  continue;
	for (names = MEMORY(ch); names && !found; names = names->next)
	  if (names->id == GET_IDNUM(vict)) {
	    found = TRUE;
	    act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.",
		FALSE, ch, 0, 0, TO_ROOM);
	    hit(ch, vict, TYPE_UNDEFINED);
	  }
      }
    }

    /* Helper Mobs */
    if (MOB_FLAGGED(ch, MOB_HELPER) && !AFF_FLAGGED(ch, AFF_BLIND + AFF_CHARM)) {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room)
	if (ch != vict && IS_NPC(vict) && FIGHTING(vict) &&
            !IS_NPC(FIGHTING(vict)) && ch != FIGHTING(vict)) {
	  act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
	  hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
	  found = TRUE;
	}
    }
    /* Add new mobile actions here */

  }				/* end for() */
  last_hour = time_info.hours;
}



/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data * ch, struct char_data * victim)
{
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    tmp->name = GET_NAME(victim);
    MEMORY(ch) = tmp;
  }
}


/* make ch forget victim */
void forget(struct char_data * ch, struct char_data * victim)
{
  memory_rec *curr, *prev = NULL;

  if (!(curr = MEMORY(ch)))
    return;

  while ((curr && curr->id != GET_IDNUM(victim)) ||
           (curr && curr->name != GET_NAME(victim))) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return;			/* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}


/* erase ch's memory */
void clearMemory(struct char_data * ch)
{
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}
