/************************************************************************* 
*
File: act.informative.c Part of CircleMUD * * Usage: Player-level commands
of an informative nature * * * * All rights reserved.  See license.doc for
complete information.  * * * * Copyright (C) 1993, 94 by the Trustees of
the Johns Hopkins University * * CircleMUD is based on DikuMUD, Copyright
(C) 1990, 1991.  *
*************************************************************************/

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
#include "clan.h"
#include "constants.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct player_index_element *player_table;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct zone_data *zone_table;
extern int top_of_p_table;
extern int top_of_zone_table;
extern int top_of_world;
extern HOMETOWN hometowns[];
extern struct clan_info *clan_index;
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;
extern const char *weekdays[];
extern const char *month_name[];
extern struct zone_data *zone_table;

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *buildbook;
extern char *spells[];
extern char *spell_tables[];
extern char *class_abbrevs[];
extern char *Mortlevels[];
extern char *pstatus[];
/* global */
int boot_high = 0;

/* extern functions */
long find_class_bitvector(char arg);
char *get_clan_rank_str(int clan, int rank);
ACMD(do_action);
int level_exp(int chclass, int level);
char *title_male(int chclass, int level);
char *title_female(int chclass, int level);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int cmp_class(char *class_str);

/* local functions */
int  print_object_location(int num, struct obj_data * obj, struct char_data * ch, int recur);
void show_obj_to_char(struct obj_data * object, struct char_data * ch, int mode);
void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode, int show);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
void perform_mortal_where(struct char_data * ch, char *arg);
void perform_immort_where(struct char_data * ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
void sort_commands(void);
ACMD(do_commands);
void diag_char_to_char(struct char_data * i, struct char_data * ch);
void look_at_char(struct char_data * i, struct char_data * ch);
void list_one_char(struct char_data * i, struct char_data * ch);
void list_char_to_char(struct char_data * list, struct char_data * ch);
void do_auto_exits(struct char_data * ch);
ACMD(do_exits);
void look_in_direction(struct char_data * ch, int dir);
void look_in_obj(struct char_data * ch, char *arg);
char *find_exdesc(char *word, struct extra_descr_data * list);
void look_at_target(struct char_data * ch, char *arg);

  


void char_examination_unit(struct char_data * object, struct char_data * ch, int level) {
char buf[1024];
int lack;
  
  if (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_EXAMUNIT)) return;
  buf[0] = '\0';
  if ((GET_LEVEL(object) >= LVL_IMMORT) || \
     (GET_LEVEL(object) > (level*LVL_IMMORT) / 100) || \
     ((GET_LEVEL(object) - GET_LEVEL(ch)) > 10)) {
       lack = 1;
     }
     else {
       lack = 0;
     }
  
  sprintf(buf,   "\r\n&g+---<=[ EXAMINATION UNIT ]=>---+\r\n");
  if (!lack) {
  sprintf(buf, "%s| Level: [&G%5d&g] Hit : [&G%5d&g] |\r\n",buf,GET_LEVEL(object), GET_HIT(object));
  sprintf(buf, "%s| Mana : [&G%5d&g] Move: [&G%5d&g] |\r\n",buf, \
     GET_MANA(object), GET_MOVE(object));
  sprintf(buf, "%s| Status: &GOK&g                   |\r\n",buf);
  } else {
  sprintf(buf, "%s| Level: [&G-----&g] Hit : [&G-----&g] |\r\n",buf);
  sprintf(buf, "%s| Mana : [&G-----&g] Move: [&G-----&g] |\r\n",buf);
  sprintf(buf, "%s| Status: &GDATA NOT ACCESSIBLE&g  |\r\n",buf);
  }
  sprintf(buf, "%s+==============================+&w\r\n",buf);
  act("&c$n&w points examination unit at you.\r\nThe laser beam scans you.\r\n", TRUE, ch, 0, object, TO_VICT);
  send_to_char(buf,ch);
}

/*
 * This function screams bitvector... -gg 6/45/98
 */
void show_obj_to_char(struct obj_data * object, struct char_data * ch,
			int mode)
{
  bool found;
  char *owner_name_ptr;

  *buf = '\0';
  if ((mode == 0) && object->description) {
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
     sprintf(buf,"%s&w[%d]&g ",buf,GET_OBJ_VNUM(object));
    strcat(buf, object->description);
  }
  else if (object->short_description && ((mode == 1) ||
				 (mode == 2) || (mode == 3) || (mode == 4)))
    strcpy(buf, object->short_description);
  else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
      if (GET_OBJ_BOTTOM_LEV(object) > GET_LEVEL(ch)) {
        if (GET_OBJ_BOTTOM_LEV(object) >= LVL_IMMORT)
          send_to_char("What a pity, you are not able to recognize those holy runes.\r\n", ch);
        else
          send_to_char("Unfortunately, you have no idea how to decipher the writing.\r\n", ch);
        return;
      }
      if (object->action_description) {
	strcpy(buf, "There is something written upon it:\r\n\r\n");
	strcat(buf, object->action_description);
	page_string(ch->desc, buf, 1);
      } else
	act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else			/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");
  }
  if (mode != 3) {
    found = FALSE;
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
      strcat(buf, " (invisible)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
      strcat(buf, " ..It glows blue!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
      strcat(buf, " ..It glows yellow!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW)) {
      strcat(buf, " ..It has a soft glowing aura!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_BURIED)) {
      strcat(buf, " ..It is burried!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_HUM)) {
      strcat(buf, " ..It emits a faint humming sound!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_ENGRAVED))
      if ((owner_name_ptr = get_name_by_id(GET_OBJ_OWNER_ID(object))) != NULL) {
        strcpy(buf1, owner_name_ptr);
        if (mode == 1 || mode == 2 || mode == 3 || mode == 4) {
          strcat(buf, " of ");
          strcat(buf, CAP(buf1));
        } else {
          strcat(buf, " ..It is engraved to "); 
          strcat(buf, CAP(buf1));
          strcat(buf, ".");
        }
        found = TRUE;
      }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, TRUE);
}


void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
		           int show)
{
  struct obj_data *i, *j;
  char buf[10];
  bool found;
  int num;

  found = FALSE;
  for (i = list; i; i = i->next_content) {
      num = 0;

      for (j = list; j != i; j = j->next_content)
	if (j->item_number==NOTHING) {
	  if(strcmp(j->short_description,i->short_description)==0) break;
	} else 
	  if (j->item_number==i->item_number) break;

      if ((GET_OBJ_TYPE(i) != ITEM_CONTAINER) && (j!=i))
        continue;

      for (j = i; j; j = j->next_content)
	if (j->item_number==NOTHING) {
	  if(strcmp(j->short_description,i->short_description)==0) num++;
	} else 
	  if (j->item_number==i->item_number) num++;

    if (CAN_SEE_OBJ(ch, i)) {
	if ((GET_OBJ_TYPE(i) != ITEM_CONTAINER) && (num!=1)) {
	  sprintf(buf,"(%2i) ", num);
	  send_to_char(buf, ch);
	}
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}


void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  
  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " is in excellent condition.\r\n");
  else if (percent >= 90)
    strcat(buf, " has a few scratches.\r\n");
  else if (percent >= 75)
    strcat(buf, " has some small wounds and bruises.\r\n");
  else if (percent >= 50)
    strcat(buf, " has quite a few wounds.\r\n");
  else if (percent >= 30)
    strcat(buf, " has some big nasty wounds and scratches.\r\n");
  else if (percent >= 15)
    strcat(buf, " looks pretty hurt.\r\n");
  else if (percent >= 0)
    strcat(buf, " is in awful condition.\r\n");
  else
    strcat(buf, " is bleeding awfully from big wounds.\r\n");

  send_to_char(buf, ch);
  
}


void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (!ch->desc)
    return;

   if (i->player.description)
    send_to_char(i->player.description, ch);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);
  
  if (RIDING(i) && RIDING(i)->in_room == i->in_room) {
    if (RIDING(i) == ch)
      act("$e is mounted on you.", FALSE, i, 0, ch, TO_VICT);
    else {
      sprintf(buf2, "$e is mounted upon %s.", PERS(RIDING(i), ch));
      act(buf2, FALSE, i, 0, ch, TO_VICT);
    }
  } else if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room) {
    if (RIDDEN_BY(i) == ch)
      act("You are mounted upon $m.", FALSE, i, 0, ch, TO_VICT);
    else {
      sprintf(buf2, "$e is mounted by %s.", PERS(RIDDEN_BY(i), ch));
      act(buf2, FALSE, i, 0, ch, TO_VICT);
    }
  }

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;
  
  if (GET_NO_FACE(ch) > 0) {
      act("$n has absolutly no face.\r\n", FALSE, i, 0, ch, TO_VICT);
    }
  if (found) {
    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
        send_to_char("&g", ch);
	send_to_char(where[j], ch);
	send_to_char("&w", ch);
	show_obj_to_char(GET_EQ(i, j), ch, 1);
      }
  }
  if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_IMMORT)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, 1);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char("You can't see anything.\r\n", ch);
  }
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "Hp: %d/%d, Mp: %d/%d, Vp: %d/%d\r\n", GET_HIT(i), GET_MAX_HIT(i),
      GET_MANA(i), GET_MAX_MANA(i), GET_MOVE(i), GET_MAX_MOVE(i));
    send_to_char(buf, i);
  } else if (!IS_NPC(ch) && GET_SKILL(ch,SKILL_EXAMINE) > 0) char_examination_unit(i,ch,GET_SKILL(ch,SKILL_EXAMINE));
}



void show_affect_to_char(struct char_data * i, struct char_data * ch)
{
  if (AFF2_FLAGGED(i, AFF2_BURNING) && !AFF2_FLAGGED(i, AFF2_PROT_FIRE))
    act("...$e is in flames!", FALSE, i, 0, ch, TO_VICT);
    
  if (AFF2_FLAGGED(i, AFF2_FREEZING) && !AFF2_FLAGGED(i, AFF2_PROT_COLD))
    act("...$e freezes!", FALSE, i, 0, ch, TO_VICT);
    
  if (AFF2_FLAGGED(i, AFF2_ACIDED))
    act("...$e is covered with concentrated acid!", FALSE, i, 0, ch, TO_VICT);

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    
  if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
      
  if (AFF2_FLAGGED(i, AFF2_FIRE_SHIELD))
    act("...$e is surrounded by the red glow!", FALSE, i, 0, ch, TO_VICT);
}

void list_one_char(struct char_data * i, struct char_data * ch)
{
  const char *positions[] = {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here.",
    " the expanded demon is lurking about here.",
    " is standing here."
  };

  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      strcpy(buf, "*");
    else
      *buf = '\0';
      
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
     sprintf(buf,"%s&w[%d]&y ",buf,GET_MOB_VNUM(i));
    }

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	strcat(buf, "(&RRed Aura&y) ");
      else if (IS_GOOD(i))
	strcat(buf, "(&BBlue Aura&y) ");
    }
    strcat(buf, i->player.long_descr);
    send_to_char(buf, ch);
    show_affect_to_char(i, ch);
    return;
  }
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
     sprintf(buf,"&w[%d]&y ", GET_MOB_VNUM(i));
    } else strcpy(buf,"\0");
    
  if (IS_NPC(i)) {
    sprintf(buf, "%s%s",buf, i->player.short_descr);
    CAP(buf);
  } else {
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWCLAN) && PLAYERCLAN(i) != 0 &&
      (GET_LEVEL(i) < LVL_IMMORT || GET_LEVEL(ch) >= LVL_IMMORT)
      && CLANRANK(i) > CLAN_APPLY)
        sprintf(buf1,"(%s)",CLANNAME(clan_index[PLAYERCLANNUM(i)]));
      else
        strcpy(buf1,"");
    sprintf(buf, "%s&c%s %s%s&y", buf, i->player.name, GET_TITLE(i), buf1);
  }
  
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_AFK))
    strcat(buf, " <AFK>");
  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    strcat(buf, " (invisible)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    strcat(buf, " (linkless)");
  if (PLR_FLAGGED(i, PLR_WRITING))
    strcat(buf, " (writing)");
    
  if (AFF_FLAGGED(i, AFF_MIRRORIMAGE))
	strcat(buf, " (multiple)");

  if (AFF2_FLAGGED(i, AFF2_PASSDOOR))
	strcat(buf, " (translucent)");

  if (MOB_FLAGGED(i, MOB_ETHEREAL))
	strcat(buf, " (ethereal)");
	
  if (AFF_FLAGGED(i, AFF_BLINK))
	strcat(buf, " (displaced)");

  if (RIDING(i) && RIDING(i)->in_room == i->in_room) {
    strcat(buf, " is here, mounted upon ");
    if (RIDING(i) == ch)
      strcat(buf, "you");
    else
      strcat(buf, PERS(RIDING(i), ch));
    strcat(buf, ".");
  } else if (GET_POS(i) != POS_FIGHTING) {
    if (GET_POS(i) != POS_STANDING)
      strcat(buf, positions[(int) GET_POS(i)]);
    else {
      if (AFF_FLAGGED(i, AFF_FLYING))
	       strcat(buf, " is flying in the air.");
        else if (AFF_FLAGGED(i, AFF_WATERWALK)) 
	       strcat(buf, " is floating in the air.");
        else strcat(buf, positions[(int) GET_POS(i)]);
    }
    
  } else {
    if (FIGHTING(i)) {
      strcat(buf, " is here, fighting ");
      if (FIGHTING(i) == ch)
	strcat(buf, "YOU!");
      else {
	if (i->in_room == FIGHTING(i)->in_room)
	  strcat(buf, PERS(FIGHTING(i), ch));
	else
	  strcat(buf, "someone who has already left");
	strcat(buf, "!");
      }
    } else			/* NIL fighting pointer */
      strcat(buf, " is here struggling with thin air.");
  }

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      strcat(buf, " (&RRed Aura&y)");
    else if (IS_GOOD(i))
      strcat(buf, " (&BBlue Aura&y)");
  }
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
  show_affect_to_char(i, ch);
}



void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room)
    if (ch != i) {
      if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room)
        continue;
        
      if (CAN_SEE(ch, i) && (!MOB_FLAGGED(i, MOB_ETHEREAL) || GET_LEVEL(ch) > LVL_IMMORT))
	list_one_char(i, ch);
      else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) &&
	       AFF_FLAGGED(i, AFF_INFRAVISION) && !MOB_FLAGGED(i, MOB_ETHEREAL))
	send_to_char("You see a pair of glowing red eyes looking your way.\r\n", ch);
    }
}


void do_auto_exits(struct char_data * ch)
{
  int door, slen = 0;

  *buf = '\0';

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE) {
      if (EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED))
      slen += sprintf(buf + slen, "*%c ", LOWER(*dirs[door]));
      else
      slen += sprintf(buf + slen, "%c ", LOWER(*dirs[door]));
    }
  sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM),
          *buf ? buf : "None! ", CCNRM(ch, C_NRM));

  send_to_char(buf2, ch);
}



ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char("Your blind eyes see less than you thought they would.\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE) {
      if(EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
        sprintf(buf2, "&W%-5s&w - ", dirs[door]);
        strcat(buf2, "A closed door.\r\n");
        }

    else if(!EXIT_FLAGGED(EXIT(ch, door), EX_CLOSED)) {
      if (GET_LEVEL(ch) >= LVL_IMMORT)
   sprintf(buf2, "&W%-5s&w - [&M%5d&w] %s\r\n", dirs[door],
                GET_ROOM_VNUM(EXIT(ch, door)->to_room),
                world[EXIT(ch, door)->to_room].name);
      else {
        sprintf(buf2, "&W%-5s&w - &M", dirs[door]);
        if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
          strcat(buf2, "Too dark to tell\r\n");
       if (IS_FOG(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
          strcat(buf2, "Fog covers this area.\r\n");
        else {
 
         strcat(buf2, world[EXIT(ch, door)->to_room].name);
          strcat(buf2, "&w\r\n");
        }
      }
     }
      strcat(buf, CAP(buf2));
    }
}
  send_to_char("&gObvious exits:&w\r\n", ch);
 
if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
}



void look_at_room(struct char_data * ch, int ignore_brief)
{
  if (!ch->desc)
    return;

  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
  }
  if (ROOM_AFFECTED(ch->in_room, RAFF_FOG) && 
    !(!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT && PRF_FLAGGED(ch, PRF_HOLYLIGHT))) {
    send_to_char("Your view is obscured by a thick fog.\r\n", ch);
    return;
  }
  send_to_char("&c", ch); 
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintbit_multi(ROOM_FLAGS_BITV(ch->in_room), room_bits, buf);
    sprintf(buf2, "[%5d] [%s] [ %s]", GET_ROOM_VNUM(IN_ROOM(ch)),
	    world[ch->in_room].name, buf);
    send_to_char(buf2, ch);
  } else
    send_to_char(world[ch->in_room].name, ch);

  send_to_char("&w\r\n", ch);

  if (!(!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(ch->in_room, ROOM_DEATH)) {
        if (zone_table[world[ch->in_room].zone].unloaded)
          reload_room_descs(ch->in_room, FALSE);
      send_to_char(world[ch->in_room].description, ch);
    }
  
  if (!IS_NPC(ch) && 
    number(1, 100)< (GET_INT(ch)*2+GET_SKILL(ch, SKILL_TRAP_AWARE))) {
    int i;
    for (i = 0; i < NUM_OF_DIRS ; i++) {
      if (EXIT(ch, i) && EXIT(ch, i)->to_room != NOWHERE)
        if (!IS_SET(EXIT(ch, i)->exit_info, EX_CLOSED)) 
          if ROOM_FLAGGED(EXIT(ch,i)->to_room, ROOM_DEATH) {
            send_to_char("&YThere seem to be a trap to the ",ch);
            send_to_char(dirs[i], ch);
            send_to_char("!&w\r\n",ch);
          }
    }
  }

  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(ch);

  /* now list characters & objects */
  send_to_char("&g", ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  send_to_char("&y", ch);
  list_char_to_char(world[ch->in_room].people, ch);
  send_to_char("&w", ch);
}


void list_scanned_chars(struct char_data * list, struct char_data * ch, 
                        int distance, int door) 
{
  const char *how_far[] = {
    "close by",
    "a ways off",
    "far off to the"
  };

  struct char_data *i;
  int count = 0;
  *buf = '\0';

/* this loop is a quick, easy way to help make a grammatical sentence
   (i.e., "You see x, x, y, and z." with commas, "and", etc.) */

  for (i = list; i; i = i->next_in_room)

/* put any other conditions for scanning someone in this if statement -
   i.e., if (CAN_SEE(ch, i) && condition2 && condition3) or whatever */

    if (CAN_SEE(ch, i))
      count++;

  if (!count)
    return;

  for (i = list; i; i = i->next_in_room) {

/* make sure to add changes to the if statement above to this one also, using
   or's to join them.. i.e., 
   if (!CAN_SEE(ch, i) || !condition2 || !condition3) */

    if (!CAN_SEE(ch, i))
      continue; 
    if (!*buf)
      sprintf(buf, "You see &c%s&w", GET_NAME(i));
    else 
      sprintf(buf, "%s&c%s&w", buf, GET_NAME(i));
    if (--count > 1)
      strcat(buf, ", ");
    else if (count == 1)
      strcat(buf, " and ");
    else {
      sprintf(buf2, " %s %s.\r\n", how_far[distance], dirs[door]);
      strcat(buf, buf2);      
    }
  }
  send_to_char(buf, ch);

}


void look_in_direction(struct char_data * ch, int dir)
{
  int room, nextroom, orig_room = ch->in_room;
  int distance;

  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(EXIT(ch, dir)->general_description, ch);
    else
      send_to_char("You see nothing special.\r\n", ch);

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The &g%s&w is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    } else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The &g%s&w is open.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    }

    if CAN_GO2(orig_room, dir)
      nextroom = EXIT2(orig_room, dir)->to_room;
    else
      nextroom = NOWHERE;

    for (distance = 0; ((nextroom != NOWHERE) && (distance < 3)); distance++) {

      if (world[nextroom].people)
	list_scanned_chars(world[nextroom].people, ch, distance, dir);

      room = nextroom;
      if CAN_GO2(room, dir)
        nextroom = EXIT2(room, dir)->to_room;
      else
        nextroom = NOWHERE;

    }

  } else
    send_to_char("Nothing special there...\r\n", ch);
}



void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Look in what?\r\n", ch);
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				 FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
    send_to_char("There's nothing inside that!\r\n", ch);
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
	send_to_char("It is closed.\r\n", ch);
      else {
	send_to_char(fname(obj->name), ch);
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(" (carried): \r\n", ch);
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(" (here): \r\n", ch);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(" (used): \r\n", ch);
	  break;
	}

	list_obj_to_char(obj->contains, ch, 2, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
	send_to_char("It is empty.\r\n", ch);
      else {
	if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) {
	  sprintf(buf, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
	  amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
	  sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
	  sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
	}
	send_to_char(buf, ch);
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return NULL;
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */

/*
 * BUG BUG: If fed an argument like '2.bread', the extra description
 *          search will fail when it works on 'bread'!
 * -gg 6/24/98 (I'd do a fix, but it's late and non-critical.)
 */
void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = FALSE, j;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  char *desc;

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("&c$n&w looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("&c$n&w looks at &c$N&w.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }
  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) {
    page_string(ch->desc, desc, FALSE);
    return;
  }
  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = TRUE;
      }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = TRUE;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = TRUE;
      }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
    else
      show_obj_to_char(found_obj, ch, 6);	/* Find hum, glow etc */
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);
}


ACMD(do_look)
{
  char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (PLR2_FLAGGED(ch, PLR2_NOFACE)) {
  send_to_char("You cannot see anything without a face.\r\n",ch);
  return;
}

  if (!ch->desc)
    return;
  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("The darkness overcomes your vision...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);	/* glowing red eyes */
 } else if (ROOM_FLAGGED(ch->in_room, ROOM_FOG) && !CAN_SEE_IN_DARK(ch)) {
   send_to_char("Your view is obscured by a thick blanket of fog.\r\n",ch);
       list_char_to_char(world[ch->in_room].people, ch); 
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Read what?\r\n", ch);
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}



ACMD(do_examine)
{
  int bits;
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg);

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char("When you look inside, you see:\r\n", ch);
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char("You're broke!\r\n", ch);
  else if (GET_GOLD(ch) == 1)
    send_to_char("You have one miserable little gold coin.\r\n", ch);
  else {
    sprintf(buf, "You have &Y%d&w gold coins.\r\n", GET_GOLD(ch));
    send_to_char(buf, ch);
  }
}


ACMD(do_score)
{
  struct time_info_data playing_time;
  
  if (IS_NPC(ch))
    return;
  sprintf(buf, "You are &M%d&w years old.", GET_AGE(ch));


  if ((age(ch)->month == 0) && (age(ch)->day == 0))
    strcat(buf, "  It's your birthday today.\r\n");
  else
    strcat(buf, "\r\n");
//   sprintf(buf + strlen(buf), "&bYou have recieved a total of %d levels 
//on the status charts.&w\r\n", 
//GET_LEGEND_LEVELS(ch));
  if (GET_CLASS(ch) == CLASS_THIEF) {
  sprintf(buf + strlen(buf),
     "&gYour blood level is at count (&W%d/100&g)\r\n", GET_COND(ch, 
THIRST));
  sprintf(buf + strlen(buf), 
     "&BYou have a total of %d research points.\r\n", GET_DEMONXP(ch));
}
   if (GET_CLASS(ch) == CLASS_CLERIC) {
     sprintf(buf + strlen(buf),"&GYou have a total of %d research points.\r\n",GET_DEMONXP(ch));
       }
    if (GET_CLASS(ch) == CLASS_WARRIOR) {
  sprintf(buf + strlen(buf),
     "&gYour soul horde count is at a total of (&W%d&w).\r\n", 
GET_DEMONXP(ch));
}
    if (GET_CLASS(ch) == CLASS_CLERIC) {
   sprintf(buf + strlen(buf), 
     "&XYour powerlevel is at (%d / 5000).\r\n",GET_POWERL(ch));
    }
  sprintf(buf + strlen(buf),
       "You have &R%d&w(&r%d&w) hit, &C%d&w(&c%d&w) mana and &G%d&w(&g%d&w) movement points.\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

      sprintf(buf + strlen(buf), 
           "&bYour current status level is %d on the status charts.&w\r\n",GET_PLAYER_KILLS(ch));  
//  if (IS_NPC(ch) || GET_LEVEL(ch) >= 10)
    sprintf(buf + strlen(buf), "Your Stats: &cStr: [&C%2d&c/&C%2d&c] Dex:[&C%2d&c]"
                 " Int: [&C%2d&c] Wis: [&C%2d&c] Con: [&C%d&c] Cha: [&C%2d&c]&w\r\n",
      GET_STR(ch), GET_ADD(ch), GET_DEX(ch), GET_INT(ch), GET_WIS(ch), 
      GET_CON(ch), GET_CHA(ch));
      
    sprintf(buf + strlen(buf), "Hitroll: [&g%d&w], Damroll: [&g%d&w], Total Level [&W%d&w], Damcap [&W%d]\r\n",
      GET_HITROLL(ch), GET_DAMROLL(ch), GET_TOT_LEVEL(ch), 
GET_HITROLL(ch) * GET_DAMROLL(ch));

  sprintf(buf + strlen(buf), "Your armor class is &W%d&w/10, and your alignment is &y%d&w.\r\n",
	  GET_AC(ch), GET_ALIGNMENT(ch));
  
  sprintf(buf + strlen(buf), "You have scored &M%d&w exp, and have &W%d&w gold coins.\r\n",
	  GET_EXP(ch), GET_GOLD(ch));
    
  if (!IS_NPC(ch)) {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      sprintf(buf + strlen(buf), "You need &Y%d&w exp to reach your next level.\r\n",
	level_exp(GET_CLASS(ch),GET_LEVEL(ch)+1) - GET_EXP(ch));
	
  if (!IS_NPC(ch)) {
    sprintf(buf + strlen(buf), "You have gained &W%d&w questpoints so far.\r\n", QPOINTS(ch));
  }

    playing_time = *real_time_passed((time(0) - ch->player.time.logon) +
				  ch->player.time.played, 0);
    sprintf(buf + strlen(buf), "You have been playing for &W%d&w days and &W%d&w hours.\r\n",
	  playing_time.day, playing_time.hours);

//    sprintf(buf + strlen(buf), "This ranks you as &C%s.\r\n",
//	  GET_NAME(ch), GET_LEVEL(ch));
    if (PLAYERCLAN(ch) != 0)
      sprintf(buf + strlen(buf),"You are currently member of &W%s&w and your rank is &G%s&w.\r\n",
        CLANNAME(clan_index[PLAYERCLANNUM(ch)]), 
        get_clan_rank_str(PLAYERCLANNUM(ch), CLANRANK(ch)));
    sprintf(buf + strlen(buf), "Your hometown is &g%s&w.\r\n", hometowns[(int) GET_HOMETOWN(ch)].homename);
    sprintf(buf + strlen(buf), "You are carrying &G%d&w of max &g%d&w items and &M%d&w of &m%d&w weight.\r\n",
      IS_CARRYING_N(ch), CAN_CARRY_N(ch), IS_CARRYING_W(ch), CAN_CARRY_W(ch));
    sprintf(buf + strlen(buf), "You have died &R%d&w times and killed &G%d&w opponents.\r\n",
          GET_RIP_CNT(ch), GET_KILL_CNT(ch));
    sprintf(buf + strlen(buf), "You have walked into a deathtrap &Y%d&w times.\r\n",
          GET_DT_CNT(ch));
   if (GET_CLASS(ch) == CLASS_CLERIC) {
      sprintf(buf, "%s&WYou are a healthy warrior of the Saiya-Jin race.&w\r\n",buf);
    } else
   if (GET_CLASS(ch) == CLASS_WARRIOR) {
     sprintf(buf, "%s&rYou are the eternal minion of Lucifer, god of hell.&w\r\n",buf);
  } else 
   if (GET_CLASS(ch) == CLASS_THIEF) {
     sprintf(buf, "%s&RYou are an eternal follower of the dammned.&w\r\n", buf);
   }
  }

  switch (GET_POS(ch)) {
  case POS_DEAD:
    strcat(buf, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    strcat(buf, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    strcat(buf, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    strcat(buf, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    strcat(buf, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    strcat(buf, "You are resting.\r\n");
    break;
  case POS_SITTING:
    if (PLR_FLAGGED(ch, PLR_MEDITATE))
      strcat(buf, "You are meditating.\r\n");
    else
      strcat(buf, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    if (FIGHTING(ch))
      sprintf(buf + strlen(buf), "You are fighting %s.\r\n",
		PERS(FIGHTING(ch), ch));
    else
      strcat(buf, "You are fighting thin air.\r\n");
    break;
  case POS_STANDING:
    strcat(buf, "You are standing.\r\n");
    break;
   case POS_CLAWS:
    strcat(buf, "You are standing.\r\n");
    break;
  default:
    strcat(buf, "You are floating.\r\n");
    break;
  }

  if (!IS_NPC(ch)) {
  if (GET_COND(ch, DRUNK) > 10)
    strcat(buf, "You are intoxicated.\r\n");

 // if (GET_COND(ch, FULL) == 0)
  //  strcat(buf, "You are hungry.\r\n");

 // if (GET_COND(ch, THIRST) == 0)
 //   strcat(buf, "You are thirsty.\r\n");
  }

  if (AFF_FLAGGED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");

  if (AFF_FLAGGED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");

  if (AFF_FLAGGED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");

  if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");
   
  if (AFF2_FLAGGED(ch, AFF2_PROT_FIRE))
    strcat(buf, "You are protected against fire.\r\n");
    
  if (AFF2_FLAGGED(ch, AFF2_PROT_COLD))
    strcat(buf, "You are protected against freezing.\r\n");
    
  if (AFF2_FLAGGED(ch, AFF2_BURNING) && !AFF2_FLAGGED(ch, AFF2_PROT_FIRE))
    strcat(buf,"You are burning!\r\n");
    
  if (AFF2_FLAGGED(ch, AFF2_FREEZING) && !AFF2_FLAGGED(ch, AFF2_PROT_COLD))
    strcat(buf,"You are freezing!\r\n");
    
  if (AFF2_FLAGGED(ch, AFF2_ACIDED))
    strcat(buf,"You are acided!\r\n");
    
  if (AFF2_FLAGGED(ch, AFF2_FIRE_SHIELD))
    strcat(buf,"You are protected by fire shield!\r\n");

  send_to_char(buf, ch);
}


ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
        send_to_char("&g",ch);
	send_to_char(where[i], ch);
	send_to_char("&W",ch);
	show_obj_to_char(GET_EQ(ch, i), ch, 1);
	found = TRUE;
      } else {
	send_to_char(where[i], ch);
	send_to_char("&wSomething.\r\n", ch);
	found = TRUE;
      }
    }
  }
  if (!found) {
    send_to_char(" Nothing.\r\n", ch);
  }
}


ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  sprintf(buf, "It is &M%d&w o'clock &m%s&w, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  day = time_info.day + 1;	/* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The &W%d%s&w Day of the &g%s&w, Year &Y%d&w.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);

  send_to_char(buf, ch);
}


ACMD(do_weather)
{
  extern struct zone_data *zone_table;

  const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if (OUTSIDE(ch)) {
    sprintf(buf, "&WThe sky is %s and %s.&w\r\n", 
            sky_look[zone_table[world[ch->in_room].zone].sky],
	    (zone_table[world[ch->in_room].zone].change >= 0 ? 
             "you feel a warm wind from south" :
	     "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "Pressure: &c%d&w, Change: &c%d&w\r\n", 
      zone_table[GET_ROOM_ZONE(ch->in_room)].pressure, 
      zone_table[GET_ROOM_ZONE(ch->in_room)].change);
    send_to_char(buf, ch);  
  }
}


ACMD(do_help)
{
  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) {
    send_to_char("No help available.\r\n", ch);
    return;
  }

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char("There is no help on that word.\r\n", ch);
      return;
    } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      if (help_table[mid].min_level <= GET_LEVEL(ch))	
        page_string(ch->desc, help_table[mid].entry, 0);
      else
        send_to_char("There is no help on that word.\r\n", ch);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}


/*********************************************************************
* New 'do_who' by Daniel Koepke [aka., "Argyle Macleod"] of The 
Keep *
******************************************************************* */

char *WHO_USAGE =
  "Usage: who [minlev[-maxlev]] [-n name] [-c classes] [-rzqimo]\r\n"
  "\r\n"
  "Classes: (M)age, (C)leric, (T)hief, (W)arrior\r\n"
  "\r\n"
  " Switches: \r\n"
  "_.,-'^'-,._\r\n"
  "\r\n"
  "  -r = who is in the current room\r\n"
  "  -z = who is in the current zone\r\n"
  "\r\n"
  "  -q = only show questers\r\n"
  "  -i = only show immortals\r\n"
  "  -m = only show mortals\r\n"
  "  -o = only show outlaws\r\n"
  "\r\n";

#define IS_UNREACH(d)	(STATE(d) != CON_PLAYING && \
			 STATE(d) != CON_OEDIT && \
			 STATE(d) != CON_REDIT && \
			 STATE(d) != CON_ZEDIT && \
			 STATE(d) != CON_SEDIT && \
			 STATE(d) != CON_MEDIT && \
			 STATE(d) != CON_GEDIT && \
			 STATE(d) != CON_TEXTED && \
			 STATE(d) != CON_CLAN_EDIT)

ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *wch;
  char Imm_buf[MAX_STRING_LENGTH];
  char Mort_buf[MAX_STRING_LENGTH];
  //char Immp_buf[MAX_STRING_LENGTH];
  char name_search[MAX_NAME_LENGTH+1];
  char mode;
  
  int low = 0, high = LVL_IMPL, showclass = 0;
  bool who_room = FALSE, who_zone = FALSE, who_quest = 0;
  bool outlaws = FALSE, noimm = FALSE, nomort = FALSE;
  
  //int Wizards = 0, Mortals = 0, Immp = 0;
  int Wizards = 0, Mortals = 0;
  size_t i;
  
   const char *legendstatus[MAX_LEGEND_LEVELS - (1-1)] = {
   "&WIconnu&w",
   "&RSaipin&w",
   "&GDesrete&w",
   "&YCaprice&w",
   "&cVisciple&w",
   "&MLegendary&w"
};

     const char *pstatus[MAX_PSTATUS_LEVELS - (1-1)] = {
   "Mortal",
   "Avatar",
   "Immortal",
   "Demi-God",
   "Deity",
   "Supreme"
};


  const char *WizLevels[LVL_IMPL - (LVL_IMMORT-1)] = {
    "Immortal",
    "Guru",
    "Half God",
    "Minor God",
    "God",
    "Greater God",
    "Builder",
    "Chief Builder",
    "Coder",
    "CoImplementor",
    "Implementor"
  };
  
        
  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';

  /* the below is from stock CircleMUD -- found no reason to rewrite it */
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);	/* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
	outlaws = TRUE;
	strcpy(buf, buf1);
	break;
      case 'z':
	who_zone = TRUE;
	strcpy(buf, buf1);
	break;
      case 'q':
	who_quest = TRUE;
	strcpy(buf, buf1);
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = TRUE;
	strcpy(buf, buf1);
	break;
      case 'c':
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      case 'i':
        nomort = TRUE;
        strcpy(buf, buf1);
        break;
      case 'm':
        noimm = TRUE;
        strcpy(buf, buf1);
        break;
      default:
	send_to_char(WHO_USAGE, ch);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(WHO_USAGE, ch);
      return;
    }
  }				/* end while (parser) */

strcpy(Imm_buf,
"\r\n    &R(&X~~~~~~~~~~~~~~~~~~~~~~~~&R=Immortals of the Realm=&X~~~~~~~~~~~~~~~~~~~~~~~~&R)&w\r\n");
//strcpy(Immp_buf,"\r\n-*----------------------------------=HEIRACHY=----------------------------------*-");   
strcpy(Mort_buf, "\r\n    &B(&W~~~~~~~~~~~~~~~~~~~~~~~~&B=_&cMortals of the Realm&B_=&W~~~~~~~~~~~~~~~~~~~~~~~~&B)\r\n&w");
  for (d = descriptor_list; d; d = d->next) {
    if (IS_UNREACH(d))
      continue;
     

    if (d->original)
      wch = d->original;
    else if (!(wch = d->character))
      continue;


    if (!CAN_SEE_ONETIME(ch, wch))
      continue;
    if (GET_LEVEL(wch) < low || GET_LEVEL(wch) > high)
      continue;
    if ((noimm && GET_LEVEL(wch) >= LVL_IMMORT) || (nomort && GET_LEVEL(wch) < LVL_IMMORT))
      continue;
    if (*name_search && str_cmp(GET_NAME(wch), name_search) && !strstr(GET_TITLE(wch), name_search))
      continue;
    if (outlaws && !PLR_FLAGGED(wch, PLR_KILLER) && !PLR_FLAGGED(wch, PLR_THIEF))
      continue;
    if (who_quest && !PRF_FLAGGED(wch, PRF_QUEST))
      continue;
    if (who_zone && world[ch->in_room].zone != world[wch->in_room].zone)
      continue;
    if (who_room && (wch->in_room != ch->in_room))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(wch))))
      continue;
      
        if (GET_LEVEL(wch) >= LVL_IMMORT) {
      if ((GET_LEVEL(ch) >= LVL_IMMORT) && (PLAYERCLAN(wch) != 0) &&
          (CLANRANK(wch) < CLAN_APPLY))
     sprintf(buf, " [%s]", CLANNAME(clan_index[PLAYERCLANNUM(wch)]));
     else
        strcpy(buf,"");
    sprintf(Imm_buf, "%s&w       .,-'^'-,.&R%s &wthe &RAlmighty &w%s of Darkened Lights.,-'^'-,.",Imm_buf, GET_NAME(wch),WizLevels[GET_LEVEL(wch)-LVL_IMMORT]); 
      Wizards++;
    } else  {
      if ((PLAYERCLAN(wch) != 0) && (CLANRANK(wch) > CLAN_APPLY)) {
        sprintf(buf, "  %s  ", CLANNAME(clan_index[PLAYERCLANNUM(wch)]));
      
      }
      else   
        strcpy(buf,"");
      sprintf(Mort_buf, "%s&B[&W%s&B]&B[&W%s&B]&B[&W%s&B]&B[ &W%d&w/&W%d &B]&g   &w%s%s", Mort_buf,
pstatus[GET_PLAYER_KILLS(wch)-1], legendstatus[GET_LEGEND_LEVELS(wch)-1],
CLASS_ABBR(wch), GET_LEVEL(wch), GET_TOT_LEVEL(wch), GET_NAME(wch), buf);
      Mortals++;
}   

    *buf = '\0'; /* **BUG FIX: Revision 2** */
     
   if (AFF_FLAGGED(wch, AFF_INVISIBLE))
      strcat(buf, "[Invis]");

    if (PLR_FLAGGED(wch, PLR_MAILING))
      strcat(buf, "&B[&WMailing&B]&w");
    else if (PLR_FLAGGED(wch, PLR_WRITING))
      strcat(buf, " (writing)");

    if (PRF_FLAGGED(wch, PRF_DEAF))
      strcat(buf, " (deaf)");
    if (PRF_FLAGGED(wch, PRF_NOTELL))
      strcat(buf, " (notell)");
    if (PRF_FLAGGED(wch, PRF_QUEST))
      strcat(buf, " (quest)");
    if (PLR_FLAGGED(wch, PLR_THIEF))
      strcat(buf, " (THIEF)");
    if (PLR_FLAGGED(wch, PLR_KILLER))
      strcat(buf, " (KILLER)");
    if (PRF_FLAGGED(wch, PRF_AFK))
        strcat(buf, " [AFK]");
    if (PLR_FLAGGED(wch, PLR_DEAD))
        strcat(buf, " &r[(DEAD)]");
    if (GET_LEVEL(wch) >= LVL_IMMORT)
      strcat(buf, CCNRM(ch, C_SPR));
    strcat(buf, "\r\n");
    
    if (GET_LEVEL(wch) >= LVL_IMMORT)
      strcat(Imm_buf, buf);
    else
      strcat(Mort_buf, buf);
  }				/* end of for */

  if (Wizards) {
    page_string(ch->desc, Imm_buf, 0);
    send_to_char("\r\n", ch);
  }
  
  if (Mortals) {
    page_string(ch->desc, Mort_buf, 0);
    send_to_char("\r\n", ch);
  }
  
  if ((Wizards + Mortals) == 0)
    strcpy(buf, "&R			    No wizards or mortals are currently visible to you.\r\n");
  if (Wizards)
    sprintf(buf, "			    There %s &C%d visible immortal%s&w\r\n%s", (Wizards == 1 ? "is" : "are"), 
Wizards, (Wizards == 1 ? "" : "s"), (Mortals ? "" : ""));
  if (Mortals)
    sprintf(buf, "%s                             And %s &G%d visible mortal%s&w.", 
(Wizards ? buf : ""), (Mortals == 1 ? "is" : "are"), Mortals, (Mortals == 1 ? "" : "s"));
  strcat(buf, "\r\n");
  
  if ((Wizards + Mortals) > boot_high)
    boot_high = Wizards+Mortals;
  sprintf(buf+strlen(buf), "			There is a boot time high of &B%d player%s&w.\r\n", boot_high, (boot_high == 1 ? "" : "s"));
  send_to_char(buf, ch);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
  const char * format = "&w%3d %-7s &g%-12s &w%-14s &W%-3s &y%-8s ";
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  size_t i;
  int low = 0, high = LVL_IMPL, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      case 'c':
	playing = 1;
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      default:
	send_to_char(USERS_FORMAT, ch);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */
  strcpy(line,
	 "&WNum Class    Name         State          Idl Login@   Site\r\n");
  strcat(line,
	 "&w--- -------- ------------ -------------- --- -------- -----------------------\r\n");
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (STATE(d) == CON_PLAYING) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (GET_INVIS_LEV(tch) > GET_LEVEL(ch))
	continue;
      if (d->character) {
        if (d->original)
  	  sprintf(classname, "&w[&c%3d &m%s&w]", GET_LEVEL(d->original),
 	    CLASS_ABBR(d->original));
        else
	  sprintf(classname, "&w[&c%3d &m%s&w]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character));
      }
    } else
      if (GET_CLASS(d->character) > CLASS_UNDEFINED)
        sprintf(classname, "&w[&c%3d &m%s&w]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character));
      else strcpy(classname, "&w    -   ");

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (STATE(d) == CON_PLAYING && d->original)
      strcpy(state, "&WSwitched");
    else
      strcpy(state, connected_types[STATE(d)]);

    if (d->character && STATE(d) == CON_PLAYING && GET_LEVEL(d->character) < LVL_GOD)
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    if (d->character && d->character->player.name) {
      if (d->original)
	sprintf(line, format, d->desc_num, classname,
		d->original->player.name, state, idletime, timeptr);
      else
	sprintf(line, format, d->desc_num, classname,
		d->character->player.name, state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED",
	      state, idletime, timeptr);

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (STATE(d) != CON_PLAYING) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (STATE(d) != CON_PLAYING ||
		(STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
      send_to_char(line, ch);
      num_can_see++;
    }
  }

  sprintf(line, "\r\n&Y%d&w visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(strcat(strcpy(buf, circlemud_version), "\r\n"), ch);
    break;
  case SCMD_BLDBOOK:
    send_to_char(buildbook, ch);
    break;
  case SCMD_WHOAMI:
    send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
    break;
  case SCMD_CLANMOTD:
    if (CLANMOTD(GET_CLAN(ch)))
      page_string(ch->desc, CLANMOTD(GET_CLAN(ch)), 0);
    break;
  default:
    return;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct descriptor_data *d;

  if (!*arg) {
    send_to_char("Players in your Zone\r\n--------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (i->in_room == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[ch->in_room].zone != world[i->in_room].zone)
	continue;
	  sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	  send_to_char(buf, ch);
	}
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (i->in_room == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[i->in_room].zone != world[ch->in_room].zone)
	continue;
      if (!isname(arg, i->player.name))
	continue;
	sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
	send_to_char(buf, ch);
	return;
      }
    send_to_char("No-one around by that name.\r\n", ch);
  }
}


int print_object_location(int num, struct obj_data * obj, struct char_data * ch,
			        int recur)
{
  if (num > 128) {
    send_to_char("&R**OVERFLOW**&w (Hint: Be more specific)\r\n", ch);
    return 0;
  }
  if (num > 0)
    sprintf(buf, "O%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(buf, "%33s", " - ");

  if (obj->in_room > NOWHERE) {
    sprintf(buf + strlen(buf), "[%5d] %s\r\n",
	    GET_ROOM_VNUM(IN_ROOM(obj)), world[obj->in_room].name);
    send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(buf + strlen(buf), "carried by %s\r\n",
	    PERS(obj->carried_by, ch));
    send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(buf + strlen(buf), "worn by %s\r\n",
	    PERS(obj->worn_by, ch));
    send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(buf + strlen(buf), "inside %s%s\r\n",
	    obj->in_obj->short_description, (recur ? ", which is" : " "));
    send_to_char(buf, ch);
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else {
    sprintf(buf + strlen(buf), "in an unknown location\r\n");
    send_to_char(buf, ch);
  }
  return 1;
}


void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0, vnum = 0;

  if (!*arg) {
    send_to_char("&WPlayers\r\n&w-------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (STATE(d) == CON_PLAYING) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
	  if (d->original)
	    sprintf(buf, "&M%-20s&w - [&c%5d&w] %s (in %s)\r\n",
		    GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		 world[d->character->in_room].name, GET_NAME(d->character));
	  else
	    sprintf(buf, "&M%-20s&w - [&c%5d&w] %s\r\n", GET_NAME(i),
		    GET_ROOM_VNUM(IN_ROOM(i)), world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {
 
    two_arguments(arg, buf, buf1);
   
    if (is_abbrev(buf, "obj")) {
      vnum = atoi(buf1);
      for (num = 0, k = object_list; k; k = k->next)
      if (GET_OBJ_VNUM(k) == vnum) {
	found = 1;
	if (!print_object_location(++num, k, ch, TRUE)) return;
      }
    } else
      if (is_abbrev(buf, "mob")) {
      vnum = atoi(buf1);
      for (i = character_list; i; i = i->next)
          if (GET_MOB_VNUM(i) == vnum) {
    	  found = 1;
	  sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
	  	GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	  send_to_char(buf, ch);
        }
    } else {
      for (i = character_list; i; i = i->next)
          if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname_list(arg, i->player.name)) {
    	  found = 1;
	  sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
	  	GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	  send_to_char(buf, ch);
        }
      for (num = 0, k = object_list; k; k = k->next)
        if (CAN_SEE_OBJ(ch, k) && isname_list(arg, k->name)) {
  	  found = 1;
 	  if (!print_object_location(++num, k, ch, TRUE)) return;
        }
    }
    if (!found)
      send_to_char("Couldn't find any such thing.\r\n", ch);
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    perform_immort_where(ch, argument);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  int i;

  if (IS_NPC(ch)) {
    send_to_char("You ain't nothin' but a hound-dog.\r\n", ch);
    return;
  }
  *buf = '\0';

  for (i = 1; i <= MAX_MORT_LEVEL; i++) {
    sprintf(buf + strlen(buf), "&w[&Y%3d&w] &c%8d-%-8d&w : &M", i,
	    level_exp(GET_CLASS(ch), i), 
	    i < MAX_MORT_LEVEL ? level_exp(GET_CLASS(ch), i+1) - 1 : 
	    level_exp(GET_CLASS(ch), LVL_IMMORT) - 1);
    switch (GET_SEX(ch)) {
    case SEX_MALE:
    case SEX_NEUTRAL:
      strcat(buf, title_male(GET_CLASS(ch), i));
      break;
    case SEX_FEMALE:
      strcat(buf, title_female(GET_CLASS(ch), i));
      break;
    default:
      send_to_char("Oh dear.  You seem to be sexless.\r\n", ch);
      break;
    }
    strcat(buf, "&w\r\n");
  }
  sprintf(buf + strlen(buf), "&w[&Y%3d&w] &c%8d          &w:&M Immortality&w\r\n",
	  LVL_IMMORT, level_exp(GET_CLASS(ch), LVL_IMMORT));
  page_string(ch->desc, buf, 1);
}



ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("Easy!  Very easy indeed!\r\n", ch);
    return;
  }
  
  if (!IS_NPC(victim)) {
    send_to_char("&RWould you like to borrow a cross and a shovel?&w\r\n", ch);
  }
  
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  send_to_char("&w[LV] ",ch);
  if (diff <= -10)
    send_to_char("&GNow where did that chicken go?\r\n", ch);
  else if (diff <= -5)
    send_to_char("&GYou could do it with a needle!\r\n", ch);
  else if (diff <= -2)
    send_to_char("&GEasy.\r\n", ch);
  else if (diff <= -1)
    send_to_char("&GFairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("&GThe perfect match!\r\n", ch);
  else if (diff <= 1)
    send_to_char("&YYou would need some luck!\r\n", ch);
  else if (diff <= 2)
    send_to_char("&YYou would need a lot of luck!\r\n", ch);
  else if (diff <= 3)
    send_to_char("&YYou would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 5)
    send_to_char("&RDo you feel lucky, punk?\r\n", ch);
  else if (diff <= 10)
    send_to_char("&RAre you mad!?\r\n", ch);
  else if (diff <= 100)
    send_to_char("&RYou ARE mad!\r\n", ch);
    
  send_to_char("&w[AC] ",ch);
  diff = -(GET_AC(victim)-GET_AC(ch));
  if (diff < -100)
    send_to_char("&GYour opponent is almost naked in comparison to you!", ch);
  else if (diff <= -30)
    send_to_char("&GYour opponent is totally weak armored.", ch);
  else if (diff <= -15)
    send_to_char("&GYou are much better armored than your opponent.", ch);
  else if (diff <=  5)
    send_to_char("&GYou are slightly better armored than your opponent.", ch);
  else if (diff <= 5)
    send_to_char("&GYour armors are nearly equal.", ch);
  else if (diff <= 15)
    send_to_char("&RYour armor is a bit worse.", ch);
  else if (diff <= 30)
    send_to_char("&RYour armor is much worse.", ch);   
  else if (diff <=  100)
    send_to_char("&RYou are as badly armored as it could be.", ch);
  else if (diff <=  200)
    send_to_char("&RYou are almost naked in comparison to your oppenent.", ch);
    
  send_to_char("\r\n&w[HP] ",ch);
  diff = GET_HIT(victim) - GET_HIT(ch);
  if (diff >= 0) {
    if (diff > GET_HIT(ch)) 
      send_to_char("&RYour chances are very weak.", ch);
    else
      send_to_char("&RYour constitution is weaker.", ch);
    
  } else {
    if (diff > -GET_HIT(ch)) 
      send_to_char("&GNo problemo.", ch);
    else
      send_to_char("&GYour constitution is better.", ch);
    
  }
  send_to_char("\r\n",ch);
}



ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    } else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}


const char *ctypes[] = {
  "off", "sparse", "normal", "complete", "\n"
};

ACMD(do_color)
{
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
  SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));

  sprintf(buf, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
	  CCNRM(ch, C_OFF), ctypes[tp]);
  send_to_char(buf, ch);
}


ACMD(do_toggle)
{
  if (IS_NPC(ch))
    return;
  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "&ROFF&w");
  else
    sprintf(buf2, "&Y%-3d&w", GET_WIMP_LEV(ch));

  sprintf(buf,
	  "       Autoloot: %s    "
	  "     Brief Mode: %s    "
	  " Summon Protect: %s\r\n"

	  "       Autogold: %s    "
	  "   Compact Mode: %s    "
	  "       On Quest: %s\r\n"

	  "     AutoAssist: %s    "
	  "         NoTell: %s    "
	  "   Repeat Comm.: %s\r\n"

	  " Auto Show Exit: %s    "
	  "           Deaf: %s    "
	  "     Wimp Level: %s\r\n"

	  " Gossip Channel: %s    "
	  "Auction Channel: %s    "
	  "  Grats Channel: %s\r\n"
          
          "     Exam. Unit: %s    "
          "      AutoSplit: %s    "
          "     Show Clans: %s\r\n"
          
          " Auto mail rec.: %s    "
	  "    Color Level: %s\r\n",

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	  YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
	  ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
	  YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

          ONOFF(PRF_FLAGGED(ch, PRF_EXAMUNIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_SHOWCLAN)),
          
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOMAIL)),

	  ctypes[COLOR_LEV(ch)]);

  send_to_char(buf, ch);
}


struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
  int a, b, tmp;

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  /* create data array */
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

  /* initialize it */
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }

  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}



ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char("You can't see the commands of people above your level.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  sprintf(buf, "&WThe following %s%s are available to %s:&w\r\n",
	  wizhelp ? "&Cprivileged&W " : "",
	  socials ? "&msocials&W" : "&ccommands&W",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
    i = cmd_sort_info[cmd_num].sort_pos;
    if (cmd_info[i].minimum_level >= 0 &&
	GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
	(cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp &&
	(wizhelp || socials == cmd_sort_info[i].is_social)) {
      sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
      if (!(no % 7))
	strcat(buf, "\r\n");
      no++;
    }
  }

  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}
 
/* utils.h: #define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door]) */
#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])


ACMD(do_scan)
{
  /* >scan
     You quickly scan the area.
     You see John, a large horse and Frank close by north.
     You see a small rabbit a ways off south.
     You see a huge dragon and a griffon far off to the west.

  */
  int door;
  
  *buf = '\0';
  
  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  /* may want to add more restrictions here, too */
  send_to_char("&WYou focus your eyesight and scan the surrounding area.&w\r\n", ch);
  for (door = 0; door < NUM_OF_DIRS - 2; door++) /* don't scan up/down */
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)
        && !IS_DARK(EXIT(ch, door)->to_room)) {
      if (world[EXIT(ch, door)->to_room].people) {
	list_scanned_chars(world[EXIT(ch, door)->to_room].people, ch, 0, door);
      } else if (_2ND_EXIT(ch, door) && _2ND_EXIT(ch, door)->to_room != 
		 NOWHERE && !IS_SET(_2ND_EXIT(ch, door)->exit_info, EX_CLOSED)
                 && !IS_DARK(_2ND_EXIT(ch, door)->to_room)) {
   /* check the second room away */
	if (world[_2ND_EXIT(ch, door)->to_room].people) {
	  list_scanned_chars(world[_2ND_EXIT(ch, door)->to_room].people, ch, 1, door);
	} else if (_3RD_EXIT(ch, door) && _3RD_EXIT(ch, door)->to_room !=
		   NOWHERE && !IS_SET(_3RD_EXIT(ch, door)->exit_info, EX_CLOSED)
                   && !IS_DARK(_3RD_EXIT(ch, door)->to_room)) {
	  /* check the third room */
	  if (world[_3RD_EXIT(ch, door)->to_room].people) {
	    list_scanned_chars(world[_3RD_EXIT(ch, door)->to_room].people, ch, 2, door);
	  }

	}
      }
    }                
}

ACMD(do_players)
{
  int i, count = 0;
  *buf = 0;
 
  for (i = 0; i <= top_of_p_table; i++) {
    sprintf(buf, "%s%s\r\n",buf,(player_table + i)->name);
    count++;
    if (count == 3) {
      count = 0;
      strcat(buf, "\r\n");
    }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}

ACMD(do_affects)
{
	struct affected_type* af;
	int type=1;
	char sname[256];
	char added_aff[MAX_STRING_LENGTH];
	extern char *spells[];

	send_to_char("You carry these affections: \r\n", ch);

	for (af = ch->affected; af; af = af->next) {
		
		strcpy(sname, spells[af->type]);
		strcat(sname, ":");
		if (GET_LEVEL(ch) >= 7) {  // first state is here
			sprintf(buf, "   %s%-22s%s    affects %s%s%s by %s%d%s for %s%d%s hours\r\n", 
				CCCYN(ch, C_NRM),	(type ? sname : ""), CCNRM(ch, C_NRM),
				CCCYN(ch, C_NRM),
				((GET_LEVEL(ch)>=14 && af->location) ? apply_types[(sh_int) af->location] : "Something"),  // second state is here
				CCNRM(ch, C_NRM),
				CCCYN(ch, C_NRM), af->modifier, CCNRM(ch, C_NRM),
				CCCYN(ch, C_NRM), af->duration, CCNRM(ch, C_NRM));			
			send_to_char(buf, ch);
			if (GET_LEVEL(ch) >= 21 && ((af->bitvector[0] || af->bitvector[1] ||
			        af->bitvector[2] || af->bitvector[3]) && 
			        (!af->next || af->next->bitvector[0] != af->bitvector[0] ||
			        af->next->bitvector[1] != af->bitvector[1] ||
			        af->next->bitvector[2] != af->bitvector[2] ||
			        af->next->bitvector[3] != af->bitvector[3]))) {  // third state is here
				sprintbit_multi(af->bitvector, affected_bits, added_aff);
				sprintf(buf1, "%35sadds %s%s\r\n", CCCYN(ch, C_NRM), added_aff, CCNRM(ch, C_NRM));
				send_to_char(buf1, ch);
			}
		}
		else if (type){
			sprintf(buf, "   %s%-25s%s\r\n", CCCYN(ch, C_NRM), sname, CCNRM(ch, C_NRM));
			send_to_char(buf, ch);
		}
		type = af->next ? (af->next->type != af->type) : 1;
	}
}

ACMD(do_areas)
{
  int i;
  sprintf(buf, "[&W  # Area/Zone                    &gCreator&w&&&mEditors&w         ]\r\n");
  for (i = 0; i<=top_of_zone_table; i++) {
    if (!(zone_table[i].zon_flags && ZON_NO_LIST)) 
      sprintf(buf,"%s&C%4d&c %-28s &g%s &m%s&w\r\n", buf,
        zone_table[i].number, zone_table[i].name, zone_table[i].creator,
        zone_table[i].builders ? zone_table[i].builders : "");
  }
  page_string(ch->desc, buf, 1);
}

ACMD(do_spells)
{
  char *arg = argument;
  int class;
  
  skip_spaces(&arg);
  if (!*arg && !IS_NPC(ch))
    class = GET_CLASS(ch);
  else
    class = cmp_class(arg);
  if (class == -1) {
    send_to_char("List spells of which class?\r\n", ch);
    return;
  }
   
  page_string(ch->desc, spell_tables[class], 0);
}
