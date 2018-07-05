/***********
* dempower.c Special file for Darkened Lights Codebase.   *
*                                                **********/

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
extern struct zone_data *zone_table;    /*. db.c        .*/


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
int mag_savingthrow(struct char_data * ch, int type);

/* local */
ACMD(do_gut);

ACMD(do_dragonform)
{
    if (GET_CLASS(ch) != CLASS_WARRIOR) {
  send_to_char("Huh!?!!?\r\n",ch);
    return;
  }

 if (GET_MITUS(ch) < 2) {
   send_to_char("You have to learn to expand first!",ch);
    return;
}

          if (GET_HYBRID(ch) < 1) {
     send_to_char("Dragonform requires level 1 hybrid.\r\n",ch);
     return;
}

if (GET_HYBRID(ch) > 0) {

  if (PLR_FLAGGED(ch, PLR_DRAGONFORM)) {
    act("$n closes thier eyes and virtually morphs into a dragon before your eyes.", 
TRUE, ch, 0, 0, TO_ROOM);
    act("You close your eyes and virtually shrink before everyone's eyes.", TRUE, ch, 0, 0, TO_CHAR);
  //GET_POS(ch) = POS_STANDING;
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DRAGONFORM);
    ch->points.max_hit -= 200;
    ch->points.hitroll -= 35;
    ch->points.damroll -= 35;
  return; }

//if (!PLR_FLAGGED(ch, PLR_EXPAND)) {

  if (GET_MOVE(ch) < 500) {
    act("You don't have the energy required to dragonform.", TRUE, ch, 0, 0, TO_CHAR);
    return; } else {
    act("$n's body shifts as it morphs into a huge dragon.", 
TRUE, ch, 0, 0, TO_ROOM);
    act("Your body shifts as it morphs into a huge dragon.", 
TRUE, ch, 0, 0, TO_CHAR);
  //  GET_POS(ch) = POS_EXPAND; /* change back to POS_EXPAND */
    SET_BIT(PLR_FLAGS(ch), PLR_DRAGONFORM);
    ch->points.max_hit += 200;
    ch->points.hitroll += 35;
    ch->points.damroll += 35;
    ch->points.move -= 500;
    return; }}} //}

ACMD(do_shitfaced)
{
  struct raff_node *raff;

if (GET_CLASS(ch) != CLASS_WARRIOR) {
  send_to_char("Huh?\r\n",ch);
  return;
}

        SET_BIT(ROOM_AFFECTIONS(raff->room), RAFF_FOG);
}



ACMD(do_swallow)
{
	struct obj_data *obj, *next_obj, *object;
	char obj_name[MAX_STRING_LENGTH];
	int found = FALSE, prob = 0;

	one_argument(argument, obj_name);
if (GET_CLASS(ch) != CLASS_WARRIOR) {
	send_to_char("You have no idea how to swallow items!\r\n", ch);
		return;
	}
       if (GET_XIAN(ch) < 4) {
     send_to_char("Fission requires level 4 xian.\r\n",ch);
     return;
}

if (GET_XIAN(ch) > 3) {

	if (!obj_name || !*obj_name) {
		send_to_char("What do you wish to swallow for energy?\r\n", ch);
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

//	if (found && (GET_OBJ_EXTRA(object) & ITEM_MAGIC)) {
//		sprintf(buf, "You are unable to swallow this item," 
//   "because %s is protected by magical forces.\r\n", obj_name);
//		send_to_char(buf, ch);
//		return;
//	}
	prob = number(1, 101);
	if (GET_LEVEL(ch)>=LVL_IMMORT) prob += 100;
	//if (prob>=100 && GET_OBJ_TYPE(object) != ITEM_WEAPON && GET_OBJ_TYPE(object) != ITEM_ARMOR) { 
	  extract_obj(object);
	  GET_HIT(ch)  = MIN(GET_MAX_HIT(ch),
	    GET_HIT(ch)+GET_OBJ_WEIGHT(object) + GET_LEVEL(ch) / 4 + 
number (1, 6));
	  GET_MOVE(ch) = MIN(GET_MAX_MOVE(ch),
	    GET_MOVE(ch)+GET_OBJ_WEIGHT(object) + GET_LEVEL(ch) / 4 + 
number (1, 6));
	  sprintf(buf, "You eyes glow bright green as you swallow %s for power!\r\n", obj_name);
		send_to_char(buf, ch);
	  act("$n swallows an object and their eyes glow with a nuclear green!\r\n",
		    FALSE, ch, 0,0, TO_ROOM);
	 GET_MOVE(ch) += 200;
	}
	else {
	  GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - number(2, 10));
	  sprintf(buf, "You failed an attempt to swallow!\r\n");
		send_to_char(buf, ch);
	  act("$n produces a wave of neutrons!\r\n",
		    FALSE, ch, 0,0, TO_ROOM);
	
	}
}

ACMD(do_frighten)
{
  struct char_data *victim, *next_v;
  int rooms_to_flee = 0;
  int level;

  ACMD(do_flee);

  if (ch == NULL)
        return;

       if (GET_XIAN(ch) < 6) {
     send_to_char("frighten requires level 6 xian.\r\n",ch);
     return;
}

if (GET_XIAN(ch) > 5) {

  send_to_char("You radiate an aura of fear into the room!\r\n", ch);
  act("$n is surrounded by an aura of fear!", TRUE, ch, 0, 0, TO_ROOM);

                for(victim=world[ch->in_room].people;victim;victim=next_v)
        {
                next_v=victim->next_in_room;

      if (next_v == NULL)
       return;
		
	
	       	
	if (GET_LEVEL(victim) >= LVL_IMMORT) 
               continue;


	if (mag_savingthrow(victim, 1)) {
       sprintf(buf, "%s is unaffected by the fear!\r\n", GET_NAME(victim));
       act(buf, TRUE, ch, 0, 0, TO_ROOM);
       send_to_char("Your victim is not afraid of the likes of you!\r\n", ch);
       if (IS_NPC(victim))
	 hit(victim, ch, TYPE_UNDEFINED);
	       } else {
    for(rooms_to_flee = level / 10; rooms_to_flee > 0; rooms_to_flee--) {
    send_to_char("You flee in terror!\r\n", victim);
    do_flee(victim, "", 0, 0);
    }
   }
  }	
 }
}

ACMD(do_shadowplane)
{
  int to_room;

  to_room = 14200;

  if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh?!?",ch);
    return;
}

         if (GET_MULIAN(ch) < 1) {
     send_to_char("Shadowplane requires level 1 mulian.\r\n",ch);
     return;
}

if (GET_MULIAN(ch) > 0) {
  send_to_char("You harness the power of the hell, taking yourself "
               "to the shadowplane.\r\n", ch);
  act("$n harnesses the power of hell, taking $mself to the shadowplane.",
      FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, real_room(to_room));
  act("$n slowly fades into the shadowplane.", FALSE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  ch->points.move -= 500;
}
}

ACMD(do_invis_weapon)
{
//   struct obj_data *obj;
}


void do_demoneq( struct char_data *ch, char *argument )
{
     struct obj_data *obj;
     char arg[MAX_INPUT_LENGTH];
     char arg2[MAX_INPUT_LENGTH];
     int r_num = 29600; /* sword */
     int vnum = 29601; /* dagger */
     int cnum = 29602; /* breast plate */
     int anum = 29603; /* Bracer */
     int bnum = 29606; /* ring */
    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

     if (IS_NPC(ch)) return;

     if (GET_CLASS(ch) != CLASS_WARRIOR)
     {
         send_to_char("Huh?\n\r", ch);
        return;
     }

    if (arg[0] == '\0') {
    send_to_char("Syntax: Demon eq <piece>\r\n",ch);
            send_to_char("Please specify which piece of demon armor you wish"
" to make: Ring Collar\n\rPlate Helmet Leggings Boots Gauntlets Sleeves Cape"
" Belt Bracer Visor.\n\r",ch);
    return;
}
    if (arg2[0] == '\0')
    {
	send_to_char("Please specify which piece of demon armor you wish" 
"to make: Ring Collar\n\rPlate Helmet Leggings Boots Gauntlets Sleeves Cape" 
"Belt Bracer Visor.\n\r",ch);
	return;
    }

   // if (!str_cmp(arg2,"sword"     )) {
   // if (!str_cmp(arg2,"dagger"   ))
   // }
     if ( GET_DEMONXP(ch) < 10000 )
     {
        send_to_char("It costs 10,050 resource points to create an unrethreal demon equipment\r\n",ch);
        return;
     }
    if (!str_cmp(arg2,"sword"     )) {
    if (GET_DEMONXP(ch) > 10000 ) {
     obj = read_object(r_num, VIRTUAL);
     obj_to_char(obj, ch);
     act("$p appears in your hands in a flash of light.",FALSE,ch,obj,0,TO_CHAR);
     act("$p appears in $n's hands in a flash of light.",FALSE,ch,obj,0,TO_ROOM);
GET_DEMONXP(ch)   -= 10000;
     return;
  }
}
  if (!str_cmp(arg2,"bracer"   )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(anum, VIRTUAL);
     obj_to_char(obj, ch);
     act("$p appears in your hands in a flash of light.",FALSE,ch,obj,0,TO_CHAR);
     act("$p appears in $n's hands in a flash of light.",FALSE,ch,obj,0,TO_ROOM);
GET_GOLD(ch)   -= 10000;
     return;
  }
}

    if (!str_cmp(arg2,"ring"     )) {
    if (GET_DEMONXP(ch) > 10000 ) {
     obj = read_object(bnum, VIRTUAL);
     obj_to_char(obj, ch);
     act("$p appears in your hands in a flash of light.",FALSE,ch,obj,0,TO_CHAR);
     act("$p appears in $n's hands in a flash of light.",FALSE,ch,obj,0,TO_ROOM);
GET_DEMONXP(ch)   -= 10000;
     return;
  }
}

  if (!str_cmp(arg2,"dagger"   )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(vnum, VIRTUAL);
     obj_to_char(obj, ch);
     act("$p appears in your hands in a flash of light.",FALSE,ch,obj,0,
TO_CHAR);
     act("$p appears in $n's hands in a flash of light.",FALSE,ch,obj,0,
TO_ROOM);
GET_GOLD(ch)   -= 10000;
     return;
  }
}
  if (!str_cmp(arg2,"armor"   )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(cnum, VIRTUAL);
     obj_to_char(obj, ch);
     act("$p appears in your hands in a flash of light.",FALSE,ch,obj,0,
TO_CHAR);
     act("$p appears in $n's hands in a flash of light.",FALSE,ch,obj,0,
TO_ROOM);
GET_GOLD(ch)   -= 10000;
     return;
  }
}

}




ACMD(do_claws)
{
//  struct char_data *victim;
 // int attacktype = 8;

  if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh?\r\n", ch);
    return; }

          if (GET_MULIAN(ch) < 2) {
     send_to_char("Claws requires level 2 mulian.\r\n",ch);
     return;
}

if (GET_MULIAN(ch) > 1) {

  if (PLR_FLAGGED(ch, PLR_CLAWS)) {
    act("$n's claws slide back into their fingertips.", TRUE, ch, 0, 0, 
TO_ROOM);
    act("You slide your claws back into your fingertips.", TRUE, ch, 0, 0, 
TO_CHAR);
   REMOVE_BIT(PLR_FLAGS(ch), PLR_CLAWS);
  return;} else {
    act("$n slides razor sharp talons from $e fingertips.", TRUE, ch, 0, 0, TO_ROOM);
    act("You fingertips split as razor sharp talons slide out from them.", TRUE, ch, 0, 0, 
TO_CHAR);
 SET_BIT(PLR_FLAGS(ch), PLR_CLAWS);
   ch->points.hitroll += 5;
   ch->points.damroll += 5;
    //	perform_remove(ch, WEAR_WIELD, TRUE);
//	perform_remove(ch, WEAR_HOLD, TRUE);
    return;} }
}
ACMD(do_fangs)
{
//  struct char_data *victim;
 // int attacktype = 8;

  if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh?\r\n", ch);
    return; }

          if (GET_MULIAN(ch) < 3) {
     send_to_char("Fangs requires level 3 mulian.\r\n",ch);
     return;
     }

if (GET_MULIAN(ch) > 2) {


  if (PLR_FLAGGED(ch, PLR_FANGS)) {
    act("$n's fangs slide back into their gums.", TRUE, ch, 0, 0, TO_ROOM);
    act("You slide your fangs back into your gums.", TRUE, ch, 0, 0, TO_CHAR);
   REMOVE_BIT(PLR_FLAGS(ch), PLR_FANGS);
  return;}
 else {
    act("$n slides razor sharp fangs from $e gums.", TRUE, ch, 0, 0, TO_ROOM);
    act("You gums split as razor sharp fangs slide out from them.", TRUE, ch, 0, 0,
TO_CHAR);
 SET_BIT(PLR_FLAGS(ch), PLR_FANGS);
   ch->points.hitroll += 5;
   ch->points.damroll += 5;
      //  perform_remove(ch, WEAR_WIELD, TRUE);
      //  perform_remove(ch, WEAR_HOLD, TRUE);
    return;} }
}
ACMD(do_piledriver)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_WARRIOR) {
send_to_char("Huh?!?\r\n",ch);
    return;
  }
              if (GET_MULIAN(ch) < 4) {
     send_to_char("Piledriver requires level 4 mulian.\r\n",ch);
     return;
}

if (GET_MULIAN(ch) > 3) {


  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Piledrive who?\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 200) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else 
    sprintf(buf, "You execute a fully inverted piledriver on %s!\r\n",GET_NAME(vict));
    act("$N fully inverts you and drives you to the ground head first!\r\n", FALSE,vict, 0, ch,TO_CHAR);
    act("$n inverts $N and drives them head first into the ground!\r\n", 
FALSE, ch, 0, vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 500;
    vict->points.hit -= 200;
  } else {
    sprintf(buf, "You execute a fully inverted piledriver on %s!\r\n",GET_NAME(vict));
    act("$N fully inverts you and drives you to the ground head first!\r\n", FALSE,vict, 0, ch,TO_CHAR);
    act("$n inverts $N and drives them head first into the ground!\r\n", 
FALSE, ch, 0, vict, TO_NOTVICT);
    damage(ch, vict, GET_LEVEL(ch) >> 1, GET_LEVEL(ch));
    ch->points.move -= 500;
    vict->points.hit -= 200;
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}
}

ACMD(do_mist)
{
 // struct affected_type *af;
  // struct char_data *ch;
  

  if (GET_CLASS(ch) != CLASS_WARRIOR) 
  {
      send_to_char("Huh?!?\r\n",ch);
      return;
  }

  if (GET_MULIAN(ch) < 6) 
  {
     send_to_char("Mist requires level 6 mulian.\r\n",ch);
     return;
  }

  if (GET_MULIAN(ch) > 5) 
  {
  

// if (ROOM_AFFECTED(ch->in_room, RAFF_FOG)) {
//   send_to_char("You suck the clouds from the room!",ch);
//    REMOVE_BIT(ROOM_AFFECTIONS(ch->in_room), RAFF_FOG);
//   return;
//} else


    if (GET_MANA(ch) > 200) 
    {
    	send_to_char("You open your mouth and suck all light from the room\r\n",ch);
    	SET_BIT(ROOM_AFFECTIONS(ch->in_room), RAFF_FOG);  
    	return;
    }
  }
}
/*
ACMD(do_scry)
{
  struct char_data *vict;
  one_argument(argument, arg);
  sh_int location;

  if ((location = find_target_room(ch, argument)) < 0)
    return;

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Scry who?\r\n",ch);
      return;
    }
  }
    look_at_room(vict, 0);
}
*/
ACMD(do_spit_venom)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_WARRIOR) {
send_to_char("Huh?!?\r\n",ch);
    return;
  }
         if (GET_HYBRID(ch) < 2) {
     send_to_char("Spitting venom requires level 2 hybrid.\r\n",ch);
     return;
}

if (GET_HYBRID(ch) > 1) {

  
  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Spit poison at who?\r\n",ch);
      return;
    }
  }

  if (AFF_FLAGGED(vict, AFF_POISON)) {
     send_to_char("They are already affect by poison.\r\n",ch);
     return;
}
           if (GET_MOVE(ch) <= 50) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else 
    send_to_char("You hock back and spit a poisonous ball!\r\n",ch);
    act("$N spits a poisonous ball at $n making them ill!\r\n", FALSE,vict, 0, ch,TO_CHAR);
    act("$n's hocks back and spits a poisonous ball at $n making them ill!\r\n", FALSE, ch, 0, vict, TO_NOTVICT);
    ch->points.move -= 50;
    vict->points.hit -= 5;
    SET_BIT(AFF_FLAGS(vict), AFF_POISON);
    WAIT_STATE(ch, PULSE_VIOLENCE * 3);
  }
}
