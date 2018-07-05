/*************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#else
#include "telnet.h"
#endif

/* extern variables */
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct dex_skill_type dex_app_skill[];
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern char *class_abbrevs[];
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int top_of_world;
extern int auto_save;
extern int train_params[6][NUM_CLASSES];
extern sh_int r_mortal_start_room;

/* extern procedures */
void list_skills(struct char_data * ch);
void appear(struct char_data * ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data * ch, struct char_data * killer);
void Crash_crashsave(struct char_data * ch);
void Crash_rentsave(struct char_data * ch, int cost);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);
int level_exp(int chclass, int level);

/* local functions */
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_sneak);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_wimpy);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_train);
ACMD(do_bribe);
ACMD(do_recall);
ACMD(do_gain);
 

ACMD(do_train_legend)
{
//   int cost = 0;

//  GET_GOLD(ch) == cost;

 if (GET_GOLD(ch) < 2000000) {
  send_to_char("You do not have enough gold! needed: 200000",ch);
   return;
} else
if (GET_LEGEND_LEVELS(ch) < MAX_LEGEND_LEVELS) {
if (GET_GOLD(ch) > 1999999) {
   send_to_char("You rise in legend status!\r\n",ch);
   GET_LEGEND_LEVELS(ch)+=1;
   GET_GOLD(ch) -= 2000000;
    return;
 }
} else if (GET_LEGEND_LEVELS(ch) > MAX_LEGEND_LEVELS) {
  send_to_char("You have already trained to the fullest legendary status\r\n",ch); 
  return;
}
}

ACMD(do_gain)
{
  int num_levels = 0;
  int is_altered = FALSE;

    if (GET_LEVEL(ch) > LVL_IMMORT) {
        send_to_char("You are currently maxed on levels\r\n",ch);
        return;
      } 

   if (GET_LEVEL(ch) == 10 && GET_PLAYER_KILLS(ch) < 1) {
        send_to_char("You need at least 1 player kill to advance to the next 10 levels.\r\n",ch);
    return;
} 


    if (GET_LEVEL(ch) == 10 && GET_PLAYER_KILLS(ch) < 1) {
        send_to_char("You need at least 1 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}

    if (GET_LEVEL(ch) == 20 && GET_PLAYER_KILLS(ch) < 3) {
        send_to_char("You need at least 3 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}

    if (GET_LEVEL(ch) == 30 && GET_PLAYER_KILLS(ch) < 8) {
        send_to_char("You need at least 8 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 40 && GET_PLAYER_KILLS(ch) < 12) {
        send_to_char("You need at least 12 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 50 && GET_PLAYER_KILLS(ch) < 20) {
        send_to_char("You need at least 20 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 60 && GET_PLAYER_KILLS(ch) < 25) {
        send_to_char("You need at least 25 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 70 && GET_PLAYER_KILLS(ch) < 31) {
        send_to_char("You need at least 31 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 80 && GET_PLAYER_KILLS(ch) < 41) {
        send_to_char("You need at least 1 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 85 && GET_PLAYER_KILLS(ch) < 46) {
        send_to_char("You need at least 46 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 90 && GET_PLAYER_KILLS(ch) < 52) {
        send_to_char("You need at least 52 player kill to advance to the next 10 levels.\r\n",ch);
    return;
}
    if (GET_LEVEL(ch) == 98 && GET_PLAYER_KILLS(ch) < 60) {
        send_to_char("You need at least 60 player kill to advance to the next 2 levels.\r\n",ch);
    return;
}
     if (GET_LEVEL(ch) <= MAX_MORT_LEVEL &&
        GET_EXP(ch) >= level_exp(GET_CLASS(ch), GET_LEVEL(ch) + 1)) {
      if (GET_LEVEL(ch) >= MAX_MORT_LEVEL && GET_LEVEL(ch) < LVL_IMMORT)
        { if (!IS_NPC(ch) && (QPOINTS(ch) >= MAX_QPOINTS))
          GET_LEVEL(ch) = LVL_IMMORT;
          else return; }
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
    }
}


ACMD(do_haveleft)
{

if (!PLR_FLAGGED(ch, PLR_DEAD)) {
  send_to_char("Huh?!?\r\n",ch);
  return;
}

if (PLR_FLAGGED(ch, PLR_DEAD)) {
  sprintf(buf, "You need %d gold coins to complete your quest.\r\n"
               "You have %d already.\r\n",
GET_LEVEL(ch) * 100, GET_GOLD(ch));
}
send_to_char(buf, ch);
}


/* Special process for Death God (must be bribed to be mortal again) */
  ACMD(do_bribe)  
  {
    

    if (!PLR_FLAGGED(ch, PLR_DEAD)) {
    send_to_char("Huh?!?\r\n",ch);
      return;}

                                      
        if (GET_GOLD(ch) >= (GET_LEVEL(ch) * 100)) {
          sprintf(buf, "&r[ <-INFO-> ] %s has quested for mortality and returns from hell!\r\n",GET_NAME(ch));
          send_to_all(buf);
          send_to_char("You see the white light fade into the distance...", ch);
          REMOVE_BIT(PLR_FLAGS(ch), PLR_DEAD);
          char_from_room(ch);
          char_to_room(ch, r_mortal_start_room);
          GET_HIT(ch) = GET_MAX_HIT(ch);
          GET_MANA(ch) = GET_MAX_MANA(ch);
          GET_MOVE(ch) = GET_MAX_MOVE(ch);
          GET_GOLD(ch) = GET_TEMP_GOLD(ch);
          update_pos(ch);
          look_at_room(ch, 0);
          return;
        } else {
           send_to_char("You don't Have the gold for that", ch);
           return;
         }
      return;
    }


ACMD(do_recall)
{

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't recall!!\r\n", ch);
    return;
  }
      if (PLR_FLAGGED(ch, PLR_DEAD)) {
    send_to_char("You can't possibly ESCAPE from hell!\r\n",ch);
      return;}
   if (GET_LEVEL(ch) <= 111) {
  send_to_char("You focus your energy and dissapear!\r\n",ch);
       act("$n concentrates and disappears.", TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, r_mortal_start_room);
    act("$n appears suddenly.", TRUE, ch, 0, 0, TO_ROOM);
    look_at_room(ch, 0);
    return;
} 
 }

ACMD(do_train)
{
  skip_spaces(&argument);

  if (!*argument)
  {   
    sprintf(buf,
"\r\n                       &W*--------------------------------------*\r\n"
"                       |            Train your stats          |\r\n"
"                       *--------------------------------------*\r\n"
"                           Hp:                    $%d              \r\n"
"                           Mana:                  $%d              \r\n"
"                           Move:                  $%d              \r\n"
"                           Legend Status:         $2000000         \r\n"
" (Mage Only)               Runes:                 $%d              \r\n"
"                           Hitroll:               $%d              \r\n"
"                           Damroll:               $%d              \r\n"
" (Saiyan Only              Powerlevel:            $%d              \r\n"
"                           Avatar:      must have 2000/hp/mana/move\r\n"
"                       *--------------------------------------*\r\n"
"                            You have %d research points\r\n"
"                       *--------------------------------------*\r\n",        
    GET_MAX_HIT(ch) * 20, GET_MAX_MANA(ch) * 20, GET_MAX_MOVE(ch) * 20, 
    GET_MAX_HIT(ch) * 400, GET_MAX_HIT(ch) * 400, GET_MAX_HIT(ch) * 400, 
GET_MAX_HIT(ch) * 20, GET_DEMONXP(ch));
    send_to_char(buf, ch);
    return;
  }

  if (GET_DEMONXP(ch) <= 0) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }

  if (!str_cmp(argument,"powerlevel"))
    {
  if (GET_DEMONXP(ch) <= GET_MAX_HIT(ch) * 20) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }
      GET_DEMONXP(ch) -= GET_MAX_HIT(ch) * 20;
      GET_POWERL(ch) +=10;
    } else


  if (!str_cmp(argument,"hit"))
    {
  if (GET_DEMONXP(ch) <= GET_MAX_HIT(ch) * 20) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }
      GET_DEMONXP(ch) -= GET_MAX_HIT(ch) * 20;
      GET_MAX_HIT(ch) +=5;
    } else
  if (!str_cmp(argument,"mana"))
    {
     if (GET_DEMONXP(ch) <= GET_MAX_MANA(ch) * 20) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }
 
      GET_DEMONXP(ch) -= GET_MAX_MANA(ch) * 20;
      GET_MAX_MANA(ch) +=5;
    } else
  if (!str_cmp(argument,"move"))
    {
  if (GET_DEMONXP(ch) <= GET_MAX_MOVE(ch) * 20) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }
      GET_DEMONXP(ch) -= GET_MAX_MOVE(ch) * 20;
      GET_MAX_MOVE(ch) +=5;
    } else
  if (!str_cmp(argument,"runes"))
   {
     if (GET_DEMONXP(ch) <= GET_MAX_HIT(ch) * 400) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }
    GET_DEMONXP(ch) -=GET_MAX_HIT(ch) * 400;
    GET_PRACTICES(ch)+=1;
  } else
   if (!str_cmp(argument,"hitroll"))
   {
     if (GET_DEMONXP(ch) <= GET_MAX_HIT(ch) * 400) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }
    GET_DEMONXP(ch) -=GET_MAX_HIT(ch) * 400;
    GET_HITROLL(ch) +=1;
   } else
   if (!str_cmp(argument,"avatar"))
   {
     if (GET_MAX_HIT(ch) && GET_MAX_MANA(ch) && GET_MAX_MOVE(ch) < 2000 ) {
    send_to_char("You must have 2000 hp/mana/move to train avatar.\r\n", ch);
    return; }
     if (GET_PLAYER_KILLS(ch) > 1) {
       send_to_char("You are already an avatar.\r\n",ch);
       return;
      
  }
   GET_PLAYER_KILLS(ch) +=1;
   sprintf(buf, "%s[ <-INFO-> ] %s has become an avatar!\r\n",buf, GET_NAME(ch)); 
   send_to_char("You are now an avatar!\r\n",ch);
   send_to_all(buf);
  } else
   if (!str_cmp(argument,"damroll"))
   {  
     if (GET_DEMONXP(ch) <= GET_MAX_HIT(ch) * 400) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return;
  }
    GET_DEMONXP(ch) -=GET_MAX_HIT(ch) * 400;
    GET_DAMROLL(ch) +=1;
   } else
  if (!str_cmp(argument,"str"))
    {
      if (GET_STR(ch) >= train_params[0][(int)GET_CLASS(ch)])
       { send_to_char("Your are already fully trained there!\r\n",ch); return; }
      GET_DEMONXP(ch) -=1;
      ch->aff_abils.str+=1;
    } else
  if (strcmp(argument,"con")==0)
    {
      if (GET_CON(ch) >= 50)//train_params[1][(int)GET_CLASS(ch)])
       { send_to_char("Your are already fully trained there!\r\n",ch); return; }
      GET_DEMONXP(ch) -=1;
  		    ch->aff_abils.con++;
		    GET_CON(ch)++;     
  //GET_CON(ch) +=1;
    } else
  if (strcmp(argument,"wis")==0)
    {
      if (GET_WIS(ch) >= 50)//train_params[2][(int)GET_CLASS(ch)])
       { send_to_char("Your are already fully trained there!\r\n",ch); return; }
      GET_GOLD(ch) -=1;
      GET_WIS(ch)+=1;
    } else
  if (strcmp(argument,"int")==0)
    {
      if (GET_INT(ch) >= train_params[3][(int)GET_CLASS(ch)])
       { send_to_char("Your are already fully trained there!\r\n",ch); return; }
      GET_GOLD(ch) -=1;
      GET_INT(ch)+=1;
    } else
  if (strcmp(argument,"dex")==0)
    {
      if (GET_DEX(ch) >= train_params[4][(int)GET_CLASS(ch)])
       { send_to_char("Your are already fully trained there!\r\n",ch); return; }
      GET_GOLD(ch) -=1;
      GET_DEX(ch)+=1;
    } else
  if (strcmp(argument,"cha")==0)
    {
      if (GET_CHA(ch) >= train_params[5][(int)GET_CLASS(ch)])
       { send_to_char("Your are already fully trained there!\r\n",ch); return; }
      GET_GOLD(ch) -=1;
      GET_CHA(ch)+=1;
    } else
 if (!str_cmp(argument,"legend"))
  {
   if (GET_DEMONXP(ch) < 2000000) {
  send_to_char("You do not have enough research points! needed: 2000000",ch);
   return;
} else
if (GET_LEGEND_LEVELS(ch) < MAX_LEGEND_LEVELS) {
if (GET_DEMONXP(ch) > 1999999) {
   send_to_char("You rise in legend status!\r\n",ch);
   GET_LEGEND_LEVELS(ch)+=1;
   GET_DEMONXP(ch) -= 2000000;
    return;
 }
} else if (GET_LEGEND_LEVELS(ch) == 5) {
  send_to_char("You are already fully trained in that area!\r\n",ch);
  return; }
} else {
      send_to_char("Train what?\r\n",ch);
      return;
    }
  send_to_char("You train for a while...\r\n",ch);
  return;
}



ACMD(do_quit)
{
  sh_int save_room;
  struct descriptor_data *d, *next_d;
  struct obj_data *obj;
  int i;
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char("You have to type quit--no less, to quit!\r\n", ch);
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch, NULL);
  } else {
    if (!GET_INVIS_LEV(ch))
    sprintf(buf, "[ <-INFO-> ] %s has left the Darkened Lights!\r\n",GET_NAME(ch));
    send_to_all(buf); 
   act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    sprintf(buf, "%s has quit the game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    send_to_char("Goodbye, friend.. Come back soon!\r\n", ch);

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
        continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
        STATE(d) = CON_DISCONNECT;
    }
   
   save_room = ch->in_room;
   if (free_rent || GET_LEVEL(ch)>=LVL_IMMORT)
      Crash_rentsave(ch, 0);
   else {
    /* transfer objects to room, if any */
    while (ch->carrying) {
      obj = ch->carrying;
      obj_from_char(obj);
      obj_to_room(obj, ch->in_room);
    }

    /* transfer equipment to room, if any */
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
        obj_to_room(unequip_char(ch, i), ch->in_room);
    Crash_crashsave(ch);
   }
   GET_LOADROOM(ch) = NOWHERE;
   extract_char(ch);		/* Char is saved in extract char */

    /* If someone is quitting in their house, let them load back here */
    if (ROOM_FLAGGED(save_room, ROOM_HOUSE))
      save_char(ch, world[save_room].number);
  }
}



ACMD(do_save)
{

  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled.
     */
 //   if (auto_save) {
 //     send_to_char("Manual saving is disabled.\r\n", ch);
 //     return;
 // }
    sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }

  save_char(ch, NOWHERE);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}



ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
  if (AFF_FLAGGED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
    return;

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector[0] = AFF_SNEAK;
  af.bitvector[1] = 0;
  af.bitvector[2] = 0;
  af.bitvector[3] = 0;
  affect_to_char(ch, &af);
}



ACMD(do_hide)
{
  byte percent;

  send_to_char("You attempt to hide yourself.\r\n", ch);

  if (AFF_FLAGGED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
    return;

  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}




ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return;
  }

  argument = one_argument(argument, obj_name);
  one_argument(argument, vict_name);

  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }

  /* 101% is a complete failure */
  percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS */

  if (!pt_allowed && !IS_NPC(vict))
    pcsteal = 1;

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
	  return;
	} else {
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
	ohoh = TRUE;
	act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
	  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char("Got it!\r\n", ch);
	  }
	} else
	  send_to_char("You cannot carry that much.\r\n", ch);
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1) {
	  sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
	  send_to_char(buf, ch);
	} else {
	  send_to_char("You manage to swipe a solitary gold coin.\r\n", ch);
	}
      } else {
	send_to_char("You couldn't get any gold...\r\n", ch);
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_practice)
{
  one_argument(argument, arg);


 if (*arg)
 list_skills(ch);
 else
   list_skills(ch);
}



ACMD(do_visible)
{
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
  } else
    send_to_char("You are already visible.\r\n", ch);
}



ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char("Your title is fine... go away.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strlen(argument) > MAX_TITLE_LENGTH) {
    sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
	    MAX_TITLE_LENGTH);
    send_to_char(buf, ch);
  } else {
    if (*argument == '\0')
      set_title(ch, NULL);
    else
      set_title(ch, argument);
    sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
  }
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return 0;

  SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return 1;
}


void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char("But you are not the member of a group!\r\n", ch);
  else {
    send_to_char("Your group consists of:\r\n", ch);

    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N (Head of group)",
	      GET_HIT(k), GET_MANA(k), GET_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	continue;

      sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N", GET_HIT(f->follower),
	      GET_MANA(f->follower), GET_MOVE(f->follower),
	      GET_LEVEL(f->follower), CLASS_ABBR(f->follower));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}



ACMD(do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char("Everyone following you is already in your group.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
	REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
	send_to_char(buf2, f->follower);
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char("You disband the group.\r\n", ch);
    return;
  }
  if (!(tch = get_char_room_vis(ch, buf))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char("That person isn't in your group.\r\n", ch);
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}



ACMD(do_report)
{
  sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
          GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
          GET_MANA(ch), GET_MAX_MANA(ch),
          GET_MOVE(ch), GET_MAX_MOVE(ch));
   act(buf, TRUE, ch, 0, 0, TO_ROOM);
   send_to_char(buf, ch);
  
}

ACMD(do_resort)
{
  struct char_data *k;
  struct follow_type *f;

 // if (!AFF_FLAGGED(ch, AFF_GROUP)) {
 //   send_to_char("But you are not a member of any group!\r\n", ch);
 //   return;
 // }
  sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char(buf, f->follower);
  if (k != ch)
    send_to_char(buf, k);
  send_to_char("You report:\r\n", ch);
  sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
          GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
          GET_MANA(ch), GET_MAX_MANA(ch),
          GET_MOVE(ch), GET_MAX_MOVE(ch));
}



ACMD(do_split)
{
  int amount, num, share;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char("You don't seem to have that much gold to split.\r\n", ch);
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP))
      share = amount / num;
    else {
      send_to_char("With whom do you wish to share your gold?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
	      amount, share);
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
		amount, share);
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);
    send_to_char(buf, ch);
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}



ACMD(do_use)
{
  struct obj_data *mag_item;
  int equipped = 1;

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      equipped = 0;
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf2, ch);
	return;
      }
      break;
    case SCMD_USE:
      sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      send_to_char(buf2, ch);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("You can only quaff potions.", ch);
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("You can only recite scrolls.", ch);
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char("You can't seem to figure out how to use it.\r\n", ch);
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  int wimp_lev;

  /* 'wimp_level' is a player_special. -gg 2/25/98 */
  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Your current wimp level is %d hit points.\r\n",
	      GET_WIMP_LEV(ch));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg))) {
      if (wimp_lev < 0)
	send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      else if (wimp_lev > GET_MAX_HIT(ch))
	send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
	send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
      else {
	sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
		wimp_lev);
	send_to_char(buf, ch);
	GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);

  return;

}


ACMD(do_display) {
  char arg[MAX_INPUT_LENGTH];
  int i, x;
  
  const char *def_prompts[][2] = {
    { "Stock Circle"		 , "&w%hhp %mmp %vmv>" 			},
 //   { "Colorized Standard Circle", "&R%h&rhp &C%m&cmp &G%v&gmv&X>"      },
    { "Standard"		 , "&R%ph&rhp &C%pm&cmp &G%pv&gmv&X>"   },
//    { "Full Featured"		 ,
 //     "&cOpponent&X: &B%o &W/ &cTank&X: &B%t%_&r%h&X(&R%H&X)&w"
 //     "hitp &c%m&X(&C%M&X)&wmana &g%v&X(&G%V&X)&wmove&W >"              
//},
    { "Mayhem's style"           , "&R%h&X(&r%H&X)&wHp &C%m&X(&c%M&X)&wMp &G%v&X(&g%V&X)&wVp &Y%X &W%po>"},
    { "Immortal Runner"          , "&W%po>"				},
    { "\n"                       , "\n"                                 }
  };
  
  one_argument(argument, arg);
  
  if (!arg || !*arg) {
    send_to_char("The following pre-set prompts are availible...\r\n", ch);
    for (i = 0; *def_prompts[i][0] != '\n'; i++) {
      sprintf(buf, "  &W%d. &w%-25s  %s\r\n", i, def_prompts[i][0], def_prompts[i][1]);
      send_to_char(buf, ch);
    }
    send_to_char("&wUsage: display <number>\r\n"
                "To create your own prompt, use PROMPT <str>.\r\n", ch);
    return;
  } else if (!isdigit(*arg)) {
    send_to_char("Usage: display <number>\r\n", ch);
    send_to_char("Type DISPLAY without arguments for a list of preset prompts.\r\n", ch);
    return;
  }
  
  i = atoi(arg);
  
  if (i < 0) {
    send_to_char("The number cannot be negative.\r\n", ch);
    return;
  }
  
  for (x = 0; *def_prompts[x][0] != '\n'; x++);
  
  if (i >= (x)) {
    sprintf(buf, "The range for the prompt number is 0-%d.\r\n", x-1);
    send_to_char(buf, ch);
    return;
  }
  
  if (GET_PROMPT(ch))
    free(GET_PROMPT(ch));
  GET_PROMPT(ch) = str_dup(def_prompts[i][1]);
  
  sprintf(buf, "Set your prompt to the %s preset prompt.\r\n", def_prompts[i][0]);
  send_to_char(buf, ch);
}


ACMD(do_prompt) {
  skip_spaces(&argument);
  
  if (!*argument) {
    sprintf(buf, "Your prompt is currently: %s\r\n", (GET_PROMPT(ch) ? GET_PROMPT(ch) : "n/a"));
    send_to_char(buf, ch);
    return;
  }
  
  delete_doubledollar(argument);
  
  if (GET_PROMPT(ch))
    free(GET_PROMPT(ch));
  GET_PROMPT(ch) = str_dup(argument);
  
  sprintf(buf, "Okay, set your prompt to: %s\r\n", GET_PROMPT(ch));
  send_to_char(buf, ch);
}
  

ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, buf[MAX_STRING_LENGTH];
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
    return;
  }
  sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  mudlog(buf, CMP, LVL_IMMORT, FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("Error statting file");
    return;
  } 
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  fclose(fl);
  send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"You will no longer Auto-Assist.\r\n",
     "You will now Auto-Assist.\r\n"},
    {"You turn off your examination unit.\r\n",
     "You turn on your examination unit.\r\n"},
    {"Autosplitting disabled.\r\n",
     "Autosplitting enabled.\r\n"},
    {"Autolooting disabled.\r\n",
     "Autolooting enabled.\r\n"},
    {"Autogold disabled.\r\n",
     "Autogold enabled.\r\n"},
    {"Clan display disabled.\r\n",
     "Clan display enabled.\r\n"},
    {"Autoreceiving mud-mail disabled.\r\n",
     "Autoreceiving mud-mail enabled.\r\n"}
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_EXAMUNIT:
    result = PRF_TOG_CHK(ch, PRF_EXAMUNIT);
    break;  
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;  
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;  
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_SHOWCLAN:
    result = PRF_TOG_CHK(ch, PRF_SHOWCLAN);
    break;
  case SCMD_AUTOMAIL:
    result = PRF_TOG_CHK(ch, PRF_AUTOMAIL);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}

#define NEW_EQUIP(ch, where, what)	{if (!GET_EQ(ch, where)) { 		\
					   obj = read_object(what, VIRTUAL); 	\
					   equip_char(ch, obj, where); 		\
					 } 					\
					}

#define NEW_INV(ch, what)	{obj = read_object(what, VIRTUAL); 				\
				 if (obj) {							\
				   if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch) && 			\
				     (IS_CARRYING_W(ch)+GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch))) \
				     obj_to_char(obj, ch); 					\
				   else 							\
				     extract_obj(obj); 						\
				 }								\
				}

ACMD(do_newbie)
{
  struct obj_data *obj;
 
  if (GET_LEVEL(ch) > MAX_NEWBIE_LEVEL) {
    send_to_char("You are too experienced to newbie!\r\n", ch);
    return;
  }
  
  if (!PLR_FLAGGED(ch, PLR_NEWBIE)) {
    send_to_char("You have been already newbie equipped in this incarnation.\r\n", ch);
    return;
  }
  
  /* Common equipment */
  NEW_EQUIP(ch, WEAR_SHIELD, 3042);
  NEW_EQUIP(ch, WEAR_ARMS, 3085);
  NEW_EQUIP(ch, WEAR_LEGS, 3080);
  NEW_EQUIP(ch, WEAR_BODY, 3045);
  NEW_EQUIP(ch, WEAR_HEAD, 3075);
  NEW_EQUIP(ch, WEAR_HANDS, 3070);
  
  /* Class specific equipment */
  switch (GET_CLASS(ch)) {
    case CLASS_WARRIOR:
      NEW_EQUIP(ch, WEAR_WIELD, 3022);
      
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_CLERIC:
      NEW_EQUIP(ch, WEAR_WIELD, 3025);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_MAGIC_USER:
      NEW_EQUIP(ch, WEAR_WIELD, 3021);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_THIEF:
      NEW_EQUIP(ch, WEAR_WIELD, 3021);
      NEW_INV(ch, 3020);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_PALADIN:
      NEW_EQUIP(ch, WEAR_WIELD, 3022);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_RANGER:
      NEW_EQUIP(ch, WEAR_WIELD, 3022);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_WARLOCK:
      NEW_EQUIP(ch, WEAR_WIELD, 3021);
      NEW_INV(ch, 3020);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_CYBORG:
      NEW_EQUIP(ch, WEAR_WIELD, 3022);
      break;
    case CLASS_DRUID:
      NEW_EQUIP(ch, WEAR_WIELD, 3025);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_NECROMANCER:
      NEW_EQUIP(ch, WEAR_WIELD, 3025);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_ALCHEMIST:
      NEW_EQUIP(ch, WEAR_WIELD, 3021);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    case CLASS_BARBARIAN:
      NEW_EQUIP(ch, WEAR_WIELD, 3024);
      NEW_INV(ch, 3020);
      NEW_INV(ch, 3009);
      NEW_INV(ch, 3103);
      break;
    default:
      log("SYSWARR: No newbie equipment specific for this class.");
      break;
  }
  /* Hometown specific equipment */
  switch (GET_HOMETOWN(ch)) {
    case 2: 
      NEW_INV(ch, 6742);
      break;
    case 3: 
      NEW_INV(ch, 6731);
      break;
    case 5: 
      NEW_INV(ch, 6707);
      break;
  }
  send_to_char("You have been equipped by the Gods.\r\n", ch);
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  send_to_char("Your hitpoints, mana and vitality have been restored to maximum\r\n", ch);
  REMOVE_BIT(PLR_FLAGS(ch), PLR_NEWBIE);
}

#undef NEW_EQUIP
#undef NEW_INV

ACMD(do_afk)
{
  int result;

  if (IS_NPC(ch)) {
    send_to_char("Mobs don't have keyboards, go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);

  if (PRF_FLAGGED(ch, PRF_AFK)) {
    result = FALSE;
    if (PRF_FLAGGED(ch, PRF_LOCKED)) {
      echo_on(ch->desc);
      GET_BAD_PWS(ch) = 0;
    }
    REMOVE_BIT(PRF_FLAGS(ch), PRF_AFK | PRF_LOCKED);
  } else {
    result = TRUE;
    SET_BIT(PRF_FLAGS(ch), PRF_AFK);
    if (subcmd == SCMD_LOCK_AFK) {
      SET_BIT(PRF_FLAGS(ch), PRF_LOCKED);
      GET_BAD_PWS(ch) = 0;
      echo_off(ch->desc);
    }
  }
  set_afk(ch, argument, result);
}

ACMD(do_test)
{
  int i = atoi(argument);
  log("Sending string to zone r-num: %d", i);
  send_to_zone("***POKUS***", i);
}
