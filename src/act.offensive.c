/* 
************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "clan.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;	/*. db.c	.*/


/* extern functions */
void raw_kill(struct char_data * ch, struct char_data * killer);
void check_killer(struct char_data * ch, struct char_data * vict);
int perform_move(struct char_data *ch, int dir, int specials_check);
void improve_skill(struct char_data *ch, int skill);

/* Daniel Houghton's revised external functions */
int skill_roll(struct char_data *ch, int skill_num);
void strike_missile(struct char_data *ch, struct char_data *tch, 
                   struct obj_data *missile, int dir, int attacktype);
void miss_missile(struct char_data *ch, struct char_data *tch, 
                struct obj_data *missile, int dir, int attacktype);
void mob_reaction(struct char_data *ch, struct char_data *vict, int dir);
void fire_missile(struct char_data *ch, char arg1[MAX_INPUT_LENGTH],
                  struct obj_data *missile, int pos, int range, int dir);

/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);



ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Whom do you wish to assist?\r\n", ch);
  else if (!(helpee = get_char_room_vis(ch, arg)))
    send_to_char(NOPERSON, ch);
  else if (helpee == ch)
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
    for (opponent = world[ch->in_room].people;
	 opponent && (FIGHTING(opponent) != helpee);
	 opponent = opponent->next_in_room)
		;

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_MURDER(ch, opponent))	/* prevent accidental pkill */
      act("You cannot assist! $N is you friend!", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}

ACMD(do_decapitate)
{
struct char_data *vict;

one_argument(argument, arg);

if (!*arg) {
    send_to_char("Decapitate who?\r\n", ch);
} else if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("They don't seem to be here.\r\n", ch);
} else if (GET_POS(vict) > POS_STUNNED) {
    send_to_char("You cannot decapitate a player who can still fight.\r\n",ch);
 } else if (!IS_NPC(ch) && !IS_NPC(vict) && GET_PLAYER_KILLS(ch) > GET_PLAYER_KILLS(vict)) {
      send_to_char("You cannot decapitate someone of a lower status than you.\r\n",ch);
      return;
      } else if (*arg) {
      if (GET_POS(vict) < POS_STUNNED) {
      sprintf(buf, "[ <-INFO-> ] %s has declared victory and decapitates %s!\r\n", GET_NAME(ch), GET_NAME(vict));
      send_to_char("You cleanly rip the head off the body of your opponent!\r\n",ch);
      act("$N cleanly rips your head off from your body!\r\n",FALSE,vict,0, ch,TO_CHAR);
      act("$n cleanly rips $N's body clear off of $S body!\r\n"
          "$n is surrounded by a white cloud, witch enters into his chest\r\n"
          "$n begins to glow with a beutiful purple aura\r\n",FALSE, ch, 0,vict, TO_NOTVICT);
      GET_PLAYER_KILLS(ch) += 1;
      GET_PLAYER_KILLS(vict) = 1;
      raw_kill(vict, ch);
      send_to_all(buf);
  }
 }
}
ACMD(do_hit)
{
  struct char_data *vict;
  struct follow_type *k;
  ACMD(do_assist);

  one_argument(argument, arg);
     

  if (!*arg)
    send_to_char("Hit who?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if (!IS_NPC(ch) && !IS_NPC(vict) && GET_PLAYER_KILLS(ch) == 1)
      send_to_char("You are a mortal you cannot attack players yet.\r\n",ch);
  else if (!IS_NPC(ch) && !IS_NPC(vict) && GET_PLAYER_KILLS(ch) > GET_PLAYER_KILLS(vict)) {
      send_to_char("You cannot attack someone of a lower status than you.\r\n",ch);
      return;
      }
  else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (!IS_NPC(vict) && GET_PLAYER_KILLS(ch) == 1) {
    send_to_char("Mortals cannot attack other player untill they are avatar.\r\n",ch);
    return;
  } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
      if (IS_NPC(ch) && LEVEL_OK(vict, ch)) {
      act("The Mark of Neutrality protects $N.", FALSE, ch, 0, vict, TO_CHAR);
      return;
    }
     
    if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
      return;			/* you can't order a charmed pet to attack a
				 * player */
    

    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      hit(ch, vict, TYPE_UNDEFINED);
      
    for (k = ch->followers; k; k=k->next) {
  	if (!IS_NPC(k->follower) && PRF_FLAGGED(k->follower, PRF_AUTOASSIST) && 
     	(k->follower->in_room == ch->in_room))
    	do_assist(k->follower, GET_NAME(ch), 0, 0);
      }

      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
    } else
      send_to_char("You do the best you can!\r\n", ch);
    
  }
  
}

ACMD(do_kill)
{
  struct char_data *vict;

  if ((GET_LEVEL(ch) < LVL_COIMPL) || IS_NPC(ch)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_room_vis(ch, arg)))
      send_to_char("They aren't here.\r\n", ch);
    else if (ch == vict)
      send_to_char("Your mother would be so sad.. :(\r\n", ch);
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict, ch);
    }
  }
}



ACMD(do_backstab)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, buf);

  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char("Backstab who?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("How can you sneak up on yourself?\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char("You can't backstab a fighting person -- they're too alert!\r\n", ch);
    return;
  }
  if (!CAN_MURDER(ch, vict)) {
    send_to_char("You can't force yourself to backstab that person!\r\n", ch);
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = !IS_NPC(ch) ? 
    GET_SKILL(ch, SKILL_BACKSTAB) : number(0, 100);

  if (AWAKE(vict) && (percent > prob))
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);
}



ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  int org_room;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Order who to do what?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, name)) && !is_abbrev(name, "followers"))
    send_to_char("That person isn't here.\r\n", ch);
  else if (ch == vict)
    send_to_char("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(OK, ch);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, vict, TO_ROOM);

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
	if (org_room == k->follower->in_room)
	  if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(OK, ch);
      else
	send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}



ACMD(do_flee)
{
  int i, attempt, loss;
  struct char_data *was_fighting;

  if (!FIGHTING(ch)) {
    send_to_char("Nobody is beating you. Why would you like to flee?\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("You are in pretty bad shape, unable to flee!\r\n", ch);
    return;
  }
  
  if (MOB2_FLAGGED(FIGHTING(ch), MOB2_CANT_FLEE) && 
    (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char("PANIC!  You couldn't escape!\r\n", ch);
    return;
  }
  
  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);
      if (do_simple_move(ch, attempt, TRUE)) {
        sprintf(buf, "You leave %s.\r\n", dirs[attempt]);
	send_to_char("You flee head over heels.\r\n", ch);
	send_to_char(buf, ch);
	sprintf(buf, "$n leaves %s.", dirs[attempt]);
	act(buf, TRUE, ch, 0, 0, TO_ROOM);
        if (was_fighting && !IS_NPC(ch)) {
	  loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
	  loss *= GET_LEVEL(was_fighting);	    gain_exp(ch, -loss);
	}
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}


ACMD(do_bash)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (!IS_NPC(ch) && !GET_SKILL(ch, SKILL_BASH)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (!CAN_MURDER(ch, vict)) {
    send_to_char("You can't force yourself to bash your friend!\r\n", ch);
    return;
  }
  
  percent = number(1, 101);	/* 101% is a complete failure */
  prob = !IS_NPC(ch) ? GET_SKILL(ch, SKILL_BASH) : number(0, 100);

  if (MOB_FLAGGED(vict, MOB_NOBASH))
    percent = 101;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
  } else {
    if (damage(ch, vict, 1, SKILL_BASH) > 0) {  /* -1 = dead, 0 = miss */
      GET_POS(vict) = POS_SITTING;
      improve_skill(ch, SKILL_BASH);
      WAIT_STATE(vict, PULSE_VIOLENCE);
    }
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  one_argument(argument, arg);

  if (!GET_SKILL(ch, SKILL_RESCUE)) {
    send_to_char("But you have no idea how!\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("Whom do you want to rescue?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("What about fleeing instead?\r\n", ch);
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
    return;
  }
  for (tmp_ch = world[ch->in_room].people; tmp_ch &&
       (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (GET_CLASS(ch) != CLASS_WARRIOR && GET_CLASS(ch) != CLASS_PALADIN
     && GET_CLASS(ch) != CLASS_WARLOCK)
    send_to_char("But only true warriors can do this!", ch);
  else {
    percent = number(1, 101);	/* 101% is a complete failure */
    prob = !IS_NPC(ch) ? GET_SKILL(ch, SKILL_RESCUE) : number(0, 100);

    if (percent > prob) {
      send_to_char("You fail the rescue!\r\n", ch);
      return;
    }
    send_to_char("Banzai!  To the rescue...\r\n", ch);
    act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    improve_skill(ch, SKILL_RESCUE);
    if (FIGHTING(vict) == tmp_ch)
      stop_fighting(vict);
    if (FIGHTING(tmp_ch))
      stop_fighting(tmp_ch);
    if (FIGHTING(ch))
      stop_fighting(ch);

    set_fighting(ch, tmp_ch);
    set_fighting(tmp_ch, ch);

    WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
  }

}


ACMD(do_kick)
{
  struct char_data *vict;
  int percent, prob;


  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_KICK) == 0) {
    send_to_char("You are too polite to get yourself to kick someone.\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (!CAN_MURDER(ch, vict)) {
    send_to_char("You can't force yourself to kick your friend!\r\n", ch);
    return;
  }
  
  percent = ((10 - (GET_AC(vict) / 10)) * 2) + number(1, 101);	/* 101% is a complete
								 * failure */
  prob = !IS_NPC(ch) ? GET_SKILL(ch, SKILL_KICK) : number(0, 100);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK);
  } else {
    damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK);
    improve_skill(ch, SKILL_KICK);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_shoot)
{ 
  struct obj_data *missile;
  char arg2[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  int dir, range;

  if (!GET_EQ(ch, WEAR_WIELD)) { 
    send_to_char("You aren't wielding a shooting weapon!\r\n", ch);
    return;
  }

  if (!GET_EQ(ch, WEAR_HOLD)) { 
    send_to_char("You need to be holding a missile!\r\n", ch);
    return;
  }

  missile = GET_EQ(ch, WEAR_HOLD);

  if ((GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_SLING) &&
      (GET_OBJ_TYPE(missile) == ITEM_ROCK))
       range = GET_EQ(ch, WEAR_WIELD)->obj_flags.value[0];
  else 
    if ((GET_OBJ_TYPE(missile) == ITEM_ARROW) &&
        (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_BOW))
         range = GET_EQ(ch, WEAR_WIELD)->obj_flags.value[0];
  else 
    if ((GET_OBJ_TYPE(missile) == ITEM_BOLT) &&
        (GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) == ITEM_CROSSBOW))
         range = GET_EQ(ch, WEAR_WIELD)->obj_flags.value[0];

  else {
    send_to_char("You should wield a missile weapon and hold a missile!\r\n", ch);
    return;
  }

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char("You should try: shoot <someone> <direction>\r\n", ch);
    return;
  }

  if (IS_DARK(ch->in_room)) { 
    send_to_char("You can't see that far.\r\n", ch);
    return;
  } 

  if ((dir = search_block(arg2, dirs, FALSE)) < 0) {
    send_to_char("What direction?\r\n", ch);
    return;
  }

  if (!CAN_GO(ch, dir)) { 
    send_to_char("Something blocks the way!\r\n", ch);
    return;
  }

  if (range > 3) 
    range = 3;
  if (range < 1)
    range = 1;

  fire_missile(ch, arg1, missile, WEAR_HOLD, range, dir);

}


ACMD(do_throw)
{

/* sample format: throw monkey east
   this would throw a throwable or grenade object wielded
   into the room 1 east of the pc's current room. The chance
   to hit the monkey would be calculated based on the pc's skill.
   if the wielded object is a grenade then it does not 'hit' for
   damage, it is merely dropped into the room. (the timer is set
   with the 'pull pin' command.) */

  struct obj_data *missile;
  int dir, range = 1;
  char arg2[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  two_arguments(argument, arg1, arg2);

/* only two types of throwing objects: 
   ITEM_THROW - knives, stones, etc
   ITEM_GRENADE - calls tick_grenades.c . */

  if (!(GET_EQ(ch, WEAR_WIELD))) { 
    send_to_char("You should wield something first!\r\n", ch);
    return;
  }

  missile = GET_EQ(ch, WEAR_WIELD);
  
  if (!((GET_OBJ_TYPE(missile) == ITEM_THROW) ||
     (GET_OBJ_TYPE(missile) == ITEM_GRENADE))) { 
    send_to_char("You should wield a throwing weapon first!\r\n", ch);
    return;
  }
  
  if (GET_OBJ_TYPE(missile) == ITEM_GRENADE) {
    if (!*arg1) {
      send_to_char("You should try: throw <direction>\r\n", ch);
      return;
    }
    if ((dir = search_block(arg1, dirs, FALSE)) < 0) { 
      send_to_char("What direction?\r\n", ch);
      return;
    }
  } else {

    two_arguments(argument, arg1, arg2);

    if (!*arg1 || !*arg2) {
      send_to_char("You should try: throw <someone> <direction>\r\n", ch);
      return;
    }

/* arg2 must be a direction */

    if ((dir = search_block(arg2, dirs, FALSE)) < 0) { 
      send_to_char("What direction?\r\n", ch);
      return;
    }
  }

/* make sure we can go in the direction throwing. */
  if (!CAN_GO(ch, dir)) { 
    send_to_char("Something blocks the way!\r\n", ch);
    return;
  }

  fire_missile(ch, arg1, missile, WEAR_WIELD, range, dir);
}   

/* NOTE: MOB_NOBASH prevents from disarming */
ACMD(do_disarm)
{
  struct obj_data *obj;
  struct char_data *vict;

  one_argument(argument, buf);

  if (!*buf) {
        send_to_char("Whom do you want to disarm?\r\n", ch);
	return;
  }
  else if (!(vict = get_char_room_vis(ch, buf))) {
        send_to_char(NOPERSON, ch);
	return;
  }
  else if (vict == ch) {
        send_to_char("Try removing your weapon instead.\r\n", ch);
	return;
  }
  else if (!CAN_MURDER(ch, vict)) {
        send_to_char("That would be seen as an act of aggression!\r\n", ch);
	return;
  }
  else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict)) {
        send_to_char("The thought of disarming your master seems revolting to you.\r\n", ch);
	return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("This room has a nice peaceful feeling.\r\n", ch);
    return;
  }
  else if (!(obj = GET_EQ(vict, WEAR_WIELD)))
        act("$N is unarmed!", FALSE, ch, 0, vict, TO_CHAR);
  else if (IS_SET(GET_OBJ_EXTRA(obj), ITEM_NO_DISARM) ||
  	   MOB_FLAGGED(vict, MOB_NOBASH) ||
           (number(1, 101) > (!IS_NPC(ch) ? 
             GET_SKILL(ch, SKILL_DISARM) : number(0, 100)))) {
        act("You failed to disarm $N!", FALSE, ch, 0, vict, TO_CHAR);
        damage(vict, ch, number(1, GET_LEVEL(vict)), TYPE_HIT);
    }
  else if (dice(2, GET_STR(ch)) + GET_LEVEL(ch) <= dice(2, GET_STR(vict)) + GET_LEVEL(vict)) {
        act("You almost succeed in disarming $N", FALSE, ch, 0, vict, TO_CHAR);
        act("You were almost disarmed by $N!", FALSE, vict, 0, ch, TO_CHAR);
        damage(vict, ch, number(1, GET_LEVEL(vict) / 2), TYPE_HIT);
  } else {
        obj_to_room(unequip_char(vict, WEAR_WIELD), vict->in_room);
        act("You succeed in disarming your enemy!", FALSE, ch, 0, 0, TO_CHAR);
        act("Your $p flies from your hands!", FALSE, vict, obj, 0, TO_CHAR);
        act("$n disarms $N, $p drops to the ground.", FALSE, ch, obj, vict, TO_ROOM);
        improve_skill(ch, SKILL_DISARM);
  }
  hit(vict , ch, TYPE_UNDEFINED);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}

ACMD(do_spring)
{
  struct obj_data *spring = NULL;
  *buf = '\0';
 
  if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_SPRING) == 0)
    {
    send_to_char("That skill is unfamiliar to you.\r\n", ch);
    return ;
    }

  if((SECT(ch->in_room) == SECT_INSIDE) || (SECT(ch->in_room) == SECT_CITY))
    {
    send_to_char("You cannot create a spring here!\r\n", ch);
    return ;
    }

  if((SECT(ch->in_room) == SECT_WATER_SWIM) || (SECT(ch->in_room) == SECT_WATER_NOSWIM))
    {
    send_to_char("How can you create a spring in water?\r\n", ch);
    return;
    }

  if(SECT(ch->in_room) == SECT_UNDERWATER)
    {
    send_to_char("You cannot create a spring underwater!\r\n", ch);
    return;
    }
 
  if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
   send_to_char("An unforseen force prevents you from casting spells.\r\n", ch);
   return;
   }

  spring = read_object(OBJ_SPRING_VNUM, VIRTUAL);
  GET_OBJ_TIMER(spring) = !IS_NPC(ch) ? 1+GET_SKILL(ch, SKILL_SPRING) / 7
    : number(3, 20); /* you may want to reflect exp or level on */
                     /* this value here, maybe 2 + GET_LEVEL(ch) */ 
  GET_MOVE(ch) -= (MAX_MORT_LEVEL - GET_LEVEL(ch) / 2); 
  obj_to_room(spring, ch->in_room);
  sprintf(buf, "You have created a small spring!\r\n"); 
  act("$n has created a spring.", FALSE, ch, 0, NULL, TO_ROOM);
  if (number(1,8) == 5) improve_skill(ch, SKILL_SPRING);
 /* You may want a message sent to the room as well */
  send_to_char(buf, ch);
  return;
}

ACMD(do_forage)
{
  struct obj_data *item_found = '\0';
  int item_no = 51; /* Initialize with first item poss. */
  *buf = '\0';

  if(!IS_NPC(ch) && GET_SKILL(ch, SKILL_FORAGE) <= 0)
   {
    send_to_char("You have no idea how to forage for survival!\r\n", ch);
    return; }

  if(GET_MOVE(ch) < 31)
    {
    send_to_char("You do not have enough energy right now.\r\n", ch); 
    return; }

  if(SECT(ch->in_room) != SECT_FIELD && SECT(ch->in_room) != SECT_FOREST && SECT(ch->in_room) != SECT_HILLS && SECT(ch->in_room) != SECT_MOUNTAIN  && SECT(ch->in_room) != SECT_SWAMP)
   {
    send_to_char("You cannot forage on this type of terrain!\r\n", ch);
    return; }

     send_to_char("You start searching the area for signs of food.\r\n", ch); 
     act("$n starts foraging the area for food.\r\n", FALSE, ch, 0, 0, TO_ROOM);
   if(number(1,101) > !IS_NPC(ch) ? GET_SKILL(ch, SKILL_FORAGE) : number (0, 100))
    {
     WAIT_STATE(ch, PULSE_VIOLENCE * 2);
     GET_MOVE(ch) -= MAX(10, (30 - GET_LEVEL(ch) / 2)); 
     send_to_char("\r\nYou have no luck finding anything to eat.\r\n", ch);
     return;
    }
   else
    {
    switch (number(1,7))
     {
     case 1:
      item_no = 51; break;  /*<--- Here are the objects you need to code */
     case 2:                   /* Add more or remove some, just change the */
      item_no = 52; break;  /* switch(number(1, X) */ 
     case 3:
      item_no = 53; break;
     case 4:
      item_no = 54; break;
     case 5:
      item_no = 55; break;
     case 6:
      item_no = 56; break;
     case 7:
      item_no = 57; break;
     }
   WAIT_STATE( ch, PULSE_VIOLENCE * 2);  /* Not really necessary */
   GET_MOVE(ch) -= MAX(10, (30 - GET_LEVEL(ch) / 2));
   item_found = read_object( item_no, VIRTUAL);
   obj_to_char(item_found, ch);
   sprintf(buf, "%sYou have found %s!\r\n", buf, item_found->short_description);
   send_to_char(buf, ch);
   act("$n has found something in his forage attempt.\r\n", FALSE, ch, 0, 0, TO_ROOM);
   improve_skill(ch, SKILL_FORAGE);
     return;
     }
}

ACMD(do_fission)
{
	struct obj_data *obj, *next_obj, *object;
	char obj_name[MAX_STRING_LENGTH];
	int found = FALSE, prob = 0;

	one_argument(argument, obj_name);

	if (!(GET_CLASS(ch) == CLASS_CYBORG || GET_LEVEL(ch) >= LVL_IMMORT)
	      || (!IS_NPC(ch) && GET_SKILL(ch, SKILL_FISSION) == 0)) {
		send_to_char("You have no idea how to ignite nuclear fission!\r\n", ch);
		return;
	}
	if (!obj_name || !*obj_name) {
		send_to_char("What do you wish to turn into energy?\r\n", ch);
		return;
	}

	for (obj = ch->carrying; obj; obj = next_obj) {
		next_obj = obj->next_content;
		if (obj == NULL)
			return;
		else if (!(object = get_obj_in_list_vis(ch, obj_name,
			ch->carrying))) 
			continue;
		else
			found = TRUE;
	}
	
	if (found == FALSE) {
		sprintf(buf, "You don't have %s in your inventory!\r\n",
			obj_name);
		send_to_char(buf, ch);
		return;
	}

	if (found && (GET_OBJ_EXTRA(object) & ITEM_MAGIC)) {
		sprintf(buf, "You are unable to initiate nuclear fission, "
		             "because %s is protected by magical forces.\r\n", obj_name);
		send_to_char(buf, ch);
		return;
	}
	prob += !IS_NPC(ch) ? 2*GET_SKILL(ch, SKILL_FISSION) : number (0, 200)
	  - GET_OBJ_WEIGHT(object);
	if (GET_LEVEL(ch)>=LVL_IMMORT) prob += 100;
	if (prob>=100 && GET_OBJ_TYPE(object) != ITEM_WEAPON && GET_OBJ_TYPE(object) != ITEM_ARMOR) { 
	  extract_obj(object);
	  GET_HIT(ch)  = MIN(GET_MAX_HIT(ch),
	    GET_HIT(ch)+GET_OBJ_WEIGHT(object) + GET_LEVEL(ch) / 4 + number (1, 6));
	  GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch),
	    GET_MOVE(ch)+GET_OBJ_WEIGHT(object) + GET_LEVEL(ch) / 4 + number (1, 6));
	  sprintf(buf, "You ignite nuclear fission and %s disapeared in flash of light!\r\n", obj_name);
		send_to_char(buf, ch);
	  act("An object in $n's hands disappeared in a flash of blinding light!\r\n",
		    FALSE, ch, 0,0, TO_ROOM);
	  improve_skill(ch, SKILL_FISSION);
	}
	else {
	  GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - number(2, 10));
	  sprintf(buf, "You failed an attempt to ignite nuclear fission!\r\n");
		send_to_char(buf, ch);
	  act("$n produces a wave of neutrons!\r\n",
		    FALSE, ch, 0,0, TO_ROOM);
	
	}
}

ACMD(do_retreat)
{
 int prob, percent, dir = 0;
 int retreat_type;

 one_argument(argument, arg);

 if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_RETREAT) == 0) {
   send_to_char("You better should try to flee instead of retreat!\r\n", ch);
   return;
 }
 if (!FIGHTING(ch))
   {
   send_to_char("You are not fighting!\r\n", ch);
   return;
   }
 if (!*arg)
   {
   send_to_char("Retreat where?!?\r\n", ch);
   return;
   }

 retreat_type = search_block(arg, dirs, FALSE);

 if (retreat_type < 0 || !EXIT(ch, retreat_type) ||
   EXIT(ch, retreat_type)->to_room == NOWHERE)
   {
   send_to_char("Retreat where?\r\n", ch);
   return;
   }

 percent = !IS_NPC(ch) ?
              GET_SKILL(ch, SKILL_RETREAT) : 50;
 prob = number(0, 101);

 if (prob <= percent){
    if (CAN_GO(ch, dir) && !IS_SET(ROOM_FLAGS(EXIT(ch,dir)->to_room),
ROOM_DEATH))
  {
       act("$n skillfully retreats from combat.", TRUE, ch, 0, 0,
TO_ROOM);
  send_to_char("You skillfully retreat from combat.\r\n", ch);
  WAIT_STATE(ch, PULSE_VIOLENCE);
  improve_skill(ch, SKILL_RETREAT);
  do_simple_move(ch, dir, TRUE);
  if (FIGHTING(ch) && FIGHTING(FIGHTING(ch)) == ch)
   stop_fighting(FIGHTING(ch));
  stop_fighting(ch);
  } else {
  act("$n tries to retreat from combat but has no where to go!", TRUE,
ch,
     0, 0, TO_ROOM);
  send_to_char("You cannot retreat in that direction!\r\n", ch);
       return;
  }
 } else {
    send_to_char("You fail your attempt to retreat!\r\n", ch);
    WAIT_STATE(ch, PULSE_VIOLENCE);
    return;
  }
}
