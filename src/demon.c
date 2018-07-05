/**********************************************************
*pp   file: demon.c                External Class Powers*
*                                                         *
*       Disclamer, For Darkened lights, external power    *
*   if you use this info put my name (Mike Ryan) in your  *
* credit information, otherwise have fun.                 *
**********************************************************/

/* For the intended use of class power mech. */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern int pk_allowed;



/* extern functions */
void raw_kill(struct char_data * ch);
void check_killer(struct char_data * ch, struct char_data * vict);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
void perform_immort_invis(struct char_data *ch, int level);
void perform_immort_vis(struct char_data *ch);

/* this code clears the players research points in case of a build-up of too much points
   seeing that i have no idea how to put a max cap on it.*/
ACMD(do_clear_research) 
{
if (GET_LEVEL(ch) > 0 )
{
  send_to_char("Clearing your research points.\r\n",ch);
  GET_DEMONXP(ch) = 1;
  return;
}
}
/* this code is intended for imp's to clear there stats for practicing the commands. */
ACMD(do_clear_demon)
{
if (GET_LEVEL(ch) > 110)
{
  send_to_char("Clearing demon stats.\r\n",ch);
  GET_MITUS(ch) = 0;
  GET_XIAN(ch) = 0;
  GET_MULIAN(ch) = 0;
  GET_HYBRID(ch) = 0;
  GET_DEMONXP(ch) = 0;
  return;
}
}
 /* this code is the main chunk of the class powers, it is the actual usage for the code
    A player uses, Master as a command to train there powers and gain them by using master 
<power listing> */

ACMD(do_master)
{    
  skip_spaces(&argument);

  	if (GET_CLASS(ch) != CLASS_WARRIOR)
	{
	send_to_char("Huh?\n\r",ch);
	return;
	} 

  if (!*argument)
  {
    sprintf(buf,"\r\nExtended Class Powers:\r\n-------------------------------------------------------------------\r\n");
     sprintf(buf,"%sMitus (%d)  Xian (%d)  Mulian (%d)  Hybrid (%d)\r\n",
buf, GET_MITUS(ch), GET_XIAN(ch), GET_MULIAN(ch), GET_HYBRID(ch));
    sprintf(buf,"%s",buf);
sprintf(buf,"%s-------------------------------------------------------------------\r\nYou have &y%d&w research points you can spend.\r\n",buf,GET_DEMONXP(ch));
    sprintf(buf,"%s\r\n- See HELP DEMON for further information.\r\n",buf);    
    send_to_char(buf,ch);
    return;
  }
/*
  if (GET_DEMONXP(ch) <= 19999) {
    send_to_char("You do not seem to be able to master any skill now.\r\n",ch);
    return;
  }
 */
  if (!str_cmp(argument,"mitus"))
    {
      if (GET_DEMONXP(ch) <= 20000) {
    send_to_char("Mitus requires 20,000 souls per level.\r\n",ch);
      return;
} 
   if (GET_MITUS(ch) == 6)
     {send_to_char("You have already researched into mitus enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 20000;  
    GET_MITUS(ch) += 1;      
} else 
      if (!str_cmp(argument,"xian"))
    {
      if (GET_DEMONXP(ch) <= 200000) {
    send_to_char("Xian requires 200,000 souls per level.\r\n",ch);
      return;
}
   if (GET_XIAN(ch) == 6)
     {send_to_char("You have already researched into xian enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 200000;
    GET_XIAN(ch) += 1;
} else
  if (!str_cmp(argument,"mulian"))
    {
      if (GET_DEMONXP(ch) <= 900000) {
    send_to_char("Mulian requires 900,000 souls per level.\r\n",ch);
      return;
}
   if (GET_MULIAN(ch) == 6)
     {send_to_char("You have already researched into Mulian enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 900000;
    GET_MULIAN(ch) += 1;
} else
  if (!str_cmp(argument,"hybrid"))
    {
      if (GET_DEMONXP(ch) <= 1500000) {
    send_to_char("Hybrid requires 1,500,000 souls per level.\r\n",ch);
      return;
}
   if (GET_HYBRID(ch) == 6)
     {send_to_char("You have already researched into hybrid enough.\r\n",ch);return;}
      GET_DEMONXP(ch) -= 1500000;
    GET_HYBRID(ch) += 1;
} else {
      send_to_char("Master what skill?!?!?!?\r\n",ch);
      return;
    }
  send_to_char("You sacrafice your souls to the gods.\r\n",ch);
  send_to_char("Your mastery increases...\r\n",ch);
  return;
}


/* start warrior power skills */

ACMD(do_dropkick)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_WARRIOR) {
  send_to_char("Huh?!?\r\n",ch);
  return;
  }
       if (GET_MITUS(ch) < 1) {
     send_to_char("Dropkick requires level 1 Mitus!\r\n",ch);
     return;
}

if (GET_MITUS(ch) > 0) {

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Dropkick who?\r\n",ch);
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
     send_to_char("Your skillfull dropkick stikes at FULL-FORCE!\r\n",ch);
    act("$N's skillfull dropkick strikes your HARD!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n's skillfull dropkick strikes $n knocking $n's balance off!\r\n",FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 90;
    vict->points.hit -= 30;
  } else {
     send_to_char("Your skillfull dropkick stikes at FULL-FORCE!\r\n",ch);
    act("$N's skillfull dropkick strikes your HARD!\r\n",FALSE,vict,0, ch,TO_CHAR);
    act("$n's skillfull dropkick strikes $n knocking $n's balance off!\r\n", FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, GET_LEVEL(ch) >> 1, GET_LEVEL(ch));
    ch->points.move -= 90;
    vict->points.hit -= 30;
}
}

/* this is a poly morph code. */
ACMD(do_expand)
{
    if (GET_CLASS(ch) != CLASS_WARRIOR) {
  send_to_char("Huh!?!!?\r\n",ch);
    return;
  }
       if (GET_MITUS(ch) < 2) {
     send_to_char("Expanding requires level 2 mitus.\r\n",ch);
     return;
}

if (GET_MITUS(ch) > 1) {
  if (PLR_FLAGGED(ch, PLR_EXPAND))/* change this back after made POS */ {
    act("$n closes thier eyes and virtually shrinks before your eyes.", TRUE, ch, 0, 0, TO_ROOM);
    act("You close your eyes and virtually shrink before everyone's eyes.", TRUE, ch, 0, 0, TO_CHAR);
  //GET_POS(ch) = POS_STANDING;
    REMOVE_BIT(PLR_FLAGS(ch), PLR_EXPAND);
    ch->points.max_hit -= 200;
    ch->points.hitroll -= 25;
    ch->points.damroll -= 25;
  return; }

  if (GET_MOVE(ch) < 200) {
    act("You don't have the energy required to expand.", TRUE, ch, 0, 0, TO_CHAR);
    return; } else {
    act("$n closes $e eyes and begins transforming before your eyes.", TRUE, ch, 0, 0, TO_ROOM);
    act("You close your eyes and begin to transform twice your size.", TRUE, ch, 0, 0, TO_CHAR);
  //  GET_POS(ch) = POS_EXPAND; /* change back to POS_EXPAND */
    SET_BIT(PLR_FLAGS(ch), PLR_EXPAND);
    ch->points.max_hit += 200;
    ch->points.hitroll += 25;
    ch->points.damroll += 25;
    ch->points.move -= 200;
    return; }}}

/* this is a travel command */
ACMD(do_distort)
{ 
  struct char_data *vict;
  sh_int location;

  one_argument(argument, arg);

  if ((location = find_target_room(ch, argument)) < 0)
    return;

    if (GET_CLASS(ch) != CLASS_WARRIOR) {
  send_to_char("Huh?!?!?\r\n",ch);
    return;
  }
          if (GET_MITUS(ch) < 3) {
     send_to_char("Running requires level 3 mitus.\r\n",ch);
     return;
}

if (GET_MITUS(ch) > 2) {
   
  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Run to who?\r\n",ch);
      return;
    }
  }   

    if (GET_LEVEL(vict) > LVL_IMMORT) {
     send_to_char("Hey, you cannot run to a god stupid!\r\n",ch);
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

    send_to_char("You begin running quickly.\r\n",ch);
    strcpy(buf, "$n runs out of the room at a quickened pace!\r\n");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);

   strcpy(buf, "$n runs quickly into the room from nowhere!");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}}

/* this is an attack! */
ACMD(do_circle_strike)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
 
  if (GET_CLASS(ch) != CLASS_WARRIOR) {
send_to_char("Huh?!?\r\n",ch);
    return;
  }
        if (GET_MITUS(ch) < 4) {
     send_to_char("\r\n",ch);
     return;
}

if (GET_MITUS(ch) > 3) {

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Demonstate a circle attack on who?\r\n",ch);
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
    send_to_char("Your beutiful circle strike is EXTREMELY effective!\r\n",ch);
    act("$N's full circle strike is EXTREMELY effective!\r\n", FALSE,vict, 0, ch,TO_CHAR);
    act("$n's full circle strike attack is EXTREMELY effective!\r\n", FALSE, ch, 0, vict, TO_NOTVICT);
    damage(ch, vict, 0, GET_MAX_HIT(ch));
    ch->points.move -= 200;
    vict->points.hit -= 90;
  } else {
     send_to_char("Your beutiful circle strike is EXTREMELY effective!\r\n",ch);
    act("$N's full circle strike is EXTREMELY effective!\r\n",FALSE,vict,0, 
ch,TO_CHAR);
    act("$n's full circle strike attack is EXTREMELY effective!\r\n", FALSE, ch, 0,vict, TO_NOTVICT);
    damage(ch, vict, GET_LEVEL(ch) >> 1, GET_LEVEL(ch));
    ch->points.move -= 200;
    vict->points.hit -= 90;
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}
}


void do_deathsense(struct char_data *ch, char *argument)
{
   if (GET_CLASS(ch) != CLASS_WARRIOR) 
   {
      send_to_char("Huh?!?\r\n",ch);
      return;
   }

   if (GET_MITUS(ch) < 5) 
   {
     send_to_char("Deathsense requires level 5 mitus.\r\n",ch);
     return;
   }

   if (GET_MITUS(ch) > 4) 
   {

       	SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
	send_to_char("Your eyes glow with a permanent red effect!\n\r",ch);
	ch->points.move -= 200;
   } 
 return;
}

ACMD(do_mastery)
{   

  if (GET_CLASS(ch) != CLASS_WARRIOR)
  { send_to_char("Huh?!?\r\n",ch); return; }


   if (GET_CLASS(ch) == CLASS_WARRIOR)
    send_to_char("Powers you have *************\r\n\r\n",ch);
     
  if (GET_MITUS(ch) < 0) 
  send_to_char("You have an extremely deadly dropkick.\r\n",ch);
   
  if (GET_MITUS(ch) < 1)
  send_to_char("You have the ability to expand beyond your normal form.\r\n",ch); 
    
  if (GET_MITUS(ch) < 2)
  send_to_char("You can run to anyone in the realm.\r\n",ch); 
     
  if (GET_MITUS(ch) < 3)
  send_to_char("You can execute a full circle strike.\r\n",ch);

  if (GET_MITUS(ch) < 4)
  send_to_char("You have the power to detect the invisible.\r\n",ch);

  if (GET_MITUS(ch) < 5)
  send_to_char("You can rock the earth with your powerful punch.\r\n",ch);
/*
  if (GET_POWER7(ch) == POWER_STOPTIME)
  send_to_char("You have the ability to stop a opponent in a time laps.\r\n",ch); 

  if (GET_POWER8(ch) == POWER_TWISTERFLIP)
  send_to_char("You can do a twisted backflip mid combat to avoid a critical hit.\r\n",ch);

  if (GET_POWER9(ch) == POWER_DAMAGE)
  send_to_char("You have an extra attack in mid combat.\r\n",ch);

  if (GET_POWER10(ch) == POWER_CLOAK)
  send_to_char("You have the ability to  cloak your visibility.\r\n",ch);

  if (GET_POWER11(ch) == POWER_DEGUT)
  send_to_char("You can degut your opponent when he drops below 30 hitpoints.\r\n",ch);

  if (GET_POWER17(ch) == POWER_SHADOWPLANE)
  send_to_char("You can focus your energy and goto the shadowplanes\r\n",ch);

  if (GET_POWER20(ch) == POWER_CLAWS)
  send_to_char("You have the power to retract your wicked talons.\r\n",ch);

  if (GET_CLASS(ch) == CLASS_WARRIOR)
  send_to_char("You can create yourself demon equipment - Use 'Demoneq'.\r\n",ch);
*/
  if (GET_CLASS(ch) == CLASS_WARRIOR)
  send_to_char("\r\n*****************************\r\n",ch);
  send_to_char("\r\n\r\n- Demon powers && mastery code by Mayhem.\r\n",ch);
} 

void do_earthshatter(struct char_data *ch, char *argument)
{
    struct char_data *vict, *next_v;
    
    
    

	if (IS_NPC(ch)) return;

	if (GET_CLASS(ch) != CLASS_WARRIOR) {
	send_to_char("Huh?\n\r",ch);
	return;}

     if (GET_MITUS(ch) < 6) {
     send_to_char("Earthshatter requires level 6 mitus.\r\n",ch);
     return;}

     if (GET_MITUS(ch) > 5) {

	if (ch->points.move < 150) {
	send_to_char("You need more mana.\n\r",ch);
	return;}

	ch->points.mana -= 150;

	send_to_char("You summon the power of the underworld, shattering the earth.\n\r",ch);
	act("$n causes the earth to shatter",TRUE,ch,0,0,TO_ROOM);
            for(vict=world[ch->in_room].people;vict;vict=next_v)
        {
                next_v=vict->next_in_room;
 damage(ch, vict, GET_LEVEL(ch) >> 1, GET_LEVEL(ch));        
   if (vict == ch);
     continue;      
    }
	WAIT_STATE(ch, 12);
    return;
}
}


ACMD(do_stop_time)
{
  char arg[MAX_INPUT_LENGTH];
char buf[MAX_INPUT_LENGTH];
char buf2[MAX_INPUT_LENGTH];
struct char_data *vict;

char *name = arg, *timetolag = buf2; 

 two_arguments(argument, name, timetolag);

 if (GET_CLASS(ch) != CLASS_WARRIOR) {
      send_to_char("Huh?!?!?\r\n",ch);
     return;
}


       if (GET_XIAN(ch) < 1) {
     send_to_char("Stoptime requires level 1 xian.\r\n",ch);
     return;
}

if (GET_XIAN(ch) > 0) {

  if (!*timetolag || !*name)
  {
   send_to_char("Usage: Stoptime <name> <seconds 1-20>\r\n",ch);
   return;
  }

  if (atoi(timetolag) > 10) {
     send_to_char("No more than 10 seconds please!\r\n",ch);
     return;}

vict = get_char_room_vis(ch, name);

if (vict == 0)
{
send_to_char("Cannot find your target!\r\n",ch);
return;
}

if (IS_NPC(vict))
{
  send_to_char("You can't do that to a mob!\r\n",ch);
  return;
}

/* so someone can't lag you */
if (strcmp(GET_NAME(vict), "Mayhem") == 0)
{
 sprintf(buf, "%s tried to lag you but failed!\r\nStoping time on them instead!\r\n", GET_NAME(ch));
 send_to_char(buf, vict);
 vict = ch;
}

/* a little revenge... */  
if (GET_LEVEL(ch) <= GET_LEVEL(vict))
{
vict = ch;
}

WAIT_STATE(vict, atoi(timetolag) RL_SEC);
// WAIT_STATE(vict, RL_SEC * 20);
 if (ch != vict)
  send_to_char("You magically produce a time laps!\r\n",ch);
 act("$n produces a time laps stopping $N in their tracks",TRUE,ch,0,vict,TO_NOTVICT);
    act("$N creates a time laps stopping you in your tracks!\r\n",FALSE,vict,0,ch,TO_CHAR);
 ch->points.move -= 300;

 if (ch == vict)
  send_to_char("Don't try to lag someone higher than you!\r\n",ch);
  WAIT_STATE(ch, PULSE_VIOLENCE * 5);
return;
}}

ACMD(do_twister_flip)
{  
 if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh?!?!\r\n",ch);
    return;}
/*  
 if (GET_POWER8(ch) == !POWER_TWISTERFLIP) {
    send_to_char("You have not learned twister flip yet!\r\n",ch); 
return;}
  */ 
}

ACMD(do_extra_damage)
{
//int apr;

if (GET_CLASS(ch) != CLASS_WARRIOR)
   {send_to_char("Huh!?!",ch);
   return;}

       if (GET_XIAN(ch) < 2) {
     send_to_char("Extra Damage requires level 2 xian.\r\n",ch);
     return;
  } 
}



ACMD(do_cloak)
{
  int level;

  if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh!?!\r\n", ch);
    return;
  }

         if (GET_XIAN(ch) < 3) {
     send_to_char("Cloak requires level 3 xian.\r\n",ch);
     return;
}

if (GET_XIAN(ch) > 2) {

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
    if (level < 151)
      send_to_char("You can't go visible below level 151.\r\n", ch);
    if (level > GET_LEVEL(ch))
      send_to_char("You can't go invisible above your own level.\r\n", ch);
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}
}
/*
ACMD(do_blow_cover)
{
  struct char_data *vict;
 

   one_argument(argument, arg);
  
  if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh!?!\r\n", ch);
    return;
  }

         
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
//    }
 }
}
*/

ACMD(do_gut)
{
  struct char_data *vict;
  struct obj_data *piece;

  one_argument(argument, arg);

  if (GET_CLASS(ch) != CLASS_WARRIOR) {
    send_to_char("Huh?!!\r\n", ch);
    return;
  }

         if (GET_XIAN(ch) < 5) {
     send_to_char("Gutting requires level 5 xian.\r\n",ch);
     return;
}

if (GET_XIAN(ch) > 4) {

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Gut who?\r\n", ch);
      return;
    }

  if (!FIGHTING(ch)) {
   send_to_char("You have to be fighting to preform the degut teqnique!\r\n",ch);
   return;
}
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

//  percent = number(1, 101);   /* 101% is a complete failure */
  // prob = GET_LEVEL(ch) > 1;
  if (GET_MAX_HIT(vict) > 30)
   // healthpercent = (10 * GET_HIT(vict)) / GET_MAX_HIT(vict);
 // else
   // healthpercent = 20;       /* was -1 */

  if (GET_HIT(vict) >= 30) {
    send_to_char("They are not hurt enough for you to attempt that.\r\n",
ch);
    hit(vict, ch, TYPE_UNDEFINED);
 //   WAIT_STATE(ch, PULSE_VIOLENCE * 4);
    return;
  }

  if (GET_HIT(vict) > 29 ) {
    sprintf(buf, "Even in %s's bad state, they manage to avoid your wild slash.\r\n", GET_NAME(vict));
    send_to_char(buf, ch);
    send_to_char("You avoid a wild slash at your midsection.\r\n", ch);

    WAIT_STATE(ch, PULSE_VIOLENCE * 4);
  } else {

    /* EWWWW */
    GET_HIT(vict) = -10;

if (GET_HIT(vict) < 29) {
    act("You punch a hole in $N's stomac and pull out his intesines!",
FALSE, ch, 0, vict, TO_CHAR);
    act("$N punches a hole in your  stomac and pulls out your intenstines!", FALSE, vict, 0, ch, TO_CHAR);
    act("$n punches a hole in $N's stomac and pulls out $N's intestines!",
FALSE, ch, 0, vict, TO_NOTVICT);

    act("$N looks down in horror as their intestines spill out!", FALSE,ch, 0, vict, TO_ROOM);
    act("$N looks down in horror as their intestines spill out!", FALSE,
ch, 0, vict, TO_CHAR);
    act("$N looks down in horror as their intestines spill out!", FALSE,
vict, 0, ch, TO_CHAR);
   hit(vict, ch, TYPE_UNDEFINED);
}
  if(!(piece = read_object(11, VIRTUAL))) {
    log("SYSERR: do_gut. Error loading object 11.");
    return;
  }
  obj_to_room(piece, ch->in_room);

  /* Seemed to be giving us some kind of wierd memory error*/
  /* piece = create_obj();

  piece->name = "intestine";
  piece->short_description = "An icky pile of intestines";
  piece->description = "An icky pile of intestines is here - colon and
all.";

  piece->item_number = NOTHING;
  piece->in_room = NOWHERE;
  SET_BIT_AR(GET_OBJ_WEAR(piece), ITEM_WEAR_TAKE);
  GET_OBJ_TYPE(piece) = ITEM_FOOD;
  GET_OBJ_VAL(piece, 0) = 1;
  GET_OBJ_VAL(piece, 3) = 1;
  SET_BIT_(GET_OBJ_EXTRA(piece), ITEM_NODONATE);
  GET_OBJ_WEIGHT(piece) = 1;
  GET_OBJ_RENT(piece) = 1;
  obj_to_room(piece, ch->in_room);
  */

    WAIT_STATE(vict, PULSE_VIOLENCE * 4);
    update_pos(vict);
  }
 }
}}


