/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
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
#include "screen.h"

/* extern variables */
extern int level_can_shout;
extern int holler_move_cost;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern void mprog_speech_trigger(char *txt, struct char_data *mob);


/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);

//---------------------------------------------------------------------
// do_say command takes one argument string and passes it to the queue
//---------------------------------------------------------------------
ACMD(do_say) {
  skip_spaces(&argument);

  // no argument means failure
  if (!*argument) {
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  // we have argument so pass the queue along
  } else {
    sprintf(buf, "&c$n says,&w \"%s\"", argument);
    MOBTrigger = FALSE;
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    //  some people dont like command repeats so stifle that here
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      sprintf(buf, "&gYou say,&w \"%s\"", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }  
    mprog_speech_trigger(argument, ch);
  }
}
// end do_say

ACMD(do_gsay) {
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char("But you are not the member of a group!\r\n", ch);
    return;
  }

  if (!*argument) {
    send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
  } else {
    if (ch->master) {
      k = ch->master;
    } else {
      k = ch;
    }
    sprintf(buf, "&W$n tells the group:&Y %s&w", argument);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch)) {
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    }
    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch)) {
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);
      }
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      sprintf(buf, "&WYou tell the group: &Y%s&w", argument);
      act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    }
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  sprintf(buf, "&m$n tells you,&w \"&c%s&w\"", arg);
  act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    sprintf(buf, "&GYou tell $N,&w \"%s\"", arg);
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  }

  if (!IS_NPC(vict) && !IS_NPC(ch))
  GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
  else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
    send_to_char("The walls seem to absorb your words.\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return TRUE;

  return FALSE;
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict;

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char("Who do you wish to tell what??\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (is_tell_ok(ch, vict)) {
    if (!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_AFK)) {
        sprintf(buf, "&c%s&w is AFK and may not hear your tell.\r\n", GET_NAME(vict));
        send_to_char(buf, ch);
        sprintf(buf, "MESSAGE: %s\r\n", GET_AFK(vict));
        send_to_char(buf, ch);
    }
    perform_tell(ch, vict, buf2);
  }
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char("You have no-one to reply to!\r\n", ch);
  else if (!*argument)
    send_to_char("What is your reply?\r\n", ch);
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */
				     
    /*
     * XXX: A descriptor list based search would be faster although
     *      we could not find link dead people.  Not that they can
     *      hear tells anyway. :) -gg 2/24/98
     */
    while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (tch == NULL)
      send_to_char("They are no longer playing.\r\n", ch);
    else if (is_tell_ok(ch, tch))
      perform_tell(ch, tch, argument);
  }
}


ACMD(do_spec_comm)
{
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  if (subcmd == SCMD_WHISPER) {
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
  } else {
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
    send_to_char(buf, ch);
  } else if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (vict == ch)
    send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
  else {
    sprintf(buf, "$n %s you, '%s'", action_plur, buf2);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper = 0, *pen = 0;
  char *papername, *penname;

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", penname);
      send_to_char(buf, ch);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = 0;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char("That thing has nothing to do with writing.\r\n", ch);
      return;
    }
    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD)) {
      sprintf(buf, "You can't write with %s %s alone.\r\n", AN(papername),
	      papername);
      send_to_char(buf, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
      send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD);
    else
      pen = GET_EQ(ch, WEAR_HOLD);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else if (paper->action_description)
    send_to_char("There's something written on it already.\r\n", ch);
  else {
    /* we can write - hooray! */
     /* this is the PERFECT code example of how to set up:
      * a) the text editor with a message already loaed
      * b) the abort buffer if the player aborts the message
      */
     ch->desc->backstr = NULL;
     send_to_char("Write your note.  (/s saves /h for help)\r\n", ch);
     /* ok, here we check for a message ALREADY on the paper */
     if (paper->action_description) {
	/* we str_dup the original text to the descriptors->backstr */
	ch->desc->backstr = str_dup(paper->action_description);
	/* send to the player what was on the paper (cause this is already */
	/* loaded into the editor) */
	send_to_char(paper->action_description, ch);
     }
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
     /* assign the descriptor's->str the value of the pointer to the text */
     /* pointer so that we can reallocate as needed (hopefully that made */
     /* sense :>) */
    ch->desc->str = &paper->action_description;
    ch->desc->max_str = MAX_NOTE_LENGTH;
  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char("Monsters can't page.. go away.\r\n", ch);
  else if (!*arg)
    send_to_char("Whom do you wish to page?\r\n", ch);
  else {
    sprintf(buf, "\007\007*%s* %s\r\n", GET_NAME(ch), buf2);
    if (!str_cmp(arg, "all")) {
      if (GET_LEVEL(ch) > LVL_GOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char("You will never be godly enough to do that!\r\n", ch);
      return;
    }
    if ((vict = get_char_vis(ch, arg)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
      return;
    } else
      send_to_char("There is no such person in the game!\r\n", ch);
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  byte gmote = FALSE;

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][4] = {
    {"You cannot holler!!\r\n",
      "holler",
      "",
    KYEL},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
    KYEL},

    {"You cannot gossip!!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n",
    KYEL},

    {"You cannot auction!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n",
    KMAG},

    {"You cannot congratulate!\r\n",
      "congrat",
      "You aren't even on the channel!\r\n",
    KGRN}
  };

  ACMD(do_gmote);

  /* to keep pets, etc from being ordered to shout */
  if (!ch->desc)
    return;

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(com_msgs[subcmd][0], ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < level_can_shout) {
    sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
	    level_can_shout, com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  /* make sure the char is on the channel */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);
  /* add these four lines */
  if(subcmd == SCMD_GMOTE || (subcmd == SCMD_GOSSIP && *argument == '@')) {
    subcmd = SCMD_GOSSIP;
    gmote = TRUE;
  }

  /* make sure that there is something there to say! */
  if (!*argument) {
    sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
	    com_msgs[subcmd][1], com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
   if (gmote) {
    if (*argument == '@')
      do_gmote(ch, argument + 1, 0, 1);
    else
      do_gmote(ch, argument, 0, 1);
    return;
  }
  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < holler_move_cost) {
      send_to_char("You're too exhausted to holler.\r\n", ch);
      return;
    } else
      GET_MOVE(ch) -= holler_move_cost;  
  }
  /* set up the color on code */
  strcpy(color_on, com_msgs[subcmd][3]);

  /* first, set up strings to be given to the communicator */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    if (COLOR_LEV(ch) >= C_CMP)
      sprintf(buf1, "%sYou %s, '%s'%s", color_on, com_msgs[subcmd][1],
	      argument, KNRM);
    else
      sprintf(buf1, "You %s, '%s'", com_msgs[subcmd][1], argument);
    act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
  }

  sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], argument);

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && i != ch->desc && i->character && (
	IS_NPC(i->character) || (!PRF_FLAGGED(i->character, channels[subcmd]) &&
	!PLR_FLAGGED(i->character, PLR_WRITING))) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	   GET_POS(i->character) < POS_RESTING))
	continue;

      if (COLOR_LEV(i->character) >= C_NRM)
	send_to_char(color_on, i->character);
      act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
      if (COLOR_LEV(i->character) >= C_NRM)
	send_to_char(KNRM, i->character);
    }
  }
}


ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char("You aren't even part of the quest!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
	    CMD_NAME);
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      if (subcmd == SCMD_QSAY)
	sprintf(buf, "You quest-say, '%s'", argument);
      else
	strcpy(buf, argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    if (subcmd == SCMD_QSAY)
      sprintf(buf, "$n quest-says, '%s'", argument);
    else
      strcpy(buf, argument);

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc &&
	  PRF_FLAGGED(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}
