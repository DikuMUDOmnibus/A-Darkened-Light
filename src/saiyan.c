/**********************************************************
*pp   file: saiyan.c                 External Class Powers*
*                                                         *
*       Disclamer, For Darkened lights, external power    *
*   if you use this info put my name (Mike Ryan) in your  *
* credit information. otherwise have fun.                 *
**********************************************************/


#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "clan.h"
//#include "merc.h"

//#include "constants.h"
#include "interpreter.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern int pk_allowed;


// EXTRA EXTERNS
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct int_app_type int_app[];
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern struct obj_data *obj_proto;
extern struct obj_data *obj;
//

/* extern functions */
void raw_kill(struct char_data * ch);
void check_killer(struct char_data * ch, struct char_data * vict);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
void perform_immort_invis(struct char_data *ch, int level);
void perform_immort_vis(struct char_data *ch);
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *obj_index;

ACMD(do_clear_disci)
{
   send_to_char("Clearing Stats.\r\n",ch);
   GET_NINJITSU(ch) = 0;
   GET_SAMURAI(ch)  = 0; 
   GET_BUDOKAI(ch)  = 0;
   GET_KI(ch)       = 0;
  }

ACMD(do_discipline)
{    
  skip_spaces(&argument);

  	if (GET_CLASS(ch) != CLASS_CLERIC)
	{
	send_to_char("Huh?\n\r",ch);
	return;
	} 

  if (!*argument)
  {
    sprintf(buf,"\r\nExtended Class Powers:\r\n-------------------------------------------------------------------\r\n");
     sprintf(buf,"%sNinjitsu (%d)  Kame (%d)  Budokai (%d)  Ki (%d)\r\n",buf, 
GET_NINJITSU(ch), GET_SAMURAI(ch), GET_BUDOKAI(ch), GET_KI(ch));
    sprintf(buf,"%s",buf);
sprintf(buf,"%s-------------------------------------------------------------------\r\nYou have &y%d&w research points you can spend.\r\n",buf,GET_DEMONXP(ch));
    sprintf(buf,"%s\r\n- See HELP SAIYAN for further information.\r\n",buf);    
    send_to_char(buf,ch);
    return;
  }
  if (GET_DEMONXP(ch) <= 2000) {
    send_to_char("You do not seem to be able to train disciplines now.\r\n",ch);
    return;
  }
 
  if (!str_cmp(argument,"ninjitsu"))
    {
      if (GET_DEMONXP(ch) <= 2000) {
    send_to_char("You lack the research points for this discipline.\r\n",ch);
      return;
} 
   if (GET_NINJITSU(ch) == 6)
     {send_to_char("You have already researched into ninjitsu enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 2000;  
    GET_NINJITSU(ch) += 1;      
} else 
      if (!str_cmp(argument,"kame"))
    {
      if (GET_DEMONXP(ch) <= 20000) {
    send_to_char("You lack the research points for this discipline.\r\n",ch);
      return;
}
   if (GET_SAMURAI(ch) == 6)
     {send_to_char("You have already researched into kame enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 20000;
    GET_SAMURAI(ch) += 1;
} else {
      send_to_char("Train what discipline?!?!?!?\r\n",ch);
      return;
    }
  send_to_char("You sacrafice research points to the mighty gods.\r\n",ch);
  send_to_char("Your martial arts increase...\r\n",ch);
  return;
}
 

ACMD(do_unveil)
{
  struct char_data *vict;
 

   one_argument(argument, arg);
  
  if (GET_CLASS(ch) != CLASS_CLERIC) {
    send_to_char("Huh!?!\r\n", ch);
    return;
  }

   if (GET_NINJITSU(ch) < 1) {
     send_to_char("Unveil requires level 1 ninjitsu!\r\n",ch);
     return;
}

if (GET_NINJITSU(ch) > 0) {
  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Blow whoms cover?\r\n",ch);
      return;
    }
  }

  //if (!*arg) {
    if (GET_INVIS_LEV(vict) > 0) {
      send_to_char("You wave your hands and blow their cover ",ch); 
       act("$N ruins your attack as he blows your cover! ",
FALSE,vict,0, ch,TO_CHAR);
     perform_immort_vis(vict);
     return;
} else {   
     send_to_char("There are already completely visible.\r\n",ch);
    }
 }
}

ACMD(do_astralwalk)
{ 
  struct char_data *vict;
  sh_int location;

  one_argument(argument, arg);

  if ((location = find_target_room(ch, argument)) < 0)
    return;

    if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?!?\r\n",ch);
    return;
  }
      if (GET_NINJITSU(ch) < 2) {
     send_to_char("Instant Teleport requires level 2 ninjitsu!\r\n",ch);
     return;
}

if (GET_NINJITSU(ch) > 1) {

   
  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Instantaneously transport to who?\r\n",ch);
      return;
    }
  }   

    if (GET_LEVEL(vict) > LVL_IMMORT) {
     send_to_char("Hey, you cannot IT to a god stupid!\r\n",ch);
     return;
}
   if (PLR_FLAGGED(ch, PLR_DEAD)) {
     send_to_char("How do you expect to do this when you are dead?!?\r\n",ch);  
     return;
}
     if (GET_MOVE(ch) <= 200) {
   send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

    send_to_char("You place two fingers on your forehead and vanish.\r\n",ch);
    strcpy(buf, "$n places two fingers on their forehead and vanishes!\r\n");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);

   strcpy(buf, "With a golden shimmer of light $n appears from seeming nowhere.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}
}
ACMD(do_parry)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (GET_CLASS(ch) != CLASS_CLERIC) {
   send_to_char("Huh?!?\r\n",ch);
    return;
  }
          if (GET_NINJITSU(ch) < 3) {
     send_to_char("Parry requires level 3 ninjitsu!\r\n",ch);
     return;
}

if (GET_NINJITSU(ch) > 2) {


  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Who's attacks would you like to parry.\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 20) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else
    send_to_char("You successfuly parry your apponents attack.\r\n",ch);
    act("$N skillfuly parries your blows.\r\n", FALSE,vict, 0, ch,TO_CHAR);
    act("$n fully side steps $N's attack with a grin!\r\n",FALSE, ch, 0, vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 50;
    vict->points.hit -= 90;
  } else {
     send_to_char("You skillfuly parry your apponents blows.\r\n",ch);
    act("$n skillfuly parries %N's blows.\r\n",FALSE,vict,0,
ch,TO_CHAR);
    act("$n's parries are very nice.\r\n",
FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, GET_LEVEL(ch) >> 1, GET_LEVEL(ch));
    ch->points.move -= 50;
    vict->points.hit -= 90;
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}}




ACMD(do_enchant)
{
  struct obj_data *obj;
    int i;
    int level;
   if (GET_CLASS(ch) != CLASS_WARRIOR) {
      send_to_char("Huh?!?\r\n",ch);
      return;
     }

             if (GET_HYBRID(ch) < 4) {
     send_to_char("Enchantments requires level 4 Hybrid!\r\n",ch);
     return;
}

if (GET_HYBRID(ch) > 3) {

   one_argument(argument, arg);
   level = MAX(MIN(level, LVL_IMPL), 1);

     if (!*arg) 
    send_to_char("You don't own anything like that!\r\n",ch);
      else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else {

   if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
   OBJ_FLAGGED(obj, ITEM_MAGIC)) {
   send_to_char("This item has already been enchanted!\r\n",ch);
   return;
}


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
      act("$p begins to shake violently and expresses a blue aura.", FALSE, ch, obj, 0, TO_CHAR);
    } else if (IS_EVIL(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p begins to shake violently and expresses a red aura.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      act("$p begins to shake violently and expresses a yellow aura.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}
}
}

ACMD(do_deathblow)
{
  struct char_data *vict;
  
  one_argument(argument, arg);

   if (GET_CLASS(ch) != CLASS_CLERIC) {
       send_to_char("Huh?!?",ch);
       return;
       }

   if (GET_NINJITSU(ch) < 4) {
     send_to_char("Deathblow requires level 4 ninjitsu!\r\n",ch);
     return;
}

if (GET_NINJITSU(ch) > 3) {


  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Deathblow who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", 
ch);
    return;
  } else
     send_to_char("You strike with an incredible force.\r\n",ch);
    act("$N's skillfull deathblow strikes you HARD!\r\n",FALSE,vict,0, 
ch,TO_CHAR);
    act("$n's skillfull deathblow strikes $n knocking $n's balance off!\r\n",FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 90;
    vict->points.hit -= 30;  
}
}


ACMD(do_trip)
{

  struct char_data *vict;
 

  
  if (GET_CLASS(ch) != CLASS_CLERIC) {
    send_to_char("Huh?!?\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Trip who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("You trip yourself.\r\n", ch);
    GET_POS(ch) = POS_SITTING;
    return;
  }
 
  if (GET_POS(vict) <= POS_SITTING) {
    send_to_char("How will you trip someone who is already on the ground?\r\n", ch);
    return;
  }


  if (MOB_FLAGGED(vict, MOB_NOBASH)) {
    send_to_char("You have little or no affect on them.\r\n",ch);
    return;
    }

  if (IS_AFFECTED(vict, AFF_FLYING)) {
    send_to_char("Your going to trip someone who is flying?\r\n",ch);
    return;
    } else
     send_to_char("You sweep the floor with one leg!\r\n",ch);
    act("$n hops down and sweeps the floor with one leg and knocks you on your ass!!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$N skillfully sweeps the floor with a leg and knocks $n on their ass!\r\n",FALSE, ch,0,vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 90;
    vict->points.hit -= 30;
}

ACMD(do_punch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Punch who?\r\n",ch);
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
  } else {
    send_to_char("You throw a punch!\r\n",ch);
    act("$N punches you!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n throws a punch at $N!\r\n",FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 50;
    vict->points.hit -= 10;
  }
}
ACMD(do_right_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Rightkick who?\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 50) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else {
    send_to_char("You execute a right-kick!\r\n",ch);
    act("$N delivers a deadly right-kick!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n delivers a deadly right-kick to $N!\r\n",FALSE, ch, 0,vict, 
TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 50;
    vict->points.hit -= 10;
  }
}
ACMD(do_left_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Leftkick who?\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 50) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else {
    send_to_char("You execute a left-kick!\r\n",ch);
    act("$N delivers a deadly left-kick!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n delivers a deadly left-kick to $N!\r\n",FALSE, ch, 0,vict, 
TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 50;
    vict->points.hit -= 10;
  }
}
ACMD(do_side_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Sidekick who?\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 50) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else {
    send_to_char("You execute a side-kick!\r\n",ch);
    act("$N delivers a deadly side-kick!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n delivers a deadly side-kick to $N!\r\n",FALSE, ch, 0,vict, 
TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 50;
    vict->points.hit -= 10;
  }
}

ACMD(do_invert_kick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Invertkick who?\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 50) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else {
    send_to_char("You execute a inverted-kick!\r\n",ch);
    act("$N delivers a deadly inverted-kick!\r\n",FALSE,vict,0, 
ch,TO_CHAR);
    act("$n delivers a deadly inverted-kick to $N!\r\n",FALSE, ch, 0,vict, 
TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 50;
    vict->points.hit -= 10;
  }
}

ACMD(do_elbow)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Elbow who?\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 50) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else {
    send_to_char("You deliver a sift elbow to the chest!\r\n",ch);
    act("$N delivers a swift elbow to your chest!\r\n",FALSE,vict,0, 
ch,TO_CHAR);
    act("$n delivers a swift elbow to the chest of $N!\r\n",FALSE, ch, 
0,vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 50;
    vict->points.hit -= 10;
  }
}

ACMD(do_kamehameha)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kamehameha who?\r\n",ch);
      return;
    }
  }
           if (GET_MOVE(ch) <= 500) {
    send_to_char("You lack the movement points for this power.\r\n",ch);
    send_to_char("Use 'train' to train your stats up!\r\n",ch);
    return; }

  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n",ch);
    return;
  } else {
    send_to_char("You focus and scream 'KA-ME-HA-ME....HA'.!\r\n",ch);
    act("KA-ME-HA-ME....HA $N completely **DECIMATES** you with a ki blast!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n screams 'KA-ME-HA-ME....HA'. and  completely **DECIMATES** $N with a ki blast!\r\n",FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 500;
    vict->points.hit -= 100;
  }
}

ACMD(do_power_up)
{
  one_argument(argument, arg);

    if (GET_CLASS(ch) != CLASS_CLERIC) {
  send_to_char("Huh!?!!?\r\n",ch);
    return;
  }

     else if (!*argument)
    { 
act("&y       0o-------------------------o0", TRUE, ch, 0, 0, TO_CHAR);
act("       |   Super Saiyan Forms      |", TRUE, ch, 0, 0, TO_CHAR);
act("       |---------------------------|", TRUE, ch, 0, 0, TO_CHAR);
act("       | You can do the following: |", TRUE, ch, 0, 0, TO_CHAR);
act("       |---------------------------|", TRUE, ch, 0, 0, TO_CHAR);
act("       |Stage:  Power Level Req.:  |", TRUE, ch, 0, 0, TO_CHAR);
act("       |First              800000  |", TRUE, ch, 0, 0, TO_CHAR);
act("       |Second             5000000 |", TRUE, ch, 0, 0, TO_CHAR);
act("       |Third              10000000|", TRUE, ch, 0, 0, TO_CHAR);
act("       |Fourth             42500000|", TRUE, ch, 0, 0, TO_CHAR);
act("       0o-------------------------o0&w", TRUE, ch, 0, 0, TO_CHAR);
return;
} 
  

      if (!str_cmp(arg,"first") && GET_POWERL(ch) < 200) {
    act("You don't have the powerlevel required to powerup.", TRUE, ch, 0, 0, TO_CHAR);
    return; } else {
    act("&y$n closes $e eyes and begins to growl as $e hair stands on end and turn to a bright gold.", TRUE, ch, 0, 0, TO_ROOM);
    act("You close your eyes and scream as your hair stands on end and begins to turn yellow.", TRUE, ch, 0, 0, TO_CHAR);
    SET_BIT(PLR_FLAGS(ch), PLR_POWERUP);
    ch->points.max_hit += 10;
    ch->points.hitroll += 5;
    ch->points.damroll += 5;
    ch->points.move -= 200;
    return; }}


void do_kiequip( struct char_data *ch, char *argument )
{
     struct obj_data *obj;
     char arg[MAX_INPUT_LENGTH];
     char arg2[MAX_INPUT_LENGTH];
     int r_num = 29700; /* sword */
     int vnum = 29701; /* dagger */
     int cnum = 29702; /* breast plate */
     int anum = 29703; /* Bracer */

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

     if (IS_NPC(ch)) return;

     if (GET_CLASS(ch) != CLASS_CLERIC)
     {
         send_to_char("Huh?\n\r", ch);
        return;
     }

    if (arg[0] == '\0') {
    send_to_char("Syntax: Vampeq eq <piece>\r\n",ch);
            send_to_char("Please specify which piece of demon armor you wish"
" to make: Ring Collar\n\rPlate Helmet Leggings Boots Gauntlets Sleeves Cape"
" Belt Bracer Visor.\n\r",ch);
    return;
}
    if (arg2[0] == '\0')
    {
	send_to_char("Please specify the type of equipment you wish" 
"to make: Ring Collar\n\rPlate Helmet Leggings Boots Gauntlets Sleeves Cape" 
"Belt Bracer Visor.\n\r",ch);
	return;
    }

  
     if ( GET_GOLD(ch) < 10000 )
     {
        send_to_char("It costs 10,050 gold pieces to create an dynamic vampire equipment\r\n",ch);
        return;
     }
    if (!str_cmp(arg2,"sword"     )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(r_num, VIRTUAL);
     obj_to_char(obj, ch);
     act("&WA tiny pixie fairie arrives and hands you $p.",FALSE,ch,obj,0,TO_CHAR);
     act("&WA tiny pixie fairie arrives and hands $n $p.",FALSE,ch,obj,0,TO_ROOM);
     GET_GOLD(ch)   -= 10000;
     return;
  }
}
  if (!str_cmp(arg2,"bracer"   )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(anum, VIRTUAL);
     obj_to_char(obj, ch);
     act("&WA tiny pixie fairie arrives and hands you $p.",FALSE,ch,obj,0,TO_CHAR);
     act("&WA tiny pixie fairie arrives and hands $n $p.",FALSE,ch,obj,0,TO_ROOM);
     GET_GOLD(ch)   -= 10000;
     return;
  }
}

  if (!str_cmp(arg2,"dagger"   )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(vnum, VIRTUAL);
     obj_to_char(obj, ch);
     act("&WA tiny pixie fairie arrives and hands you $p.",FALSE,ch,obj,0,TO_CHAR);
     act("&WA tiny pixie fairie arrives and hands $n $p.",FALSE,ch,obj,0,TO_ROOM);
GET_GOLD(ch)   -= 10000;
     return;
  }
}
  if (!str_cmp(arg2,"armor"   )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(cnum, VIRTUAL);
     obj_to_char(obj, ch);
     act("&WA tiny pixie fairie arrives and hands you $p.",FALSE,ch,obj,0,TO_CHAR);
     act("&WA tiny pixie fairie arrives and hands $n $p.",FALSE,ch,obj,0,TO_ROOM);
GET_GOLD(ch)   -= 10000;
     return;
  }}}
