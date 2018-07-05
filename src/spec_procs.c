/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
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
#include "clan.h"


/*   external vars  */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern HOMETOWN hometowns[];
extern struct spell_info_type spell_info[];
// extern struct char_data *character_list;
extern int top_of_world;
extern int guild_info[][3];
extern sh_int r_mortal_start_room;


/* extern functions */
void add_follower(struct char_data * ch, struct char_data * leader);
void store_mail(long to, long from, struct obj_data *atch, char *message_pointer);
void list_stations_wagon(struct char_data *ch, int room_vnum);
int get_transroom(int room_rnum, int dir);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
ACMD(do_kick);
ACMD(do_bash);
ACMD(do_disarm);

/* local functions */
void sort_spells(void);
const char *how_good(int percent);
void list_skills(struct char_data * ch);
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
void npc_steal(struct char_data * ch, struct char_data * victim);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(transporter);

/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */





SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return 0;

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    act("You are awarded for outstanding performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return 1;
}

SPECIAL(hometown_room)
{
  int i, count = 0;
  int cost;

  if (!CMD_IS("home"))
    return 0;
  skip_spaces(&argument);
  cost = GET_LEVEL(ch)*1000;
  if (!*argument) {
    sprintf(buf,"&wThere are following hometowns available to you in &CDarkened&cLights&w:\r\n");  
    for (i = 0; i < NUM_HOMETOWNS; i++) {
         if (!(((hometowns[i].flags & HT_IMM_ONLY) && GET_LEVEL(ch)<LVL_IMMORT) 
            || (hometowns[i].flags & HT_CLAN_ONLY) || (hometowns[i].flags & HT_NOHOMETOWN))) {
            sprintf(buf,"%s  %s&w\r\n", buf, hometowns[i].homename);
            count++;
         }
       }
    if (PLAYERCLAN(ch) == 0)
      sprintf(buf, "%s\r\nUse &WHOME <town name>&w to change your hometown.\r\n"
      	           "It will cost you &M%d&w gold coins	\r\n", buf, cost);
    send_to_char(buf, ch);
    return 1;
  } else {
    if (PLAYERCLAN(ch) != 0) {
      send_to_char("Members of clan may not change their hometowns.\r\n"
                   "You must be more patriotic!\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch)<cost) {
      send_to_char("You have not enough money to change your hometown.\r\nCome whenyou earn some more!\r\n", ch);
      return 1;
    }
    for (i = 0; i < NUM_HOMETOWNS; i++) {
         if (!(((hometowns[i].flags & HT_IMM_ONLY) && GET_LEVEL(ch)<LVL_IMMORT) 
            || (hometowns[i].flags & HT_CLAN_ONLY) || (hometowns[i].flags & HT_NOHOMETOWN))) {
                if (!strcasecmp(argument, hometowns[i].homename)) {
                if (i == GET_HOMETOWN(ch)) {
                  sprintf(buf,"%s is already your hometown.\r\n",
                    hometowns[i].homename );
                  send_to_char(buf, ch);
                  return 1;
                }
                GET_HOMETOWN(ch) = i;
                send_to_char("&WWorld around you slowly fades and you fall asleep.&w\r\n", ch);
                act("Electric willows sourrounded $N as his body slowly fades out.\r\n", FALSE,
                  ch, 0, ch, TO_NOTVICT);
                char_from_room(ch);
                char_to_room(ch, real_room(hometowns[i].magic_room));
                GET_GOLD(ch) -= cost;
                look_at_room(ch, TRUE);
                send_to_char("&WAs you awakened, you realized you were magically relocated to your new hometown.&w\r\n", ch);
                act("The body of $N materializes suddenly.\r\n", FALSE,
                  ch, 0, ch, TO_NOTVICT);
      	  	return 1;             
              }
            count++;
         }
       }
    sprintf(buf, "Where do you wish to relocate?\r\n");
    send_to_char(buf, ch);
    return 1; 
  }
}


SPECIAL(mayor)
{
  const char open_path[] =
  "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
  "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path;
  static int index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return FALSE;

  switch (path[index]) {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, "gate", 0, SCMD_UNLOCK);
    do_gen_door(ch, "gate", 0, SCMD_OPEN);
    break;

  case 'C':
    do_gen_door(ch, "gate", 0, SCMD_CLOSE);
    do_gen_door(ch, "gate", 0, SCMD_LOCK);
    break;

  case '.':
    move = FALSE;
    break;

  }

  index++;
  return FALSE;
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data * ch, struct char_data * victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;

  if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
    if (gold > 0) {
      GET_GOLD(ch) += gold;
      GET_GOLD(victim) -= gold;
    }
  }
}

SPECIAL(meta_physician)
{
   /* Special Procedure  - meta_physician                */
   /* Player Characters can train stats or gain          */
   /* health/mana via a metaphysician                    */
   /* Muerte of ButterMud - Telnet://betterbox.net:4000  */

 char meta_type[256]= {"     "}; /* Array for meta type  name */
 long meta_gold = 0;
 long meta_exp  = 0;

 /* Parse command parm and set meta cost accordingly */

  if (CMD_IS("meta")) {
     argument = one_argument(argument, meta_type);
 
      if (strcmp(meta_type,"health")==0) {
        meta_gold = 250000;
        meta_exp  = 1200000;}

      if (strcmp(meta_type, "mana")==0) {
        meta_gold = 250000;
        meta_exp  = 1200000;}
        
      if (strcmp(meta_type, "move")==0) {
        meta_gold = 250000;
        meta_exp  = 900000;}
        
      if (strcmp(meta_type, "practice")==0) {
        meta_gold = 200000;
        meta_exp  = 120000;}

      if (strcmp(meta_type, "str")==0) {
        meta_gold = 750000;
        meta_exp  = 3000000;}

      if (strcmp(meta_type, "int")==0) {
        meta_gold = 750000;
        meta_exp  = 3000000;}

      if (strcmp(meta_type, "wis")==0) {
        meta_gold = 750000;
        meta_exp  = 3000000;}

      if (strcmp(meta_type, "dex")==0) {
        meta_gold = 750000;
        meta_exp  = 3000000;}

      if (strcmp(meta_type, "con")==0) {
        meta_gold = 750000;
        meta_exp  = 3000000;}

      if (strcmp(meta_type, "cha")==0) {
        meta_gold = 750000;
        meta_exp  = 3000000;}


      /* Gold and Exp validity check */

      if (meta_gold > 0) {
      
      if ((GET_GOLD(ch) < meta_gold)) {
          send_to_char("You don't have enough gold!\r\n", ch);
          return (TRUE);}

      if ((GET_EXP(ch) < meta_exp)) {
          send_to_char("You haven't the experience!\r\n", ch);
          return (TRUE);} 

      /* Extract Cash and experience */
   
       GET_GOLD(ch) -= meta_gold;
       GET_EXP(ch) -= meta_exp;
       send_to_char("The metaphysician accepts your payment and begins the procedure... \r\n",ch);
    
      /* Boost Stats */       
   
      if (strcmp(meta_type,"health")==0) {
      GET_MAX_HIT(ch) += number(15, 25);
      send_to_char("Your vitality increases!\r\n",ch);
      return 1;}

      if (strcmp(meta_type,"mana")==0) {
      GET_MAX_MANA(ch) += number(20, 35);
      send_to_char("You feel a surge in magical power!\r\n",ch);
      return 1;}
      
      if (strcmp(meta_type,"move")==0) {
      GET_MAX_MOVE(ch) += number(20, 35);
      send_to_char("You feel a flow of vital energy!\r\n",ch);
      return 1;}
      
      if (strcmp(meta_type,"practice")==0) {
      GET_PRACTICES(ch) += number(5, 7);
      send_to_char("Your practice attempts increase!\r\n",ch);
      return 1;}

      if (strcmp(meta_type,"str")==0) {
       if (GET_STR(ch) <= 24) {
        GET_STR(ch) += 1;
        send_to_char("You feel a sudden surge in your strength.\r\n",ch);
        return 1;} else 
        send_to_char("You are as naturally strong as you can be.\r\n",ch);
      }
            
      if (strcmp(meta_type,"int")==0) {
       if (GET_INT(ch) < 25) {
        GET_INT(ch) += 1;
        send_to_char("You feel an increase in your understanding.\r\n",ch);
        return 1;} else 
        send_to_char("Sorry, you can understand no more.\r\n",ch);
      }

      if (strcmp(meta_type,"wis")==0) {
       if (GET_WIS(ch) < 25) {
        GET_WIS(ch) += 1;
        send_to_char("You suddenly feel more wise.\r\n",ch);
        return 1;} else
        send_to_char("Sorry, we cannot make you any more wise.\r\n",ch);
      }

      if (strcmp(meta_type,"dex")==0) {
       if (GET_DEX(ch) < 25) {
        GET_DEX(ch) += 1;
        send_to_char("You feel suddenly more agile!\r\n",ch);
        return 1;} else
        send_to_char("Sorry, but you are allready as agile as can be.\r\n",ch);
      }

      if (strcmp(meta_type,"con")==0) {
       if (GET_CON(ch) < 25) {
        GET_CON(ch) += 1;
        send_to_char("You feel suddenly more sturdy.\r\n",ch);
        return 1;} else
        send_to_char("Sorry, you are as hardy an individual as can be.\r\n",ch);
      }

      if (strcmp(meta_type,"cha")==0) {
       if (GET_CHA(ch) < 25) {
        GET_CHA(ch) += 1;
        send_to_char("Your social engineering skills increase!\r\n",ch);
        return 1;} else
        send_to_char("Sorry, your charisma stuns me allready.\r\n",ch);}
      }

     /* If it gets this far, show them the menu */

      send_to_char("&GSelect an operation from the following...\r\n",ch);
      send_to_char("&w_____________________________________________ \r\n",ch);
      send_to_char("&WMeta Operation    Modification   Exp    Gold \r\n",ch);
      send_to_char("&w_____________________________________________ \r\n",ch);
      send_to_char("&chealth            &g15-25         &M1200k   &Y250k \r\n",ch);
      send_to_char("&cmana              &g20-35         &M1200k   &Y250k \r\n",ch);
      send_to_char("&cmove              &g20-35         &M1000k   &Y250k \r\n",ch);      
      send_to_char("&w-------- \r\n",ch);
      send_to_char("&cpractice           &g5- 7          &M120k   &Y200k \r\n",ch);
      send_to_char("&w-------- \r\n",ch);
      send_to_char("&cstr                 &g1            &M3mil   &Y750k \r\n",ch);
      send_to_char("&cint                 &g1            &M3mil   &Y750k \r\n",ch);
      send_to_char("&cwis                 &g1            &M3mil   &Y750k \r\n",ch);
      send_to_char("&cdex                 &g1            &M3mil   &Y750k \r\n",ch);
      send_to_char("&ccon                 &g1            &M3mil   &Y750k \r\n",ch);
      send_to_char("&ccha                 &g1            &M3mil   &Y750k \r\n",ch);
      return 1; }
      return 0;}


SPECIAL(alchemist)
{
  long alch_gold = 2000;
  int j, cnt;
  struct obj_data *obj;
  struct char_data *alch = (struct char_data *) me;
  
  if (!CMD_IS("uncurse")) return FALSE;
  if ((GET_GOLD(ch) < alch_gold)) {
  	send_to_char("You don't have enough gold!\r\n", ch);
  	return (TRUE);
  }
  
  cnt = 0;
  for (j = 0; j < NUM_WEARS; j++)
    if ((obj = GET_EQ(ch, j)))
      if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        act("$n makes a magical gesture and $p worn by $N dissolves into nothingness.",FALSE,alch,obj,
       		ch,TO_NOTVICT);
  	act("$n makes a magical gesture and $p worn by you dissolves into nothingness.",FALSE,alch,
       		obj,ch,TO_VICT);
  	act("You make a magical gesture and you remove $p from $N.",FALSE,alch,
       		obj, ch,TO_CHAR);
        obj = unequip_char(ch, j);
        extract_obj(obj);
        cnt++;
      }
  if (cnt != 0)
    GET_GOLD(ch) -= alch_gold;
  else
    send_to_char("You have no cursed items worn!\r\n", ch);
        
  return TRUE;
}

SPECIAL(snake)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0)) {
    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_STANDING)
    return FALSE;

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4))) {
      npc_steal(ch, cons);
      return TRUE;
    }
  return FALSE;
}


SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return TRUE;

  if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
    cast_spell(ch, vict, NULL, 0, SPELL_SLEEP);

  if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
    cast_spell(ch, vict, NULL, 0, SPELL_BLINDNESS);

  if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, 0, SPELL_ENERGY_DRAIN);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, 0, SPELL_DISPEL_EVIL);
  }
  if (number(0, 4))
    return TRUE;
    
  if ((GET_LEVEL(ch) > 20) && number(0, 10)) {
    cast_spell(ch, vict, NULL, 0, SPELL_EARTH_ELEMENTAL);
    return TRUE;
  }

  switch (GET_LEVEL(ch) + number(-2, 2)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, 0, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, 0, SPELL_CHILL_TOUCH);
    break;
  case 8:
    cast_spell(ch, vict, NULL, 0, SPELL_ACID_ARROW);
    break;
  case 9:
    cast_spell(ch, vict, NULL, 0, SPELL_BURNING_HANDS);
    break;
  case 10:
    cast_spell(ch, vict, NULL, 0, SPELL_FLAME_ARROW);
    break;
  case 11:
    cast_spell(ch, vict, NULL, 0, SPELL_SHOCKING_GRASP);
    break;
  case 12:
    cast_spell(ch, vict, NULL, 0, SPELL_CONE_OF_COLD);
    break;
  case 13:
    cast_spell(ch, vict, NULL, 0, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, 0, SPELL_COLOR_SPRAY);
    break;
  case 37:
  case 38:
  case 39:
  case 40:
    cast_spell(ch, vict, NULL, 0, SPELL_DISINTEGRATE);
  default:
    cast_spell(ch, vict, NULL, 0, SPELL_FIREBALL);
    break;
  }
  return TRUE;

}

SPECIAL(evil_cleric)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
    cast_spell(ch, vict, NULL, 0, SPELL_SLEEP);

  if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
    cast_spell(ch, vict, NULL, 0, SPELL_BLINDNESS);

  if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, 0, SPELL_HARM);
    else if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, 0, SPELL_DISPEL_GOOD);
  }
  if (number(0, 4))
    return TRUE;

  switch (GET_LEVEL(ch) + number(-2, 2)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, 0, SPELL_EARTHQUAKE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, 0, SPELL_HARM);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, 0, SPELL_PARALYZE);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, 0, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, 0, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, 0, SPELL_MAGIC_MISSILE);
    break;
  default:
    cast_spell(ch, vict, NULL, 0, SPELL_FIREBALL);
    break;
  }
  return TRUE;

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
  int i;
  struct char_data *guard = (struct char_data *) me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  for (i = 0; guild_info[i][0] != -1; i++) {
    if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
	GET_ROOM_VNUM(IN_ROOM(ch)) == guild_info[i][1] &&
	cmd == guild_info[i][2]) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  return FALSE;
}



SPECIAL(puff)
{
  if (cmd)
    return (0);

  switch (number(0, 60)) {
  case 0:
    do_say(ch, "My god!  It's full of stars!", 0, 0);
    return (1);
  case 1:
    do_say(ch, "How'd all those fish get up here?", 0, 0);
    return (1);
  case 2:
    do_say(ch, "I'm a very female dragon.", 0, 0);
    return (1);
  case 3:
    do_say(ch, "I've got a peaceful, easy feeling.", 0, 0);
    return (1);
  default:
    return (0);
  }
}



SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (IS_CORPSE(i)) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for (temp = i->contains; temp; temp = next_obj) {
	next_obj = temp->next_content;
	obj_from_obj(temp);
	obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
  }
  return (FALSE);
}



SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return TRUE;
  }

  return FALSE;
}


SPECIAL(cityguard)
{
  struct char_data *tch, *evil, *vict = NULL;
  int max_evil;

  if (!cmd && GET_POS(ch) == POS_FIGHTING) {

    /* pseudo-randomly choose someone in the room who is fighting me */
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
      if (FIGHTING(vict) == ch && vict != ch && !number(0, 4))
        break;

    /* if I didn't pick any of those, then just slam the guy I'm fighting */
    if (vict == NULL)
      vict = FIGHTING(ch);

    switch (number(0, MAX(4, 8 - GET_LEVEL(ch) / 10))) {
      case 1:
        if (!IS_NPC(vict))
          do_kick(ch, vict->player.name, 0, 0);
        break;
      case 2:
        if (!IS_NPC(vict))
          do_bash(ch, vict->player.name, 0, 0);
        break;
      case 3:
        if (!IS_NPC(vict))
          do_disarm(ch, vict->player.name, 0, 0);
        break;
      default:
        break;
    }
    return TRUE;
  }

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return FALSE;

  max_evil = -100;
  evil = NULL;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && PLR_FLAGGED(tch, PLR_THIEF)){
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
	  (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
	max_evil = GET_ALIGNMENT(tch);
	evil = tch;
      }
    }
  }

  if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED);
    return (TRUE);
  }
  return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  int pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("list")) {
    send_to_char("Available pets are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    argument = one_argument(argument, buf);
    argument = one_argument(argument, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such pet!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= PET_PRICE(pet);

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);
    SET_BIT(MOB_FLAGS(pet), MOB_PET);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("May you enjoy your pet.\r\n", ch);
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }
  /* All commands except list and buy */
  return 0;
}

#define HORSE_PRICE(pet) (GET_LEVEL(pet) * 900)

SPECIAL(horse_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  int pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("list")) {
    send_to_char("Available horses are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    argument = one_argument(argument, buf);
    argument = one_argument(argument, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such horse!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= PET_PRICE(pet);

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);
    SET_BIT(MOB_FLAGS(pet), MOB_PET);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      /* sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name); */
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("Enjoy your horse, sir.\r\n", ch);
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }
  /* All commands except list and buy */
  return 0;
}

SPECIAL(silktrader)
{
  ACMD(do_say);
  
  if (cmd)
    return 0;

  if (world[ch->in_room].sector_type == SECT_CITY)
  switch (number(0, 30)) {
   case 0:
      act("$n eyes a passing woman.", FALSE, ch, 0, 0,TO_ROOM);
      do_say(ch, "Come, m'lady, and have a look at this precious silk!", 0, 0);
      return(1);
   case 1:
      act("$n says to you, 'Wouldn't you look lovely in this!'", FALSE, ch, 0, 0,TO_ROOM);
      act("$n shows you a gown of indigo silk.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 2:
      act("$n holds a pair of silk gloves up for you to inspect.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 3:
      act("$n cries out, 'Have at this fine silk from exotic corners of the world you will likely never see!", FALSE, ch, 0, 0,TO_ROOM);
      act("$n smirks.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 4:
      do_say(ch, "Step forward, my pretty locals!", 0, 0);
      return(1);
   case 5:
      act("$n shades his eyes with his hand.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 6:
      do_say(ch, "Have you ever seen an ogre in a silken gown?", 0, 0);
      do_say(ch, "I didn't *think* so!", 0, 0);
      act("$n throws his head back and cackles with insane glee!", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 7:
      act("$n hands you a glass of wine.", FALSE, ch, 0, 0,TO_ROOM);
      do_say(ch, "Come, have a seat and view my wares.", 0, 0);
      return(1);
   case 8:
      act("$n looks at you.", FALSE, ch, 0, 0,TO_ROOM);
      act("$n shakes his head sadly.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 9:
      act("$n fiddles with some maps.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 10:
      do_say(ch, "Here here! Beggars and nobles alike come forward and make your bids!", 0, 0);
      return(1);
   case 11:
      do_say(ch, "I am in this bourgeois hamlet for a limited time only!", 0, 0);
      act("$n swirls some wine in a glass.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
  }

  if (world[ch->in_room].sector_type != SECT_CITY)
  switch (number(0, 20)) {
   case 0:
      do_say(ch, "Ah! Fellow travellers! Come have a look at the finest silk this side of the infamous Ched Razimtheth!", 0, 0);
      return(1);
   case 1:
      act("$n looks at you.", FALSE, ch, 0, 0,TO_ROOM);
      do_say(ch, "You are feebly attired for the danger that lies ahead.", 0, 0);
      do_say(ch, "Silk is the way to go.", 0, 0);
      act("$n smiles warmly.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 2:
      do_say(ch, "Worthy adventurers, hear my call!", 0, 0);
      return(1);
   case 3:
      act("$n adjusts his cloak.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 4:
      act("$n says to you, 'Certain doom awaits you, therefore shall you die in silk.'", FALSE, ch, 0, 0,TO_ROOM);
      act("$n bows respectfully.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 5:
      do_say(ch, "Can you direct me to the nearest tavern?", 0, 0);
      return(1);
   case 6:
      do_say(ch, "Heard the latest ogre joke?", 0, 0);
      act("$n snickers to himself.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 7:
      do_say(ch, "What ho, traveller! Rest your legs here for a spell and peruse the latest in fashion!", 0, 0);
      return(1);
   case 8:
      do_say(ch, "Beware ye, traveller, lest ye come to live in Exile!", 0, 0);
      act("$n grins evilly.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 9:
      act("$n touches your shoulder.", FALSE, ch, 0, 0,TO_ROOM);
      do_say(ch, "A word of advice. Beware of any ale labled 'mushroom' or 'pumpkin'.", 0, 0);
      act("$n shivers uncomfortably.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
  
  }
  return(0);
}


SPECIAL(athos)
{
  ACMD(do_say);
  
  if(cmd)
   return 0;
    switch (number(0, 20)) {
    case 0:
      act("$n gazes into his wine gloomily.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
    case 1:
      act("$n grimaces.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
    case 2:
      act("$n asks you, 'Have you seen the lady, pale and fair, with a heart of stone?'", FALSE, ch, 0, 0,TO_ROOM);
      do_say(ch, "That monster will be the death of us all.", 0, 0);
      return(1);
    case 3:
      do_say(ch, "God save the King!", 0, 0);
      return(1);
    case 4:
      do_say(ch, "All for one and .. one for...", 0, 0);
      act("$n drowns himself in a swig of wine.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
    case 5:
      act("$n looks up with a philosophical air.", FALSE, ch, 0, 0,TO_ROOM);
      do_say(ch, "Women - God's eternal punishment on man.", 0, 0);
      return(1);
    case 6:
      act("$n downs his glass and leans heavily on the oaken table.", FALSE, ch, 0, 0,TO_ROOM);
      do_say(ch, "You know, we would best band together and wrestle the monstrous woman from her lair and home!", 0, 0);
      return(1);
  default: return(FALSE);
                break; }
    return(0);
}



SPECIAL(hangman)
{
ACMD(do_say);
if(cmd) return 0;
  switch (number(0, 15)) {
  case 0:
    act("$n whirls his noose like a lasso and it lands neatly around your neck.", FALSE, ch, 0, 0,TO_ROOM);
    do_say(ch, "You're next, you ugly rogue!", 0, 0);
    do_say(ch, "Just kidding.", 0, 0);
    act("$n pats you on your head.", FALSE, ch, 0, 0,TO_ROOM);
    return(1);
  case 1:
    do_say(ch, "I was conceived in Exile and have been integrated into society!", 0, 0);
    do_say(ch, "Muahaha!", 0, 0);
    return(1);
  case 2:
    do_say(ch, "Anyone have a butterknife I can borrow?", 0, 0);
    return(1);
  case 3:
    act("$n suddenly pulls a lever.", FALSE, ch, 0, 0,TO_ROOM);
    act("With the flash of light on metal a giant guillotine comes crashing down!", FALSE, ch, 0, 0,TO_ROOM);
    act("A head drops to the ground from the platform.", FALSE, ch, 0, 0,TO_ROOM);
    act("$n looks up and shouts wildly.", FALSE, ch, 0, 0,TO_ROOM);
    act("$n shouts, 'Next!'", FALSE, ch, 0, 0, TO_ROOM); 
    return(1);
  case 4:
   act("$n whistles a local tune.", FALSE, ch, 0, 0,TO_ROOM);
   return(1);
   default:
     return(FALSE);
     break;
  }
  return(0);
}   



SPECIAL(butcher)
{
ACMD(do_say);
if(cmd) return 0;
  switch (number(0, 40)) {
   case 0:
      do_say(ch, "I need a Union.", 0, 0);
      act("$n glares angrily.", FALSE, ch, 0, 0,TO_ROOM);
      act("$n rummages about for an axe.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 1:
      act("$n gnaws on a toothpick.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 2:
      act("$n runs a finger along the edge of a giant meat cleaver.", FALSE, ch, 0, 0,TO_ROOM);
      act("$n grins evilly.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 3:
      do_say(ch, "Pork for sale!", 0, 0);
      return(1);
   case 4:
      act("$n whispers to you, 'I've got some great damage eq in the back room. Wanna see?'", FALSE, ch, 0, 0,TO_ROOM);
      act("$n throws back his head and cackles with insane glee!", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 5:
      act("$n yawns.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 6:
      act("$n throws an arm around the headless body of an ogre and asks to have his picture taken.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 7:
      act("$n listlessly grabs a cleaver and hurls it into the wall behind your head.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 8:
      act("$n juggles some fingers.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 9:
      act("$n eyes your limbs.", FALSE, ch, 0, 0,TO_ROOM);
      act("$n chuckles.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 10:
      do_say(ch, "Hi, Alice.", 0, 0);
      return(1);
   case 11:
      do_say(ch, "Everyone looks like food to me these days.", 0, 0);
      act("$n sighs loudly.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 12:
      act("$n throws up his head and shouts wildly.", FALSE, ch, 0, 0,TO_ROOM);
      act("$n shouts, 'Bring out your dead!'", FALSE, ch, 0, 0, TO_ROOM);
      return(1);
   case 13:
      do_say(ch, "The worms crawl in, the worms crawl out..", 0, 0);
      return(1);
   case 14:
      act("$n sings 'Brave, brave Sir Patton...'", FALSE, ch, 0, 0,TO_ROOM);
      act("$n whistles a tune.", FALSE, ch, 0, 0,TO_ROOM);
      act("$n smirks.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 15:
      do_say(ch, "Get Lurch to bring me over a case and I'll sport you a year's supply of grilled ogre.", 0, 0);
      return(1);
    default: return(FALSE);
                break; }
    return(0);
}



SPECIAL(stu)
{
  ACMD(do_say);
  ACMD(do_flee);
  if(cmd)
    return 0;

  switch (number(0, 60)) {
    case 0:
      do_say(ch, "I'm so damn cool, I'm too cool to hang out with myself!", 0, 0);
      break;
    case 1:
      do_say(ch, "I'm really the NICEST guy you ever MEET!", 0, 0);
      break;
    case 2:
      do_say(ch, "Follow me for exp, gold and lessons in ADVANCED C!", 0, 0);
      break;
    case 3:
      do_say(ch, "Mind if I upload 200 megs of pregnant XXX gifs with no descriptions to your bbs?", 0, 0);
      break;
    case 4:
      do_say(ch, "Sex? No way! I'd rather jog 20 miles!", 0, 0);
      break;
    case 5:
      do_say(ch, "I'll take you OUT!!   ...tomorrow", 0, 0);
      break;
    case 6:
      do_say(ch, "I invented Mud you know...", 0, 0);
      break;
    case 7:
      do_say(ch, "Can I have a cup of water?", 0, 0);
      break;
    case 8:
      do_say(ch, "I'll be jogging down ventnor ave in 10 minutes if you want some!", 0, 0);
      break;
    case 9:
      do_say(ch, "Just let me pull a few strings and I'll get ya a site, they love me! - doesnt everyone?", 0, 0);
      break;
    case 10:
      do_say(ch, "Pssst! Someone tell Mercy to sport me some levels.", 0, 0);
      act("$n nudges you with his elbow.", FALSE, ch, 0, 0,TO_ROOM);
      break;
    case 11:
      do_say(ch, "Edgar! Buddy! Let's group and hack some ogres to tiny quivering bits!", 0, 0);
      break;
    case 12:
      act("$n tells you, 'Skylar has bad taste in women!'", FALSE, ch, 0, 0,TO_ROOM);
      act("$n screams in terror!", FALSE, ch, 0, 0,TO_ROOM);
      do_flee(ch, 0, 0, 0);
      break;
    case 13:
      if (number(0, 32767)<10){
      act("$n whispers to you, 'Dude! If you fucking say 'argle bargle' to the glowing fido he'll raise you a level!'", FALSE, ch, 0, 0,TO_ROOM);
      act("$n flexes.", FALSE, ch, 0, 0,TO_ROOM);}
      return(1);
    default:
      return(FALSE);
      break;
   return(1);
  }
  return 0;
}


SPECIAL(sund_earl)
{
  ACMD(do_say);
  if (cmd)
    return(FALSE);
  switch (number(0, 20)) {
   case 0:
      do_say(ch, "Lovely weather today.", 0, 0);
      return(1);
   case 1:
    act("$n practices a lunge with an imaginary foe.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
   case 2:
      do_say(ch, "Hot performance at the gallows tonight.", 0, 0);
     act("$n winks suggestively.", FALSE, ch, 0, 0,TO_ROOM); 
     return(1);
   case 3:
      do_say(ch, "Must remember to up the taxes at my convenience.", 0, 0);
      return(1);
   case 4:
      do_say(ch, "Sundhaven is impermeable to the enemy!", 0, 0);
      act("$n growls menacingly.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
 case 5:
      do_say(ch, "Decadence is the credence of the abominable.", 0, 0);
      return(1);
 case 6:
      do_say(ch, "I look at you and get a wonderful sense of impending doom.", 0, 0);
      act("$n chortles merrily.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
 case 7:
      act("$n touches his goatee ponderously.", FALSE, ch, 0, 0,TO_ROOM);
      return(1);
 case 8:
      do_say(ch, "It's Mexican Madness night at Maynards!", 0, 0);
      act("$n bounces around.", FALSE, ch, 0, 0, TO_ROOM);
      return(1); 
    default: return(FALSE);
              break;
    return(0);  
 }
}


SPECIAL(blinder)
{
  ACMD(do_say);
  
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 100)+GET_LEVEL(ch) >= 50)) {
    act("$n whispers, 'So, $N! You wouldst share my affliction!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n whispers, 'So, $N! You wouldst share my affliction!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    act("$n's frayed cloak blows as he points at $N.", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n's frayed cloak blows as he aims a bony finger at you.", 1, ch, 0, FIGHTING(ch), TO_VICT);
    act("A flash of pale fire explodes in $N's face!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("A flash of pale fire explodes in your face!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, 0, SPELL_BLINDNESS, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}


SPECIAL(idiot)
{
  ACMD(do_say);
  
if(cmd) return FALSE;
  switch (number(0, 40)) {
   case 0:
      do_say(ch, "even if idiot = god", 0, 0);
      do_say(ch, "and Stu = idiot", 0, 0);
      do_say(ch, "Stu could still not = god.", 0, 0);
      act("$n smiles.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 1:
      act("$n balances a newbie sword on his head.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 2:
      act("$n doesn't think you could stand up to him in a duel.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 3:
      do_say(ch, "Rome really was built in a day.", 0, 0);
      act("$n snickers.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 4:
      act("$n flips over and walks around on his hands.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 5:
      act("$n cartwheels around the room.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 6:
      do_say(ch, "How many ogres does it take to screw in a light bulb?", 0, 0);
      act("$n stops and whaps himself upside the head.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 7:
      do_say(ch, "Uh huh. Uh huh huh.", 0, 0);
      return TRUE;
   case 8:
      act("$n looks at you.", FALSE, ch, 0, 0,TO_ROOM);
      act("$n whistles quietly.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 9:
      act("$n taps out a tune on your forehead.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 10:
      act("$n has a battle of wits with himself and comes out unharmed.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 11:
      do_say(ch, "All this and I am just a number.", 0, 0);
      act("$n cries on your shoulder.", FALSE, ch, 0, 0,TO_ROOM);
      return TRUE;
   case 12:
      do_say(ch, "A certain hunchback I know dresses very similar to you, very similar...", 0, 0);
      return TRUE;
   default: 
      return FALSE;
  }
 return FALSE;
}

/* This special procedure makes a mob into a 'rent-a-cleric', who sells spells
   by the sea shore... uuh, maybe not.  Anyway, the mob will also cast certain
   spells on low-level characters in the room for free.  
   By:  Wyatt Bode	Date:  April, 1996
*/
SPECIAL(cleric)
{
  int i;
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct price_info {
    short int number;
    char name[25];
    short int price;
  } prices[] = {
    /* Spell Num (defined)	Name shown	  Price  */
    { SPELL_ARMOR, 		"armor          ", 700 },
    { SPELL_BLESS, 		"bless          ", 1400 },
    { SPELL_REMOVE_POISON, 	"remove poison  ", 2000 },
    { SPELL_CURE_BLIND, 	"cure blindness ", 2000 },
    { SPELL_CURE_CRITIC, 	"critic         ", 2300 },
    { SPELL_SANCTUARY, 		"sanctuary      ", 3000 },
    { SPELL_HEAL, 		"heal           ", 3100 },
    { SPELL_DISPEL_MAGIC,        "dispel magic   ", 10000 },

    /* The next line must be last, add new spells above. */ 
    { -1, "\r\n", -1 }
  };

/* NOTE:  In interpreter.c, you must define a command called 'heal' for this
   spec_proc to work.  Just define it as do_not_here, and the mob will take 
   care of the rest.  (If you don't know what this means, look in interpreter.c
   for a clue.)
*/

  if (CMD_IS("heal")) {
    argument = one_argument(argument, buf);

    if (GET_POS(ch) == POS_FIGHTING) return TRUE;

    if (*buf) {
      for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
	if (is_abbrev(buf, prices[i].name)) {
	  if (GET_GOLD(ch) < prices[i].price) {
	    act("$n tells you, 'You don't have enough gold for that spell!'",
		FALSE, (struct char_data *) me, 0, ch, TO_VICT);
            return TRUE;
          } else {
	    
	    act("$N gives $n some money.",
		FALSE, (struct char_data *) me, 0, ch, TO_NOTVICT);
	    sprintf(buf, "You give %s %d coins.\r\n", 
		    GET_NAME((struct char_data *) me), prices[i].price);
	    send_to_char(buf, ch);
	    GET_GOLD(ch) -= prices[i].price;
	    /* Uncomment the next line to make the mob get RICH! */
            /* GET_GOLD((struct char_data *) me) += prices[i].price; */

            cast_spell((struct char_data *) me, ch, NULL, 0, prices[i].number);
            return TRUE;
          
	  }
	}
      }
      act("$n tells you, 'I do not know of that spell!"
	  "  Type 'heal' for a list.'", FALSE, (struct char_data *) me, 
	  0, ch, TO_VICT);
	  
      return TRUE;
    } else {
      act("$n tells you, 'Here is a listing of the prices for my services.'",
	  FALSE, (struct char_data *) me, 0, ch, TO_VICT);
      for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
        sprintf(buf, "%s%d\r\n", prices[i].name, prices[i].price);
        send_to_char(buf, ch);
      }
      return TRUE;
    }
  }

  if (cmd) return FALSE;

  /* pseudo-randomly choose someone in the room */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (!number(0, 3))
      break;
  
  /* change the level at the end of the next line to control free spells */
  if (vict == NULL || IS_NPC(vict) || (GET_LEVEL(vict) > 10))
    return FALSE;

  switch (number(1, GET_LEVEL(vict))) { 
      case 1: cast_spell(ch, vict, NULL, 0, SPELL_CURE_LIGHT); break; 
      case 2: cast_spell(ch, vict, NULL, 0, SPELL_BLESS); break; 
      case 3: cast_spell(ch, vict, NULL, 0, SPELL_DISPEL_MAGIC); break;
      case 4: cast_spell(ch, vict, NULL, 0, SPELL_CURE_LIGHT); break; 
      case 5: cast_spell(ch, vict, NULL, 0, SPELL_BLESS); break; 
      case 6: cast_spell(ch, vict, NULL, 0, SPELL_CURE_CRITIC); break; 
      case 7: cast_spell(ch, vict, NULL, 0, SPELL_ARMOR); break;
      case 8: cast_spell(ch, vict, NULL, 0, SPELL_CURE_CRITIC); break; 
      case 9: cast_spell(ch, vict, NULL, 0, SPELL_ARMOR); break; 
      case 10: 
        /* special wacky thing, your mileage may vary */ 
	act("$n utters the words, 'energizer'.", TRUE, ch, 0, vict, TO_ROOM);
	act("You feel invigorated!", FALSE, ch, 0, vict, TO_VICT);
	GET_MANA(vict) = 
	   MIN(GET_MAX_MANA(vict), MAX((GET_MANA(vict) + 10), number(50, 200)));
        break; 
  }
  return TRUE; 
}               

SPECIAL(wiseman)
{
  struct obj_data *object = NULL;
  char buf[MAX_STRING_LENGTH];
  
  if (CMD_IS("whatis")) {
    argument = one_argument(argument, buf);

    if (GET_POS(ch) == POS_FIGHTING) return TRUE; 
    if (*buf) {
      object = get_obj_car(ch , buf);
      if (object) {
          act("$n does several strange gestures above that object.", FALSE, (struct char_data *) me, 0, ch, TO_VICT);
          call_magic(ch, NULL, object, 0, SPELL_IDENTIFY, 30, 0);
          act("$n looked at some object.", FALSE, (struct char_data *) me, 0, ch, TO_NOTVICT);
          return TRUE;
        }
      else
        {
          act("$n shouts: 'Show me that thing or go away.'", FALSE, (struct char_data *) me, 0, ch, TO_VICT);
          return TRUE;
        }
      }
    else
      {
        act("$n asks you what do you want to know.", FALSE, (struct char_data *) me, 0, ch, TO_VICT);
        return TRUE;
      }
  }
  return FALSE;
}

SPECIAL(sea_serpent)
{
  struct char_data *vict;
  if (cmd)
     return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
     return FALSE;
  if (!ch->char_specials.fighting)
     return FALSE;

  vict=ch->char_specials.fighting; 

   if (!vict)
      return FALSE;
   if ((GET_LEVEL(ch)<10) && (number(0,100)==10))
      {
        act("$n utters the words, 'hisssssss'.",1, ch, 0, 0, TO_ROOM);
        call_magic(ch, vict, NULL, 0, SPELL_FIREBALL, 15, CAST_SPELL);
      }
   else if (number(0,100)<=15) 
      {
        act("$n utters the words, 'hissssss'.",1, ch, 0,0, TO_ROOM);
        call_magic(ch, vict, NULL, 0, SPELL_FIREBALL, GET_LEVEL(ch), CAST_SPELL);
      }
   return(TRUE);
}

SPECIAL(Leviathan)
{
  struct char_data *vict;
  if (cmd)
     return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
     return FALSE;
  if (!ch->char_specials.fighting)
     return FALSE;

   vict=ch->char_specials.fighting; 

   if (!vict)
      return FALSE;

  switch(number(0,20))
    {
    case 1:
      act("$n utters the words, 'transvecta aqua'.",1,ch,0,0,TO_ROOM);
      call_magic(ch, vict, NULL, 0, SPELL_COLOR_SPRAY, 30, CAST_SPELL);
       break;
    case 6:
      act("$n looks at you with the deepest sorrow.",1,ch,0,0,TO_ROOM);
       break;
    case 12:
      act("$n utters the words, 'transvecta talon'.", 1,ch,0,0,TO_ROOM);
      call_magic(ch, ch, NULL, 0, SPELL_COLOR_SPRAY, 30, CAST_SPELL);
       break;
    default:
      break;     
 
    }
 return(TRUE);
}



SPECIAL(master)
{ 
   char buf[MAX_STRING_LENGTH];
   ACMD(do_order);
 
  if (cmd || !AWAKE(ch)) return(FALSE);

  switch(number(0,8))
  {
     case 0: 
     case 1:
     case 2:
         if (GET_POS(ch)!=POS_FIGHTING)
	   {
         act("The master looks at his minions and growls, 'get to work, bitches!'", FALSE,ch, 0, 0, TO_ROOM); 
 /*        sprintf(buf, "followers bow master");
         do_order(ch, buf,0,0); */
         return(TRUE);
         break;}
     case 3:
     case 4: 
     case 5:
     case 6:
        if (GET_POS(ch)==POS_FIGHTING)
        {
           act("The master screams loudly, 'HELP ME, MY SLAVES!'", FALSE, ch, 0, 0, TO_ROOM); 
           sprintf(buf, "followers kill %s",GET_NAME(ch->char_specials.fighting));
                do_order(ch, buf, 0, 0);
                return(TRUE);
               
	 }
         
      break;
     default: return(FALSE);
              break;
   }
   return(TRUE);
}       
 
SPECIAL(slave)
{
   struct char_data *find_npc_by_name(struct char_data *chAtChar, char *pszName, 
                                                             int iLen);
   struct char_data *master;
//   char buf[MAX_STRING_LENGTH];

   if (!AWAKE(ch)) return FALSE;
  
   sprintf(buf, "a fat harem master");

   if ((!cmd) && ((master = find_npc_by_name(ch, buf, 18))))
   {
    if (ch->master!=master)
    {
      if(ch->master)
        stop_follower(ch);
      add_follower(ch, master);
      SET_BIT(AFF_FLAGS(ch), AFF_CHARM);
      IS_CARRYING_W(ch) = 1000;
      IS_CARRYING_N(ch) = 100; 
      return TRUE;
    }	    
   }

   return(FALSE);
 }
  
SPECIAL(Priest_of_Fear)
{
   
 struct char_data *vict;
  if (cmd)
     return(FALSE);
  if (GET_POS(ch) != POS_FIGHTING)
     return(FALSE);
  if (!ch->char_specials.fighting)
     return(FALSE);

  vict=ch->char_specials.fighting; 

   if (!vict)
      return(FALSE);
   if (number(0,100)<35)
      {
        act("$n utters the words, 'pabrow'.",1, ch, 0, 0, TO_ROOM);
        call_magic(ch, vict, NULL, 0, SPELL_FEAR, GET_LEVEL(ch), CAST_SPELL);
      }
   return(TRUE);
}
/*
SPECIAL(butcher)
{
  if (cmd) return(FALSE);
  if (GET_POS(ch)==POS_FIGHTING) return(FALSE);
  if (ch->char_specials.fighting) return(FALSE);
   
  if (number(0,100)<20)
    act("$n begins sharpening $s knife.",1,ch,0,0,TO_ROOM);
  return(TRUE);
}
*/
SPECIAL(tarbaby)
{
  struct char_data *vict, *temp;

  if (cmd!=224) return(FALSE);

  for (vict=character_list;vict;vict=temp)
  {
    temp = vict->next;
    if (ch->in_room == vict->in_room) 
    {
      if (!IS_NPC(vict) && (vict!=ch)) 
      {
        if (ch->master!=vict)
        {
         if(ch->master)
            stop_follower(ch);
         add_follower(ch, vict);
         IS_CARRYING_W(ch) = 100;
         IS_CARRYING_N(ch) = 10; 
         return(TRUE);
        }            
      }
     }
  }       

  if (!vict) return(FALSE);
  
  if (cmd==224)
  {
     send_to_char("Ho ho, hee hee, you are sooo funneeee!\n\r",ch);
     return(TRUE);
  }
  
 
   else if (number(0,100)<20)
     {
        act("$n looks at you with the cutest expression.",FALSE,ch,0,vict,TO_CHAR);
        act("$n looks at $N with the cutest expression.",FALSE,ch,0,vict, TO_ROOM);
        return(TRUE);
      }
  return(FALSE);
}
   
SPECIAL(Grand_Inquisitor)
{
 struct char_data *vict;
  if (cmd)
     return(FALSE);
  if (GET_POS(ch) != POS_FIGHTING)
     return(FALSE);
  if (!ch->char_specials.fighting)
     return(FALSE);

  vict=ch->char_specials.fighting; 

   if (!vict)
      return(FALSE);
   if (number(0,100)<15)
      {
        act("$n utters the words, 'ordalaba'.",1, ch, 0, 0, TO_ROOM);
        call_magic(ch, vict, NULL, 0, SPELL_ENERGY_DRAIN, GET_LEVEL(ch), CAST_SPELL);
        return(TRUE);
      }
   else if(number(0,100)<30)
     { act("$n evilly grins and snaps his hands.",1,ch,0,0,TO_ROOM);
       call_magic(ch, vict, NULL, 0, SPELL_ANIMATE_DEAD, GET_LEVEL(ch), CAST_SPELL);
       return(TRUE);
     }
   return(TRUE);
}  
  
SPECIAL(High_Priest_of_Terror)
{
 struct char_data *vict;
 struct char_data *newvict,*temp;
  if (cmd)
     return(FALSE);
  if (GET_POS(ch) != POS_FIGHTING)
     return(FALSE);
  if (!ch->char_specials.fighting)
     return(FALSE);

  vict=ch->char_specials.fighting; 

   if (!vict)
      return(FALSE);

   if (number(0,100)<10)
      {
        act("$n utters the words, 'ordalaba'.",1, ch, 0, 0, TO_ROOM);
        call_magic(ch, vict, NULL, 0, SPELL_CHARM, GET_LEVEL(ch), CAST_SPELL);
        return(TRUE);
        for (newvict=character_list;newvict;newvict=temp)
	{
           temp = newvict->next;
           if (ch->in_room == newvict->in_room) 
           {
              if (!IS_NPC(newvict)) 
	      {
                if (!IS_SET(AFF_FLAGS(newvict), AFF_CHARM))
		{
                  ch->char_specials.fighting=newvict;
                  newvict->char_specials.fighting=ch;
                }
	      }
	    }
	 }          
      }
   else if(number(0,100)<30)
     { 
       act("$n waves $s hands in a swirling motion.",1,ch,0,0,TO_ROOM);
       call_magic(ch, vict, NULL, 0, SPELL_EARTHQUAKE, GET_LEVEL(ch), CAST_SPELL);
       return(TRUE);
     }
   return(TRUE);
 }
	/* I can't find an example of a mobile casting an area spell so I'm going to
	 * fake it using the magic user special.
	 */
SPECIAL(dragon_fire)
	{
//          struct char_data *dragon = (struct char_data *) me;

	  /* I don't know what 'cmd' is but we never do anything if we don't breathe
	   * fire unless we're fighting.
	   */
	  if (cmd || GET_POS(ch) != POS_FIGHTING)
	    return FALSE;

	  /* Only breathe fire 20% of the time */
	  if (number(0, 4))
	    return FALSE;

	  /* We could actually pass GET_LEVEL(ch) instead of 0 for the level of the
	   * breath so we could have tougher dragons.  Right now, it does damage
	   * equal to a fireball in all cases.
	   */
	  call_magic(ch, NULL, NULL, 0, SPELL_FIRE_BREATH, 0, CAST_BREATH); 
//          damage(dragon, FIGHTING(dragon), <damage here>, 0);
/* If you use the damage call, you don't need the spell, but if you use
 * the spell, you should add the no_magic room information below.
 */
	  return TRUE;
	}

SPECIAL(dragon_gas)
	{
//          struct char_data *dragon = (struct char_data *) me;

	  /* I don't know what 'cmd' is but we never do anything if we don't breathe
	   * gas unless we're fighting.
	   */
	  if (cmd || GET_POS(ch) != POS_FIGHTING)
	    return FALSE;

	  /* Only breathe gas 20% of the time */
	  if (number(0, 4))
	    return FALSE;

	  /* We could actually pass GET_LEVEL(ch) instead of 0 for the level of the
	   * breath so we could have tougher dragons.  Right now, it does damage
	   * equal to a fireball in all cases.
	   */
	  call_magic(ch, NULL, NULL, 0, SPELL_GAS_BREATH, 0, CAST_BREATH); 
//          damage(dragon, FIGHTING(dragon), <damage here>, 0);
/* If you use the damage call, you don't need the spell, but if you use
 * the spell, you should add the no_magic room information below.
 */
	  return TRUE;
	}

SPECIAL(dragon_frost)
	{
//          struct char_data *dragon = (struct char_data *) me;

	  /* I don't know what 'cmd' is but we never do anything if we don't breathe
	   * frost unless we're fighting.
	   */
	  if (cmd || GET_POS(ch) != POS_FIGHTING)
	    return FALSE;

	  /* Only breathe frost 20% of the time */
	  if (number(0, 4))
	    return FALSE;

	  /* We could actually pass GET_LEVEL(ch) instead of 0 for the level of the
	   * breath so we could have tougher dragons.  Right now, it does damage
	   * equal to a fireball in all cases.
	   */
	  call_magic(ch, NULL, NULL, 0, SPELL_FROST_BREATH, 0, CAST_BREATH); 
//          damage(dragon, FIGHTING(dragon), <damage here>, 0);
/* If you use the damage call, you don't need the spell, but if you use
 * the spell, you should add the no_magic room information below.
 */
	  return TRUE;
	}

SPECIAL(dragon_acid)
	{
//          struct char_data *dragon = (struct char_data *) me;

	  /* I don't know what 'cmd' is but we never do anything if we don't breathe
	   * acid unless we're fighting.
	   */
	  if (cmd || GET_POS(ch) != POS_FIGHTING)
	    return FALSE;

	  /* Only breathe acid 20% of the time */
	  if (number(0, 4))
	    return FALSE;

	  /* We could actually pass GET_LEVEL(ch) instead of 0 for the level of the
	   * breath so we could have tougher dragons.  Right now, it does damage
	   * equal to a fireball in all cases.
	   */
	  call_magic(ch, NULL, NULL, 0, SPELL_ACID_BREATH, 0, CAST_BREATH); 
//          damage(dragon, FIGHTING(dragon), <damage here>, 0);
/* If you use the damage call, you don't need the spell, but if you use
 * the spell, you should add the no_magic room information below.
 */
	  return TRUE;
	}

SPECIAL(dragon_lightning)
	{
//          struct char_data *dragon = (struct char_data *) me;

	  /* I don't know what 'cmd' is but we never do anything if we don't breathe
	   * lightning unless we're fighting.
	   */
	  if (cmd || GET_POS(ch) != POS_FIGHTING)
	    return FALSE;

	  /* Only breathe lightning 20% of the time */
	  if (number(0, 4))
	    return FALSE;

	  /* We could actually pass GET_LEVEL(ch) instead of 0 for the level of the
	   * breath so we could have tougher dragons.  Right now, it does damage
	   * equal to a fireball in all cases.
	   */
	  call_magic(ch, NULL, NULL, 0, SPELL_LIGHTNING_BREATH, 0, CAST_BREATH); 
//          damage(dragon, FIGHTING(dragon), <damage here>, 0);
/* If you use the damage call, you don't need the spell, but if you use
 * the spell, you should add the no_magic room information below.
 */
          return TRUE;
	}

	/* This is the special for the brass dragon in the desert */
SPECIAL(dragon_guard)
	{
	  int i;
	  extern int guild_info[][3];
	  struct char_data *dragon = (struct char_data *) me;
	  char *buf = "The dragon growls at you, blocking your way.\r\n";
	  char *buf2 = "The dragon scares $n by growling fiercely, stopping $m in $s tracks.";

	  /* Check to see if a character is trying to move past the dragon */
	  if (IS_MOVE(cmd) && !IS_AFFECTED(dragon, AFF_BLIND)) {
	    for (i = 0; guild_info[i][0] != -1; i++) {
	      if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
		  world[ch->in_room].number == guild_info[i][1] &&
		  cmd == guild_info[i][2]) {
	        send_to_char(buf, ch);
	        act(buf2, FALSE, ch, 0, 0, TO_ROOM);
	        return TRUE;
	      }
	    }
	  }

	  /* Nothing left to do except cast spells if we are fighting */
	  if (cmd || GET_POS(ch) != POS_FIGHTING)
	    return FALSE;

	  /* Only breathe lightning 20% of the time */
	  if (number(0, 4))
	    return FALSE;

	  /* We could actually pass GET_LEVEL(ch) instead of 0 for the level of the
	   * breath so we could have tougher dragons.  Right now, it does damage
	   * equal to a fireball in all cases.
	   */
	  call_magic(ch, NULL, NULL, 0, SPELL_LIGHTNING_BREATH, 0, CAST_BREATH); 
//          damage(dragon, FIGHTING(dragon), <damage here>, 0);
/* If you use the damage call, you don't need the spell, but if you use
 * the spell, you should add the no_magic room information below.
 */
	  return TRUE;
	}  


/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

/* token machines & conductors in one proc! */
SPECIAL(tokens)
{ 
   struct obj_data *o, *obj, *temptok;
   bool has_token;
   int tokennum, transroom;
   ACMD(do_look);
   void list_stations(struct char_data *ch);
   tokennum = 9016; /* obj number of your tokens! */

   if (CMD_IS("list")) {
     send_to_char("Type 'buy' to buy a token.\n\r", ch);
     list_stations(ch);
     return(TRUE); 
   } else if (CMD_IS("buy")) {
     if (GET_GOLD(ch) < 500) {
     send_to_char("You don't have enough gold! A token costs 500 coins.\n\r", ch);
     return(TRUE);
     }
     send_to_char("You buy a transporter token.\n\r", ch);
     act("$n buys a transporter token.",
     FALSE, ch, 0, 0, TO_ROOM);
     GET_GOLD(ch) -= 500;
     temptok = read_object(9016, VIRTUAL);
     obj_to_char(temptok, ch);
     return(TRUE);
   } else if (cmd >= 1 && cmd <= 6) {
     /* transporter there? */
     if ((transroom = real_room(get_transroom(ch->in_room, cmd - 1))) > NOWHERE) {
       has_token = FALSE;
       for (obj = ch->carrying; obj; obj = obj->next_content)
         if (GET_OBJ_VNUM(obj) == tokennum) {
           has_token = TRUE;
           o = obj; 
         }
       if (!has_token) {
         send_to_char("You need a token to use the transporter.\n\r", ch);
         return(TRUE);
       } else {
         send_to_char("The conductor takes your token, and you enter the transporter\n\r", ch);
         act("The conductor takes $n's token, and $e leaves east.\n\r", FALSE, ch, 0, 0, TO_ROOM);
         obj_from_char(o);
         extract_obj(o);
         char_from_room(ch);
         char_to_room(ch, transroom);
         do_look(ch, "", 0, 0);
         return(TRUE);
       }
     } /* end if transporter is there */
   } /* end if cmd = 2 */
  return(FALSE); /* all other cmds */
} /* end spec proc */

#define XFER_TAX(amount)	(amount * 5 / 100)
#define MAX_XFER		999999999
#define MIN_XFER		1000

SPECIAL(bank)
{
  int amount;
  int temp;
  long pos_i;
  struct char_file_u chdata;
  struct char_data * vict;

  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is &Y%d&w coins.\r\n",
	      GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) -= amount;
    GET_BANK_GOLD(ch) += amount;
    sprintf(buf, "You deposit &Y%d&w coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins deposited!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) += amount;
    GET_BANK_GOLD(ch) -= amount;
    sprintf(buf, "You withdraw &Y%d&w coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("bankxfer")) {
    two_arguments(argument, buf1, buf2);
    if ((!buf1) || (!*buf1) || (amount = atoi(buf1)) <= 0) {
      send_to_char("How much do you want to transfer?\r\n", ch);
      return 1;
    }
    if ((!buf2) || (!*buf2)) {
      send_to_char("Uhm, and who is the recipient?\r\n", ch);
      return 1;
    }
    if (amount > MAX_XFER) {
      send_to_char("Bank cannot transfer such huge amounts of gold!\r\n", ch);
      return 1;
    }
    
    if (amount < MIN_XFER) {
      send_to_char("Bank will not transfer little amounts of money!\r\n", ch);
      return 1;
    }
    /* I must to do it so strange because of overflowing */
    temp = GET_BANK_GOLD(ch);
    temp -= amount;
    temp -=XFER_TAX(amount);
    if (temp < 0) {
      send_to_char("You don't have that many coins deposited (you need "
        "xfer tax of 5\%)!\r\n", ch);
      return 1;
    }
    if ((vict = get_char_vis(ch, buf2))) {

      GET_BANK_GOLD(ch) = temp;
      GET_BANK_GOLD(vict) += amount;
      strcpy(buf2, vict->player.name);
      sprintf(buf, "Tranfered &Y%d&w coins to &c%s's&w account.\r\n", amount, CAP(buf2));
      send_to_char(buf, ch);
      strcpy(buf2, ch->player.name);
      sprintf(buf, "&r-&m=&M#&m=&r- &WFirst National Bank of DarkenedLights &r-&m=&M#&m=&r-&w\r\n\r\n");
      sprintf(buf, "%s%s tranfered &Y%d&w coins to your account.\r\n\r\n", buf, CAP(buf2), amount);
      store_mail(GET_IDNUM(vict), GET_IDNUM(ch), NULL, buf);
//      send_to_char(buf, ch);
      act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    } else if ((pos_i = load_char(buf2, &chdata))) {
      GET_BANK_GOLD(ch) = temp;     
      chdata.points.bank_gold += amount;
      save_char_raw(&chdata, pos_i);
      sprintf(buf, "Tranferred &Y%d&w coins to &c%s's&w account.\r\n", amount, CAP(chdata.name));
      send_to_char(buf, ch);
      act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
      strcpy(buf2, ch->player.name);
      sprintf(buf, "&r-&m=&M#&m=&r- &WFirst National Bank of DarkenedLights &r-&m=&M#&m=&r-&w\r\n\r\n");
      sprintf(buf, "%s%s tranfered &Y%d&w coins to your account.\r\n\r\n", buf, CAP(buf2), amount);
      store_mail(chdata.char_specials_saved.idnum, GET_IDNUM(ch), NULL, buf);
    } else {
      send_to_char("Such player does not exist!", ch);
    }
    return 1;
  } else
    return 0;
}

SPECIAL(transporter)
{
  if (!CMD_IS("list")) return 0;
  list_stations_wagon(ch, GET_ROOM_VNUM(ch->in_room));
  return 1;
}

/* weapon_spell() function and special procedures -dak */
void weapon_spell(char *to_ch, char *to_vict, char *to_room,
                  struct char_data *ch, struct char_data *vict, 
                  struct obj_data *obj, int spl)
{
  int level = LVL_IMPL+1, i;
  
  for (i=0; i<NUM_CLASSES; i++)
    if (spell_info[spl].min_level[i] < level)
      level = spell_info[spl].min_level[i];
  level = MAX(1, MIN(LVL_IMMORT-1, level));

  act(to_ch, FALSE, ch, obj, vict, TO_CHAR);
  act(to_vict, FALSE, ch, obj, vict, TO_VICT);
  act(to_room, FALSE, ch, obj, vict, TO_NOTVICT);
  call_magic(ch, vict, 0, 0, spl, level, CAST_SPELL);
}


SPECIAL(blind_weapon)
{
  struct char_data *vict = FIGHTING(ch);
  
  if (cmd || !vict || !number(0, 9))
    return 0;
  if (IS_AFFECTED(vict, AFF_BLIND) || MOB_FLAGGED(vict, MOB_NOBLIND))
    return 0;
    
  weapon_spell("You scream, \"MIDNIGHT!\" at $N.",
               "$n screams, \"Midnight!\" at you.",
               "$n screams, \"Midnight!\" at $N.",
               ch, vict, (struct obj_data *) me, SPELL_BLINDNESS);
  return 1;
}

SPECIAL(fireball_weapon)
{
  struct char_data *vict = FIGHTING(ch);
  
  if (cmd || !vict || !number(0, 14))
    return 0;
    
  weapon_spell("Fire seems to shoot from your $F and roar through the air at $N!",
               "Fire tears from $n's $F and roars through the air at you!",
               "$n's $F comes alive with a fireball that roars towards $N.",
               ch, vict, (struct obj_data *) me, SPELL_FIREBALL);
  return 1;
}

SPECIAL(curse_weapon)
{
  struct char_data *vict = FIGHTING(ch);
  
  if (cmd || !vict || !number(0, 11))
    return 0;
  if (IS_AFFECTED(vict, AFF_CURSE))
    return 0;
    
  weapon_spell("Your $F turns black for a brief moment.",
               "$n's $F turns obsidian as it nears contact with you.",
               "$n's $F turns black as it contacts $N.",
               ch, vict, (struct obj_data *) me, SPELL_CURSE);
  return 1;
}
SPECIAL(marbles)
{
  struct obj_data *tobj = me;

  if (tobj->in_room == NOWHERE)
    return 0;

  if (CMD_IS("north") || CMD_IS("south") || CMD_IS("east") || CMD_IS("west") ||
      CMD_IS("up") || CMD_IS("down")) {
    if (number(1, 100) + GET_DEX(ch) > 50) {
      act("You slip on $p and fall.", FALSE, ch, tobj, 0, TO_CHAR);
      act("$n slips on $p and falls.", FALSE, ch, tobj, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      return 1;
    }
    else {
      act("You slip on $p, but manage to retain your balance.", FALSE, ch, tobj, 0, TO_CHAR);
      act("$n slips on $p, but manages to retain $s balance.", FALSE, ch, tobj, 0, TO_ROOM);
    }
  }
  return 0;
}

SPECIAL (portal)
{
  struct obj_data *obj = (struct obj_data *) me;
  struct obj_data *port;
  char obj_name[MAX_STRING_LENGTH];

    if (!CMD_IS("enter")) return FALSE;

    argument = one_argument(argument,obj_name);
    if (!(port = get_obj_in_list_vis(ch, obj_name, world[ch->in_room].contents)))	{
      return(FALSE);
    }
    
    if (port != obj)
      return(FALSE);
    
    if (port->obj_flags.value[1] <= 0 ||
	port->obj_flags.value[1] > 32000) {
      send_to_char("The portal leads nowhere\n\r", ch);
      return TRUE;
    }
    
    act("$n enters $p, and vanishes!", FALSE, ch, port, 0, TO_ROOM);
    act("You enter $p, and you are transported elsewhere", FALSE, ch, port, 0, TO_CHAR);
    char_from_room(ch);  
    char_to_room(ch, port->obj_flags.value[1]);
    look_at_room(ch,0);
    act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);
  return TRUE;
}

SPECIAL(recharger)
{
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  int maxcharge = 0, mincharge = 0, chargeval = 0;

  if (CMD_IS("list"))
  {
    send_to_char("You may use the SWEET machine to recharge a staff or wand.\r\n", ch);
    send_to_char("It costs 10000 coins to recharge a staff or wand.\r\n", ch);
    send_to_char("To recharge and item type: 'recharge <staff or wand>'.\r\n", ch);
    return (TRUE);
  } else
    if (CMD_IS("recharge")) {
    argument = one_argument(argument, buf);

    if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying)))
    {
      send_to_char("You don't have that!\r\n", ch);
      return (TRUE);
    }
    if (GET_OBJ_TYPE(obj) != ITEM_STAFF &&
        GET_OBJ_TYPE(obj) != ITEM_WAND)
    {
      send_to_char("Are you daft!  You can't recharge that!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < 10000) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    maxcharge = GET_OBJ_VAL(obj, 1);
    mincharge = GET_OBJ_VAL(obj, 2);

    if (mincharge < maxcharge)
    {
     chargeval = maxcharge - mincharge;
     GET_OBJ_VAL(obj, 2) += chargeval;
     GET_GOLD(ch) -= 10000;
     send_to_char("Grrrr... Hmmmmm... Belch... BING!\r\n",ch);
     sprintf(buf, "The item now has %d charges remaining.\r\n", maxcharge);
     send_to_char(buf, ch);
     act("The machine hums and churns as it recharges the item.",
          FALSE, ch, obj, 0, TO_ROOM);
     }
   else
     {
     send_to_char("The item does not need recharging.\r\n", ch);
     act("The machine hums, churns, and then goes quiet.",
         FALSE, ch, obj, 0, TO_ROOM);
     }
    return 1;
  }
  return 0;
}

SPECIAL(pop_dispenser)
{
 struct obj_data *obj = me, *drink;
 int give_coke = 21032; /* Vnum of the can of coke */
 if (CMD_IS("list"))
     {
     send_to_char("To buy a coke, type 'buy coke'.\r\n", ch);
     return (TRUE);
     }
 else if (CMD_IS("buy")) {
          if (GET_GOLD(ch) < 25) {
          send_to_char("You don't have enough gold!\r\n", ch);
          return (TRUE);
          } else {
            drink = read_object(give_coke, VIRTUAL); 
            obj_to_char(drink, ch);
            send_to_char("You insert your money into the machine\r\n",ch);
            GET_GOLD(ch) -= 25; /* coke costs 25 gold */
            act("$n gets a pop can from $p.", FALSE, ch, obj, 0, TO_ROOM);
            send_to_char("You get a pop can from the machine.\r\n",ch);
          }
          return 1;
 }
 return 0;
}


SPECIAL(newbie_guide)
{
 

 static char tour_path[] =
 "WAAA2E3J1230D22G032K011110I22M033212L030014R530A00001HBC32222Z."; 
/* The above is the path that the guide takes. The numbers are the   */
/* Directions in which he moves                                      */

/* If you find your guide is getting lost, it's probably because you */
/* don't have the newbie area included in stock bpl11.  To fix it,   */
/* use this path instead:                                            */
/*
static char tour_path[] =
 "WAAA2E3J1230D22G032K0115S4110I22M033212L030014R530Z."; 
*/

static char *path; 
static int index; 
static bool move = FALSE;

  if (!move) {
       if (time_info.hours == 1) { /* Tour starts at 1 am*/
       move = TRUE;
       path = tour_path;
       index = 0;
     } else if (time_info.hours == 12) { /* And at 12 pm */
       move = TRUE;
       path = tour_path;
       index = 0;
}
  }
  if (cmd || !move || (GET_POS(ch) < POS_RESTING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return FALSE;

  switch (path[index]) {
  case '0': 
  case '1': 
  case '2': 
  case '3': 
  case '4':
  case '5':
    perform_move(ch, path[index] - '0', 1); 
    break;
  case 'W':
    GET_POS(ch)=POS_STANDING;
    act("$n stands up and announces 'The tour is going to start soon!'", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 'Z':
    act("$n sits and rests for his next journey.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch)=POS_RESTING;
    break;
  case 'M': 
    act("$n says 'This is the enterence to the WARRIORS guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
   case 'L': 
    act("$n says 'This is the enterence to the THEVES guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'K': 
    act("$n says 'This is the enterence to the MAGES guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
    case 'J': 
    act("$n says 'This is the enterence to the CLERICS guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'H': 
    act("$n says 'Right now, you may find it usefull to type 'WEAR ALL''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
    case 'A': 
    act("$n says 'Newbies!! Type 'FOLLOW GUIDE' for a guided tour'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'B': 
    act("$n says 'This here is the enterence to the newbie area. Please, type 'FOLLOW SELF''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'C': 
    act("$n says 'Now have fun out there, and be careful!'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'D': 
    act("$n says 'This is our dear friend the baker, to buy bread from him, type 'BUY BREAD''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'E': 
    act("$n says 'This is the Fountain, to drink from it, type 'DRINK FOUNTAIN''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'F': 
    act("$n says 'This is our dear friend Wally, he will sell you water, type 'LIST' to see a list of what he has.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'G': 
    act("$n says 'This is the Armorer, he makes armor, type LIST to see what he has to sell'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'S': 
    act("$n says 'This is the Midgaard Transporter System station. You can use it to travel " 
         "around the MUD. To buy token type BUY TOKEN. Then leave east. To get off go west " 
         "when the door opens.'", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case '.':
    move = FALSE;
    break;
  case 'R':
    act("$n says 'This is the RECEPTION, in this MUD, you must RENT.'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'To see how much your rent will cost, type 'OFFER'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'To rent, type RENT.'", FALSE, ch, 0, 0, TO_ROOM);
    break;
}
index++;
return FALSE;
}

SPECIAL(engraver)
{
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;

  if (IS_NPC(ch)) {
    send_to_char("Monsters cannot use engraver. Go away!\r\n", ch);
    return TRUE;
  }

  if (CMD_IS("list")) {
    send_to_char("Engraving an item makes that item permanently yours.\r\n", ch);
    send_to_char("It costs 100000 coins to engrave an item.\r\n", ch);
    send_to_char("Of course, you can unengrave an item, but that costs five times more.\r\n", ch);
    send_to_char("To engrave use &WENGRAVE <item>&w and to unengrave use &WUNENGRAVE <item>&w.\r\n", ch);
    return (TRUE);
  } else if (CMD_IS("engrave")) {

    argument = one_argument(argument, buf);

    if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
      send_to_char("You don't have that!\r\n", ch);
      return (TRUE);
    }
    if (IS_OBJ_STAT(obj, ITEM_ENGRAVED)) {
      send_to_char("That item's already engraved!\r\n", ch);
      return (TRUE);
    }
    if (IS_OBJ_STAT(obj, ITEM_AUTOENGRAVE)) {
      send_to_char("You can't engrave that!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < 100000) {
      send_to_char("You don't have enough coins!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= 100000;

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ENGRAVED);
    GET_OBJ_OWNER_ID(obj) = GET_IDNUM(ch);
    send_to_char("There you go, enjoy!!\r\n", ch);
    act("$n engraves $p.", FALSE, ch, obj, 0, TO_ROOM);
    return TRUE;
  } else if (CMD_IS("unengrave")) {

    argument = one_argument(argument, buf);

    if (!(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
      send_to_char("You don't have that!\r\n", ch);
      return (TRUE);
    }
    if (!IS_OBJ_STAT(obj, ITEM_ENGRAVED)) {
      send_to_char("That item isn't engraved!\r\n", ch);
      return (TRUE);
    }
    if (IS_OBJ_STAT(obj, ITEM_AUTOENGRAVE)) {
      send_to_char("You can't unengrave that!\r\n", ch);
      return (TRUE);
    }
    if (GET_OBJ_OWNER_ID(obj) != GET_IDNUM(ch)) {
      send_to_char("Excuse me, but that item isn't yours to unengrave!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < 500000) {
      send_to_char("You don't have enough coins!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= 500000;

    REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_ENGRAVED);
    GET_OBJ_OWNER_ID(obj) = 0;
    send_to_char("There you go, it's unengraved now!!\r\n", ch);
    act("$n unengraves $p.", FALSE, ch, obj, 0, TO_ROOM);
    return TRUE;
  }

  /* All commands except list and engrave and unengrave */
  return 0;
}

SPECIAL(fighter)
{ 
  struct char_data *vict = NULL;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && vict != ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

/*
  switch (number(0, MAX(4, 8 - GET_LEVEL(ch) / 10))) {
    case 1:
      do_kick(ch, vict->player.name, 0, 0);
      break;
    case 2:
      do_bash(ch, vict->player.name, 0, 0);
      break;
    case 3:
      do_disarm(ch, vict->player.name, 0, 0);
      break;
    default:
      break;
  }
  */
  return TRUE;

}

/*
Wand of Wonder: The wand of wonder is a strange and unpredictable device
that will generate any number of strange effects, randomly, each time it
is used. The usual effects are shown on the table below, but you may alter
these for any or all of these wands in your campaign as you see fit.
Possible of the wand include:

D100
Roll            Effect
01-10           Slow creature pointed at for one turn
11-18           Deludes wielder for one round into believing the wand
		functions as indicated by a second die roll
19-25           Gust of wind, double force of spell
26-30           Stinking cloud at 30-foot  range
31-33           Heavy rain falls for one round in 60-foot radius of wand
		wielder
34-36           Summon rhino (1-25), elephant (26-50), or mouse (51-00)
37-46           Lightning bolt (70' x 5') as wand
47-49           Stream of 600 large butterflies pour forth and flutter
		around for two rounds, blinding everyone (including
		wielder)
50-53           Enlarge target if within 60 feet of wand
54-58           Darkness in a 30-foot diameter hemisphere at 30 feet
		center distance from wand
59-62           Grass grows in area of 160 square feet before the wand, or
		grass existing there grows to 10 times normal size
63-65           Vanish any nonliving object of up to 1,000 pounds mass and
		up to 30 cubic feet in size (object is ethereal)
66-69           Diminish wand wielder to 1/12 height
70-79           Fireball as wand
80-84           Invisibility covers wand wielder
85-87           Leaves grow from target if within 60 feet of wand
88-90           10-40 gems of 1 gp base value shoot forth in a
		30-foot-long stream, each causing one point of damage to
		any creature in path -- roll 5d4 for number of hits
91-97           Shimmering colors dance and play over a 40-by 30-foot area
		in front of wandùcreatures therein blinded for 1d6 rounds
98-00           Flesh to stone (or reverse if target is stone) if target
		is within range

*/
SPECIAL(wand_of_wonder)
{ 
  struct descriptor_data *d;
  struct obj_data *wand = (struct obj_data *) me, *sobj;
  int i, k;
  struct char_data *tch = NULL, *next_tch, *mob;
  struct obj_data *tobj = NULL;

  void die(struct char_data * ch);
  void death_cry(struct char_data * ch);

/* this part emulates the do_use bit for a reason */

  if(!CMD_IS("use") || !(wand->worn_by == ch) ||
        GET_EQ(ch,WEAR_HOLD) != wand) {  /*overkill i guess*/
    return FALSE;
  }

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return TRUE;
  }

  if (!isname(arg, wand->name)) {
      sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg),arg);
      send_to_char(buf2, ch);
      return TRUE;
  }
  /* we have the wand, now we need a target */
  one_argument(buf, arg);

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
                   FIND_OBJ_EQUIP, ch, &tch, &tobj);

  if (k == FIND_CHAR_ROOM) {
      if (tch == ch) {
        act("You point $p at yourself.", FALSE, ch, wand, 0, TO_CHAR);
        act("$n points $p at $mself.", FALSE, ch, wand, 0, TO_ROOM);
      } else {
        act("You point $p at $N.", FALSE, ch, wand, tch, TO_CHAR);
        act("$n points $p at $N.", TRUE, ch, wand, tch, TO_ROOM);
      }
  } else if (tobj != NULL) {
        act("You point $p at  $P.", FALSE, ch, wand, tobj, TO_CHAR);
        act("$n points $p at $P.", TRUE, ch, wand, tobj, TO_ROOM);
  } else {
    act("At what should $p be pointed?", FALSE, ch, wand, NULL, TO_CHAR);
    return TRUE;
  }

 /* now, we'll make life simple on us, at least for WoW I...
        assume that there are no object focused spells */

  if(tobj) {
    send_to_char("That seems to have no effect!\r\n",ch);
    return TRUE;
  }
  if (GET_OBJ_VAL(wand, 2) <= 0) {
    act("It seems powerless.", FALSE, ch, wand, 0, TO_CHAR);
    act("Nothing seems to happen.", FALSE, ch, wand, 0, TO_ROOM);
    return TRUE;
  }
  GET_OBJ_VAL(wand, 2)--;
  WAIT_STATE(ch, PULSE_VIOLENCE);

  i=number(0,100);
  if(i <= 10) {  /* slow target for a few rounds */
    act("A numbing blue glow encases $N - slowing him down!", FALSE, ch,
                0, tch, TO_ROOM);
    act("A numbing blue glow encases you - slowing you down!", FALSE, ch,
                0, tch, TO_VICT);
    WAIT_STATE(tch,PULSE_VIOLENCE*10); /* less you have a slow spell */
  } else if (i >= 11 && i <= 18) {    /* delude caster?..ouch */
    k=number(1,19);
    switch(k) {
      case 1:
        act("A numbing blue glow encases $N - slowing him down!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 2:
        act("$N turns into a large grey elephant!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 3:
        act("A fierce gust of wind hits $N!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 4:
        act("Your wand releases a poisonous stinking cloud!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 5:
        act("The heavens release a torrent of rain, blurring your visibility, and making movement difficult!"          , FALSE, ch, 0 , tch, TO_CHAR);
        break;
      case 6:
        k=number(1,3);
        switch(k) {
          case 1:
             send_to_char("You have summoned a rhino!\r\n",ch);
             break;
          case 2:
             send_to_char("You have summoned an elephant!\r\n",ch);
             break;
          case 3:
             send_to_char("You have summoned a dangerous mouse!\r\n",ch);
             break;
 
         }
        break;
      case 7:
        act("$N is hit by three consecutive lighting bolts!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 8:
        act("A stream of 600 large butterflies pours forth from the $p -Blinding everyone in the room!",FALSE,ch,wand,tch,TO_CHAR);
        break;
      case 9:
        act("$N grows to double normal size!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 10:
             act("The entire area becomes dark!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 11:
             act("The plants and grasses in the area surge forth in unparalled growth!",FALSE,ch,0,tch,TO_CHAR);
        break;
      case 12:
             act("$N fades from sight!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 13:
        act("You shrink to 1/12'th your height!!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 14:
        act("Your fireball hits $N squarely in the chest!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 15:
        act("You turn invisible!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 16:
        act("$N begins to sprout leaves from his body!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 17:
        act("A stream of gemstones is expelled from the $p, striking all those in its path!",
                 FALSE, ch, wand, tch, TO_CHAR);
        break;
      case 18:
        act("Shimmering colors dance and play in the area - blinding all in the area!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
      case 19:
        act("$n is turned to stone!!!", FALSE,
                ch, 0, tch, TO_CHAR);
        break;
    }
    return TRUE;
  } else if (i >= 19 && i <= 25) { /* gust of wind */
        act("A fierce gust of wind hits $N!", FALSE,
                ch, 0, tch, TO_ROOM);
        act("A fierce gust of wind hits you!", FALSE,
                ch, 0, tch, TO_VICT);

      k = dice(5, 8) + 5;
   /* i'd use damage, but ... i don't want any more messages */
      if(GET_HIT(tch) > k) {
        GET_HIT(tch) = GET_HIT(tch) - k;
      } else {
        die(tch);
      }
      return TRUE;
  } else if (i >=26 && i <= 30) { /* stinking cloud. ish. */
    act("$n's $p releases a poisonous stinking cloud!", FALSE,
                ch, wand, tch, TO_ROOM);
    act("Your $p releases a poisonous stinking cloud!", FALSE,
                ch, wand, tch, TO_CHAR);
    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;
      call_magic(ch, tch, 0, 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
    }
    return TRUE;
  } else if (i >= 31 && i <=33) {
    act("The heavens release a torrent of rain, blurring your visibility, and making movement difficult!"
          , FALSE, ch, 0 , tch, TO_ROOM);
    act("The heavens release a torrent of rain, blurring your visibility, and making movement difficult!"
          , FALSE, ch, 0 , tch, TO_CHAR);

    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;
      call_magic(ch, tch, 0, 0, SPELL_BLINDNESS, 3, CAST_SPELL);
      GET_MOVE(tch) = GET_MOVE(tch)/2;
    }
    return TRUE;

  } else if (i >=34 && i <=36) { /* conjure a mob */

   /* 2 choices... load an existing mob, or make a new one */
   /* rough, but i'll make a puff mob, but alter stats, easier */
   /* so, I guess, PUFF must exist. (vn 1) */
    k=number(1,4);

    mob = read_mobile(1, VIRTUAL);
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
    add_follower(mob, ch);
    free(GET_NAME(mob));
    free(mob->player.long_descr);
    free(mob->player.description);
    free(mob->player.short_descr);

    if(k==1) { /* rhino */
     /*
      * GET_NAME(mob)=str_dup("rhino");
      * since it's an NPC? we'll just assign it rather than do an invalid left value assignment 
      */
      mob->player.short_descr=str_dup("rhino");
      mob->player.short_descr=str_dup("rhino");
      mob->player.long_descr=str_dup("A wild looking rhino");
      mob->player.description=str_dup("The beast looks quite dangerous....");
      GET_LEVEL(mob)=20;
      GET_MAX_HIT(mob)=350;
      GET_HIT(mob)=350;
      GET_MAX_MOVE(mob)=350;
      GET_MOVE(mob)=350;
      GET_AC(mob)=0;
      mob->mob_specials.damnodice=4;
      mob->mob_specials.damsizedice=10;
    } else if (k ==2) {
      /*
      * GET_NAME(mob)=str_dup("elephant");
      * since it's an NPC? we'll just assign it rather than do an invalid left value assignment
      */
      mob->player.short_descr=str_dup("elephant");
      mob->player.long_descr=str_dup("A fierce bull elephant");
      mob->player.description=str_dup("This raging bull elephant looks really pissed off!");
      GET_LEVEL(mob)=15;
      GET_MAX_HIT(mob)=650;
      GET_HIT(mob)=650;
      GET_MAX_MOVE(mob)=350;
      GET_MOVE(mob)=350;
      GET_AC(mob)=20;
      mob->mob_specials.damnodice=2;
      mob->mob_specials.damsizedice=15;
    } else {
      // needed? GET_NAME(mob)=str_dup("mouse");
      mob->player.short_descr=str_dup("mouse");
      mob->player.long_descr=str_dup("A tiny squeaking mouse");
      mob->player.description=str_dup("The mouse looks more interested in running than fighting!");
      GET_LEVEL(mob)=2;
      GET_MAX_HIT(mob)=20;
      GET_HIT(mob)=20;
      GET_MAX_MOVE(mob)=90;
      GET_MOVE(mob)=90;
      GET_AC(mob)=100;
      mob->mob_specials.damnodice=2;
      mob->mob_specials.damsizedice=1;
    }
    char_to_room(mob,ch->in_room);
    return TRUE;
  } else if (i >= 37 && i <= 46) {
    act("$N is hit by three consecutive lighting bolts!", FALSE,
                ch, 0, tch, TO_ROOM);
    act("You've been hit by three consecutive lighting bolts!",
                FALSE,ch,0,tch,TO_VICT);
    call_magic(ch, tch, 0, 0, SPELL_LIGHTNING_BOLT, 30, CAST_SPELL);
    if(tch && GET_HIT(tch) > 0)
      call_magic(ch, tch, 0, 0, SPELL_LIGHTNING_BOLT, 30, CAST_SPELL);
    if(tch && GET_HIT(tch) > 0)
      call_magic(ch, tch, 0, 0, SPELL_LIGHTNING_BOLT, 30, CAST_SPELL);
    return TRUE;

  } else if(i >= 47 && i <= 49) {
    act("A stream of 600 large butterflies pours forth from the $p - blinding everyone in the room!",FALSE,ch,wand,tch,TO_ROOM);
    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;
      call_magic(ch, tch, 0, 0, SPELL_BLINDNESS, 3, CAST_SPELL);
    }
    return TRUE;

  } else if (i >= 50 && i <= 53) {
    act("$N suddenly doubles in size!", FALSE,
                ch, 0, tch, TO_ROOM);
    act("You suddenly double in size!",
                FALSE,ch,0,tch,TO_VICT);
    call_magic(ch, tch, 0, 0, SPELL_STRENGTH, 20, CAST_SPELL);
    call_magic(ch, tch, 0, 0, SPELL_ARMOR, 20, CAST_SPELL);
    return TRUE;

  } else if (i >= 54 && i <= 58) {
    act("All your lights suddenly go out!", FALSE,ch,0,tch,TO_ROOM);
    world[ch->in_room].light = 0;
    if (!ROOM_FLAGGED(ch->in_room, ROOM_DARK)) {
      SET_BIT(ROOM_FLAGS(ch->in_room), ROOM_DARK);
    }
    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;
      if (GET_EQ(ch, WEAR_LIGHT) != NULL)
        if (GET_OBJ_TYPE(GET_EQ(tch, WEAR_LIGHT)) == ITEM_LIGHT)
          GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) = 0;
    }
    return TRUE;

  } else if (i >= 59 && i <= 62) {
    act("All the vegetation in the area suddenly doubles - no quadruples in size!", FALSE,ch,0,tch,TO_ROOM);
    world[ch->in_room].sector_type=SECT_FOREST;
    return TRUE;
  } else if (i >= 63 && i <= 65) {
    act("$N fades from sight!", FALSE,
                ch, 0, tch, TO_ROOM);
    act("You fade from sight!", FALSE,
                ch, 0, tch, TO_VICT);
    call_magic(ch, tch, 0, 0, SPELL_INVISIBLE, 20, CAST_SPELL);
    return TRUE;
  } else if (i >= 66 && i <= 69) {
    act("$n suddenly shrinks to 1/12 of his normal size!", FALSE,
        ch, 0, tch, TO_ROOM);
    act("You suddenly shrink to 1/12 of your normal size!", FALSE,
        ch, 0, tch, TO_CHAR);
    call_magic(ch, ch, 0, 0, SPELL_CHILL_TOUCH, 25, CAST_SPELL);
    return TRUE;
  } else if (i >= 70 && i <= 79) {
    act("$N is engulfed in a nasty fireball!", FALSE, ch, 0, tch, TO_ROOM);
    act("You are engulfed in a nasty fireball!", FALSE, ch, 0 , tch, TO_VICT);
    call_magic(ch, tch, 0 , 0, SPELL_FIREBALL, 30, CAST_SPELL);
    return TRUE;
  } else if (i >= 80 && i <= 84) {
    act("You turn invisible!", FALSE,
                ch, 0, tch, TO_CHAR);
    act("$n turns invisibile!",FALSE,ch,0,tch,TO_ROOM);
    call_magic(ch, ch, 0, 0, SPELL_INVISIBLE,10,CAST_SPELL);
    return TRUE;
  } else if (i >= 85 && i <= 87) {
    act("How odd - $N starts to sprout leaves and thorny bristles!", FALSE, ch, 0, tch, TO_ROOM);
    act("You start to sprout leaves and thorny bristles!", FALSE, ch,0,
        tch, TO_VICT);
    call_magic(ch,tch,0, 0, SPELL_ARMOR,10,CAST_SPELL);
    return TRUE;
  } else if (i >= 88 && i <= 90) {
    act("A spray of well cut gems sprays forth from the $p, striking all in the room!",
         FALSE, ch, 0 , tch, TO_ROOM);
    for (tch = world[ch->in_room].people; tch; tch = next_tch) {
      next_tch = tch->next_in_room;
      if(tch != ch) {
        GET_HIT(tch) = GET_HIT(tch) - number(0,40);
      }
      if(GET_HIT(tch) <= 0) {
        die(tch);
      }
    }
    for(k=0;k!=40;k++) {
      sobj=create_obj();

      sobj->item_number = NOTHING;
      sobj->in_room = NOWHERE;
      sobj->name = str_dup("gem");
      sobj->description = str_dup("a glittering gem is lying here");
      sobj->short_description = str_dup("a glittering gem");
      GET_OBJ_TYPE(sobj) = ITEM_TREASURE;
      GET_OBJ_WEAR(sobj) = ITEM_WEAR_TAKE & ITEM_WEAR_HOLD;
      GET_OBJ_COST(sobj) = 1 + number(0,10);
      GET_OBJ_WEIGHT(sobj) = 0;
      obj_to_room(sobj,ch->in_room);
    }
    return TRUE;
  } else if (i >= 91 && i <= 97) {
    act("An amazing field of shimmering colors plays over the entire area!", FALSE,
     ch, 0, tch, TO_ROOM);
    /* lets blind EVERYONE in the zone cept for user. I'm feeling pissy.*/
    for (d = descriptor_list; d; d = d->next) {
      if(!IS_NPC(d->character) && (world[d->character->in_room].zone ==
           world[ch->in_room].zone) && (d->character != ch)) {
        send_to_char("A bright flash of color blinds you!\r\n",d->character);
        call_magic(ch, d->character, 0, 0, SPELL_BLINDNESS, 10, CAST_SPELL);
      }
    }
    return TRUE;
  } else {
    act("$N has been turned to STONE!",FALSE,ch,0,tch,TO_ROOM);
    act("You have been turned to stone! What suckage!", FALSE, ch,0,tch,
                TO_VICT);
    for (k = 0; k < NUM_WEARS; k++) {
      if (GET_EQ(ch, k)) {
        unequip_char(tch, k);
      }
    }

    /* delete all items he has */
    for(sobj=tch->carrying;sobj;sobj=sobj->next_content) {
      extract_obj(sobj);
    }

    /* kill the sorry bastard now */
    if (!IS_NPC(tch))
      REMOVE_BIT(PLR_FLAGS(tch), PLR_KILLER | PLR_THIEF);
    if (FIGHTING(tch))
      stop_fighting(tch);
    while (tch->affected)
      affect_remove(tch, tch->affected);
    death_cry(tch);

    /* build a monmument to the dead, pathetic slob */

    sobj=create_obj();
    sobj->item_number = NOTHING;
    sobj->in_room = NOWHERE;
    sobj->name = str_dup("stone");

    sprintf(buf2, "The statue of %s stands here.", GET_NAME(tch));
    sobj->description = str_dup(buf2);

    sprintf(buf2, "the statue of %s", GET_NAME(tch));
    sobj->short_description = str_dup(buf2);
    GET_OBJ_TYPE(sobj)= ITEM_TRASH;
    GET_OBJ_WEAR(sobj)= ITEM_WEAR_TAKE;
    GET_OBJ_EXTRA(sobj)= ITEM_NODONATE;
    GET_OBJ_WEIGHT(sobj) = GET_WEIGHT(tch) + IS_CARRYING_W(tch) + 300;

    /* thats good for now */
    obj_to_room(sobj, tch->in_room);

    extract_char(tch);
    return TRUE;
    }
  return TRUE;
}

 
  
