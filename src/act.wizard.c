/* 
************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
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
#include "house.h"
#include "screen.h"
#include "olc.h"
#include "teleport.h"
#include "constants.h"
#include "clan.h"

/*   external vars  */
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct zone_data *zone_table;
extern struct attack_hit_type attack_hit_text[];
extern char *class_abbrevs[];
extern time_t boot_time;
extern int top_of_zone_table;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int top_of_world;
extern int buf_switches, buf_largecount, buf_overflows;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;
extern struct specproc_info mob_procs[];
extern struct specproc_info obj_procs[];
extern struct specproc_info room_procs[];
extern FILE *player_fl;
extern struct raff_node *raff_list;
extern int load_into_inventory;
extern struct time_info_data time_info;
extern int max_level_diff;

/* external functions */
void die(struct char_data * ch, struct char_data * killer);

/* local functions */
int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
void perform_immort_invis(struct char_data *ch, int level);
ACMD(do_echo);
ACMD(do_send);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
void do_stat_room(struct char_data * ch);
void do_stat_object(struct char_data * ch, struct obj_data * j);
void do_stat_character(struct char_data * ch, struct char_data * k);
int  can_edit_zone(struct char_data *ch, int number);
ACMD(do_stat);
ACMD(do_shutdown);
void stop_snooping(struct char_data * ch);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
void perform_immort_vis(struct char_data *ch);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
void print_zone_to_buf(char *bufptr, int zone);
ACMD(do_show);
ACMD(do_set);
int real_zone(int number);

/* for chars */
extern const char *spells[];
extern const char *pc_class_types[];
/* for rooms */

/* extern functions */
void save_world_settings(void);
void Read_Invalid_List(void);
int level_exp(int chclass, int level);
void show_shops(struct char_data * ch, char *value);
void hcontrol_list_houses(struct char_data *ch);
void do_start(struct char_data *ch);
void appear(struct char_data *ch);
void reset_zone(int zone);
void roll_real_abils(struct char_data *ch);
int parse_class(char arg);
int is_empty(int zone_nr);
void unload_room_descs(int zone);

#define CHECK_OLC_PERM(ch, zone) \
  { if (!can_edit_zone(ch, real_zone(zone))) { \
    send_to_char("You have not permission to edit that zone.\r\n", ch); \
    return; \
    } \
  }

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes.. but what?\r\n", ch);
  else {
    if (subcmd == SCMD_EMOTE)
      sprintf(buf, "$n %s", argument);
    else
      strcpy(buf, argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (!IS_NPC(ch)) {
      if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
        send_to_char(OK, ch);
      } else {
        act(buf, FALSE, ch, 0, 0, TO_CHAR);
      }
    }
  }
}



ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Send what to who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  send_to_char(buf, vict);
  send_to_char("\r\n", vict);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("Sent.\r\n", ch);
  else {
    sprintf(buf2, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
    send_to_char(buf2, ch);
  }
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data * ch, char *rawroomstr)
{
  int tmp;
  sh_int location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char("A name please!\r\n", ch);
    return NOWHERE;
  }
  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return NOWHERE;
    }
  } else if ((target_mob = get_char_vis(ch, roomstr)))
    location = target_mob->in_room;
  else if ((target_obj = get_obj_vis(ch, roomstr))) {
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("That object is not available.\r\n", ch);
      return NOWHERE;
    }
  } else {
    send_to_char("No such creature or object around.\r\n", ch);
    return NOWHERE;
  }

  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (GET_LEVEL(ch) < LVL_GRGOD) {
    if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	world[location].people && world[location].people->next_in_room) {
      send_to_char("There's a private conversation going on in that room.\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
	!House_can_enter(ch, GET_ROOM_VNUM(location))) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return NOWHERE;
    }
  }
  return location;
}
 
ACMD(do_scry)
{
  char command[MAX_INPUT_LENGTH];
  int location, original_loc;
  ACMD(do_look);
  
  if (GET_CLASS(ch) != CLASS_WARRIOR) {
      send_to_char("Huh?!?\r\n",ch);
      return;
      }
   
   
          if (GET_HYBRID(ch) < 3) {
     send_to_char("Scrying requires level 3 hybrid.\r\n",ch);
     return;
}

if (GET_HYBRID(ch) > 2) {

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("You must supply the name of the person you wish to locate.\r\n", ch);
    return;
  }


  if ((location = find_target_room(ch, buf)) < 0)
    return;

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);
  do_look(ch, "", 0, 0);

  /* check if the char is still there */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
    }
  } 
}
ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  int location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("A name please!\r\n", ch);
    return;
  }

  if (!*command) {
    send_to_char("What do you want to do there?\r\n", ch);
    return;
  }

  if ((location = find_target_room(ch, buf)) < 0)
    return;

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  sh_int location;

  if ((location = find_target_room(ch, argument)) < 0)
    return;

  if (POOFOUT(ch))
    sprintf(buf, "$n %s", POOFOUT(ch));
  else
    strcpy(buf, "$n disappears in a puff of smoke.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);

  if (POOFIN(ch))
    sprintf(buf, "$n %s", POOFIN(ch));
  else
    strcpy(buf, "$n appears with an ear-splitting bang.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}



ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to transfer?\r\n", ch);
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf)))
      send_to_char(NOPERSON, ch);
    else if (victim == ch)
      send_to_char("That doesn't make much sense, does it?\r\n", ch);
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char("Go transfer someone your own size.\r\n", ch);
	return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, ch->in_room);
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char("I think not.\r\n", ch);
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, ch->in_room);
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
      }
    send_to_char(OK, ch);
  }
}



ACMD(do_teleport)
{
  struct char_data *victim;
  sh_int target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char("Whom do you wish to teleport?\r\n", ch);
  else if (!(victim = get_char_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (victim == ch)
    send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("Maybe you shouldn't do that.\r\n", ch);
  else if (!*buf2)
    send_to_char("Where do you wish to send this person?\r\n", ch);
  else if ((target = find_target_room(ch, buf2)) >= 0) {
    send_to_char(OK, ch);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
  }
}



ACMD(do_vnum)
{
  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj"))) {
    send_to_char("Usage: vnum { obj | mob } <name>\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
      send_to_char("No mobiles by that name.\r\n", ch);

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(buf2, ch))
      send_to_char("No objects by that name.\r\n", ch);
}



void do_stat_room(struct char_data * ch)
{
  struct extra_descr_data *desc;
  struct room_data *rm = &world[ch->in_room];
  int i, found = 0;
  struct obj_data *j = 0;
  struct char_data *k = 0;
  struct raff_node *raff;

  sprintf(buf, "&wRoom name: &C%s&w\r\n", rm->name);
  send_to_char(buf, ch);

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "Zone: [&g%3d&w], VNum: [&W%5d&w], RNum: [&W%5d&w], Type: &c%s&w\r\n",
	  zone_table[rm->zone].number, rm->number, ch->in_room, buf2);
  send_to_char(buf, ch);

  sprintbit_multi(rm->room_flags, room_bits, buf2);
  sprintf(buf, "SpecProc: &c%s&w, Flags: &g%s&w\r\n",
	  room_procs[get_spec_name(room_procs, rm->func)].name, buf2);
  send_to_char(buf, ch);
  sprintbit((long) rm->room_affections, room_affections, buf2);
  sprintf(buf, "Room affections: &g%s&w\r\n", buf2);
  send_to_char(buf, ch);
  
  for (raff = raff_list; raff; raff = raff->next)
    if (raff->room == ch->in_room) {
      sprintbit(raff->affection, room_affections, buf1);
      sprintf(buf, "ROOMAFF SPL: (%3dhr) &g%-21s&w sets &c%s&w\r\n",
      raff->timer + 1, spells[raff->spell], buf1);
      send_to_char(buf, ch);
    }
  
  if (rm->tele != NULL) {
        sprintf(buf, "Teleports every %d0 (current %d0) seconds to %20s (Room %d)\r\n",
                rm->tele->time, rm->tele->cnt,
                world[real_room(rm->tele->targ)].name,  rm->tele->targ);
        send_to_char(buf, ch);
        send_to_char("Teleport Flags   :", ch);
        sprintbit(rm->tele->mask, teleport_bits, buf);
        send_to_char(buf, ch);
        send_to_char("\r\n",ch);
        if (IS_SET(rm->tele->mask, TELE_OBJ) ||
            IS_SET(rm->tele->mask, TELE_NOOBJ)) {
          j = read_object(rm->tele->obj, VIRTUAL);
          sprintf(buf, "Teleport Object : %s\r\n", j->short_description);
          send_to_char(buf, ch);
          extract_obj(j);
        }
  }
  send_to_char("&wDescription:&c\r\n", ch);
  if (rm->description)
    send_to_char(rm->description, ch);
  else
    send_to_char("&r  None.\r\n", ch);

  if (rm->ex_description) {
    sprintf(buf, "&wExtra descs:&c");
    for (desc = rm->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    send_to_char(strcat(buf, "&w\r\n"), ch);
  }
  sprintf(buf, "&wChars present:&y");
  for (found = 0, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;
    sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
	    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (k->next_in_room)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "&w\r\n"), ch);

  if (rm->contents) {
    sprintf(buf, "&wContents:&g");
    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;
      sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (rm->dir_option[i]) {
      if (rm->dir_option[i]->to_room == NOWHERE)
	sprintf(buf1, " &rNONE&w");
      else
	sprintf(buf1, "&c%5d&w", 
		GET_ROOM_VNUM(rm->dir_option[i]->to_room));
      sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
      sprintf(buf, "&wExit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: &g%s&w\r\n ",
	      CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1, rm->dir_option[i]->key,
	   rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None",
	      buf2);
      send_to_char(buf, ch);
      if (rm->dir_option[i]->general_description)
	strcpy(buf, rm->dir_option[i]->general_description);
      else
	strcpy(buf, "  No exit description.\r\n");
      send_to_char(buf, ch);
    }
  }
}



void do_stat_object(struct char_data * ch, struct obj_data * j)
{
  int i, vnum, found;
  struct obj_data *j2;
  struct extra_descr_data *desc;
  char *owner_name_ptr;
  char owner_name[MAX_NAME_LENGTH+1];

  vnum = GET_OBJ_VNUM(j);
  sprintf(buf, "Name: '&C%s&w', Aliases: %s\r\n",
	  ((j->short_description) ? j->short_description : "<None>"),
	  j->name);
  send_to_char(buf, ch);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  if (GET_OBJ_RNUM(j) >= 0) {
    strcpy(buf2, obj_procs[GET_OBJ_SPEC_INDEX(j)].name);
  }
  else
    strcpy(buf2, "NONE");
  sprintf(buf, "VNum: [&W%5d&w], RNum: [&W%5d&w], Type: &c%s&w, SpecProc: &c%s&w\r\n",
   vnum, GET_OBJ_RNUM(j), buf1, buf2);
  send_to_char(buf, ch);
  sprintf(buf, "L-Des: %s\r\n", ((j->description) ? j->description : "None"));
  send_to_char(buf, ch);

  if (j->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = j->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  send_to_char("Can be worn on: &g", ch);
  sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
  strcat(buf, "&w\r\n");
  send_to_char(buf, ch);

  send_to_char("Set char bits : &y", ch);
  sprintbit_multi(j->obj_flags.bitvector, affected_bits, buf);
  strcat(buf, "&w\r\n");
  send_to_char(buf, ch);

  send_to_char("Extra flags   : &c", ch);
  sprintbit_multi(GET_OBJ_EXTRA_BITV(j), extra_bits, buf);
  strcat(buf, "&w\r\n");
  send_to_char(buf, ch);

  sprintf(buf, "Weight: &g%d&w, Value: &y%d&w, Cost/day: &y%d&w, Timer: &c%d&w, "
  "Levels: [&m%d-%d&w]\r\n",
     GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), 
     GET_OBJ_BOTTOM_LEV(j), GET_OBJ_TOP_LEV(j));
  send_to_char(buf, ch);

  strcpy(buf, "In room: &r");
  if (j->in_room == NOWHERE)
    strcat(buf, "Nowhere");
  else {
    sprintf(buf2, "%d", GET_ROOM_VNUM(IN_ROOM(j)));
    strcat(buf, buf2);
  }
  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  strcat(buf, "&w, In object: &r");
  strcat(buf, j->in_obj ? j->in_obj->short_description : "None");
  strcat(buf, "&w, Carried by: &g");
  strcat(buf, j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  strcat(buf, "&w, Worn by: &c");
  strcat(buf, j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
  strcat(buf, "&w\r\n");
  send_to_char(buf, ch);

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, 2) == -1)
      strcpy(buf, "Hours left: Infinite");
    else
      sprintf(buf, "Hours left: [&c%d&w]", GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    sprintf(buf, "Spells: (Level &c%d&w) &g%s&w, &g%s&w, &g%s&w", GET_OBJ_VAL(j, 0),
	    skill_name(GET_OBJ_VAL(j, 1)), skill_name(GET_OBJ_VAL(j, 2)),
	    skill_name(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprintf(buf, "Spell: &g%s&w at level &c%d&w, &g%d&w (of &g%d&w) charges remaining",
	    skill_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_THROW:
  case ITEM_ROCK:
  case ITEM_BOLT:
  case ITEM_ARROW:
    sprintf(buf, "Number dam dice: &c%d&w Size dam dice: &c%d&w",
           GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
    break;
  case ITEM_GRENADE:
    sprintf(buf, "Timer: &c%d&w Num dam dice: &c%d&w Size dam dice: &c%d&w",
            GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
    break;
  case ITEM_BOW:
  case ITEM_CROSSBOW:
  case ITEM_SLING:
    sprintf(buf, "Range: &c%d&w", GET_OBJ_VAL(j, 1));
    break;
  case ITEM_WEAPON:
    sprintf(buf, "Todam: &c%dd%d&w, Message type: &g%d&w",
	    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_ARMOR:
    sprintf(buf, "AC-apply: [&c%d&w]", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_TRAP:
    sprintf(buf, "Spell: %d, - Hitpoints: %d",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_CONTAINER:
    sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
    sprintf(buf, "Weight capacity: &c%d&w, Lock Type: &c%s&w, Key Num: &g%d&w, Corpse: %s",
	    GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2),
	    YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
    sprintf(buf, "Capacity: &c%d&w, Contains: &c%d&w, Poisoned: %s, Liquid: &g%s&w",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), YESNO(GET_OBJ_VAL(j, 3)),
	    buf2);
    break;
  case ITEM_NOTE:
    sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    strcpy(buf, "");
    break;
  case ITEM_FOOD:
    sprintf(buf, "Makes full: &c%d&w, Poisoned: %s", GET_OBJ_VAL(j, 0),
	    YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_MONEY:
    sprintf(buf, "Coins: &c%d&w", GET_OBJ_VAL(j, 0));
    break;
  default:
    sprintf(buf, "Values 0-3: [&c%d&w] [&c%d&w] [&c%d&w] [&c%d&w]",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains) {
    sprintf(buf, "\r\nContents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j2->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  found = 0;
  send_to_char("Affections:", ch);
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s %+d to %s", found++ ? "," : "",
	      j->affected[i].modifier, buf2);
      send_to_char(buf, ch);
    }
  if (!found)
    send_to_char(" None", ch);
  
  send_to_char("\r\n", ch);
  
  if (IS_OBJ_STAT(j, ITEM_ENGRAVED)) {
    owner_name_ptr = get_name_by_id(GET_OBJ_OWNER_ID(j));
    if (owner_name_ptr)
      strcpy(owner_name, owner_name_ptr);
    else
      strcpy(owner_name, "Unknown person!");
    sprintf(buf, "Engraved to: &y%s&w\r\n", CAP(owner_name));
    send_to_char(buf, ch);
  }
}


void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;
  

  switch (GET_SEX(k)) {
  case SEX_NEUTRAL:    strcpy(buf, "NEUTRAL-SEX");   break;
  case SEX_MALE:       strcpy(buf, "MALE");          break;
  case SEX_FEMALE:     strcpy(buf, "FEMALE");        break;
  default:             strcpy(buf, "ILLEGAL-SEX!!"); break;
  }

  sprintf(buf2, " %s '&C%s&w'  IDNum: [&W%5ld&w], In room [&W%5d&w]\r\n",
	  (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	  GET_NAME(k), GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)));
  send_to_char(strcat(buf, buf2), ch);
  if (IS_MOB(k)) {
    sprintf(buf, "Alias: &g%s&w, VNum: [&W%5d&w], RNum: [&W%5d&w]\r\n",
	    k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    send_to_char(buf, ch);
  }
  sprintf(buf, "Title: %s\r\n", (k->player.title ? k->player.title : "<None>"));
  send_to_char(buf, ch);

  sprintf(buf, "L-Des: %s", (k->player.long_descr ? k->player.long_descr : "<None>\r\n"));
  send_to_char(buf, ch);

  if (IS_NPC(k)) {  /* Use GET_CLASS() macro? */
    strcpy(buf, "Monster Class: &c");
    sprinttype(k->player.chclass, npc_class_types, buf2);
  } else {
    strcpy(buf, "Class: &c");
    sprinttype(k->player.chclass, pc_class_types, buf2);
  }
  strcat(buf, buf2);
  strcat(buf, "&w");

  sprintf(buf2, ", Lev: [&R%2d&w], XP: [&R%7d&w], Align: [&R%4d&w]",
	  GET_LEVEL(k), GET_EXP(k), GET_ALIGNMENT(k));
  strcat(buf, buf2);
  if (!IS_NPC(k)) {
    sprintf(buf2, ", Quest: [&Y%3d&w]\r\n", QPOINTS(k));
    strcat(buf, buf2);
  } else strcat(buf, "\r\n");
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
    strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
    buf1[10] = buf2[10] = '\0';

    sprintf(buf, "Created: [&g%s&w], Last Logon: [&g%s&w], Played [&c%dh %dm&w], Age [&c%d&w]\r\n",
	    buf1, buf2, k->player.time.played / 3600,
	    ((k->player.time.played % 3600) / 60), age(k)->year);
    send_to_char(buf, ch);

    sprintf(buf, "Hometown: [&W%d&w], Speaks: [&b%d/%d/%d&w], (STL[&g%d&w]/per[&g%d&w]/NSTL[&g%d&w]), Clan: [&g%d&w]",
	 k->player.hometown, GET_TALK(k, 0), GET_TALK(k, 1), GET_TALK(k, 2),
	    GET_PRACTICES(k), int_app[GET_INT(k)].learn,
	    wis_app[GET_WIS(k)].bonus, PLAYERCLAN(k));
    /*. Display OLC zone for immorts .*/
    if(GET_LEVEL(k) >= LVL_IMMORT)
      sprintf(buf + strlen(buf), ", OLC[&Y%d&w]", GET_OLC_ZONE(k));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
  }
  sprintf(buf, "Str: [%s%d/%d%s]  Int: [%s%d%s]  Wis: [%s%d%s]  "
	  "Dex: [%s%d%s]  Con: [%s%d%s]  Cha: [%s%d%s]\r\n",
	  CCCYN(ch, C_NRM), GET_STR(k), GET_ADD(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_INT(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_WIS(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_DEX(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_CON(k), CCNRM(ch, C_NRM),
	  CCCYN(ch, C_NRM), GET_CHA(k), CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprintf(buf, "Hit p.:[%s%d/%d+%d%s]  Mana p.:[%s%d/%d+%d%s]  Move p.:[%s%d/%d+%d%s]\r\n",
	  CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k), CCNRM(ch, C_NRM),
	  CCGRN(ch, C_NRM), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k), CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprintf(buf, "Coins: [&W%9d&w], Bank: [&W%9d&w] (Total: &c%d&w)\r\n",
	  GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));
  send_to_char(buf, ch);

  sprintf(buf, "AC: [&W%d&w/10], Hitroll: [&c%2d&w], Damroll: [&c%2d&w], Saving throws: [%d/%d/%d/%d/%d]\r\n",
	  GET_AC(k), k->points.hitroll, k->points.damroll, GET_SAVE(k, 0),
	  GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3), GET_SAVE(k, 4));
  send_to_char(buf, ch);

  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "Pos: %s, Fighting: %s", buf2,
	  (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));

  if (IS_NPC(k)) {
    strcat(buf, ", Attack type: ");
    strcat(buf, attack_hit_text[(int) (k->mob_specials.attack_type)].singular);
  }
  if (k->desc) {
    sprinttype(STATE(k->desc), connected_types, buf2);
    strcat(buf, ", Connected: ");
    strcat(buf, buf2);
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  strcpy(buf, "Default position: ");
  sprinttype((k->mob_specials.default_pos), position_types, buf2);
  strcat(buf, buf2);

  sprintf(buf2, ", Idle Timer (in tics) [%d]\r\n", k->char_specials.timer);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (IS_NPC(k)) {
    sprintbit_multi(MOB_FLAGS_BITV(k), action_bits, buf2);
    sprintf(buf, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
  } else {
    sprintbit_multi(PLR_FLAGS_BITV(k), player_bits, buf2);
    sprintf(buf, "PLR: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit_multi(PRF_FLAGS_BITV(k), preference_bits, buf2);
    sprintf(buf, "PRF: %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    if (PLAYERCLAN(k) != 0) {
      sprintf(buf, "Clan: [&c%2d&w], Rank: [&g%2d&w]\r\n", PLAYERCLAN(k), CLANRANK(k));
      send_to_char(buf, ch);
    }
  }

  if (IS_MOB(k)) {
    sprintf(buf, "Mob Spec-Proc: &c%s&w, NPC Bare Hand Dam: &g%dd%d&w\r\n",
	    (mob_procs[get_spec_name(mob_procs, mob_index[GET_MOB_RNUM(k)].func)].name),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice);
    send_to_char(buf, ch);
  }
  sprintf(buf, "Carried: weight: %d, items: %d; ",
	  IS_CARRYING_W(k), IS_CARRYING_N(k));

  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
  sprintf(buf + strlen(buf), "Items in: inventory: %d, ", i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  sprintf(buf2, "eq: %d\r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    sprintf(buf, "Hunger: %d, Thirst: %d, Drunk: %d\r\n",
	  GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
    send_to_char(buf, ch);
    sprintf(buf, "Rip: [%d], Kills: [%d], DeathTraps: [%d]\r\n",
    GET_RIP_CNT(k), GET_KILL_CNT(k), GET_DT_CNT(k));
    send_to_char(buf, ch);
  }
  sprintf(buf, "Master is: %s, Followers are:",
	  ((k->master) ? GET_NAME(k->master) : "<none>"));

  for (fol = k->followers; fol; fol = fol->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (fol->next)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);

  /* Showing the bitvector */
  sprintbit_multi(AFF_FLAGS_BITV(k), affected_bits, buf2);
  sprintf(buf, "AFF: %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  /* Show player on mobs piss list */

  {
    memory_rec *names;
      if (IS_NPC(k) && (MOB_FLAGGED(k, MOB_MEMORY))) {
        sprintf(buf, "Pissed List:\r\n");
        for (names = MEMORY(k); names && !found; names = names->next) {
          sprintf(buf2, "  %s\r\n", names->name);
          strcat(buf, buf2);
        }
        send_to_char(strcat(buf, "\r\n"), ch);
      } 
  }

  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      *buf2 = '\0';
      sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1,
	      CCCYN(ch, C_NRM), (aff->type >= 0 && aff->type <= MAX_SPELLS) ?
	      spells[aff->type] : "TYPE UNDEFINED", CCNRM(ch, C_NRM));
      if (aff->modifier) {
	sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
	strcat(buf, buf2);
      }
      if (aff->bitvector) {
	if (*buf2)
	  strcat(buf, ", sets ");
	else
	  strcat(buf, "sets ");
	sprintbit_multi(aff->bitvector, affected_bits, buf2);
	strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }
}


ACMD(do_stat)
{
  struct char_data *victim = 0;
  struct obj_data *object = 0;
  struct char_file_u tmp_store;
  int tmp;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char("Stats on who or what?\r\n", ch);
    return;
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char("Stats on which mobile?\r\n", ch);
    else {
      if ((victim = get_char_vis(ch, buf2)))
	do_stat_character(ch, victim);
      else
	send_to_char("No such mobile around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      if ((victim = get_player_vis(ch, buf2, 0)))
	do_stat_character(ch, victim);
      else
	send_to_char("No such player around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1) {
	store_to_char(&tmp_store, victim);
	victim->player.time.logon = tmp_store.last_logon;
	char_to_room(victim, 0);
	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char("Sorry, you can't do that.\r\n", ch);
	else
	  do_stat_character(ch, victim);
	extract_char(victim);
      } else {
	send_to_char("There is no such player.\r\n", ch);
	free(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char("Stats on which object?\r\n", ch);
    else {
      if ((object = get_obj_vis(ch, buf2)))
	do_stat_object(ch, object);
      else
	send_to_char("No such object around.\r\n", ch);
    }
  } else {
    if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)))
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)))
      do_stat_object(ch, object);
    else if ((victim = get_char_room_vis(ch, buf1)))
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room].contents)))
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, buf1)))
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, buf1)))
      do_stat_object(ch, object);
    else
      send_to_char("Nothing around by that name.\r\n", ch);
  }
}


ACMD(do_shutdown)
{
  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("If you want to shut something down, say so!\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else
    send_to_char("Unknown shutdown option.\r\n", ch);
}


void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping)
    send_to_char("You aren't snooping anyone.\r\n", ch);
  else {
    send_to_char("You stop snooping.\r\n", ch);
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}


ACMD(do_snoop)
{
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg)))
    send_to_char("No such person around.\r\n", ch);
  else if (!victim->desc)
    send_to_char("There's no link.. nothing to snoop.\r\n", ch);
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char("Busy already. \r\n", ch);
  else if (victim->desc->snooping == ch->desc)
    send_to_char("Don't be stupid.\r\n", ch);
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch)) {
      send_to_char("You can't.\r\n", ch);
      return;
    }
    send_to_char(OK, ch);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}



ACMD(do_switch)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char("You're already switched.\r\n", ch);
  else if (!*arg)
    send_to_char("Switch with who?\r\n", ch);
  else if (!(victim = get_char_vis(ch, arg)))
    send_to_char("No such character.\r\n", ch);
  else if (ch == victim)
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
  else if (victim->desc)
    send_to_char("You can't do that, the body is already in use!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_IMPL) && !IS_NPC(victim))
    send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
  else {
    send_to_char(OK, ch);

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char("You return to your original body.\r\n", ch);

    /* JE 2/22/95 */
    /* if someone switched into your original body, disconnect them */
    if (ch->desc->original->desc)
      STATE(ch->desc->original->desc) = CON_DISCONNECT;

    if (affected_by_spell(ch->desc->original, SPELL_CORPSE_HOST))
      affect_from_char(ch->desc->original, SPELL_CORPSE_HOST);

    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
    
    
    if (affected_by_spell(ch, SPELL_CORPSE_HOST))
      die(ch, NULL);
    
  }
}



ACMD(do_load)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: load { obj | mob } <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, ch->in_room);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    if (load_into_inventory)
      obj_to_char(obj, ch);
    else
      obj_to_room(obj, ch->in_room);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}



ACMD(do_vstat)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf) {			/* argument supplied. destroy single object
				 * or char */
    if ((vict = get_char_room_vis(ch, buf))) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
	send_to_char("Fuuuuuuuuu!\r\n", ch);
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	sprintf(buf, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	mudlog(buf, BRF, LVL_GOD, TRUE);
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("Nothing here by that name.\r\n", ch);
      return;
    }

    send_to_char(OK, ch);
  } else {			/* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

    for (vict = world[ch->in_room].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict))
	extract_char(vict);
    }

    for (obj = world[ch->in_room].contents; obj; obj = next_o) {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}



const char *logtypes[] = {
  "off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog)
{
  int tp;

  one_argument(argument, arg);

  if (!*arg) {
    tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
    sprintf(buf, "Your syslog is currently %s.\r\n", logtypes[tp]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char("Usage: syslog { Off | Brief | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
  SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

  sprintf(buf, "Your syslog is now %s.\r\n", logtypes[tp]);
  send_to_char(buf, ch);
}



ACMD(do_advance)
{
  struct char_data *victim;
  char *name = arg, *level = buf2;
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name))) {
      send_to_char("That player is not here.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Advance who?\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("Maybe that's not such a great idea.\r\n", ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("NO!  Not on NPC's.\r\n", ch);
    return;
  }
  newlevel = get_level_from_string(level);
  if (newlevel == 0)
    newlevel = atoi(level);
  if (!*level || newlevel <= 0 || (newlevel>MAX_MORT_LEVEL && newlevel<LVL_IMMORT) || newlevel > LVL_IMPL) {
    sprintf(buf,"That's not a level!\r\nMortal Levels: %d-%d, Immortal Levels: %d-%d\r\n",
    1, MAX_MORT_LEVEL, LVL_IMMORT, LVL_IMPL);
    send_to_char(buf, ch);
    return;
  }
  if (newlevel > LVL_IMPL) {
    sprintf(buf, "%d is the highest possible level.\r\n", LVL_IMPL);
    send_to_char(buf, ch);
    return;
  }
  if (newlevel > GET_LEVEL(ch)) {
    send_to_char("Yeah, right.\r\n", ch);
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char("They are already at that level.\r\n", ch);
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    GET_LEVEL(victim) = 0;
    do_start(victim);
//   GET_LEVEL(victim) = newlevel;
   GET_TOT_LEVEL(victim) -= (oldlevel - newlevel);
    send_to_char("You are momentarily enveloped by darkness!\r\n"
		 "You feel somewhat diminished.\r\n", victim);
  } else {
    act("$n makes some strange gestures.\r\n"
	"A strange feeling comes upon you,\r\n"
	"Like a giant hand, light comes down\r\n"
	"from above, grabbing your body, that\r\n"
	"begins to pulse with colored lights\r\n"
	"from inside.\r\n\r\n"
	"Your head seems to be filled with demons\r\n"
	"from another plane as your body dissolves\r\n"
	"to the elements of time and space itself.\r\n"
	"Suddenly a silent explosion of light\r\n"
	"snaps you back to reality.\r\n\r\n"
	"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(OK, ch);

  log("(GC) %s has advanced %s to level %d (from %d)",
	  GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
  if (newlevel < LVL_IMMORT)
    REMOVE_BIT(PRF_FLAGS(victim), PRF_HOLYLIGHT);
  gain_exp_regardless(victim,
	 level_exp(GET_CLASS(victim), newlevel) - GET_EXP(victim));
  save_char(victim, NOWHERE);
}



ACMD(do_restore)
{
  struct char_data *vict;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to restore?\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

    if ((GET_LEVEL(ch) >= LVL_GRGOD) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
      for (i = 1; i <= MAX_SKILLS; i++)
	SET_SKILL(vict, i, 100);

      if (GET_LEVEL(vict) >= LVL_GRGOD) {
	vict->real_abils.str_add = 100;
	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
	vict->real_abils.cha = 25;
      }
      vict->aff_abils = vict->real_abils;
    }
    update_pos(vict);
    send_to_char(OK, ch);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}


void perform_immort_vis(struct char_data *ch)
{
   struct char_data *tch;

  if (GET_INVIS_LEV(ch) == 0 && !AFF_FLAGGED(ch, AFF_HIDE | AFF_INVISIBLE)) {
    send_to_char("You are already visible to the world.\r\n", ch);
    return;
  }
   
  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char("You shimmer with a golden light and become fully visible!\r\n", ch);
        act("$n shimmers with a golden light and becomes fully visible!", 
FALSE, ch, 0,
          tch, TO_ROOM);
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  if (IS_NPC(ch))
    return;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
	  tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  sprintf(buf, "Your slowly distort from vision and dissapear!\r\n");
  act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
          tch, TO_ROOM);
  send_to_char(buf, ch);
}
  

ACMD(do_invis)
{
  int level;

  if (IS_NPC(ch)) {
    send_to_char("You can't do that!\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_LEVEL(ch))
      send_to_char("You can't go invisible above your own level.\r\n", ch);
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char("That must be a mistake...\r\n", ch);
  else {
    sprintf(buf, "%s\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next)
      if (STATE(pt) == CON_PLAYING && pt->character && pt->character != ch)
	send_to_char(buf, pt->character);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      send_to_char(buf, ch);
  }
}


ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
  case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
  case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
  default:    return;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);

  if (!*argument)
    *msg = NULL;
  else
    *msg = str_dup(argument);

  if (*msg && strlen(*msg) > MAX_POOF_LENGTH)
    send_to_char("Warning: your poof is too long and will not be saved completely.\r\n", ch);

  send_to_char(OK, ch);
}



ACMD(do_dc)
{
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char("Usage: DC <user number> (type USERS for a list)\r\n", ch);
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char("No such connection.\r\n", ch);
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char("No such connection.\r\n", ch);
    else
      send_to_char("Umm.. maybe that's not such a good idea...\r\n", ch);
    return;
  }

  /* We used to just close the socket here using close_socket(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor.
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   */
  STATE(d) = CON_DISCONNECT;
  sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
  send_to_char(buf, ch);
  log("(GC) Connection closed by %s.", GET_NAME(ch));
}



ACMD(do_wizlock)
{
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char("Invalid wizlock value.\r\n", ch);
      return;
    }
    circle_restrict = value;
    when = "now";
    save_world_settings();
  } else
    when = "currently";

  switch (circle_restrict) {
  case 0:
    sprintf(buf, "The game is %s completely &gopen&w.\r\n", when);
    break;
  case 1:
    sprintf(buf, "The game is %s &rclosed&w to new players.\r\n", when);
    break;
  default:
    sprintf(buf, "Only level &g%d&w and above may enter the game %s.\r\n",
	    circle_restrict, when);
    break;
  }
  send_to_char(buf, ch);
}


ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    sprintf(buf, "Current machine time: &W%s&w\r\n", tmstr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
	    ((d == 1) ? "" : "s"), h, m);
  }

  send_to_char(buf, ch);
}



ACMD(do_last)
{
  struct char_file_u chdata;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("For whom do you wish to search?\r\n", ch);
    return;
  }
  if (load_char(arg, &chdata) < 0) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if ((chdata.level > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_IMPL)) {
    send_to_char("You are not sufficiently godly for that!\r\n", ch);
    return;
  }
  sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
	  chdata.char_specials_saved.idnum, (int) chdata.level,
	  class_abbrevs[(int) chdata.chclass], chdata.name, chdata.host,
	  ctime(&chdata.last_logon));
  send_to_char(buf, ch);
}


ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char("Whom do you wish to force do what?\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_GRGOD) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg)))
      send_to_char(NOPERSON, ch);
    else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char("No, no, no!\r\n", ch);
    else {
      send_to_char(OK, ch);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      sprintf(buf, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(OK, ch);
    sprintf(buf, "(GC) %s forced all to %s", GET_NAME(ch), to_force);
    mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}



ACMD(do_wiznet)
{
  struct descriptor_data *d;
  char emote = FALSE;
  char any = FALSE;
  int level = LVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
		 "       wiznet @<level> *<emotetext> | wiz @\r\n", ch);
    return;
  }
  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), LVL_IMMORT);
      if (level > GET_LEVEL(ch)) {
	send_to_char("You can't wizline above your own level.\r\n", ch);
	return;
      }
    } else if (emote)
      argument++;
    break;
  case '@':
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) == CON_PLAYING && GET_LEVEL(d->character) >= LVL_IMMORT &&
	  !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	  (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_IMPL)) {
	if (!any) {
	  strcpy(buf1, "&WGods online:&w\r\n");
	  any = TRUE;
	}
	sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
	if (PRF_FLAGGED(d->character, PRF_AFK))
	  strcat(buf1, " (Afk)\r\n");
	if (PLR_FLAGGED(d->character, PLR_WRITING))
	  strcat(buf1, " (Writing)\r\n");
	else if (PLR_FLAGGED(d->character, PLR_MAILING))
	  strcat(buf1, " (Writing mail)\r\n");
	else
	  strcat(buf1, "\r\n");

      }
    }
    any = FALSE;
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) == CON_PLAYING && GET_LEVEL(d->character) >= LVL_IMMORT &&
	  PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	  CAN_SEE(ch, d->character)) {
	if (!any) {
	  strcat(buf1, "Gods offline:\r\n");
	  any = TRUE;
	}
	sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
      }
    }
    send_to_char(buf1, ch);
    return;
  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }
  if (level > LVL_IMMORT) {
    sprintf(buf1, "&m%s:&R <%d> &c%s%s&w\r\n", GET_NAME(ch), level,
	    emote ? "<--- " : "", argument);
    sprintf(buf2, "&mSomeone:&R <%d> &c%s%s&w\r\n", level, emote ? "<--- " : "",
	    argument);
  } else {
    sprintf(buf1, "&m%s: &c%s%s&w\r\n", GET_NAME(ch), emote ? "<--- " : "",
	    argument);
    sprintf(buf2, "&mSomeone: &c%s%s&w\r\n", emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && (GET_LEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	(!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(CCCYN(d->character, C_NRM), d->character);
      if (CAN_SEE(d->character, ch))
	send_to_char(buf1, d->character);
      else
	send_to_char(buf2, d->character);
      send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
}



ACMD(do_zreset)
{
  int i, j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("You must specify a zone.\r\n", ch);
    return;
  }
  if (*arg == '*') {
    for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i);
    send_to_char("Reset world.\r\n", ch);
    sprintf(buf, "(GC) %s reset entire world.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
    if (is_empty(i) && !zone_table[i].unloaded)
        unload_room_descs(i);
    return;
  } else if (*arg == '.') {
      if (GET_LEVEL(ch) < LVL_IMMORT) {
        if (can_edit_zone(ch, world[ch->in_room].zone))
          i = world[ch->in_room].zone;
        else {
          send_to_char("You are not allowed to do this!\r\n", ch);
          return;
        } 
      }
  } else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i >= 0 && i <= top_of_zone_table) {
    reset_zone(i);
    sprintf(buf, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number,
	    zone_table[i].name);
    send_to_char(buf, ch);
    sprintf(buf, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
    mudlog(buf, NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE);
    if (is_empty(i) && !zone_table[i].unloaded)
        unload_room_descs(i);
  } else
    send_to_char("Invalid zone number.\r\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  struct char_data *vict;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Yes, but for whom?!?\r\n", ch);
  else if (!(vict = get_char_vis(ch, arg)))
    send_to_char("There is no such player.\r\n", ch);
  else if (IS_NPC(vict))
    send_to_char("You can't do that to a mob!\r\n", ch);
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    send_to_char("Hmmm...you'd better not.\r\n", ch);
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char("Rerolled...\r\n", ch);
      roll_real_abils(vict);
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      sprintf(buf, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	      GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      send_to_char(buf, ch);
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
	send_to_char("Your victim is not flagged.\r\n", ch);
	return;
      }
      REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
      send_to_char("Pardoned.\r\n", ch);
      send_to_char("You have been pardoned by the Gods!\r\n", vict);
      sprintf(buf, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      sprintf(buf, "(GC) Notitle %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      sprintf(buf, "(GC) Squelch %s for %s by %s.", ONOFF(result),
	      GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Your victim is already pretty cold.\r\n", ch);
	return;
      }
      SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
      send_to_char("Frozen.\r\n", ch);
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      sprintf(buf, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
	   GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	send_to_char(buf, ch);
	return;
      }
      sprintf(buf, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE);
      REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
      send_to_char("Thawed.\r\n", ch);
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected) {
	while (vict->affected)
	  affect_remove(vict, vict->affected);
	send_to_char("There is a brief flash of light!\r\n"
		     "You feel slightly different.\r\n", vict);
	send_to_char("All spells removed.\r\n", ch);
      } else {
	send_to_char("Your victim does not have any affections!\r\n", ch);
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
    }
    save_char(vict, NOWHERE);
    }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, int zone)
{
  sprintf(bufptr + strlen(bufptr), "%3d %-30.30s Age: %3d; Reset: %3d (%1d); Top: %5d %c%c\r\n",
	  zone_table[zone].number, zone_table[zone].name,
	  zone_table[zone].age, zone_table[zone].lifespan,
	  zone_table[zone].reset_mode, zone_table[zone].top, 
	  zone_table[zone].unloaded ? ' ' : 'L',
	  zone_table[zone].locked ? '*' : ' ');
}


ACMD(do_show)
{
  struct char_file_u vbuf;
  int i, j, k, l, con;
  char self = 0;
  struct char_data *vict;
  struct obj_data *obj;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], birth[80];

  struct show_struct {
    const char *cmd;
    const char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_IMMORT },			/* 1 */
    { "player",		LVL_GOD },
    { "rent",		LVL_GOD },
    { "stats",		LVL_IMMORT },
    { "errors",		LVL_IMPL },			/* 5 */
    { "death",		LVL_GOD },
    { "godrooms",	LVL_GOD },
    { "shops",		LVL_IMMORT },
    { "houses",		LVL_GOD },
    { "teleport",	LVL_GURU },			/* 10 */
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	sprintf(buf + strlen(buf), "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (!strcmp(value, "."))
    self = 1;
  buf[0] = '\0';
  switch (l) {
  case 1:			/* zone */
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, world[ch->in_room].zone);
    else if (*value && is_number(value)) {
      for (j = atoi(value), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++);
      if (i <= top_of_zone_table)
	print_zone_to_buf(buf, i);
      else {
	send_to_char("That is not a valid zone.\r\n", ch);
	return;
      }
    } else
      for (i = 0; i <= top_of_zone_table; i++)
	print_zone_to_buf(buf, i);
    page_string(ch->desc, buf, TRUE);
    break;
  case 2:			/* player */
    if (!*value) {
      send_to_char("A name would help.\r\n", ch);
      return;
    }

    if (load_char(value, &vbuf) < 0) {
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
    sprintf(buf, "Player: %-12s (%s) [%2d %s]\r\n", vbuf.name,
      genders[(int) vbuf.sex], vbuf.level, class_abbrevs[(int) vbuf.chclass]);
    sprintf(buf,
	 "%sAu: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
	    buf, vbuf.points.gold, vbuf.points.bank_gold, vbuf.points.exp,
	    vbuf.char_specials_saved.alignment,
	    vbuf.player_specials_saved.spells_to_learn);
    strcpy(birth, ctime(&vbuf.birth));
    sprintf(buf,
	    "%sStarted: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
	    buf, birth, ctime(&vbuf.last_logon), (int) (vbuf.played / 3600),
	    (int) (vbuf.played / 60 % 60));
    send_to_char(buf, ch);
    break;
  case 3:
    Crash_listrent(ch, value);
    break;
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    strcpy(buf, "Current stats:\r\n");
    sprintf(buf + strlen(buf), "  %5d players in game  %5d connected\r\n",
		i, con);
    sprintf(buf + strlen(buf), "  %5d registered\r\n",
		top_of_p_table + 1);
    sprintf(buf + strlen(buf), "  %5d mobiles          %5d prototypes\r\n",
		j, top_of_mobt + 1);
    sprintf(buf + strlen(buf), "  %5d objects          %5d prototypes\r\n",
		k, top_of_objt + 1);
    sprintf(buf + strlen(buf), "  %5d rooms            %5d zones\r\n",
		top_of_world + 1, top_of_zone_table + 1);
    sprintf(buf + strlen(buf), "  %5d large bufs\r\n",
		buf_largecount);
    sprintf(buf + strlen(buf), "  %5d buf switches     %5d overflows\r\n",
	    buf_switches, buf_overflows);
    send_to_char(buf, ch);
    break;
  case 5:
    strcpy(buf, "Errant Rooms\r\n------------\r\n");
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
	if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0)
	  sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, GET_ROOM_VNUM(i),
		  world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 6:
    strcpy(buf, "Death Traps\r\n-----------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j,
		GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 7:
    strcpy(buf, "Godrooms\r\n--------------------------\r\n");
    for (i = 0, j = 0; i < top_of_world; i++)
    if (ROOM_FLAGGED(i, ROOM_GODROOM))
      sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n",
		++j, GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, TRUE);
    break;
  case 8:
    show_shops(ch, value);
    break;
  case 9:
    hcontrol_list_houses(ch);
    break;
  case 10:      /* teleport */
    strcpy(buf, "Teleport Rooms\r\n--------------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
        if (world[i].tele != NULL)
          sprintf(buf, "%s%2d: [%5d] %-24.24s Targ: %5d\r\n", buf, ++j,
                  world[i].number, world[i].name, world[i].tele->targ);
    page_string(ch->desc, buf, 1);
    break;
  default:
    send_to_char("Sorry, I don't understand that.\r\n", ch);
    break;
  }
}


/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
  struct set_struct {
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] = {
   { "brief",		LVL_GOD, 	PC, 	BINARY },  /* 0 */
   { "invstart", 	LVL_GOD, 	PC, 	BINARY },  /* 1 */
   { "title",		LVL_GOD, 	PC, 	MISC },
   { "nosummon", 	LVL_GRGOD, 	PC, 	BINARY },
   { "maxhit",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "maxmana", 	LVL_GRGOD, 	BOTH, 	NUMBER },  /* 5 */
   { "maxmove", 	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "hit", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "mana",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "move",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "align",		LVL_GOD, 	BOTH, 	NUMBER },  /* 10 */
   { "str",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "stradd",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "int", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "wis", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "dex", 		LVL_GRGOD, 	BOTH, 	NUMBER },  /* 15 */
   { "con", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "cha",		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "ac", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "gold",		LVL_GOD, 	BOTH, 	NUMBER },
   { "bank",		LVL_GOD, 	PC, 	NUMBER },  /* 20 */
   { "exp", 		LVL_GRGOD, 	BOTH, 	NUMBER },
   { "hitroll", 	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "damroll", 	LVL_GRGOD, 	BOTH, 	NUMBER },
   { "invis",		LVL_COIMPL, 	PC, 	NUMBER },
   { "nohassle", 	LVL_GRGOD, 	PC, 	BINARY },  /* 25 */
   { "frozen",		LVL_FREEZE, 	PC, 	BINARY },
   { "practices", 	LVL_GRGOD, 	PC, 	NUMBER },
   { "lessons", 	LVL_GRGOD, 	PC, 	NUMBER },
   { "drunk",		LVL_GRGOD, 	BOTH, 	MISC },
   { "hunger",		LVL_GRGOD, 	BOTH, 	MISC },    /* 30 */
   { "thirst",		LVL_GRGOD, 	BOTH, 	MISC },
   { "killer",		LVL_GOD, 	PC, 	BINARY },
   { "thief",		LVL_GOD, 	PC, 	BINARY },
   { "level",		LVL_COIMPL, 	BOTH, 	NUMBER },
   { "room",		LVL_COIMPL, 	BOTH, 	NUMBER },  /* 35 */
   { "roomflag", 	LVL_GRGOD, 	PC, 	BINARY },
   { "siteok",		LVL_GRGOD, 	PC, 	BINARY },
   { "deleted", 	LVL_COIMPL, 	PC, 	BINARY },
   { "class",		LVL_CODER, 	BOTH, 	MISC },
   { "nowizlist", 	LVL_GOD, 	PC, 	BINARY },  /* 40 */
   { "quest",		LVL_GOD, 	PC, 	BINARY },
   { "loadroom", 	LVL_GRGOD, 	PC, 	MISC },
   { "color",		LVL_GOD, 	PC, 	BINARY },
   { "idnum",		LVL_COIMPL, 	PC, 	NUMBER },
   { "passwd",		LVL_COIMPL, 	PC, 	MISC },    /* 45 */
   { "nodelete", 	LVL_GOD, 	PC, 	BINARY },
   { "sex", 		LVL_GRGOD, 	BOTH, 	MISC },
   { "olc",		LVL_CHBUILD, 	PC, 	NUMBER },
   { "hometown",	LVL_GOD,	PC,	NUMBER },
   { "age",		LVL_GRGOD,	BOTH,	NUMBER },  /* 50 */
   { "newbie",		LVL_GOD,	PC,	BINARY },
   { "qpoints",		LVL_GRGOD,	PC,	NUMBER },
   { "clan",            LVL_CODER,      PC,     NUMBER },
   { "clanrank",	LVL_CODER,	PC,	NUMBER },
   { "trust",           LVL_COIMPL,     PC,     BINARY },  /* 55 */
   { "ripcnt",          LVL_GRGOD,      PC,     NUMBER },
   { "killcnt",         LVL_GRGOD,      PC,     NUMBER },
   { "\n", 0, BOTH, MISC }
  };

  int perform_set(struct char_data *ch, struct char_data *vict, int mode,
 		char *val_arg)
  {
    int i, on = 0, off = 0, value = 0;
    char output[MAX_STRING_LENGTH];
 
    /* Check to make sure all the levels are correct */
    if (GET_LEVEL(ch) != LVL_IMPL) {
      if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
        send_to_char("Maybe that's not such a great idea...\r\n", ch);
        return 0;
      }
    }
    if (GET_LEVEL(ch) < set_fields[mode].level) {
      send_to_char("You are not godly enough for that!\r\n", ch);
      return 0;
    }
  
    /* Make sure the PC/NPC is correct */
    if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
      send_to_char("You can't do that to a beast!\r\n", ch);
      return 0;
    } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
      send_to_char("That can only be done to a beast!\r\n", ch);
      return 0;
    }
  
    /* Find the value of the argument */
    if (set_fields[mode].type == BINARY) {
      if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
        on = 1;
      else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
        off = 1;
      if (!(on || off)) {
        send_to_char("Value must be 'on' or 'off'.\r\n", ch);
        return 0;
      }
      sprintf(output, "%s %s for %s.", set_fields[mode].cmd, ONOFF(on),
  	    GET_NAME(vict));
    } else if (set_fields[mode].type == NUMBER) {
      value = atoi(val_arg);
      sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
  	    set_fields[mode].cmd, value);
    } else {
      strcpy(output, "Okay.");  /* can't use OK macro here 'cause of \r\n */
    }
  
  switch (mode) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 2:
    set_title(vict, val_arg);
    sprintf(output, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    sprintf(output, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 4:
    vict->points.max_hit = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    affect_total(vict);
    break;
  case 8:
    vict->points.mana = RANGE(0, vict->points.max_mana);
    affect_total(vict);
    break;
  case 9:
    vict->points.move = RANGE(0, vict->points.max_move);
    affect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 11:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.str = value;
    vict->real_abils.str_add = 0;
    affect_total(vict);
    break;
  case 12:
    vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
    break;
  case 13:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_GRGOD)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.cha = value;
    affect_total(vict);
    break;
  case 18:
    vict->points.armor = RANGE(-100, 100);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 21:
    vict->points.exp = RANGE(0, 50000000);
    break;
  case 22:
    vict->points.hitroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 23:
    vict->points.damroll = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 24:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char("You aren't godly enough for that!\r\n", ch);
      return 0;
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
    break;
  case 25:
    if (GET_LEVEL(ch) < LVL_IMPL && ch != vict) {
      send_to_char("You aren't godly enough for that!\r\n", ch);
      return 0;
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict) {
      send_to_char("Better not -- could be a long winter!\r\n", ch);
      return 0;
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
  case 28:
    GET_PRACTICES(vict) = RANGE(0, 100);
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off")) {
      GET_COND(vict, (mode - 29)) = (char) -1; /* warning: magic number here */
      sprintf(output, "%s's %s now off.", GET_NAME(vict), set_fields[mode].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (mode - 29)) = (char) value; /* and here too */
      sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
	      set_fields[mode].cmd, value);
    } else {
      send_to_char("Must be 'off' or a value from 0 to 24.\r\n", ch);
      return 0;
    }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    if (value > GET_LEVEL(ch) || value > LVL_IMPL) {
      send_to_char("You can't do that.\r\n", ch);
      return 0;
    }
    RANGE(0, LVL_IMPL);
    vict->player.level = (byte) value;
    break;
  case 35:
    if ((i = real_room(value)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return 0;
    }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
    char_from_room(vict);
    char_to_room(vict, i);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = parse_class(*val_arg)) == CLASS_UNDEFINED) {
      send_to_char("That is not a class.\r\n", ch);
      return 0;
    }
    GET_CLASS(vict) = i;
     break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "off")) {
      REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      if (real_room(value) != NOWHERE) {
        SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
	GET_LOADROOM(vict) = value;
	sprintf(output, "%s will enter at room #%d.", GET_NAME(vict),
		GET_LOADROOM(vict));
      } else {
	send_to_char("That room does not exist!\r\n", ch);
	return 0;
      }
    } else {
      send_to_char("Must be 'off' or a room's virtual number.\r\n", ch);
      return 0;
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
    break;
  case 44:
    if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
      return 0;
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (GET_LEVEL(ch) < LVL_COIMPL) {
      send_to_char("Please don't use this command, yet.\r\n", ch);
      return 0;
    }
    if (GET_LEVEL(vict) >= LVL_GRGOD) {
      send_to_char("You cannot change that.\r\n", ch);
      return 0;
    }
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    sprintf(output, "Password changed to '%s'.", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    if (!str_cmp(val_arg, "male"))
      vict->player.sex = SEX_MALE;
    else if (!str_cmp(val_arg, "female"))
      vict->player.sex = SEX_FEMALE;
    else if (!str_cmp(val_arg, "neutral"))
      vict->player.sex = SEX_NEUTRAL;
    else {
      send_to_char("Must be 'male', 'female', or 'neutral'.\r\n", ch);
      return 0;
    }
    break;
  case 48:
    GET_OLC_ZONE(vict) = value;
    break;
  case 49:
    GET_HOMETOWN(vict) = RANGE(1, NUM_HOMETOWNS-1);
    break;
  case 50:	/* set age */
    if (value < 2 || value > 200) {	/* Arbitrary limits. */
      send_to_char("Ages 2 to 200 accepted.\r\n", ch);
      return 0;
    }
    /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
    ch->player.time.birth = time(0) - ((value - 17) * SECS_PER_MUD_YEAR);
    break;
  case 51:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NEWBIE);
    break;
  case 52:
    QPOINTS(vict) = RANGE(0, MAX_QPOINTS);
    break;
  case 53:
    if (real_clan(value) < 0) {
      send_to_char("Such clan does not exist.\r\n", ch);
    } else {
      PLAYERCLAN(vict) = RANGE(0, 9999);
      CLANRANK(vict) = 1;
    }
    break;
  case 54:
    if (real_clan(PLAYERCLAN(vict)) < 0) {
      send_to_char("That player does not belong to any clan.\r\n", ch);
    } else {
      CLANRANK(vict) = RANGE(CLAN_APPLY, CLAN_RETIRED);
    }
    break;
  case 55:
    if (!PLR_FLAGGED(vict, PLR_TRUST) && GET_LEVEL(vict) != LVL_BUILDER) {
      send_to_char("You aren't godly enough for that!\r\n", ch);
      return 0;
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_TRUST);
    break;
  case 56:
    GET_RIP_CNT(ch) = RANGE(0, 9999999);
    break;
  case 57:
    GET_KILL_CNT(ch) = RANGE(0, 9999999);
    break;
  default:
    send_to_char("Can't set that!\r\n", ch);
    return 0;
  }
  strcat(output, "\r\n");
  send_to_char(CAP(output), ch);
  return 1;
}

ACMD(do_tedit) {
   int l, i;
   char field[MAX_INPUT_LENGTH];
   extern char *credits;
   extern char *news;
   extern char *motd;
   extern char *imotd;
   extern char *help;
   extern char *info;
   extern char *background;
   extern char *handbook;
   extern char *policies;
   extern char *buildbook;
   
   struct editor_struct {
      char *cmd;
      char level;
      char *buffer;
      int  size;
      char *filename;
   } fields[] = {
      /* edit the lvls to your own needs */
	{ "credits",	LVL_IMPL,	credits,	2400,	CREDITS_FILE},
	{ "news",	LVL_GRGOD,	news,		8192,	NEWS_FILE},
	{ "motd",	LVL_GRGOD,	motd,		2400,	MOTD_FILE},
	{ "imotd",	LVL_COIMPL,	imotd,		2400,	IMOTD_FILE},
	{ "help",       LVL_GRGOD,	help,		2400,	HELP_PAGE_FILE},
	{ "info",	LVL_GRGOD,	info,		8192,	INFO_FILE},
	{ "background",	LVL_COIMPL,	background,	8192,	BACKGROUND_FILE},
	{ "handbook",   LVL_COIMPL,	handbook,	8192,   HANDBOOK_FILE},
	{ "buildbook",  LVL_COIMPL,	buildbook,	8192,	BLDBOOK_FILE},
	{ "policies",	LVL_COIMPL,	policies,	8192,	POLICIES_FILE},
	{ "\n",		0,		NULL,		0,	NULL }
   };

   if (ch->desc == NULL) {
      send_to_char("Get outta here you linkdead head!\r\n", ch);
      return;
   }
   
   if (GET_LEVEL(ch) < LVL_GRGOD) {
      send_to_char("You do not have text editor permissions.\r\n", ch);
      return;
   }
   
   half_chop(argument, field, buf);

   if (!*field) {
      strcpy(buf, "Files available to be edited:\r\n");
      i = 1;
      for (l = 0; *fields[l].cmd != '\n'; l++) {
	 if (GET_LEVEL(ch) >= fields[l].level) {
	    sprintf(buf, "%s%-11.11s", buf, fields[l].cmd);
	    if (!(i % 7)) strcat(buf, "\r\n");
	    i++;
	 }
      }
      if (--i % 7) strcat(buf, "\r\n");
      if (i == 0) strcat(buf, "None.\r\n");
      send_to_char(buf, ch);
      return;
   }
   for (l = 0; *(fields[l].cmd) != '\n'; l++)
     if (!strncmp(field, fields[l].cmd, strlen(field)))
     break;
   
   if (*fields[l].cmd == '\n') {
      send_to_char("Invalid text editor option.\r\n", ch);
      return;
   }
   
   if (GET_LEVEL(ch) < fields[l].level) {
      send_to_char("You are not godly enough for that!\r\n", ch);
      return;
   }

   switch (l) {
    case 0: ch->desc->str = &credits; break;
    case 1: ch->desc->str = &news; break;
    case 2: ch->desc->str = &motd; break;
    case 3: ch->desc->str = &imotd; break;
    case 4: ch->desc->str = &help; break;
    case 5: ch->desc->str = &info; break;
    case 6: ch->desc->str = &background; break;
    case 7: ch->desc->str = &handbook; break;
    case 8: ch->desc->str = &buildbook; break;
    case 9: ch->desc->str = &policies; break;
    default:
      send_to_char("Invalid text editor option.\r\n", ch);
      return;
   }
   
   /* set up editor stats */
   send_to_char("\x1B[H\x1B[J", ch);
   send_to_char("Edit file below: (/s saves /h for help)\r\n", ch);
   ch->desc->backstr = NULL;
   if (fields[l].buffer) {
      send_to_char(fields[l].buffer, ch);
      ch->desc->backstr = str_dup(fields[l].buffer);
   }
   ch->desc->max_str = fields[l].size;
   ch->desc->mail_to = 0;
   ch->desc->storage = str_dup(fields[l].filename);
   act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
   SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
   STATE(ch->desc) = CON_TEXTED;
}


/* (FIDO) New command: world <field> <on|off>      */
/*        It modifies the global variables:        */ 
/*        pk/sleep/charm/summon/roomaffect_allowed */
void send_to_imms(char *msg)
{
  struct descriptor_data *pt;

  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && GET_LEVEL(pt->character) >= LVL_GOD)
	send_to_char(msg, pt->character);

}

struct world_setting_t {
	char	*pref_name;
	char 	*shortcut;
	int	kind;
	int	level;
	int	*ptr;
};

struct world_setting_t world_settings[] = {
  { "PlayerKilling", 	"PKILL", BINARY, LVL_GRGOD, 	&pk_allowed },
  { "Max PKill Difference", "PDIFF", NUMBER, LVL_COIMPL, 	&max_level_diff },
  { "PlayerThieving",	"PTHIEF",BINARY, LVL_GRGOD,	&pt_allowed },
  { "Casting sleep on players", "PSLEEP", BINARY, LVL_GRGOD, &sleep_allowed },
  { "Casting charm on players", "PCHARM", BINARY, LVL_GRGOD, &charm_allowed }, 
  { "Summoning enemies",	"PSUMMON", BINARY, LVL_GRGOD, &summon_allowed },
  { "God OLC",	"GODOLC", BINARY, LVL_COIMPL, &olc_allowed },
  { "Clan OLC",	"CLANOLC", BINARY, LVL_CHBUILD, &clan_olc_allowed },
  { "\n",	"\n",	0,	0 }

};

void print_world(char *buffer, int level) {
  int i  = 0;
  
  strcpy(buffer, "&YCurrent world settings:&w\r\n");
  while (world_settings[i].pref_name[0] != '\n') {
    if (level >= LVL_IMMORT)
      sprintf(buffer + strlen(buffer), "%s%-10s", (level >= world_settings[i].level) ?
        "&g" : "&w", world_settings[i].shortcut);
      sprintf(buffer + strlen(buffer), "&c%-30s", world_settings[i].pref_name);
      switch (world_settings[i].kind) {
        case BINARY:
          sprintf(buffer + strlen(buffer), "%s\r\n", *(world_settings[i].ptr) 
            ? "&GYES": "&RNO");
          break;
        case NUMBER:
          sprintf(buffer + strlen(buffer), "&W%d\r\n", *(world_settings[i].ptr));
          break;
        default:
          sprintf(buffer + strlen(buffer), "\r\n");
          break;
     }
     i++;       
  }
  //strcat(buffer, "&w\r\n");
}

ACMD(do_world)
{
  int i = 0, value;

  if (!argument || !*argument) {
    print_world(buf, GET_LEVEL(ch));
    send_to_char(buf, ch);
    return;
  }
  skip_spaces(&argument);
  two_arguments(argument, buf1, buf);
  while (world_settings[i].pref_name[0] != '\n') {
    if (GET_LEVEL(ch) >= world_settings[i].level && is_abbrev(buf1, world_settings[i].shortcut)) {
      switch (world_settings[i].kind) {
        case BINARY:
          if (!str_cmp(buf, "on") || !str_cmp(buf, "yes") ||
            !str_cmp(buf, "1")) value = 1;
          else if (!str_cmp(buf, "off") || !str_cmp(buf, "no") || !str_cmp(buf, "0")) value = 0;
   	  else {
   	    send_to_char("Value must be ON/OFF, YES/NO, 1/0\r\n", ch);
   	    return;
   	  } 
   	  if (*world_settings[i].ptr == value) {
   	    send_to_char("It's already set.\r\n", ch);
   	    return;
   	  }
   	  *world_settings[i].ptr = value;
   	  sprintf(buf, "(GC) %s %s %s", GET_NAME(ch), value ? "enables": "disables",world_settings[i].pref_name);
   	  mudlog(buf, BRF, LVL_IMMORT, TRUE);
   	  save_world_settings();
   	  return;
        case NUMBER:
          value = atoi(buf);
          *world_settings[i].ptr = value;
          sprintf(buf, "(GC) %s sets %s to %d", GET_NAME(ch), world_settings[i].pref_name,
   	    value);
   	  mudlog(buf, BRF, LVL_IMMORT, TRUE);
   	  save_world_settings();
          return;
          break;
        default:
          return;
      }
    }
    i++;

  }  
  send_to_char("You can't set that.\r\n", ch);
  /*
  if (strcmp(field, "pk") == 0 && GET_LEVEL(ch)>=LVL_GRGOD)
  { 
    if (pk_allowed == 1)
    {
      pk_allowed = 0;
      sprintf(buf, "[SYS: %s disallows PKilling]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
    else
    {
      pk_allowed = 1;
      sprintf(buf, "[SYS: %s allows PKilling]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
  }
  else if (strcmp(field, "pt") == 0 && GET_LEVEL(ch)>=LVL_GRGOD)
  { 
    if (pt_allowed == 1)
    {
      pt_allowed = 0;
      sprintf(buf, "[SYS: %s disallows PThieving]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
    else
    {
      pt_allowed = 1;
      sprintf(buf, "[SYS: %s allows PThieving]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
  }
  else if (strcmp(field, "sleep") == 0 && GET_LEVEL(ch)>=LVL_GRGOD)
  {
    if (sleep_allowed == 1)
    {
      sleep_allowed = 0;
      sprintf(buf, "[SYS: %s disallows players from casting sleep on each other]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
    else
    {
      sleep_allowed = 1;
      sprintf(buf, "[SYS: %s allows players to cast sleep on each other]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
  }
  else if (strcmp(field, "summon") == 0 && GET_LEVEL(ch)>=LVL_GRGOD)
  {
    if (summon_allowed == 1)
    {
      summon_allowed = 0;
      sprintf(buf, "[SYS: %s disallows players from summoning one another]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
    else
    {
      summon_allowed = 1;
      sprintf(buf, "[SYS: %s allows players to summon one another]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
  }
  else if (strcmp(field, "charm") == 0 && GET_LEVEL(ch)>=LVL_GRGOD)
  {
    if (charm_allowed == 1)
    {
      charm_allowed = 0;
      sprintf(buf, "[SYS: %s disallows players from charming one another]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
    else
    {
      charm_allowed = 1;
      sprintf(buf, "[SYS: %s allows players to charm one another]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
  }
  else if (strcmp(field, "roomaffect") == 0 && GET_LEVEL(ch)>=LVL_GRGOD)
  {
    if (roomaffect_allowed == 1)
    {
      roomaffect_allowed = 0;
      sprintf(buf, "[SYS: %s disallows room affect spells from hurting other players]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
    else
    {
      roomaffect_allowed = 1;
      sprintf(buf, "[SYS: %s allows room affect spells to hurt other players]\r\n", GET_NAME(ch));
      send_to_imms(buf);
      log(buf);
      save_world_settings();
    }
  }
  else
  {
    send_to_char("&y[&YCurrent world status:&y]&w\r\n", ch);
    if (pk_allowed == 1) send_to_char("Pkilling &Gallowed&w,\r\n", ch);
    if (pk_allowed == 0) send_to_char("Pkilling &Rnot allowed&w,\r\n", ch);
    if (pt_allowed == 1) send_to_char("Pthieving &Gallowed&w,\r\n", ch);
    if (pt_allowed == 0) send_to_char("Pthieving &Rnot allowed&w,\r\n", ch);
    if (sleep_allowed == 1) send_to_char("Casting sleep on other players &Gallowed&w,\r\n", ch);
    if (sleep_allowed == 0) send_to_char("Casting sleep on other players &Rnot allowed&w,\r\n", ch);
    if (summon_allowed == 1) send_to_char("Summoning other players &Gallowed&w,\r\n", ch);
    if (summon_allowed == 0) send_to_char("Summoning other players &Rnot allowed&w,\r\n", ch);
    if (charm_allowed == 1) send_to_char("Charming other players &Gallowed&w,\r\n", ch);
    if (charm_allowed == 0) send_to_char("Charming other players &Rnot allowed&w,\r\n", ch);
    if (roomaffect_allowed == 1) send_to_char("Room affect spells &Gwill&w hurt other players.\r\n", ch);
    if (roomaffect_allowed == 0) send_to_char("Room affect spells &Rwill not&w hurt other players.\r\n", ch);
    send_to_char("\r\nChange: WORLD [pk | pt | sleep | summon | charm | roomaffect ]\r\n", ch);
  }
  */

}


/* dnsmod */
ACMD(do_dns)
{
  int i;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char ip[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char buf[16384];
  struct dns_entry *dns, *tdns;

  extern struct dns_entry *dns_cache[DNS_HASH_NUM];

  void save_dns_cache(void);

  half_chop(argument, arg1, arg2);

  if(!*arg1) {
    send_to_char("You shouldn't be using this if you don't know what it does!\r\n", ch);
    return;
  }

  if(is_abbrev(arg1, "delete")) {
    if(!*arg2) {
      send_to_char("Delete what?\r\n", ch);
      return;
    }
    CREATE(dns, struct dns_entry, 1);
    if(sscanf(arg2, "%d.%d.%d", dns->ip, dns->ip + 1,
      dns->ip + 2) != 3) {
      send_to_char("Delete what?\r\n", ch);
      return;
    }
    for(i = 0; i < DNS_HASH_NUM; i++) {
      if(dns_cache[i]) {
	for(tdns = dns_cache[i]; tdns; tdns = tdns->next) {
	  if(dns->ip[0] == tdns->ip[0] && dns->ip[1] == tdns->ip[1] &&
	    dns->ip[2] == tdns->ip[2]) {
	    sprintf(arg1, "Deleting %s.\r\n", tdns->name);
	    send_to_char(arg1, ch);
	    tdns->ip[0] = -1;
	  }
	}
      }
    }
    save_dns_cache();
    return;
  } else if(is_abbrev(arg1, "add")) {
    two_arguments(arg2, ip, name);
    if(!*ip || !*name) {
      send_to_char("Add what?\r\n", ch);
      return;
    }
    CREATE(dns, struct dns_entry, 1);
    dns->ip[3] = -1;
    if(sscanf(ip, "%d.%d.%d.%d", dns->ip, dns->ip + 1,
      dns->ip + 2, dns->ip + 3) < 3) {
      send_to_char("Add what?\r\n", ch);
      return;
    }
    i = (dns->ip[0] + dns->ip[1] + dns->ip[2]) % DNS_HASH_NUM;
    dns->name = str_dup(name);
    dns->next = dns_cache[i];
    dns_cache[i] = dns;
    save_dns_cache();
    send_to_char("OK!\r\n", ch);
    return;
  } else if(is_abbrev(arg1, "list")) {
    *buf = '\0';
    for(i = 0; i < DNS_HASH_NUM; i++) {
      if(dns_cache[i]) {
	for(tdns = dns_cache[i]; tdns; tdns = tdns->next) {
	  sprintf(buf, "%s%d.%d.%d.%d %s\r\n", buf, tdns->ip[0],
	    tdns->ip[1], tdns->ip[2], tdns->ip[3], tdns->name);
	}
      }
    }
    page_string(ch->desc, buf, 1);
    return;
  }
}


extern int mother_desc, port;
extern FILE *player_fl;
void Crash_rentsave(struct char_data * ch, int cost);

#define EXE_FILE "circle" /* maybe use argv[0] but it's not reliable */

/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover) {
  FILE *fp;
  struct descriptor_data *d, *d_next;

  #ifndef CIRCLE_UNIX
    send_to_char("This feature is not available on your platform\r\n", ch);
  #else 
	
  log("(GC) Copyover initiated by %s", GET_NAME(ch));
	
  fp = fopen (COPYOVER_FILE, "w");
	
  if (!fp) {
    send_to_char ("Copyover file not writeable, aborted.\n\r",ch);
    return;
  }
  if (PLR_FLAGGED(ch, PLR_EXPAND)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_EXPAND);
    ch->points.max_hit -= 200;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_POWERUP);
    ch->points.max_hit -= 10;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP2)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_POWERUP2);
    ch->points.max_hit -= 10;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP3)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_POWERUP3);
    ch->points.max_hit -= 10;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP4)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_POWERUP4);
    ch->points.max_hit -= 10;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP5)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_POWERUP5);
    ch->points.max_hit -= 10;
  }
  if (PLR_FLAGGED(ch, PLR_POWERUP6)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_POWERUP6);
    ch->points.max_hit -= 10;
  }
  if (PLR_FLAGGED(ch, PLR_VAMPED)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_VAMPED);
  }
  if (PLR_FLAGGED(ch, PLR_DRAGONFORM)) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_DRAGONFORM);
  }
  /* Consider changing all saved areas here, if you use OLC */
  sprintf (buf1, "\n\r *** COPYOVER by %s - please remain seated!\n\r", GET_NAME(ch));
	
  clan_save(TRUE, NULL); /* Fido - Crash save and auto save clans */
	
  /* For each playing descriptor, save its state */
  for (d = descriptor_list; d ; d = d_next) {
    struct char_data * och = d->character;
    d_next = d->next; /* We delete from the list , so need to save this */
		
    if (!d->character || d->connected > CON_PLAYING) { /* drop those logging on */
      write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r");
      close_socket (d); /* throw'em out */
    } else {
      fprintf (fp, "%d %s %d %s\n", d->descriptor, GET_NAME(och), 
      GET_ROOM_VNUM(och->in_room), d->host);

      /* save och */
      Crash_rentsave(och,0);
      save_char(och, och->in_room);
      write_to_descriptor (d->descriptor, buf1);
    }
  }
	
  fprintf (fp, "-1\n");
  fclose (fp);
	
  /* Close reserve and other always-open files and release other resources */

  fclose(player_fl);
    
  /* exec - descriptors are inherited */
	
  sprintf(buf, "%d", port);
  sprintf(buf2, "-C%d", mother_desc);
  sprintf(buf1, "-q");

  /* Ugh, seems it is expected we are 1 step above lib - this may be dangerous! */
  chdir ("../..");

//  execl (EXE_FILE, "circle", buf1, buf2, buf, (char *) NULL);
  execl (EXE_FILE, "circle", buf1, buf2, buf, (char *) NULL);

  /* Failed - sucessful exec will not return */
	
  perror("do_copyover: execl");
  send_to_char("Copyover FAILED!\n\r",ch);
	
  exit (1); /* too much trouble to try to recover! */
  #endif
}

int spell_sort_info[MAX_SKILLS+1];

void sort_for_skillstat(void)
{
  int a, b, tmp;

  /* initialize array */
  for (a = 1; a < MAX_SKILLS; a++)
    spell_sort_info[a] = a;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < MAX_SKILLS - 1; a++)
    for (b = a + 1; b < MAX_SKILLS; b++)
      if (strcmp(spells[spell_sort_info[a]], spells[spell_sort_info[b]]) > 0) {
        tmp = spell_sort_info[a];
        spell_sort_info[a] = spell_sort_info[b];
        spell_sort_info[b] = tmp;
      }
}

char *show_skill_quality(int percent)
{
  static char buf[256];

  if (percent == 0)
    strcpy(buf, " (not learned)");
  else if (percent <= 10)
    strcpy(buf, " (awful)");
  else if (percent <= 20)
    strcpy(buf, " (bad)");
  else if (percent <= 40)
    strcpy(buf, " (poor)");
  else if (percent <= 55)
    strcpy(buf, " (average)");
  else if (percent <= 70)
    strcpy(buf, " (fair)");
  else if (percent <= 80)
    strcpy(buf, " (good)");
  else if (percent <= 85)
    strcpy(buf, " (very good)");
  else
    strcpy(buf, " (superb)");

  return (buf);
}

ACMD(do_skillstat)
{
  extern struct spell_info_type spell_info[];
  int i, sortpos = 0;
  struct char_data *vict = 0;

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Stats on who's skills?\r\n", ch);
    return;
  }

  if ((vict = get_char_vis(ch, arg))) {
    sprintf(buf, "%s has %d practice sessions left.\r\n", GET_NAME(vict),
      GET_PRACTICES(vict));
    sprintf(buf + strlen(buf), "And knows the following skills/spells:\r\n");
    strcpy(buf2, buf);

    for (sortpos = 1; sortpos < MAX_SKILLS; sortpos++) {
      i = spell_sort_info[sortpos];
      if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
        strcat(buf2, "**OVERFLOW**\r\n");
        break;
      }
      if (GET_LEVEL(vict) >= spell_info[i].min_level[(int) GET_CLASS(vict)]) {
        sprintf(buf, "%-20s (%-3d) %s\r\n", spells[i], 
              GET_SKILL(vict, i), show_skill_quality(GET_SKILL(vict, i)));
        strcat(buf2, buf);
      }
    }
  }
  else
    strcpy(buf2, "No such player around.\r\n");

  page_string(ch->desc, buf2, 1);
}

ACMD(do_file)
{
  FILE *req_file;
  int cur_line = 0,
      num_lines = 0,
      req_lines = 0,
      i, 
      j;  
  int l;
  char field[MAX_INPUT_LENGTH], 
       value[MAX_INPUT_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
  } fields[] = {
    { "none",           LVL_GRGOD,    "Does Nothing" },     
    { "bug",	        LVL_GRGOD,  "../lib/misc/bugs"},   
    { "typo",		LVL_GRGOD,  "../lib/misc/typos"},  
    { "ideas",		LVL_GRGOD,  "../lib/misc/ideas"},
    { "xnames",		LVL_CODER,   "../lib/misc/xnames"},
    { "levels",         LVL_GRGOD,    "../log/levels" }, 
    { "rip",            LVL_GRGOD,    "../log/rip" }, 
    { "players",        LVL_GRGOD,    "../log/newplayers" },
    { "rentgone",       LVL_GRGOD,    "../log/rentgone" }, 
    { "godcmds",        LVL_BUILDER,    "../log/godcmds" }, 
    { "syslog",         LVL_BUILDER,    "../syslog" },  
    { "crash",          LVL_CODER,    "../syslog.CRASH" },  
    { "dts",            LVL_GRGOD,    "../log/dts" }, 
    { "errors",         LVL_CODER,    "../log/errors" }, 
      
    { "\n", 0, "\n" }
  };

  skip_spaces(&argument);

  if (!*argument) 
  {
    strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	sprintf(buf, "%s%-15s%s\r\n", buf, fields[i].cmd, fields[i].file);
    send_to_char(buf, ch);
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;
      
  if(*(fields[l].cmd) == '\n')
  {
    send_to_char("That is not a valid option!\r\n", ch);
    return;
  }     

  if (GET_LEVEL(ch) < fields[l].level) 
  {
    send_to_char("You are not godly enough to view that file!\r\n", ch);
    return;
  }
 
  if(!*value)
     req_lines = 15; /* default is the last 15 lines */
  else
     req_lines = atoi(value);

  /* open the requested file */
  if (!(req_file=fopen(fields[l].file,"r")))
  {
     sprintf(buf2, "SYSERR: Error opening file %s using 'file' command.", 
             fields[l].file);
     mudlog(buf2, BRF, LVL_IMPL, TRUE);
     return;
  }

  /* count lines in requested file */
  get_line(req_file,buf);
  while (!feof(req_file))
  {
     num_lines++;
     get_line(req_file,buf);
  }
  fclose(req_file);
  
  
  /* Limit # of lines printed to # requested or # of lines in file or
     150 lines */
  if(req_lines > num_lines) req_lines = num_lines;
  if(req_lines > 150) req_lines = 150;


  /* close and re-open */
  if (!(req_file=fopen(fields[l].file,"r")))
  {
     sprintf(buf2, "SYSERR: Error opening file %s using 'file' command.", 
             fields[l].file);
     mudlog(buf2, BRF, LVL_IMPL, TRUE);
     return;
  }
  
  buf2[0] = '\0';

  /* and print the requested lines */
  get_line(req_file,buf);
  while (!feof(req_file))
  {
     cur_line++;
     if(cur_line > (num_lines - req_lines))
     {
        sprintf(buf2,"%s%s\r\n",buf2, buf);
     }
     get_line(req_file,buf);
   }
   page_string(ch->desc, buf2, 1);
   
   
   fclose(req_file);
}

/* Change Ownership of item from player to IMM
   Idea from Highlands II (hlii.highlands.org 9001)

   Additional code by:
   Christian Duvall - Getting Equipped EQ From Char
   Patrick Dughi    - Schooling me in act()
   AFCervo          - Moral support

   Thanks to everyone that helped make this command! */

ACMD(do_chown)
{
  struct char_data *victim;
  struct obj_data *obj;
  char buf2[80];
  char buf3[80];
  int i, k = 0;

  two_arguments(argument, buf2, buf3);

  if (!*buf2)
    send_to_char("Syntax: chown <object> <character>.\r\n", ch);
  else if (!(victim = get_char_vis(ch, buf3)))
    send_to_char("No one by that name here.\r\n", ch);
  else if (victim == ch)
    send_to_char("Are you sure you're feeling ok?\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("That's really not such a good idea.\r\n", ch);
  else if (!*buf3)
    send_to_char("Syntax: chown <object> <character>.\r\n", ch);
  else {
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) &&
         isname(buf2, GET_EQ(victim, i)->name)) {
        obj_to_char(unequip_char(victim, i), victim);
        k = 1;
      }
    }

  if (!(obj = get_obj_in_list_vis(victim, buf2, victim->carrying))) {
    if (!k && !(obj = get_obj_in_list_vis(victim, buf2, victim->carrying))) {
      sprintf(buf, "%s does not appear to have the %s.\r\n", GET_NAME(victim), buf2);
      send_to_char(buf, ch);
      return;
    }
  }

  act("&n$n makes a magical gesture and $p&n flies from $N to $m.",FALSE,ch,obj,
       victim,TO_NOTVICT);
  act("&n$n makes a magical gesture and $p&n flies away from you to $m.",FALSE,ch,
       obj,victim,TO_VICT);
  act("&nYou make a magical gesture and $p&n flies away from $N to you.",FALSE,ch,
       obj, victim,TO_CHAR);

  obj_from_char(obj);
  obj_to_char(obj, ch);
  save_char(ch, NOWHERE);
  save_char(victim, NOWHERE);
  }
}

ACMD(do_relist) {

void relist();
void reboot_lists();

  relist();
  reboot_wizlists();
  send_to_char("&gWizlist and Immlist regenerated and reloaded.\r\n&w",ch);
}

ACMD(do_addquest)
{
  struct char_data *vict = NULL;
  int addp;
  
  two_arguments(argument, buf, buf2);
  if (!argument || !*argument || !buf || !*buf) {
    send_to_char("Usage: addquest <victim> [points]\r\n", ch);
    return;
  }
  if (!(vict = get_player_vis(ch, buf, 0))) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  } 
  if ((vict == ch) || (GET_LEVEL(vict) >= GET_LEVEL(ch)) ||
    (GET_LEVEL(vict) >= LVL_IMMORT)) {
      send_to_char("AAAAh. Better not.\r\n", ch);
      return;
  }
  addp = atoi(buf2);
  if (addp <= 0 || ((GET_LEVEL(ch) < LVL_IMMORT) && (addp > MAX_QPOINT_ADD))) {
    sprintf(buf, "You may add 1-%d questpoints.\r\n", MAX_QPOINT_ADD);
    send_to_char(buf, ch);
    return;
  }
  if ((addp + QPOINTS(vict)) > MAX_QPOINTS) addp = MAX_QPOINTS-QPOINTS(vict);
  QPOINTS(vict) += addp;
  sprintf(buf, "&m%s has increased your questpoints by &M%d&m.&w\r\n", GET_NAME(ch), addp);
  send_to_char(buf, vict);
  sprintf(buf, "You have increased %s's questpoints by %d.\r\n", GET_NAME(vict), addp);
  send_to_char(buf, ch);
  sprintf(buf, "QUEST: %s adds %d questpoints to %s.", GET_NAME(ch), addp, GET_NAME(vict));
  mudlog(buf, BRF, LVL_IMMORT, TRUE);
  return;
}

ACMD(do_set)
  {
  struct char_data *vict = NULL, *cbuf = NULL;
  struct char_file_u tmp_store;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],
	val_arg[MAX_INPUT_LENGTH];
  int mode = -1, len = 0, player_i = 0, retval;
  char is_file = 0, is_mob = 0, is_player = 0;
  
  half_chop(argument, name, buf);
  
  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
      half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob")) {
    is_mob = 1;
    half_chop(buf, name, buf);
    }
  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field) {
    send_to_char("Usage: set <victim> <field> <value>\r\n", ch);
      return;
    }
  
  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, 0))) {
	send_to_char("There is no such player.\r\n", ch);
	return;
      }
    } else {
      if (!(vict = get_char_vis(ch, name))) {
	send_to_char("There is no such creature.\r\n", ch);
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if ((player_i = load_char(name, &tmp_store)) > -1) {
      store_to_char(&tmp_store, cbuf);
      if (GET_LEVEL(cbuf) >= GET_LEVEL(ch)) {
	free_char(cbuf);
	send_to_char("Sorry, you can't do that.\r\n", ch);
	return;
      }
      vict = cbuf;
    } else {
      free(cbuf);
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;

  /* perform the set */
  retval = perform_set(ch, vict, mode, val_arg);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict, NOWHERE);
    if (is_file) {
      char_to_store(vict, &tmp_store);
      fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
      fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
      send_to_char("Saved in file.\r\n", ch);
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
  }

#define IN_ZONE(zone, room)	(((100*zone) <= room) && (zone_table[real_zone(zone*100)].top >= room))

ACMD(do_links)
{
  int zonenum;
  int i,j,r, room;
  

  skip_spaces(&argument);
  half_chop(argument, buf1, buf);
  if (!argument || !*argument) {
    send_to_char("Syntax: links <zone_vnum> ('.' for zone you are standing in)\r\n", ch);
    return;
  }
  
  if (strcmp(buf1,".")==0) {
    zonenum = zone_table[world[ch->in_room].zone].number;
  }
  else zonenum = atoi(buf1);

  sprintf(buf,"Zone: &M%d&w is linked to the following zones:\r\n",zonenum);
  for (i = zonenum*100; i<= zone_table[real_zone(zonenum*100)].top; i++) {
    r = real_room(i);
    if (r != -1) {
      for (j = 0; j < NUM_OF_DIRS; j++) {
        if (world[r].dir_option[j]) {
          room = world[r].dir_option[j]->to_room;
          if (room != NOWHERE && (!IN_ZONE(zonenum, world[room].number)))
          sprintf(buf,"%s&C%3d &c%-28s&w at &g%5d&w (&m%-5s&w) ---> &g%5d&w\r\n", buf, 
            zone_table[world[room].zone].number, 
            zone_table[world[room].zone].name, i, dirs[j], world[room].number);
        }
      }
    }
  }
  send_to_char(buf, ch);
}
  
ACMD(do_depiss)
{
  void forget(struct char_data * ch, struct char_data * victim);

  char buf3[80];
  struct char_data *vic = 0;
  struct char_data *mob  = 0;
 
  two_arguments(argument, buf1, buf2);

  if (!*buf1 || !*buf2) {
    send_to_char("Usage: depiss <player> <mobile>\r\n", ch);
    return;
  }

  if (((vic = get_char_vis(ch, buf1))) && (!IS_NPC(vic))) {
    if (((mob = get_char_vis(ch, buf2))) && (IS_NPC(mob))) {
      if (MOB_FLAGGED(mob, MOB_MEMORY)) 
        forget(mob, vic);
      else {
        send_to_char("Mobile does not have the memory flag set!\r\n", ch);
        return;
      }
    }
    else {
      send_to_char("Sorry, Player Not Found!\r\n", ch);
      return;
    }
  } 
  else {
    send_to_char("Sorry, Mobile Not Found!\r\n", ch);
    return;
  }
  sprintf(buf3, "%s has been removed from %s pissed list.\r\n", 
          GET_NAME(vic), GET_NAME(mob));
  send_to_char(buf3, ch);
}

ACMD(do_repiss)
{
  void remember(struct char_data * ch, struct char_data * victim);

  char buf3[80];
  struct char_data *vic = 0;
  struct char_data *mob  = 0;
 
  two_arguments(argument, buf1, buf2);

  if (!*buf1 || !*buf2) {
    send_to_char("Usage: repiss <player> <mobile>\r\n", ch);
    return;
  }

  if (((vic = get_char_vis(ch, buf1))) && (!IS_NPC(vic))) {
    if (((mob = get_char_vis(ch, buf2))) && (IS_NPC(mob))) {
      if (MOB_FLAGGED(mob, MOB_MEMORY)) 
        remember(mob, vic);
      else {
        send_to_char("Mobile does not have the memory flag set!\r\n", ch);
        return;
      }
    }
    else {
      send_to_char("Sorry, Player Not Found!\r\n", ch);
      return;
    }
  } 
  else {
    send_to_char("Sorry, Mobile Not Found!\r\n", ch);
    return;
  }
  sprintf(buf3, "%s has been added to %s pissed list.\r\n", 
          GET_NAME(vic), GET_NAME(mob));
  send_to_char(buf3, ch);
}

ACMD(do_peace)
{
        struct char_data *vict, *next_v;
        act ("$n decides that everyone should just be friends.",
                FALSE,ch,0,0,TO_ROOM);
        send_to_room("Everything is quite peaceful now.\r\n",ch->in_room);
        for(vict=world[ch->in_room].people;vict;vict=next_v)
        {
                next_v=vict->next_in_room;
                if(IS_NPC(vict)&&(FIGHTING(vict)))
                {
                if(FIGHTING(FIGHTING(vict))==vict)
                        stop_fighting(FIGHTING(vict));
                stop_fighting(vict);

                }
        }
}

ACMD(do_xname)
{
   char tempname[MAX_INPUT_LENGTH+1];
   int i = 0;
   FILE *fp;
   *buf = '\0';

   one_argument(argument, buf);
   
   if(!*buf) {
      send_to_char("Xname which name?\r\n", ch);
      return;
   }
      
   if (strlen(buf) >= MAX_INPUT_LENGTH) {
     send_to_char("Too long name for xname!\r\n", ch);
     return;
   }
      
   if(!(fp = fopen(XNAME_FILE, "a"))) {
      perror("Problems opening xname file for do_xname");
      return;
   }
   
   strcpy(tempname, buf);
   for (i = 0; tempname[i]; i++)
      tempname[i] = LOWER(tempname[i]);
   fprintf(fp, "%s\n", tempname);
   fclose(fp);
   sprintf(buf1, "%s has been xnamed!\r\n", tempname);
   send_to_char(buf1, ch);
   Read_Invalid_List();
}

ACMD(do_copyto)
{

/* Only works if you have Oasis OLC */
extern void olc_add_to_save_list(int zone, byte type);

  int iroom = 0, rroom = 0;
  struct room_data *room;

  one_argument(argument, buf2); 
  /* buf2 is room to copy to */

	CREATE (room, struct room_data, 1);	

	iroom = atoi(buf2);
	rroom = real_room(atoi(buf2));
	*room = world[rroom];

 if (!*buf2) {
    send_to_char("Format: copyto <room number>\r\n", ch); 
    return; }
 if (rroom <= 0) {
	sprintf(buf, "There is no room with the number %d.\r\n", iroom);
	send_to_char(buf, ch);
	return; }
   CHECK_OLC_PERM(ch, iroom);
/* Main stuff */
  if (world[ch->in_room].description) {
    if (world[rroom].description)
      free(world[rroom].description);
    world[rroom].description = str_dup(world[ch->in_room].description);
    /* Only works if you have Oasis OLC */
    olc_add_to_save_list((iroom/100), OLC_SAVE_ROOM);
    sprintf(buf, "You copy the description to room %d.\r\n", iroom);
    send_to_char(buf, ch); 
  } else
    send_to_char("This room has no description!\r\n", ch);
}


void list_bit(const char *bitv_strings[], int count, int bit_per_line, char *str_buf)
{
  int i, c = 0;
  *str_buf = '\0';
  for (i = 0; i < count; i++) {
    sprintf(buf1, "%-*s", 80 / bit_per_line - 1, bitv_strings[i]);
    strcat(str_buf, buf1);
    c++;
    if (c >= bit_per_line) {
      c = 0;
      strcat(str_buf, "\r\n");
    }
  }
  if (c) strcat(str_buf, "\r\n");
}

int search_bit(const char *bitv_strings[], int count, char *arg)
{
  int i;
  for (i = 0; i < count; i++) {
    if (is_abbrev(arg, bitv_strings[i])) return i;
  }
  return -1;
}

#define MASK_ITEM_TYPE		0
#define MASK_WEAR		1

void list_objects(int mask_type, int mask, char *str_buf)
{
  int nr, found = 0, ok;
  *str_buf = '\0';
  for (nr = 0; nr <= top_of_objt; nr++) {
    ok = 0;
    switch (mask_type) {
      case MASK_ITEM_TYPE:
        if (GET_OBJ_TYPE(&obj_proto[nr]) == mask) ok = 1;
        break;
      case MASK_WEAR:
        if (GET_OBJ_WEAR(&obj_proto[nr]) & mask) ok = 1;
        break;
    }
    if (ok) {
      sprintf(str_buf + strlen(str_buf), "%3d. %s[%5d] %s\r\n", ++found, 
        IS_OBJ_STAT2(&obj_proto[nr], ITEM2_DELETED) ? "&R*&w" : " ",
        obj_index[nr].vnum, obj_proto[nr].short_description);
    }
  }
  if (!found) strcpy(str_buf, "&WNone found.&w\r\n");
}

ACMD(do_vwear)
{
  int i;
 
  one_argument(argument, buf);
  if (!buf || !*buf) {
    send_to_char("Usage: vwear <wear position | item type>\r\n", ch);
    send_to_char("&WPossible wear positions:&w\r\n", ch);
    list_bit(wear_bits, NUM_ITEM_WEARS, 5, buf);
    send_to_char(buf, ch);
    send_to_char("&WPossible item types:&w\r\n", ch);
    list_bit(item_types, NUM_ITEM_TYPES, 5, buf);
    send_to_char(buf, ch);
    return;
  }
  i = search_bit(wear_bits, NUM_ITEM_WEARS, buf);
  if (i == -1) {
    i = search_bit(item_types, NUM_ITEM_TYPES, buf);  
    if (i == -1) {
      send_to_char("Parameter is neither wear position nor item type, use vwear w/o parameters\r\n"
                   "to see valid parameters.\r\n", ch);
      return;
    }
    list_objects(MASK_ITEM_TYPE, i, buf);
    page_string(ch->desc, buf, TRUE);
    return;
  }
  list_objects(MASK_WEAR, 1 << i, buf);
  page_string(ch->desc, buf, TRUE);
}

typedef const char *(*verify_func)(int);

#define VALID_EXIT(x, y) (world[(x)].dir_option[(y)] && \
                          (TOROOM(x, y) != NOWHERE))
#define IS_DOOR(x, y) (IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_ISDOOR))
#define IS_CLOSED(x, y) (IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_CLOSED))
#define MAX_REPORT_ERR		50

const char *check_exits(int rnum)
{
  static char retbuf[MAX_STRING_LENGTH];
  int i;

  if (rnum == NOWHERE) return NULL;

  retbuf[0] = 0;

  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (VALID_EXIT(rnum, i)) {
      if (TOROOM(TOROOM(rnum, i), rev_dir[i]) != rnum) {
        sprintf(retbuf + strlen(retbuf), "Room %d: going %s then %s doesn't return to starting room\r\n",
                world[rnum].number, dirs[i], dirs[rev_dir[i]]);
      }
      if (TOROOM(rnum, i) == NOWHERE) {
        sprintf(retbuf + strlen(retbuf), "Room %d: door to the %s leads nowhere\r\n",
                world[rnum].number, dirs[i]);
      }
      if (IS_DOOR(rnum, i) && (!VALID_EXIT(TOROOM(rnum, i), rev_dir[i]) || !IS_DOOR(TOROOM(rnum, i), rev_dir[i]))) {
        sprintf(retbuf + strlen(retbuf), "Room %d: door to the %s doesn't have a door on the other side\r\n",
                world[rnum].number, dirs[i]);
      }
      if (IS_CLOSED(rnum, i) && (!VALID_EXIT(TOROOM(rnum, i), rev_dir[i]) || !IS_CLOSED(TOROOM(rnum, i), rev_dir[i]))) {
        sprintf(retbuf + strlen(retbuf), "Room %d: door to the %s is closed, but other side isn't\r\n",
                world[rnum].number, dirs[i]);
      }
    }
  }
  if (retbuf[0])
    return retbuf;
  else
    return NULL;
}

ACMD(do_verify)
{
  int a = -1, b = -1, i, vfn;
  verify_func vfs[] = {check_exits, NULL};
  const char *s;
  int cnt;

  sscanf(argument, "%d %d", &a, &b);

  buf[0] = 0;

  if (a < 0 || b < 0 || a == b) {
    send_to_char("Usage: verify <first room> <last room>\r\n", ch);
    return;
  }
  cnt = 0;
  for (vfn = 0; vfs[vfn]; vfn++) {
    for (i = a; i <= b; i++) {
      s = vfs[vfn](real_room(i));
      if (s && (cnt > MAX_REPORT_ERR)) break;
      
      if (s) {
        strcat(buf, s);
        cnt++;
      }
    }
  }

  if (buf[0]) {
    if (cnt > MAX_REPORT_ERR)
      strcat(buf, "&ROVERFLOW.&w\r\n");
    page_string(ch->desc, buf, 1);
  } else
    send_to_char("No errors found.\r\n", ch);
}



ACMD(do_settime)
{
  int h;
  
  one_argument(argument, buf);
  if (!buf || !*buf) {
    send_to_char("Usage: settime <hour>\r\n", ch);
    return;
  }
  h = atoi(buf);
  if (h < 0 || h > 23) {
    send_to_char("Hour: 0-23\r\n", ch);
    return;
  }
  if ((time_info.hours >= 21 || time_info.hours < 6) && (h >= 6 && h < 21))
    send_to_all("The sun suddenly appears above the horizon. You are totally confused, "
    "because you thougth it should be the dark night at this time.\r\n");
  else if ((time_info.hours >= 6 && time_info.hours < 21) && (h < 6 || h >= 21 ))
    send_to_all("The sun suddenly disappears replaced by the white, coldly shining moon.\r\nYou are confused, "
    "because you thougth that several hours left till darkening.\r\n");
  else send_to_all("You are suddenly a bit confused about the time.\r\n");
  
  sprintf(buf, "(GC) %s has set time from %d to %d o'clock.", GET_NAME(ch), 
        time_info.hours, h);
	mudlog(buf, NRM, LVL_GOD, TRUE);
  time_info.hours = h;
}

ACMD(do_create)
{
  struct obj_data *miscobj; 
  skip_spaces(&argument);


  if (!*argument) {
    send_to_char("Create what?\r\n", ch);
    return;
  }

  miscobj = create_obj();

  miscobj->item_number = NOTHING;
  miscobj->in_room = NOWHERE;

  sprintf(buf2, "createobj %s", argument);
  miscobj->name = str_dup(buf2);

  sprintf(buf2, "%s has been left here.", argument);
  miscobj->description = str_dup(buf2);

  sprintf(buf2, "%s", argument);
  miscobj->short_description = str_dup(buf2);

  GET_OBJ_TYPE(miscobj) = ITEM_OTHER;
  GET_OBJ_WEAR(miscobj) = ITEM_WEAR_TAKE + ITEM_WEAR_HOLD;
  GET_OBJ_EXTRA(miscobj) = ITEM_NORENT;
  GET_OBJ_WEIGHT(miscobj) = 1;

  obj_to_char(miscobj, ch);       
 
  act("$n skillfully creates something!\r\n", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char("You skillfully create something! \r\n", ch);
}


ACMD(do_scare)
{
struct obj_data *whip; 
  skip_spaces(&argument);


  if (!*argument) {
    send_to_char("Specify a mortal to scare.\r\n", ch);
    return;
  }

  whip = create_obj();

  whip->item_number = NOTHING;
  whip->in_room = NOWHERE;
  whip->name = str_dup("whip");

  sprintf(buf2, "A whip made out of %s hide is lying here.", argument);
  whip->description = str_dup(buf2);

  sprintf(buf2, "a whip made out of %s hide", argument);
  whip->short_description = str_dup(buf2);

  GET_OBJ_TYPE(whip) = ITEM_WEAPON;
  GET_OBJ_WEAR(whip) = ITEM_WEAR_TAKE + ITEM_WEAR_WIELD;
  GET_OBJ_EXTRA(whip) = ITEM_NORENT;
  GET_OBJ_VAL(whip, 0) = 5;
  GET_OBJ_VAL(whip, 1) = 10;
  GET_OBJ_VAL(whip, 2) = 10;  
  GET_OBJ_WEIGHT(whip) = 1;
  GET_OBJ_RENT(whip) = 100000;
  GET_OBJ_TIMER(whip) = 100000;

  obj_to_char(whip, ch); 

  act("$n skillfully creates a whip! You feel scared!", FALSE, ch, 0, 0, TO_ROOM);
  send_to_char("You skillfully create a whip! \r\n", ch);
}
