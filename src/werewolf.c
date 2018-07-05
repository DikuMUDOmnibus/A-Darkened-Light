/**********************************************************
*pp   file: wizard.c                 External Class Powers*
*                                                         *
*       Disclamer, For Darkened lights, external power    *
*   if you use this info put my name (Mike Ryan) in your  *
* credit information, otherwise have fun.                 *
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
#include "guild.h"

/* extern variables */
extern struct char_data *ch;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern int pk_allowed;
extern struct spell_info_type spell_info[];
extern struct int_app_type int_app[];
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct time_info_data time_info;
extern char *item_types[];
extern char *extra_bits[];
extern int spell_sort_info[MAX_SKILLS+1];
extern int prac_params[4][NUM_CLASSES];
extern int cmd_say, cmd_tell;
extern int spell_sort_info[MAX_SKILLS+1];
extern char *spells[];
extern struct zone_data *zone_table;
extern int top_of_zone_table;

/* extern functions */
void raw_kill(struct char_data * ch);
void check_killer(struct char_data * ch, struct char_data * vict);
room_rnum find_target_room(struct char_data * ch, char *rawroomstr);
void perform_immort_invis(struct char_data *ch, int level);
void perform_immort_vis(struct char_data *ch);
void list_skills(struct char_data * ch);



char *prac_types[] = {
  "spell",
  "skill"
};

ACMD(do_book)
{
if (GET_CLASS(ch) != CLASS_MAGIC_USER) {
   send_to_char("Huh?!\r\n",ch);
    return;
}

if (GET_CLASS(ch) == CLASS_MAGIC_USER) {
    list_skills(ch);
    return;
 }
}

ACMD(do_runes)
{
  int skill_num, percent;

  extern struct spell_info_type spell_info[];
  extern struct int_app_type int_app[];

  if (GET_CLASS(ch) != CLASS_MAGIC_USER) {
     send_to_char("Huh?!?\r\n",ch);
     return;
  }

  //if (IS_NPC(ch) || !CMD_IS("practice"))
  //  return 0;

  skip_spaces(&argument);

  if (!*argument) {
   send_to_char("Syntax: Study <spell or combat>, use BOOK to see spells & combat variations.\r\n",ch);
//    list_skills(ch);
    return;
  }
  if (GET_PRACTICES(ch) <= 0) {
    send_to_char("You do not seem to be able to study now.\r\n", ch);
    return;
  }

  skill_num = find_skill_num(argument);

  if (skill_num < 1 ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
    sprintf(buf, "You do not know of that %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return;
  }
if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
  send_to_char("You are already learned in that area.\r\n", ch);
  return;
}
  send_to_char("You gain knowlege from a study of your spell book...\r\n", 
ch);
  GET_PRACTICES(ch)--;

  percent = GET_SKILL(ch, skill_num);
  percent += MIN(MAXGAIN(ch, skill_num), MAX(MINGAIN(ch, skill_num), int_app[GET_INT(ch)].learn));

  SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);

  return;
}
ACMD(do_evap_fog)
{

  if (GET_CLASS(ch) != CLASS_MAGIC_USER) {
  send_to_char("Huh?!?\r\n",ch);
  return;
}
   if (!ROOM_AFFECTED(ch->in_room, RAFF_FOG)) {
      send_to_char("There is no fog to evaporate here.\r\n",ch);
      return;
      }

  if (ROOM_AFFECTED(ch->in_room, RAFF_FOG)) {
   send_to_char("You generate heat from your body and force the fog to clear the room!",ch);
    REMOVE_BIT(ROOM_AFFECTIONS(ch->in_room), RAFF_FOG);
   return;
 }
}

ACMD(do_fog)
{
 // struct affected_type *af;
  // struct char_data *ch;
  

  if (GET_CLASS(ch) != CLASS_THIEF) {
      send_to_char("Huh?!?\r\n",ch);
      return;
}


   if (GET_CELERITY(ch) < 4) {
   send_to_char("fog requires level 4 celerity to be preformed.\r\n",ch);
   return;
}
        

  if (GET_CELERITY(ch) > 3) {


// if (ROOM_AFFECTED(ch->in_room, RAFF_FOG)) {
//   send_to_char("You suck the clouds from the room!",ch);
//    REMOVE_BIT(ROOM_AFFECTIONS(ch->in_room), RAFF_FOG);
//   return;
//} else


if (GET_MANA(ch) > 200) {
    send_to_char("You open your mouth and suck all light from the room\r\n",ch);
    SET_BIT(ROOM_AFFECTIONS(ch->in_room), RAFF_FOG);  
    return;
  }
 }
}
ACMD(do_darkness)
{
  // struct char_data *ch;

  if (GET_CLASS(ch) != CLASS_WARRIOR) {
      send_to_char("Huh?!?\r\n",ch);
      return;
}  
        if (GET_MULIAN(ch) < 5) {
     send_to_char("Darkness requires level 5 mulian.\r\n",ch);
     return;
}

if (GET_MULIAN(ch) > 4) {


if (GET_MANA(ch) > 200) {
    send_to_char("You open your mouth and suck all light from the room",ch);
    SET_BIT(ROOM_FLAGS(IN_ROOM(ch)), ROOM_DARK);
     return;
}
}
}
