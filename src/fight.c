/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "clan.h"
//#include "class.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct index_data *mob_index;
extern struct str_app_type str_app[];
extern struct dex_app_type dex_app[];
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct index_data *mob_index;
extern int top_of_zone_table;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern int auto_save;		/* see config.c -- not used in this file */
extern int max_exp_gain;	/* see config.c */
extern int max_exp_loss;	/* see config.c */
extern int top_of_world;
extern int max_npc_corpse_time, max_pc_corpse_time;
extern struct clan_info *clan_index;
extern int cross_reference[9999];

/* Daniel Houghton's revision */
extern char *dirs[];
extern int rev_dir[];
extern struct index_data *obj_index;
extern struct room_data *world;


/* External procedures */
char *fread_action(FILE * fl, int nr);
ACMD(do_flee);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * 
victim);
void mprog_hitprcnt_trigger(struct char_data * mob, struct char_data * ch);
void mprog_death_trigger(struct char_data * mob, struct char_data * killer);
void mprog_fight_trigger(struct char_data * mob, struct char_data * ch);
int backstab_mult(int level);
int thaco(int ch_class, int level);


/* local functions */
void perform_group_gain(struct char_data * ch, int base, struct char_data * victim);
void dam_message(int dam, struct char_data * ch, struct char_data * victim, int w_type);
void appear(struct char_data * ch);
void load_messages(void);
void check_killer(struct char_data * ch, struct char_data * vict);
void make_corpse(struct char_data * ch);
void change_alignment(struct char_data * ch, struct char_data * victim);
void death_cry(struct char_data * ch);
void raw_kill(struct char_data * ch, struct char_data * killer);
void die(struct char_data * ch, struct char_data * killer);
void group_gain(struct char_data * ch, struct char_data * victim);
void solo_gain(struct char_data * ch, struct char_data * victim);
//void do_decapitate(struct char_data *ch, struct char_data *killer);
//void check_increment_pkcounter(struct char_data* killer_ch, struct 
//char_data* killed_ch)
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
//void gain_demonxp(struct char_data * ch, int gain)

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))
#define GET_KILL_EXP(ch)	(IS_NPC(ch) ? GET_EXP(ch) /3 : GET_EXP(ch) / 5)

/* The Fight related routines */

void check_increment_pkcounter(struct char_data* killer_ch, struct 
char_data* killed_ch)
{
  //int ch = (killer_ch && !IS_NPC(killer_ch)) ? 1 : 0;
  //int vict = (killed_ch && !IS_NPC(killed_ch) && (killed_ch != 
//killer_ch)) ? ch : !ch;

//  if (ch == vict) {
  //    GET_PLAYER_KILLS(killer_ch)+=1;
  //}
  //if (ch == vict) {
    // GET_PLAYER_KILLS(killed_ch) =1;
//}
}


void improve_skill(struct char_data *ch, int skill)
{
  extern char *spells[];
  int percent = !IS_NPC(ch) ? GET_SKILL(ch, skill) : 0;
  int newpercent;
  char skillbuf[MAX_STRING_LENGTH];

  if (IS_NPC(ch)) return;
  if (number(1, 200) > GET_WIS(ch) + GET_INT(ch))
     return;
  if (percent >= 97 || percent <= 0)
     return;
  newpercent = number(1, 3);
  percent += newpercent;
  SET_SKILL(ch, skill, percent);
  if (newpercent >= 1) {
     if (IS_SKILL(skill))
       sprintf(skillbuf, "&MYou feel your skill in &c%s&M improving.&w\r\n", spells[skill]);
     else
       sprintf(skillbuf, "&MYou relized your ability to cast &c%s&M has improved.&w\r\n", spells[skill]);
     send_to_char(skillbuf, ch);
  }
}

void make_part(struct char_data *ch, char *argument)
{
    struct obj_data *obj;
    int vnum;
  
    one_argument(argument, arg);
    vnum = 0;
    
    if (arg[0] == '\0') return;
    if (!str_cmp(arg,"head")) vnum = OBJ_VNUM_SEVERED_HEAD;
    else if (!str_cmp(arg,"larm")) vnum = OBJ_VNUM_SLICED_LARM;
    else if (!str_cmp(arg,"rarm")) vnum = OBJ_VNUM_SLICED_RARM;
    else if (!str_cmp(arg,"lleg")) vnum = OBJ_VNUM_SLICED_LLEG;
    else if (!str_cmp(arg,"rleg")) vnum = OBJ_VNUM_SLICED_RLEG;
    else if (!str_cmp(arg,"heart")) vnum = OBJ_VNUM_TORN_HEART;
    else if (!str_cmp(arg,"turd")) vnum = OBJ_VNUM_TORN_HEART;
    else if (!str_cmp(arg,"entrails")) vnum = OBJ_VNUM_SPILLED_ENTRAILS;
    else if (!str_cmp(arg,"brain")) vnum = OBJ_VNUM_QUIVERING_BRAIN;
    else if (!str_cmp(arg,"eyeball")) vnum = OBJ_VNUM_SQUIDGY_EYEBALL;
    else if (!str_cmp(arg,"blood")) vnum = OBJ_VNUM_SPILT_BLOOD;
    else if (!str_cmp(arg,"face")) vnum = OBJ_VNUM_RIPPED_FACE;
    else if (!str_cmp(arg,"windpipe")) vnum = OBJ_VNUM_TORN_WINDPIPE;
    else if (!str_cmp(arg,"cracked_head")) vnum = OBJ_VNUM_CRACKED_HEAD;
    else if (!str_cmp(arg,"ear")) vnum = OBJ_VNUM_SLICED_EAR;
    else if (!str_cmp(arg,"nose")) vnum = OBJ_VNUM_SLICED_NOSE;
    else if (!str_cmp(arg,"tooth")) vnum = OBJ_VNUM_KNOCKED_TOOTH;
    else if (!str_cmp(arg,"tongue")) vnum = OBJ_VNUM_TORN_TONGUE;
    else if (!str_cmp(arg,"hand")) vnum = OBJ_VNUM_SEVERED_HAND;
    else if (!str_cmp(arg,"foot")) vnum = OBJ_VNUM_SEVERED_FOOT;
    else if (!str_cmp(arg,"thumb")) vnum = OBJ_VNUM_SEVERED_THUMB;
    else if (!str_cmp(arg,"index")) vnum = OBJ_VNUM_SEVERED_INDEX;
    else if (!str_cmp(arg,"middle")) vnum = OBJ_VNUM_SEVERED_MIDDLE;
    else if (!str_cmp(arg,"ring")) vnum = OBJ_VNUM_SEVERED_RING;
    else if (!str_cmp(arg,"little")) vnum = OBJ_VNUM_SEVERED_LITTLE;
    else if (!str_cmp(arg,"toe")) vnum = OBJ_VNUM_SEVERED_TOE;

    if ( vnum != 0 )
    {
	struct obj_data *obj;
	char *name;

	name		= IS_NPC(ch) ? obj->short_description : GET_NAME(ch);
	obj		= create_obj();
        GET_OBJ_TIMER(obj) = 5;}
        obj = read_object(vnum, VIRTUAL);

	if (vnum == OBJ_VNUM_SPILT_BLOOD) GET_OBJ_TIMER(obj) = 5;

	if (!IS_NPC(ch))
	{
	    sprintf( buf, obj->name, GET_NAME(ch) );
	    obj->name = str_dup( buf );
	}
	else
	{
	    sprintf( buf, obj->name, "mob" );
	    obj->name = str_dup( buf );
	}

	sprintf( buf, obj->short_description, GET_NAME(ch) );
	obj->short_description = str_dup( buf );

	sprintf( buf, obj->description, GET_NAME(ch) );
	obj->description = str_dup( buf );
        obj_to_room(obj, ch->in_room );

    return;
}
void appear(struct char_data * ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    sprintf(buf2, "SYSERR: Error reading combat message file %s", MESS_FILE);
    perror(buf2);
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{
  //struct char_data *ch;
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -2000)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -10)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -8)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;

}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF)
      && !PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(ch) && !IS_NPC(vict) &&
      (ch != vict)) {
    char buf[256];

    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
    sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    mudlog(buf, BRF, LVL_IMMORT, TRUE);
    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
  }
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!pk_allowed)
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}



void make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  //struct char_data *killer;
  int i;
  int room;

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_GHOST)){
  
  /* *** GHOST ***/
  room = ch->in_room;
  for (o = ch->carrying; o; o = o->next_content)
    obj_to_room(o, room);

  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_room(unequip_char(ch, i), room);

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_GOLD(ch));
      obj_to_room(money, room);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  return;
  } /* *** NOT GHOST *** */
  else {
  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;
  corpse->name = str_dup("corpse");

  sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = str_dup(buf2);

  sprintf(buf2, "the corpse of %s", GET_NAME(ch));
  corpse->short_description = str_dup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 1) = 0;
  GET_OBJ_VAL(corpse, 2) = MAX(1, GET_MAX_HIT(ch));   /* original hitpoints */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;
  // GET_OBJ_VAL(corpse, 1) = GET_OBJ_TIMER(corpse);

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_obj(unequip_char(ch, i), corpse);

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, ch->in_room);
  return;
  }
}


/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(struct char_data * ch)
{
  int door, was_in;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (CAN_GO(ch, door)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("Your blood freezes as you hear someone's death cry.", FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}



void raw_kill(struct char_data * ch, struct char_data * killer)
{
  // struct char_data *victim;

   if (FIGHTING(ch))
    stop_fighting(ch);
  while (ch->affected) {
    if ((ch->affected->type = SPELL_CORPSE_HOST) && ch->desc && ch->desc->original) {
      affect_remove(ch, ch->affected);
      return;
    }
    affect_remove(ch, ch->affected);
  }
/*
  if (!IS_NPC(victim) && GET_MAX_HIT(victim) <=-11 && GET_PKILLP == 2) {
     GET_MAX_HIT(victim) = -10;
     stop_fighting(ch);
     return;
}
  */   
/*  death_cry(ch); */
  if (killer)
    mprog_death_trigger(ch, killer);

  if (!IS_NPC(ch) && GET_LEVEL(ch) <= MAX_NEWBIE_LEVEL)
    SET_BIT(PLR_FLAGS(ch), PLR_NEWBIE);

  
  if (IS_NPC(ch)) {
  make_corpse(ch);
  extract_char(ch);
      } else {
        send_to_char("You feel yourself being pulled into a bright light...\r\n", ch);
        char_from_room(ch);
        char_to_room(ch, real_room(number(SPIRIT_MIN, SPIRIT_MIN)));
        GET_HIT(ch) = GET_MAX_HIT(ch);
        GET_MANA(ch) = GET_MAX_MANA(ch);
        GET_MOVE(ch) = GET_MAX_MOVE(ch);
        //GET_PLAYER_KILLS(ch) = 1;
        update_pos(ch);
        look_at_room(ch, 0);
        if (!PLR_FLAGGED(ch, PLR_DEAD)){
          GET_TEMP_GOLD(ch) = GET_GOLD(ch);
          GET_GOLD(ch) = 0;
          SET_BIT(PLR_FLAGS(ch), PLR_DEAD);
         send_to_char("\r\n\r\n\r\n\r\n\r\nSEE: HELP RESURECT  - FOR DETAILS ON HOW TO RESURECT FROM THE DEAD\r\n",ch); 
       }
      }
  
 return;
}

int get_mastered_zone(int mob_num) 
{
  int i;
  for (i=0; i<=top_of_zone_table; i++)
    if (ZON_MASTER(&zone_table[i]) == mob_num) return i;
  return -1;
}

void die(struct char_data * ch, struct char_data * killer)
{
  int zn;      

  gain_demonxp(ch, -(GET_DEMONXP(ch) / 2 ));  
  gain_exp(ch, -(GET_EXP(ch) / 2));
  
//   if (!IS_NPC(ch))
//    REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  if (IS_NPC(ch) && killer && !IS_NPC(killer) && PLAYERCLAN(killer) != 0) {
    if ((zn = get_mastered_zone(GET_MOB_VNUM(ch))) != -1)
      ZON_OWNER(&zone_table[zn]) = PLAYERCLAN(killer);
  }
  raw_kill(ch, killer);
  return;
}



void perform_group_gain(struct char_data * ch, int base,
			     struct char_data * victim)
{
  int share;

  share = MIN(max_exp_gain, MAX(1, base));

  if (share > 1) {
    sprintf(buf2, "&WYou receive your share of experience -- &C%d&W points.&w\r\n", share);
    send_to_char(buf2, ch);
  } else
    send_to_char("&WYou receive your share of experience -- &Cone&W measly little point!&w\r\n", ch);

  gain_exp(ch, share);
  change_alignment(ch, victim);
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base, tot_gain;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      tot_members++;

  /* round up to the next highest tot_members */
  tot_gain = (GET_KILL_EXP(victim)) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(max_exp_loss * 2 / 3, tot_gain);

  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  if (AFF_FLAGGED(k, AFF_GROUP) && k->in_room == ch->in_room)
    perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      perform_group_gain(f->follower, base, victim);
}


void solo_gain(struct char_data * ch, struct char_data * victim)
{
  int exp;
 

  exp = MIN(max_exp_gain, GET_KILL_EXP(victim));

  /* Calculate level-difference bonus */
  if (IS_NPC(ch))
    exp = MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 
8);
  else
    exp = MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 
8);

  exp = MAX(exp, 1);

  if (exp > 1) {
    sprintf(buf2, "&WYou receive &C%d&W experience points.&w\r\n", exp);
    send_to_char(buf2, ch);
  } else
    send_to_char("&WYou receive one lousy experience point.&w\r\n", ch);

  gain_exp(ch, exp);
  gain_demonxp(ch, exp);
   (ch)->points.demonxp *=GET_LEVEL(ch)*2;
  change_alignment(ch, victim);

  if (GET_CLASS(ch) == CLASS_WARRIOR) {
      gain_condition(ch, FULL, +20);
   send_to_char("&rYou devour the soul of your victim.&w\r\n",ch);
   return;
  }
  if (GET_CLASS(ch) == CLASS_THIEF) {
  send_to_char("&bYou drain some research points from your victim.&w\r\n", ch);
gain_demonxp(ch, (GET_DEMONXP(ch) * 2));
    GET_DEMONXP(ch) *=GET_LEVEL(ch) * 2;
    return;
 }
}
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim,
		      int w_type)
{
  //struct char_data *killer;
  char *buf;
  int msgnum;
  //int chance;
   
 // chance = number(1, 50);

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
     },

    {
      "$n tickles $N as $e #W $M.",	/* 1: 1..2  */
      "You tickle $N as you #w $M.",
      "$n tickles you as $e #W you."
    },

    {
      "$n barely #W $N.",		/* 2: 3..4  */
      "You barely #w $N.",
      "$n barely #W you."
    },

    {
      "$n #W $N.",			/* 3: 5..6  */
      "You #w $N.",
      "$n #W you."
    },

    {
      "$n #W $N hard.",			/* 4: 7..10  */
      "You #w $N hard.",
      "$n #W you hard."
    },

    {
      "$n #W $N very hard.",		/* 5: 11..14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },

    {
      "$n #W $N extremely hard.",	/* 6: 15..19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },

    {
      "$n massacres $N to small fragments with $s #w.",	/* 7: 19..23 */
      "You massacre $N to small fragments with your #w.",
      "$n massacres you to small fragments with $s #w."
    },

    {
      "$n OBLITERATES $N with $s deadly #w!!",	/* 8: > 23   */
      "You OBLITERATE $N with your deadly #w!!",
      "$n OBLITERATES you with $s deadly #w!!"
    }
  };


  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else			msgnum = 8;

if (dam == 1 && !PLR2_FLAGGED(victim, PLR2_NOFACE)) { 
act("You ram your fingers into $N's eye sockets and rip $S face off.", FALSE,ch, 0, victim, TO_CHAR);
act("$n rams $s fingers into $N's eye sockets and rips $S face off.", FALSE,ch, 0, victim, TO_NOTVICT);
act("$n rams $s fingers into your eye sockets and rips your face off.",FALSE, ch, 0, victim, TO_VICT);
SET_BIT(PLR2_FLAGS(victim), PLR2_NOFACE);
make_part(victim,"face");
} else
if (dam == 25 && !PLR2_FLAGGED(victim, PLR2_NOLARM)) {
act("You masterfully slice $N's left arm and leave $S limbless.", FALSE, ch, 0,victim, TO_CHAR);
act("$n masterfully slices $N's left arm leaving $S limbless.", FALSE, ch, 0, victim, TO_NOTVICT);
act("$n rams $s fingers into your eye sockets and rips your face off.", FALSE, ch, 0, victim, TO_VICT);
SET_BIT(PLR2_FLAGS(victim), PLR2_NOLARM);
make_part(victim,"larm");
} else
 if ( dam == 100 && !PLR2_FLAGGED(victim, PLR2_NOWPIPE)) {
act("You grab $N by the throat and tear $S windpipe out.", FALSE, ch, 0, victim, TO_CHAR);
act("$n grabs $N by the throat and tears $S windpipe out.", FALSE, ch, 0, victim, TO_NOTVICT);
act("$n grabs you by the throat and tears your windpipe out.",FALSE, ch, 0, victim, TO_VICT);
SET_BIT(PLR2_FLAGS(victim), PLR2_NOWPIPE);
make_part(victim,"windpipe");
}
  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, 
attack_hit_text[w_type].plural);
  strcat(buf, " ");
  sprintf(buf2, "[%d]", dam);
  strcat(buf, buf2);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);
  send_to_char(CCNRM(ch, C_CMP), ch);
  /* damage message to damager */
  send_to_char(CCYEL(ch, C_CMP), ch);
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, 
attack_hit_text[w_type].plural);
  strcat(buf, " ");
  sprintf(buf2, "[%d]", dam);
  strcat(buf, buf2);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(CCNRM(ch, C_CMP), ch);
  
  /* damage message to damagee */
  send_to_char(CCRED(victim, C_CMP), victim);
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, 
attack_hit_text[w_type].plural);
  strcat(buf, " ");
  sprintf(buf2, "[%d]", dam);
  strcat(buf, buf2);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(victim, C_CMP), victim);

  
}


/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data * ch, struct char_data * vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
	if (GET_POS(vict) == POS_DEAD) {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
	send_to_char(CCYEL(ch, C_CMP), ch);
	act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	send_to_char(CCNRM(ch, C_CMP), ch);

	send_to_char(CCRED(vict, C_CMP), vict);
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(vict, C_CMP), vict);

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return 1;
    }
  }
  return 0;
}


/* Increase RIP/KILL counters (players and clans) */
void inc_kill_counters(struct char_data *ch, struct char_data *victim)
{
  if (!IS_NPC(victim))
    GET_RIP_CNT(victim) += 1;
  if (!IS_NPC(ch))
    GET_KILL_CNT(ch) += 1;
    
  if (!IS_NPC(ch) && !IS_NPC(victim) && (PLAYERCLAN(ch) > 0) &&
    (PLAYERCLAN(victim) > 0)) {
    if (PLAYERCLAN(ch) == PLAYERCLAN(victim)) {
      log("SYSERR: Friends killing one another.");
      return;
    }
    
    if (CLANRANK(victim) > CLAN_APPLY && CLANRANK(victim) < CLAN_RETIRED)
      CLANRIPS(GET_CLAN(victim))++;
    if (CLANRANK(ch) > CLAN_APPLY && CLANRANK(ch) < CLAN_RETIRED) {
      CLANKILLS(GET_CLAN(ch))++;
      if (GET_LEVEL(victim) >= 25 && QPOINTS(ch) < MAX_QPOINTS) {
        send_to_char("&mYou have just gained a questpoint!&w\r\n", ch);
        QPOINTS(ch)++;
      }
    }
  }
}


/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data * ch, struct char_data * victim, int dam,
	    int attacktype)
{
  ACMD(do_get);
  ACMD(do_split);
  long local_gold = 0;
  char local_buf[256];

/* Daniel Houghton's missile modification */
  bool missile = FALSE;

      


  if (GET_POS(victim) <= POS_DEAD) {
    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
	GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    inc_kill_counters(ch, victim);
    die(victim, ch);
    return 0;			/* -je, 7/7/92 */
  }

/* Daniel Houghton's missile modification */
  if (ch->in_room != victim->in_room)
    missile = TRUE;

  /* peaceful rooms */
  if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return 0;
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return 0;

  /* You can't damage an immortal! 
  if (!IS_NPC(victim) && (GET_POS(victim) == POS_INCAP)) {
    stop_fighting(ch);
    stop_fighting(victim);
  }
*/
  //if (!CAN_MURDER(ch, victim)) dam = 0;  
    
   
      
/* Daniel Houghton's missile modification */
  if ((victim != ch) && (!missile)) {
    if (GET_POS(ch) > POS_STUNNED) {
      if (FIGHTING(ch) == NULL)
	set_fighting(ch, victim);

      if (IS_NPC(ch) && IS_NPC(victim) && victim->master &&
	  !number(0, 10) && IS_AFFECTED(victim, AFF_CHARM) &&
	  (victim->master->in_room == ch->in_room)) {
	if (FIGHTING(ch) != NULL)
	  stop_fighting(ch);
	hit(ch, victim->master, TYPE_UNDEFINED);
	return 0;
      }
    }
    if (GET_POS(victim) > POS_STUNNED && FIGHTING(victim) == NULL) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch) &&
	  (GET_LEVEL(ch) < LVL_IMMORT))
	remember(victim, ch);
    }
  }
  /* Attack pet? */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;
    
  if (PLR_FLAGGED(ch, PLR_CLAWS))
    dam += number(2, dam / 6);

    if (PLR_FLAGGED(ch, PLR_FANGS))
    dam += number(2, dam / 6);

  if (IS_AFFECTED(ch, AFF_DEATHDANCE))
    dam += number(1, dam / 4);
    
  /* Check for PK if this is not a PK MUD */
  if (!pk_allowed) {
    check_killer(ch, victim);

  /* For BLOCK */
  if (GET_POS(victim)==POS_FIGHTING) {
    //if (GET_POWER8(ch) == POWER_TWISTERFLIP) {
    
        act("You block $N's vicious attack!", FALSE, victim, 0, ch, TO_CHAR);
        act("$n blocks your vicious attack!", FALSE, victim, 0, ch, TO_VICT);
        act("$n blocks $N's vicious attack!", FALSE, victim, 0, ch, TO_ROOM);
        return 3;
      //}
    }
 

/* Daniel Houghton's modification:  Let the PK flag be enough! */
/*    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0; */
  }

  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 1000), 0);
  GET_HIT(victim) -= dam;

  /* Gain exp for the hit */
  if ((ch != victim) && (!missile))
    gain_exp(ch, GET_LEVEL(victim) * dam);

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (attacktype != -1) {
    if ((!IS_WEAPON(attacktype)) || missile)
      skill_message(dam, ch, victim, attacktype);
    else {
      if (GET_POS(victim) == POS_DEAD || dam == 0) {
        if (!skill_message(dam, ch, victim, attacktype))
  	  dam_message(dam, ch, victim, attacktype);
      } else  {
        dam_message(dam, ch, victim, attacktype);
      }
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("&c$n&w is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou are mortally wounded, and will die soon, if not aided.\r\n", victim);
    break;
  case POS_INCAP:
    act("&c$n&w is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou are incapacitated an will slowly die, if not aided.\r\n", victim);
    break;
  case POS_STUNNED:
    act("&c$n&w is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou're stunned, but will probably regain consciousness again.\r\n", victim);
    break;
  case POS_DEAD:
    act("&c$n&w is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("&RYou are dead!  Sorry...\r\n", victim);
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      act("&RThat really did HURT!", FALSE, victim, 0, 0, TO_CHAR);
      if (!IS_NPC(victim) && GET_MAX_HIT(victim) <=-11 && GET_PKILLP == 2) {
     GET_MAX_HIT(victim) = -10;
     stop_fighting(ch);
     }

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 2)) {
      sprintf(buf2, "%s&RYou wish that your wounds would stop BLEEDING so much!%s\r\n",
	      CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      send_to_char(buf2, victim);
      if ((ch != victim) && MOB_FLAGGED(victim, MOB_WIMPY)
        && !(AFF_FLAGGED(victim, AFF_HOLD) && (number(1,7) != 3)))
	do_flee(victim, NULL, 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
      send_to_char("&WYou wimp out, and attempt to flee!\r\n", victim);
      do_flee(victim, NULL, 0, 0);
      /*
      if (IS_NPC(ch) && GET_POS(ch) > POS_STUNNED && MOB_FLAGGED(ch, MOB_HUNT)) {
        SET_BIT(MOB_FLAGS(ch), MOB_MEMORY);
	  remember(ch, victim);
	  HUNTING(ch) = victim;
      }
      */
    }
    break;
  }

  mprog_hitprcnt_trigger(ch, FIGHTING(ch));
  mprog_fight_trigger(ch, FIGHTING(ch));

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && (!(victim->desc) || PRF_FLAGGED(victim, PRF_AFK)) &&
    !affected_by_spell(victim, SPELL_CORPSE_HOST)) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("&C$n&w is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = victim->in_room;
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }
  
  /* stop someone from fighting if they're stunned or worse */
  if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
    stop_fighting(victim);
    else
  if (!IS_NPC(victim) && (GET_POS(victim) <= POS_MORTALLYW) && (FIGHTING(ch) != NULL))
    stop_fighting(ch);
  
  if (AFF2_FLAGGED(victim, AFF2_FIRE_SHIELD) && attacktype >= TYPE_HIT &&
    attacktype <= TYPE_STAB && (number(1, 81) < GET_LEVEL(victim) ||
    GET_POS(victim) == POS_DEAD)) {
    damage(victim, ch, GET_POS(victim) == POS_DEAD ? dam / 2 : dam / 4,
    TYPE_UNDEFINED);
    act("$N blows you with his fire shield.", FALSE, ch, 0, victim, TO_CHAR);
    act("You blow $n with your fire shield.", FALSE, ch, 0, victim, TO_VICT);
  }
  
  if (GET_POS(victim) == POS_DEAD) {
    if ((ch != victim) && (IS_NPC(victim) || victim->desc)) {
      if (IS_AFFECTED(ch, AFF_GROUP))
        {
        if (!missile)
	  group_gain(ch, victim);
	}
      else {
        solo_gain(ch, victim);
        /*
	exp = MIN(max_exp_gain, GET_KILL_EXP(victim));

	if (IS_NPC(ch))
	  exp = MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	else
	  exp = MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) >> 3);
	exp = MAX(exp, 1);
	if (exp > 1) {
	  sprintf(buf2, "&WYou receive &C%d&w experience points.&w\r\n", exp);
	  send_to_char(buf2, ch);
	} else
	  send_to_char("&WYou receive &Cone&w lousy experience point.&w\r\n", ch);
	  if (!missile)
	    gain_exp(ch, exp);
            gain_demonxp(ch, exp);
	change_alignment(ch, victim); */
      }
    }
    if (!IS_NPC(victim)) {
     //check_increment_pkcounter(ch, victim);
    sprintf(buf2, "&r[ <-INFO-> ] %s killed by %s at %s!\r\n", 
GET_NAME(victim), GET_NAME(ch), world[victim->in_room].name);   
    send_to_all(buf2);
   sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
	      world[victim->in_room].name);
      mudlog(buf2, BRF, LVL_IMMORT, TRUE);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    inc_kill_counters(ch, victim);
     
    
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }
   if (!IS_NPC(victim) && GET_LEVEL(victim) == GET_LEVEL(ch)) {
     send_to_char("You drain the spirit of your victim and rise in status",ch);
     GET_LEGEND_LEVELS(ch) +=1;
     return 1;
}
/*
      if (!IS_NPC(victim) && GET_MAX_HIT(victim) <=-11 && GET_PKILLP == 2)
{
     GET_MAX_HIT(victim) = -10;
     stop_fighting(ch);
     return 1;
}
*/
 die(victim, ch);
      if (!IS_NPC(ch)) {
        if (PRF_FLAGGED(ch, PRF_AUTOLOOT)) do_get(ch,"all corpse",0,0);
        else if (PRF_FLAGGED(ch, PRF_AUTOGOLD)) do_get(ch,"gold corpse",0,0);
      }
    
      if (!IS_NPC(ch) && IS_AFFECTED(ch, AFF_GROUP) && (local_gold > 0) &&
          PRF_FLAGGED(ch, PRF_AUTOSPLIT) && 
          (PRF_FLAGGED(ch,PRF_AUTOLOOT) || PRF_FLAGGED(ch,PRF_AUTOGOLD))) 
            do_split(ch,local_buf,0,0);   
      
      return -1;  
  }
  return dam;
}


void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
//  struct char_data *killer;
  int w_type, victim_ac, calc_thaco, dam, diceroll;

    /* Do some sanity checking, in case someone flees, etc. */
  if (ch->in_room != victim->in_room) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }
  if (PLR_FLAGGED(ch, PLR_CLAWS))
  w_type = TYPE_CLAW;
  else
  if (PLR_FLAGGED(ch, PLR_FANGS))
  w_type = TYPE_BITE;
  else
  /* Find the weapon type (for display purposes only) */
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  /* Calculate the THAC0 of the attacker */

  if (!IS_NPC(ch))
    calc_thaco = thaco((int) GET_CLASS(ch), (int) GET_LEVEL(ch));
  else		/* THAC0 for monsters is set in the HitRoll */
    calc_thaco = 20;

  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  calc_thaco -= GET_HITROLL(ch);
  calc_thaco -= (int) ((GET_INT(ch) - 13) / 1.5);	/* Intelligence helps! */
  calc_thaco -= (int) ((GET_WIS(ch) - 13) / 1.5);	/* So does wisdom */
  diceroll = number(1, 20);

  /* Calculate the raw armor including magic armor.  Lower AC is better. */
  victim_ac = GET_AC(victim) / 10;


  if (AWAKE(victim))
    victim_ac += dex_app[GET_DEX(victim)].defensive;

  victim_ac = MAX(-10, victim_ac);	/* -10 is lowest */

  /* roll the die and take your chances... */
  diceroll = number(1, 20);

  /* decide whether this is a hit or a miss */
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    /* the attacker missed the victim */
    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, SKILL_BACKSTAB);
    else
      damage(ch, victim, 0, w_type);
  } else
    if (!IS_NPC(victim) && number(1, 201) <= GET_SKILL(victim, SKILL_PARRY)) {
	act("$N parries your attack.", FALSE, ch, 0, victim, TO_CHAR);
	act("You parry $n's attack.", FALSE, ch, 0, victim, TO_VICT);
    } else if (AFF_FLAGGED(victim, AFF_BLINK) && 
	       (number(1, 60) <= GET_LEVEL(victim))) {
		act("$N blinks away from your attack.", FALSE, ch, 0, victim, TO_CHAR);
		act("$N blinks out of $n's way.", FALSE, ch, 0, victim, TO_NOTVICT);
	        act("You blink out of $n's way.", FALSE, ch, 0, victim, TO_VICT);
    } else if (AFF_FLAGGED(victim, AFF_MIRRORIMAGE) &&
	       (number(1, 40) > GET_INT(ch)) && (number(1, 40) <= GET_INT(victim))) {
		act("One of $N's false images dissipates and is instantly replaced!", FALSE, ch, 0, victim, TO_CHAR);
		act("One of $N's false images takes the blow from $n and is instantly replaced!", FALSE, ch, 0, victim, TO_NOTVICT);
	        act("One of your images takes the blow from $n and is replaced by another image.", FALSE, ch, 0, victim, TO_VICT);
    } else {
    /* okay, we know the guy has been hit.  now calculate damage. */
    /* Start with the damage bonuses: the damroll and strength apply */
    dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
    dam += GET_DAMROLL(ch);

    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      /* Add weapon-based damage if a weapon is being wielded */
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
    } else {
      /* If no weapon, add bare hand damage instead */
      if (IS_NPC(ch)) {
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      } else {
	dam += number(0, 2);	/* Max. 2 dam with bare hands */
      }
    }


 
    /*
     * Include a damage multiplier if victim isn't ready to fight:
     *
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     *
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants.
     */
      if (GET_POS(victim) < POS_FIGHTING)
        dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;
  
    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);
    
    if (type == SKILL_BACKSTAB) {
      dam *= backstab_mult(GET_LEVEL(ch));
      damage(ch, victim, dam, SKILL_BACKSTAB);
    } else
      damage(ch, victim, dam, w_type);
      
    /* Now deal with bloodlust */
    if (AFF2_FLAGGED(ch, AFF2_BLOODLUST) && w_type == TYPE_HIT && 
      number(1, 35) < GET_LEVEL(ch)) {
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + dam / 2);
      send_to_char("&RAaah blood....&r\r\n", ch);
    }
  }
}

int weapon_special(struct obj_data *wpn, struct char_data *ch)
{
  extern struct index_data *obj_index;
  int (*name)(struct char_data *ch, void *me, int cmd, char *argument);
  SPECIAL(blind_weapon);
  SPECIAL(fireball_weapon);
  SPECIAL(curse_weapon);
  
  name = obj_index[GET_OBJ_RNUM(wpn)].func;
  if (name != blind_weapon && name != fireball_weapon &&
      name != curse_weapon)
    return 0;
  return (name)(ch, wpn, 0, "");
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  //struct char_data *victim;
  struct obj_data *wpn;
  int w_type; 
  int apr;


  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;
    apr = 0;

    if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
      stop_fighting(ch);
      continue;
    }
/* 
      if (!IS_NPC(victim) && GET_MAX_HIT(victim) <=-11 && GET_PKILLP == 2) 
{
     GET_MAX_HIT(victim) = -10;
     stop_fighting(ch);
     return;
}
*/


    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
	continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
	GET_POS(ch) = POS_FIGHTING;
	act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char("You can't fight while sitting!!\r\n", ch);
      continue;
    }
    if (IS_NPC(ch)) {
      apr += number(0, GET_LEVEL(ch) / 4);
    } else 
      if (GET_SKILL(ch, SKILL_SECOND_ATTACK) >= number(1, 101)) {
//      if (GET_POWER9(ch) && (GET_CLASS(ch) == CLASS_WARRIOR)) 
      if (GET_SKILL(ch, SKILL_THIRD_ATTACK) >= number(1, 201)) 
        apr++; apr++;
      //} 
    //if (GET_POWER9(ch) && (GET_CLASS(ch) == CLASS_WARRIOR)) {
      apr++;
}

    


    if (GET_BRENNUM(ch) == 6) {
        apr++;
}
    if (PLR_FLAGGED(ch, PLR_FANGS)) 
     w_type = TYPE_BITE;
      apr++;

    
    if (!IS_NPC(ch) && GET_SKILL(ch, SKILL_HAND_TO_HAND) >= number(1, 101)){
     if (!GET_EQ(ch, WEAR_WIELD))
       apr = 9;
    } else if (!IS_NPC(ch) && !GET_EQ(ch, WEAR_WIELD) && 
      GET_SKILL(ch, SKILL_UNARMED_COMBAT) >= number(1, 101)){
       apr = 3;
    } else {
      apr = MAX(-1, MIN(apr, 4));
    }
    
    if (AFF_FLAGGED(ch, AFF_HASTE))
      apr += number(0, 2);
 
    if(AFF_FLAGGED(ch, AFF_DEATHDANCE) && !AFF_FLAGGED(ch, AFF_HASTE))
      apr += dice(2, lvD6(ch));
    
    /* if (AFF_FLAGGED(ch, AFF_HASTE))
      apr *= 2; */
      
    /* increment apr by one for every attack they are supposed to get,
       for the multiple attack skills, you should make sure they only
       get a subsequent attack if they properly got the previous one.
       For instance, you only get third attack if you are getting a
       second attack.  This doesn't need to be skill based, you can
       easily make it based upon class/level... see the second example
       below.
      
         if (AFF_FLAGGED(ch, AFF_HASTE))
           apr += number(0, 2);
          
         if (GET_CLASS(ch) == CLASS_WARRIOR && GET_LEVEL(ch) >= 10)
           apr++;
      
       If apr is negative they get no attacks, if apr is 0 they get
       one attack.  APR has a range of -1 to 4, giving a minimum of
       no attacks, to a maximum of 4.  See the below line for changing
       that (eg., MAX(-1, MIN(apr, 6)) means a max of 6). */
    
    apr = MAX(-1, MIN(apr, 6));
 
    if (apr >= 0) {
      for (; apr >= 0 && FIGHTING(ch); apr--) {
        if (!(wpn = GET_EQ(ch, WEAR_WIELD)) || GET_OBJ_TYPE(wpn) != ITEM_WEAPON ||
          !weapon_special(wpn, ch))
            hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
      }
      /* XXX: Need to see if they can handle "" instead of NULL. */
      if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
        (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, "");
    }
  }
}


int skill_roll(struct char_data *ch, int skill_num) 
{
  if (number(0, 101) > GET_SKILL(ch, skill_num))
    return FALSE;
  else
    return TRUE;
}


void strike_missile(struct char_data *ch, struct char_data *tch, 
                   struct obj_data *missile, int dir, int attacktype)
{
  int dam;
  extern struct str_app_type str_app[];

  dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
  dam += dice(missile->obj_flags.value[1],
              missile->obj_flags.value[2]); 
  dam += GET_DAMROLL(ch);
  if (AFF_FLAGGED(tch, AFF_SHIELD)) dam = dam / 2;
             
  send_to_char("You hit!\r\n", ch);
  sprintf(buf, "$P flies in from the %s and strikes %s.", 
          dirs[rev_dir[dir]], GET_NAME(tch));
  act(buf, FALSE, tch, 0, missile, TO_ROOM);
  sprintf(buf, "$P flies in from the %s and hits YOU!",
                dirs[rev_dir[dir]]);
  act(buf, FALSE, tch, 0, missile, TO_CHAR);
  damage(ch, tch, dam, attacktype);
  if (IS_NPC(tch) && !IS_NPC(ch) && GET_POS(tch) > POS_STUNNED) {
    SET_BIT(MOB_FLAGS(tch), MOB_MEMORY);
    remember(tch, ch);
    HUNTING(tch) = ch;
  }
  return;
} 


void miss_missile(struct char_data *ch, struct char_data *tch, 
                struct obj_data *missile, int dir, int attacktype)
{
  sprintf(buf, "$P flies in from the %s and hits the ground!",
                dirs[rev_dir[dir]]);
  act(buf, FALSE, tch, 0, missile, TO_ROOM);
  act(buf, FALSE, tch, 0, missile, TO_CHAR);
  send_to_char("You missed!\r\n", ch);
}


void mob_reaction(struct char_data *ch, struct char_data *vict, int dir)
{
  if (IS_NPC(vict) && !FIGHTING(vict) && GET_POS(vict) > POS_STUNNED) {

     /* can remember so charge! */
    if (IS_SET(MOB_FLAGS(vict), MOB_MEMORY)) {
      remember(vict, ch);
      sprintf(buf, "$n bellows in pain!");
      act(buf, FALSE, vict, 0, 0, TO_ROOM); 
      if (GET_POS(vict) == POS_STANDING) {
        if (!do_simple_move(vict, rev_dir[dir], 1))
          act("$n stumbles while trying to run!", FALSE, vict, 0, 0, TO_ROOM);
      } else
      GET_POS(vict) = POS_STANDING;
      
    /* can't remember so try to run away */
    } else {
      do_flee(vict, "", 0, 0);
    }
  }
}


void fire_missile(struct char_data *ch, char arg1[MAX_INPUT_LENGTH],
                  struct obj_data *missile, int pos, int range, int dir)
{
  bool shot = FALSE, found = FALSE;
  int attacktype;
  int room, nextroom, distance;
  struct char_data *vict;
    
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return;
  }

  room = ch->in_room;

  if CAN_GO2(room, dir)
    nextroom = EXIT2(room, dir)->to_room;
  else
    nextroom = NOWHERE;
  
  if (GET_OBJ_TYPE(missile) == ITEM_GRENADE) {
    send_to_char("You throw it!\r\n", ch);
    sprintf(buf, "$n throws %s %s.", 
      GET_EQ(ch,WEAR_WIELD)->short_description, dirs[dir]);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    sprintf(buf, "%s flies in from the %s.\r\n",
            missile->short_description, dirs[rev_dir[dir]]);
    send_to_room(buf, nextroom);
    obj_to_room(unequip_char(ch, pos), nextroom);
    return;
  }

  for (distance = 1; ((nextroom != NOWHERE) && (distance <= range)); distance++) {

    for (vict = world[nextroom].people; vict ; vict= vict->next_in_room) {
      if ((isname(arg1, GET_NAME(vict))) && (CAN_SEE(ch, vict))) {
        found = TRUE;
        break;
      }
    }

    if (found == 1) {

      /* Daniel Houghton's missile modification */
      if (missile && ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL)) {
        send_to_char("Nah.  Leave them in peace.\r\n", ch);
        return;
      }

      switch(GET_OBJ_TYPE(missile)) {
        case ITEM_THROW:
          send_to_char("You throw it!\r\n", ch);
          sprintf(buf, "$n throws %s %s.", GET_EQ(ch,WEAR_WIELD)->short_description,
                       dirs[dir]);
          attacktype = SKILL_THROW;
          break;
        case ITEM_ARROW:
          act("$n aims and fires!", TRUE, ch, 0, 0, TO_ROOM);
          send_to_char("You aim and fire!\r\n", ch);
          attacktype = SKILL_BOW;
          break;
        case ITEM_ROCK:
          act("$n aims and fires!", TRUE, ch, 0, 0, TO_ROOM);
          send_to_char("You aim and fire!\r\n", ch);
          attacktype = SKILL_SLING;
          break;
        case ITEM_BOLT:
          act("$n aims and fires!", TRUE, ch, 0, 0, TO_ROOM);
          send_to_char("You aim and fire!\r\n", ch);
          attacktype = SKILL_CROSSBOW;
          break;
        default:
          attacktype = TYPE_UNDEFINED;
        break;
      }

      if (attacktype != TYPE_UNDEFINED)
        shot = skill_roll(ch, attacktype);
      else
        shot = FALSE;

      if (AFF_FLAGGED(vict, AFF_BLINK) && 
        (number(1, 50) <= GET_LEVEL(vict))) {
	act("$N blinks away from your $p.", FALSE, ch, missile, vict, TO_CHAR);
        act("You blink away from the $p $n launched at you.", FALSE, ch, missile, vict, TO_VICT);
        shot = FALSE;
      } else if (AFF_FLAGGED(vict, AFF_MIRRORIMAGE) &&
        (number(1, 40) > GET_INT(ch)) && (number(1, 40) <= GET_INT(vict))) {
	act("One of $N's false images dissipate and is instantly replaced!", FALSE, ch, missile, vict, TO_CHAR);
        act("One of your images takes the $p and is replaced by another image.", FALSE, ch, missile, vict, TO_VICT);
        shot = FALSE;
      }

      if (shot == TRUE) {
        strike_missile(ch, vict, missile, dir, attacktype);
        if ((number(0, 1)) || (attacktype == SKILL_THROW))
          obj_to_char(unequip_char(ch, pos), vict);
        else
          extract_obj(unequip_char(ch, pos));
        improve_skill(ch, attacktype);
      } else {
      /* ok missed so move missile into new room */
        miss_missile(ch, vict, missile, dir, attacktype);
        if ((!number(0, 2)) || (attacktype == SKILL_THROW))
          obj_to_room(unequip_char(ch, pos), vict->in_room);
        else
          extract_obj(unequip_char(ch, pos));
      }

      /* either way mob remembers */
      mob_reaction(ch, vict, dir);
      WAIT_STATE(ch, PULSE_VIOLENCE);
      return; 

    } 

    room = nextroom;
    if CAN_GO2(room, dir)
      nextroom = EXIT2(room, dir)->to_room;
    else
      nextroom = NOWHERE;
  }

  send_to_char("Can't find your target!\r\n", ch);
  return;

}


void tick_grenade(void)
{
  struct obj_data *i, *tobj;
  struct char_data *tch, *next_tch;
  int s, t, dam, door;
  /* grenades are activated by pulling the pin - ie, setting the
     one of the extra flag bits. After the pin is pulled the grenade
     starts counting down. once it reaches zero, it explodes. */

  for (i = object_list; i; i = i->next) { 

    if (IS_SET(GET_OBJ_EXTRA(i), ITEM_LIVE_GRENADE)) {
      /* update ticks */
      if (i->obj_flags.value[0] >0)
        i->obj_flags.value[0] -=1;
      else { 
        t = 0;

        /* blow it up */
        /* checks to see if inside containers */
        /* to avoid possible infinite loop add a counter variable */
        s = 0; /* we'll jump out after 5 containers deep and just delete
                       the grenade */

        for (tobj = i; tobj; tobj = tobj->in_obj) {
          s++;
          if (tobj->in_room != NOWHERE) { 
            t = tobj->in_room;
            break;
          } else
            if ((tch = tobj->carried_by)) { 
              t = tch->in_room;
              break;
            } else 
              if ((tch = tobj->worn_by)) { 
                t = tch->in_room;
                break;
              }
          if (s == 5)
            break;
        }

        /* then truly this grenade is nowhere?!? */
        if (t <= 0) {
          sprintf(buf, "serious problem, grenade truly in nowhere\r\n");
          log(buf);
          extract_obj(i);
        } else { /* ok we have a room to blow up */

        /* peaceful rooms */
          if (ROOM_FLAGGED(t, ROOM_PEACEFUL)) {
            sprintf(buf, "You hear %s explode harmlessly, with a loud POP!\n\r", 
              i->short_description);
            send_to_room(buf, t);
            extract_obj(i);
            return;
          }

          dam = dice(i->obj_flags.value[1], i->obj_flags.value[2]);

          sprintf(buf, "Oh no - %s explodes!  KABOOOOOOOOOM!!!\r\n", 
            i->short_description);
          send_to_room(buf, t);

          for (door = 0; door < NUM_OF_DIRS; door++)
            if (CAN_GO2(t, door)) 
              send_to_room("You hear a loud explosion!\r\n", 
              world[t].dir_option[door]->to_room);

          for (tch = world[t].people; tch; tch = next_tch) {
            next_tch= tch->next_in_room;

            if (GET_POS(tch) <= POS_DEAD) {
              log("SYSERR: Attempt to damage a corpse.");
              return;			/* -je, 7/7/92 */
            }

            /* You can't damage an immortal! */
            if (IS_NPC(tch) || (GET_LEVEL(tch) < LVL_IMMORT)) {
              if (AFF_FLAGGED(tch, AFF_SHIELD))
                GET_HIT(tch) -= dam / 2;
              else
                GET_HIT(tch) -= dam;
              act("$n is blasted!", TRUE, tch, 0, 0, TO_ROOM);
              act("You are caught in the blast!", TRUE, tch, 0, 0, TO_CHAR);
              update_pos(tch);

    
             
              if (GET_POS(tch) <= POS_DEAD) { 
                make_corpse(tch);
                death_cry(tch);
                extract_char(tch);
              }
            }
         
     }          /* ok hit all the people now get rid of the grenade and 
                  any container it might have been in */

          extract_obj(i);

        }
      } /* end else stmt that took care of explosions */    
    } /* end if stmt that took care of live grenades */
  } /* end loop that searches the mud for objects. */

  return;

}

