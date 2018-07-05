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
extern room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
extern void perform_remove(struct char_data * ch, int pos);


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

/* start vampire shit */
ACMD(do_clear_vamp)
{
if (GET_LEVEL(ch) > 110)
{
  send_to_char("Clearing vampire stats.\r\n",ch);
  GET_CELERITY(ch) = 0;
  GET_BRENNUM(ch) = 0;
  GET_AFILASE(ch) = 0;
  GET_TUTULA(ch) = 0;
  GET_DEMONXP(ch) = 0;
  return;
}
}

ACMD(do_divinity)
{    
  skip_spaces(&argument);

  	if (GET_CLASS(ch) != CLASS_THIEF)
	{
	send_to_char("Huh?\n\r",ch);
	return;
	} 

  if (!*argument)
  {
    sprintf(buf,"%s\r\nExtended Class Powers:\r\n-------------------------------------------------------------------\r\n", buf);
     sprintf(buf,"%sCelerity (%d) Brennum (%d) Afilase (%d) Tutula (%d)\r\n",buf, GET_CELERITY(ch), GET_BRENNUM(ch), GET_AFILASE(ch), 
GET_TUTULA(ch));
    sprintf(buf,"%s",buf);
sprintf(buf,"%s-------------------------------------------------------------------\r\nYou have &y%d&w research points you can spend.\r\n",buf,GET_DEMONXP(ch));
    sprintf(buf,"%s\r\n- See HELP VAMPIRE for further information.\r\n",buf);    
    send_to_char(buf,ch);
    return;
  }
  if (GET_DEMONXP(ch) <= 3000) {
    send_to_char("You do not seem to be able to master any skill now.\r\n",ch);
    return;
  }
 
  if (!str_cmp(argument,"celerity"))
    {
      if (GET_DEMONXP(ch) <= 2000) {
    send_to_char("You lack the research points for this divinity.\r\n",ch);
      return;
} 
   if (GET_CELERITY(ch) == 6)
     {send_to_char("You have already researched into celerity enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 2000;  
    GET_CELERITY(ch) += 1;      
} else 
      if (!str_cmp(argument,"brennum"))
    {
      if (GET_DEMONXP(ch) <= 20000) {
    send_to_char("You lack the research points for this divinity.\r\n",ch);
      return;
}
   if (GET_BRENNUM(ch) == 6)
     {send_to_char("You have already researched into Brennum enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 20000;
    GET_BRENNUM(ch) += 1;
} else {
      send_to_char("divine what skill?!?!?!?\r\n",ch);
      return;
    }
  send_to_char("You sacrafice your research points to the mighty gods.\r\n",ch);
  send_to_char("Your divinity increases...\r\n",ch);
  return;
}

ACMD(do_grant_vamp)
{
 if (GET_LEVEL(ch) < LVL_IMMORT) {
   send_to_char("Huh?\r\n", ch);
    return;
}
  if (GET_LEVEL(ch) > LVL_IMMORT) {
     send_to_char("Granting 5000 research points.\r\n",ch);
      GET_DEMONXP(ch) += 5000;
       return;
    }
}

ACMD(do_vamp_damage)
{
//int apr;

if (GET_CLASS(ch) != CLASS_WARRIOR)
   {send_to_char("Huh!?!",ch);
   return;}

if (GET_BRENNUM(ch) > 6) {
   send_to_char("you haven't learned extra damage yet",ch);
     return;

 } 
}


ACMD(do_shadowwalk)
{ 
  struct char_data *vict;
  sh_int location;

  one_argument(argument, arg);

  if ((location = find_target_room(ch, argument)) < 0)
    return;

    if (GET_CLASS(ch) != CLASS_THIEF) {
  send_to_char("Huh?!?!?\r\n",ch);
    return;
  }
     if (GET_BRENNUM(ch) < 5) {
     send_to_char("Shadowwalk requires level 5 brennum!\r\n",ch);
     return;
}

if (GET_BRENNUM(ch) > 4) {
   
  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Shadowwalk to who?\r\n",ch);
      return;
    }
  }   

    if (GET_LEVEL(vict) > LVL_IMMORT) {
     send_to_char("Hey, you cannot shadowwalk to a god stupid!\r\n",ch);
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

    send_to_char("You begin to dissapear into the shadows.\r\n",ch);
    strcpy(buf, "$n slowly dissapears into the shadows and is gone!\r\n");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);

   strcpy(buf, "$n appears from the shadows out of nowhere!");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}}


void do_vampeq( struct char_data *ch, char *argument )
{
     struct obj_data *obj;
     char arg[MAX_INPUT_LENGTH];
     char arg2[MAX_INPUT_LENGTH];
     int r_num = 29600; /* sword */
     int vnum = 29601; /* dagger */
     int cnum = 29602; /* breast plate */
     int anum = 29603; /* Bracer */

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

     if (IS_NPC(ch)) return;

     if (GET_CLASS(ch) != CLASS_THIEF)
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
	send_to_char("Please specify which piece of demon armor you wish" 
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
     act("&WA tiny pixie fairie arrives and hands you $p.",FALSE,ch,obj,0,
TO_CHAR);
     act("&WA tiny pixie fairie arrives and hands $n $p.",FALSE,ch,obj,0,
TO_ROOM);
GET_GOLD(ch)   -= 10000;
     return;
  }
}
  if (!str_cmp(arg2,"armor"   )) {
    if (GET_GOLD(ch) > 10000 ) {
     obj = read_object(cnum, VIRTUAL);
     obj_to_char(obj, ch);
     act("&WA tiny pixie fairie arrives and hands you $p.",FALSE,ch,obj,0,
TO_CHAR);
     act("&WA tiny pixie fairie arrives and hands $n $p.",FALSE,ch,obj,0,
TO_ROOM);
GET_GOLD(ch)   -= 10000;
     return;
  }}}
ACMD(do_vampout)
{
     if (GET_CLASS(ch) != CLASS_THIEF) {
  send_to_char("Huh!?!!?\r\n",ch);
    return;
  }


if (GET_BRENNUM(ch) < 4) {
     send_to_char("You have not yet learned to vampin and out!\r\n",ch);
     return;
}



if (GET_BRENNUM(ch) > 3) {

 if (!PLR_FLAGGED(ch, PLR_VAMPED)) {
     send_to_char("You are already in human-form!\r\n",ch);
      return;
}

  if (PLR_FLAGGED(ch, PLR_VAMPED))/* change this back after made POS */ {
    act("$n closes thier eyes and becomes fully human again.",
TRUE, ch, 0, 0, TO_ROOM);
    act("You close your eyes and disguise yourself in human form.", TRUE, 
ch, 0, 0, TO_CHAR);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_VAMPED);
    ch->points.max_hit -= 200;
    ch->points.hitroll -= 25;
    ch->points.damroll -= 25;
  return; }
}
}

ACMD(do_vampin)
{
    if (GET_CLASS(ch) != CLASS_THIEF) {
  send_to_char("Huh!?!!?\r\n",ch);
    return;
  }
  
if (GET_BRENNUM(ch) < 4) {
     send_to_char("You have not yet learned to vampin and vampout!\r\n",ch);
     return;
}

if (GET_BRENNUM(ch) > 3) {

if (PLR_FLAGGED(ch, PLR_VAMPED)) {
     send_to_char("You are already in vampire-form.\r\n",ch);
      return;
}

  if (GET_MOVE(ch) < 200) {
    act("You don't have the energy required to vamp-in!", TRUE, ch, 0, 0, 
TO_CHAR);
    return; } else {
    act("$n's face deforms and there body pulsates with energy.", 
TRUE, ch, 0, 0, TO_ROOM);
    act("You close your eyes and vamp-out instantaneously.", 
TRUE, ch, 0, 0, TO_CHAR);
    SET_BIT(PLR_FLAGS(ch), PLR_VAMPED);
    ch->points.max_hit += 200;
    ch->points.hitroll += 25;
    ch->points.damroll += 25;
    ch->points.move -= 200;
    return; }}}

void do_nightsight(struct char_data *ch, char *argument)
{
   if (GET_CELERITY(ch) < 3) {
     send_to_char("Nightsight requires level 3 brennum!\r\n",ch);
     return;
}

if (GET_CELERITY(ch) > 2) { 
        SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
	send_to_char("Your eyes glow with a permanent red effect!\n\r",ch);
       return;
     ch->points.move -= 200; 
   } 
 return;
}


ACMD(do_retract)
{
//  struct char_data *victim;
 // int attacktype = 8;

  if (GET_CLASS(ch) != CLASS_THIEF) {
    send_to_char("Huh?\r\n", ch);
    return; }

if (GET_CELERITY(ch) < 1) {
     send_to_char("You have not yet learned to retract your fangs!\r\n",ch);
     return;
}

if (GET_CELERITY(ch) > 0) {



  if (PLR_FLAGGED(ch, PLR_FANGS)) {
    act("$n's fangs slide back into their gums.", TRUE, ch, 0, 0, 
TO_ROOM);
    act("You slide your fangs back into your gums.", TRUE, ch, 0, 0, 
TO_CHAR);
   REMOVE_BIT(PLR_FLAGS(ch), PLR_FANGS);
  return;}
 else {
    act("$n slides razor sharp fangs from $e gums.", TRUE, ch, 0, 0, 
TO_ROOM);
    act("You gums split as razor sharp fangs slide out from them.", TRUE, 
ch, 0, 0,
TO_CHAR);
 SET_BIT(PLR_FLAGS(ch), PLR_FANGS);
   ch->points.hitroll += 5;
   ch->points.damroll += 5;
      //  perform_remove(ch, WEAR_WIELD, TRUE);
      //  perform_remove(ch, WEAR_HOLD, TRUE);
    return;} }
}

ACMD(do_talon_twist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_THIEF) {
send_to_char("Huh?!?\r\n",ch);
    return;
  }
       if (GET_CELERITY(ch) < 6) {
     send_to_char("You have not yet learned to talon twist!\r\n",ch);
     return;
}

if (GET_CELERITY(ch) > 5) {


  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Talon twist whom?\r\n",ch);
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
  } else 
     SET_BIT(PLR_FLAGS(ch), PLR_CLAWS);
    send_to_char("You begin to twist in a 360 type motion with talons wielded!\r\n",ch);
    act("$N spins quickly registering sharp talons to your belly!\r\n", FALSE,vict, 0, ch,TO_CHAR);
    act("$n executes a 360 spin slicing $N's belly with deadly talons!\r\n", FALSE, ch, 0, vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 200;
    vict->points.hit -= 90;
  } else {
    send_to_char("You begin to twist in a 360 type motion with talons wielded!\r\n",ch);
    act("$N spins quickly registering sharp talons to your belly!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n executes a 360 spin slicing $N's belly with deadly talons!\r\n", FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, GET_LEVEL(ch) >> 1, GET_LEVEL(ch));
    ch->points.move -= 200;
    vict->points.hit -= 90;
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}
}

ACMD(do_burning)
{
  if (GET_CLASS(ch) != CLASS_THIEF) {
    send_to_char("Huh?\r\n", ch);
    return; }
if (GET_BRENNUM(ch) < 1) {
     send_to_char("You have not yet learned to summon a globe of fire!\r\n",ch);
     return;
}

if (GET_BRENNUM(ch) > 0) {



  if (IS_AFFECTED(ch, AFF_SHIELD)) {
    act("$n swarns away a shield of fire.", TRUE, ch, 0, 0, TO_ROOM);
    act("You swarn away a shield of fire.", TRUE, ch, 0, 0, TO_CHAR);
    REMOVE_BIT(AFF_FLAGS(ch), AFF_SHIELD);
    GET_DEX(ch) -= 2;
  return;}


 else {
    act("$n summons a shield of fire to protect $e.", TRUE, ch, 0, 
0, TO_ROOM);
    act("&rYou summon a shield of fire to protect yourself.", 
TRUE, ch, 0, 0, TO_CHAR);
    SET_BIT(AFF_FLAGS(ch), AFF_SHIELD);
    GET_DEX(ch) += 2;
    return;} } }


ACMD(do_freeze)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_THIEF) {
send_to_char("Huh?!?\r\n",ch);
    return;
  }
       if (GET_CELERITY(ch) < 3)
    {
     send_to_char("Freeze requires celerity level 3 to function!\r\n",ch);
      return;
} else
 if (GET_CELERITY(ch) > 2) {


  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Cast an ice cloud on who?\r\n",ch);
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
    send_to_char("You mutter 'freeze' and cast a cloud of ice!\r\n",ch);
    act("&b$N mutter's 'freeze' and cast's and ice cloud you.!\r\n", 
FALSE,vict, 0, ch,TO_CHAR);
    act("$n mutter's 'freeze' and cast's and ice cloud on $N\r\n", FALSE, 
ch, 0, vict, TO_NOTVICT);
    SET_BIT(AFF2_FLAGS(vict), AFF2_FREEZING);
    ch->points.move -= 500;
    damage(ch, vict, 0, GET_MAX_HIT(ch));

  //WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}
}

ACMD(do_sense_spawn)
{
 if (GET_CLASS(ch) != CLASS_THIEF) {
    send_to_char("Huh?!?",ch);
      return;
}
   if (GET_CELERITY(ch) < 2) {
   send_to_char("Sense spawn requires level 2 celerity to be preformed.\r\n",ch);
   return;
}
  
  if (GET_CELERITY(ch) > 1) {
if (GET_CLASS(ch) == CLASS_THIEF) {
  sprintf(buf, "This zone should respawn in %d minutes.\r\n",
         (zone_table[world[ch->in_room].zone].lifespan -
          zone_table[world[ch->in_room].zone].age));
  send_to_char(buf, ch);
}
}
}
ACMD(do_talons)
{
//  struct char_data *victim;
 // int attacktype = 8;

  if (GET_CLASS(ch) != CLASS_THIEF) {
    send_to_char("Huh?\r\n", ch);
    return; }

        if (GET_CELERITY(ch) < 5)
    {
     send_to_char("Talons requires celerity level 5 to function!\r\n",ch);
      return;
} else
 if (GET_CELERITY(ch) > 4) {

  if (PLR_FLAGGED(ch, PLR_CLAWS)) {
    act("$n's talons slide back into their fingertips.", TRUE, ch, 0, 0, 
TO_ROOM);
    act("You slide your talons back into your fingertips.", TRUE, ch, 0, 
0, TO_CHAR);
   REMOVE_BIT(PLR_FLAGS(ch), PLR_CLAWS);
  return;}


 else {
    act("$n slides razor sharp talons from $e fingertips.", TRUE, ch, 0, 
0, TO_ROOM);
    act("You fingertips split as razor sharp talons slide out from them.", 
TRUE, ch, 0, 0, 
TO_CHAR);
 SET_BIT(PLR_FLAGS(ch), PLR_CLAWS);
   ch->points.hitroll += 5;
   ch->points.damroll += 5;
    	perform_remove(ch, WEAR_WIELD);
	perform_remove(ch, WEAR_HOLD);
    return;} }
}
