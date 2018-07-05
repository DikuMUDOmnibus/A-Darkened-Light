/***********
* extras.c Special file for Darkened Lights Codebase.   *
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
//#include "constants.h"
#include "clan.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;    /*. db.c        .*/
extern int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);

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



/*MJ*/
int perform_push(struct char_data *ch, int dir, int need_specials_check,
            struct char_data *attacker )
{
  extern char *dirs[];
  int was_in;
  int House_can_enter(struct char_data * ch, sh_int house);
  void death_cry(struct char_data * ch);
  int special(struct char_data *ch, int cmd, char *arg);

  if (need_specials_check && special(ch, dir + 1, ""))
    return 0;

  /* charmed? */
  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master && ch->in_room == ch->master->in_room) {
    send_to_char("The thought of leaving your master makes you weep.\r\n.\r\n", ch);
    act("$n burst into tears.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, world[EXIT(ch, dir)->to_room].number)) {
      send_to_char("You are pushed, but you can't tresspass!\r\n", ch);
      return 0;
    }
  }
  if (IS_SET(ROOM_FLAGS(EXIT(ch, dir)->to_room), ROOM_TUNNEL) &&
      world[EXIT(ch, dir)->to_room].people != NULL) {
    send_to_char("You are pushed, but there isn't enough room.\r\n", ch);
    return 0;
  }
  sprintf(buf2, "$n is pushed to the %s by $N.", dirs[dir] );
  act(buf2, TRUE, ch, 0, attacker, TO_NOTVICT);
  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  if (!IS_AFFECTED(ch, AFF_SNEAK))
    act("$n fall rolling on the ground", TRUE, ch, 0, 0, TO_ROOM);

  if (ch->desc != NULL)
    look_at_room(ch, 0);

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH) && GET_LEVEL(ch) < 
LVL_IMMORT)
  {
    death_cry(ch);
    extract_char(ch);
    return 0;
  }
  return 1;
}


ACMD(do_push)
{
  char name[100], todir[256];
  int to;
  struct char_data *victim=NULL;
  extern char *dirs[];

  if (GET_CLASS(ch) != CLASS_THIEF) {
    send_to_char("Huh?!",ch);
     return;
 }

             if (GET_HYBRID(ch) < 3) {
     send_to_char("Enchantments requires level 3 Hybrid!\r\n",ch);
     return;
}

if (GET_HYBRID(ch) > 2) {

  half_chop(argument, name, todir);
  if (!*name || !*todir)
    send_to_char("Push whom where?\r\n", ch);
  else if (!(victim = get_char_room_vis(ch, name)) )
    send_to_char("Nowhere by that name here.\r\n", ch);
 // else if (MOB_FLAGGED(victim, MOB_SPEC))
 //   send_to_char("You cannot push this one, you haven't the 
//strength.\r\n", ch);
  else if (ch == victim)
    send_to_char("But... can't you just walk?\r\n", ch);
  else if (!ok_damage_shopkeeper(ch, victim))
    send_to_char("You can't push shopkeepers, sorry.\r\n", ch);
//  else if (IS_AFFECTED(ch, AFF_CHARM))
//    send_to_char("No, no... that's your master.\r\n", ch);
  else if ((to = search_block(todir, (const char **)dirs, FALSE)) < 0) {
    send_to_char( "That is not a direction.\r\n", ch );
    return;
  } else {
    strcpy( todir, dirs[to] );
    if (GET_POS(victim) <= POS_SITTING ) {
      send_to_char( "You can't push anybody who is lying on the ground.\r\n", ch);
      return;
    }
    if (GET_POS(victim) == POS_FIGHTING) {
      sprintf( buf, "No! you can't push %s while fighting!\r\n", HSSH(ch));
      send_to_char( buf, ch );
      return;
    }
//    if (IS_AFFECTED(victim, AFF_BALANCE)) {
//      act("$n tries to push $N, but $N easily keeps $S footing.",
//           FALSE, ch, 0, victim, TO_NOTVICT);
//      act("You try to push $N out of the room, but he is too well 
//balanced.",
//           FALSE, ch, 0 , victim, TO_CHAR);
//      act("$n tries to push you, but you are too well balanced.",
//           FALSE, ch, 0, victim, TO_VICT);
//      return;
   // }
    sprintf(buf, "$n is trying to push you to the %s!", todir);
    act( buf, FALSE, ch, 0, victim, TO_VICT );
    act( "$n is trying to push $N", FALSE, ch, 0, victim, TO_NOTVICT);
    if (!CAN_GO( victim, to)) {
      act("You can't push $M there - there's a closed door.", 
           FALSE, ch, 0, victim, TO_CHAR);
    } else if ( GET_LEVEL(victim) >= LVL_IMMORT && GET_LEVEL(ch) != 
LVL_IMPL) {
      send_to_char( "Oh, no, no, no.\r\n", ch );
      send_to_char( "Is trying to push you... what a mistake!\r\n", 
victim);
    } else if ( (GET_LEVEL(victim) - GET_LEVEL(ch) > 4) &&
                 GET_CLASS(ch) < CLASS_WARRIOR ) {
      sprintf( buf, "You can't push %s.\r\n", HMHR(victim) );
      send_to_char( buf, ch );
      sprintf( buf, "%s can't push you.\r\n", GET_NAME(ch) );
      send_to_char( buf, victim );
    } 
//else if ( MOB_FLAGGED(victim, MOB_NOBASH)) {
//      send_to_char( "Ouch! Is too big for you!\r\n", ch );
     else if ((dice(1,20)+3)-(GET_STR(ch)-GET_STR(victim)) < GET_STR(ch)) 
{
     /* You can balance the check above, this works fine for me */
      if (perform_push(victim, to, TRUE, ch)) {
        sprintf(buf, "\r\nYou give %s a good shove.\r\n", 
GET_NAME(victim));
        send_to_char(buf, ch);
        sprintf( buf, "\r\n/cw%s has pushed you!/c0\r\n", GET_NAME(ch));
        send_to_char( buf, victim );
      }
    } else {
      send_to_char( "Oops... you fail.", ch );
      sprintf( buf, "%s fail.\r\n", HSSH(ch) );
      *buf = UPPER(*buf);
      send_to_char( buf, victim );
    }
  }
}
}
