/*************************************************************************
*   File: clan.c				    Addition to CircleMUD *
*  Usage: Source file for clan-specific code                              *
*                                                                         *
*  Copyright (c) to Daniel Muller					  *
*  Clans ][ Copyright (c)1999 Vladimir Marko (Yilard of VisionMUD)        *
*                         "Choose Your Destiny!"                          *
*									  *
************************************************************************ */

#define __CLAN_C__

#include "conf.h"
#include "sysdep.h"
#include "string.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "comm.h"
#include "screen.h"
#include "clan.h"
#include "handler.h"
#include "olc.h"
#include "constants.h"

/*   external vars  */
extern struct clan_info *clan_index;
extern struct descriptor_data *descriptor_list;
extern char *class_abbrevs[];
extern FILE *player_fl;
extern int clan_top;
extern int mini_mud;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct char_data *mob_proto;
extern HOMETOWN hometowns[];			/* constants.c */
extern struct room_data *world;

/* Local functions */
void prepare_colour_table(struct char_data *ch);


void write_clan(FILE *fl, int cnum);
void cedit_display_menu(struct descriptor_data *d);
int is_clannum_already_used(int cnum);
void cedit_setup_new(struct descriptor_data *ch, int cnum);
void cedit_setup_existing(struct descriptor_data *d, int cnum);
void display_rank_menu(struct descriptor_data *d, int num);
void display_colour_menu(struct descriptor_data *d);
void fix_illegal_cnum();
int already_being_edited(struct char_data *ch, int cnum);
void save_zone_owners(void);
void free_clan(struct clan_info *clan);
void strip_string(char *buffer);
ACMD(do_clan);

/* Function prototypes */
void page_string(struct descriptor_data * d, char*str, int keep_internally);
ACMD(do_help);
ACMD(do_gen_tog);
ACMD(do_gen_ps);

/* global constants */
char *notitle = "You should never see this-REPORT TO IMPLEMENTOR!";
char *applytitle = "Applying";
char *no_permission = "You have insufficient priority to modify this.\r\n"
                      "Press any key...";

/*
 * Clan levels
*/

/* Define quick reference for colours (global) */
struct clan_colours clan_colour[8];

/*
 * 			General functions go here
*/

#define CHECK_CLAN(value)	{ if (value <= 0 || (ccmd = real_clan(value)) < 0) { \
				    send_to_char("&RThere is no clan with this number!&w\r\n", ch); \
				    return; \
				  } \
				}  

#define DEF_HOMETOWN(ch)	((GET_LEVEL(ch) < LVL_IMMORT) ? \
				   DEFAULT_HOMETOWN : GOD_HOMETOWN)

/* clan is clan real number */
char * get_clan_rank_str(int clan, int rank)
{
   switch (rank) {
     case CLAN_APPLY:   return "(Only applying)"; break;
     case CLAN_SOLDIER: return SOLDIERTITLE(clan_index[clan]); break;
     case CLAN_SARGEANT: return SARGEANTTITLE(clan_index[clan]); break;
     case CLAN_CAPTAIN: return CAPTAINTITLE(clan_index[clan]); break;
     case CLAN_RULER: return RULERTITLE(clan_index[clan]); break;
     case CLAN_BUILDER: return BUILDERTITLE(clan_index[clan]); break;
     case CLAN_RETIRED: return RETIREDTITLE(clan_index[clan]); break;
   
     default: return "ERROR - Report to Implementor!"; break;
   }
}

int check_clan_level(struct char_data *ch)
{
  return 1;
  if (PLAYERCLAN(ch) == 0) return 1;
  if (GET_LEVEL(ch) < CLAN_LIMIT_DOWN(clan_index[PLAYERCLANNUM(ch)]) ||
     GET_LEVEL(ch) > CLAN_LIMIT_UP(clan_index[PLAYERCLANNUM(ch)])) {
     CLANRANK(ch) = CLAN_RETIRED;
     return 0;
     }
  return 1;

}

int can_edit_clan(struct char_data *ch, int clan)
{
  if (clan == 0) return FALSE;
  
  if (GET_LEVEL(ch) >= CLAN_EDIT_GOD_LEVEL)
    return TRUE;
    
  if (PLAYERCLAN(ch) == clan && CLANRANK(ch) >= CLAN_EDIT_LEVEL
    && real_clan(clan) >= 0)
    return TRUE;
    
  return FALSE; 
}

ACMD(do_clan_list)
{
  int ccmd;
  
  /* Display title for list */
  send_to_char("[&W   # Clan Title                Clan Owner     &w]\r\n", ch);
  send_to_char("------------------------------------------------\r\n", ch);
  /* List all clans and owners */
  for (ccmd = 0; ccmd <= clan_top; ccmd++) {
    sprintf(buf, " &R%4d &G%s%-25s&W &c%-15s&w\r\n", 
      CLANNUM(clan_index[ccmd]), CLANCOLOUR(ccmd), 
      CLANNAME(clan_index[ccmd]), 
      CLANOWNER(clan_index[ccmd]));
      send_to_char(buf, ch);
  }
  sprintf(buf, "\r\n[&M%d&w clans displayed.]\r\n", (ccmd));
  send_to_char(buf, ch);

  return;
}

ACMD(do_clan_edit)
{
  int value;
  one_argument(argument, buf);
  
  if ((buf && *buf) || PLAYERCLAN(ch) == 0)
    value = atoi(buf);
  else
    value = PLAYERCLAN(ch);
     
  if (value == 0)
  {
    send_to_char("&RIllegal clan number.&w\r\n", ch);
    return;
  }
      
  if (!can_edit_clan(ch, value))
  {
    send_to_char("Huh?!", ch);
    return;
  }
  
  if (already_being_edited(ch, value) == TRUE) return;
   
  CREATE(ch->desc->olc, struct olc_data, 1);
  if (is_clannum_already_used(value) == TRUE)
    cedit_setup_existing(ch->desc, real_clan(value));
  else
    cedit_setup_new(ch->desc, value);
  OLC_NUM(ch->desc) = value;
  OLC_VAL(ch->desc) = 0;

  STATE(ch->desc) = CON_CLAN_EDIT;
    
  act("$n starts editing clans.", TRUE, ch, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);

  return;  
}

ACMD(do_clan_save)
{
  /* Save clans */
  clan_save(FALSE, ch);
  return;
}

ACMD(do_clan_info)
{
  int value, temp, temp2, temp3, ccmd;
  struct descriptor_data *pt;
  /* Use index to retrieve information on clan */
  value = atoi(argument);
  if (value == 0 && PLAYERCLAN(ch) != 0) value = PLAYERCLAN(ch);
  CHECK_CLAN(value);
  ccmd = real_clan(value);
  temp2 = 0;
	  
  /* Find number of members in clan */
  temp = CLANPLAYERS(clan_index[ccmd]);
  temp2 = 0;
  temp3 = 0;
  /* Find number of clan members currently online */
  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && 
      pt->character->player_specials->saved.clannum == value &&
      CAN_SEE_ONETIME(ch, pt->character)) {
        if (pt->character->player_specials->saved.clanrank > CLAN_APPLY)
          temp2++;
   	else
   	  temp3++;
    }
   	    
   /* Display info header */
   if (GET_LEVEL(ch) >= LVL_IMMORT)	 
     sprintf(buf1, " &m(%d)&w", CLANROOM(clan_index[ccmd]));
   else
     buf1[0] = '\0';
       
   sprintf(buf, "&W[%s", CLANCOLOUR(ccmd));
	  
   sprintf(buf + strlen(buf), 
     "%-25s&W]&w\r\n\r\n"
     "Owner: &c%-15s&w\r\n"
     "Levels: &m%d-%d&w\r\n"
     "Clan hometown: &m%s&w\r\n"
     "Clan room: &r%s%s&w\r\n"
     "Clan Gold: &g%ld&w\r\n"
     "Members currently online: &m%-3d&w\r\nTotal number of members: %-3d\r\n"
     "Applying online: &y%-3d&w\r\n"
     "Clan kills: &g%d&w\r\n"
     "Clan rips: &g%d&w\r\n\r\n"
     "%s%s%s\r\n",
       
     CLANNAME(clan_index[ccmd]),
     CLANOWNER(clan_index[ccmd]),
     CLAN_LIMIT_DOWN(clan_index[ccmd]), CLAN_LIMIT_UP(clan_index[ccmd]),
     hometowns[CLANTOWN(clan_index[ccmd])].homename,
     world[real_room(CLANROOM(clan_index[ccmd]))].name, buf1,
     CLANGOLD(clan_index[ccmd]),
     temp2, temp,
     temp3,
     CLANKILLS(clan_index[ccmd]),
     CLANRIPS(clan_index[ccmd]),
     CCGRN(ch, C_NRM), CLANDESC(clan_index[ccmd]), CCNRM(ch, C_NRM)
         
   );
   send_to_char(buf, ch);
   return;	
}

ACMD(do_clan_apply)
{
  int value, found, ccmd;
  /* This handles clan applications */
  if (CLANRANK(ch) != 0)
  {
    send_to_char("&RYou must resign from the clan you currently belong to, first.&w\r\n", ch);
    return;
  }

  value = 0;
  found = 0;
  value = atoi(argument);
  if (value >= 0)
  {
    for (ccmd = 0; ((ccmd <= clan_top) && (found != 1)); ccmd++)
      if (CLANNUM(clan_index[ccmd]) == value) {
	found = 1;

        if (GET_LEVEL(ch) < CLAN_LIMIT_DOWN(clan_index[ccmd]) ||
          GET_LEVEL(ch) > CLAN_LIMIT_UP(clan_index[ccmd])) {
          send_to_char("You are not proper level to apply to this clan!\r\n", ch);
          return;
        }

	PLAYERCLAN(ch) = CLANNUM(clan_index[ccmd]);
	CLANRANK(ch) = CLAN_APPLY;

	sprintf(buf, "&GYou have applied to the clan '&W%s&G'&w.\r\n", 
          CLANNAME(clan_index[ccmd]));
	send_to_char(buf, ch);
      }

      if (found != 1)
      {
	send_to_char("&RUnable to find that clan.&w\r\n", ch);
	return;
      }
    }
    else
    {
      send_to_char("Which clan number?!\r\n", ch);
      return;
    }
}

ACMD(do_clan_resign)
{
  int ccmd = PLAYERCLANNUM(ch);

  /* This handles clan resignations */
  if ((PLAYERCLANNUM(ch) == 0) && (CLANRANK(ch) == 0))
  {
    send_to_char("&WBut you don't belong to any clan!&w\r\n", ch);
    return;
  }

  if (strcasecmp(GET_NAME(ch), CLANOWNER(clan_index[ccmd])) == 0)
  {
    send_to_char("&RYou can't resign from a clan you own!&w\r\n", ch);
    return;
  }

  /* Update number of members in clan */
  if (CLANRANK(ch) != CLAN_APPLY)
    CLANPLAYERS(clan_index[ccmd]) = CLANPLAYERS(clan_index[ccmd]) - 1;  

  PLAYERCLAN(ch) = 0;
  CLANRANK(ch) = 0;
  GET_HOMETOWN(ch) = DEF_HOMETOWN(ch);
  send_to_char("&WYou have resigned from your clan.&w\r\n", ch);
  return;
}

ACMD(do_clan_enlist)
{
  /* This handles the enlistment of characters who have applied to your
   * clan.
   *
   * Implementors can force an enlistment.  So, when creating a new clan,
   * make sure your owner applies to join it.  Then the implementor must
   * do a 'clan enlist <character>' so that the owner belongs to the clan.
   */
    
  struct char_data *vict;
  int ccmd = PLAYERCLANNUM(ch);
    
  if (GET_LEVEL(ch) < LVL_CHBUILD)
  {
    if ((CLANRANK(ch) == 0) && (PLAYERCLANNUM(ch) == 0))
    {
      send_to_char("&RBut you don't belong to any clan!&w\r\n", ch);
      return;
    }

    if (!(CLANRANK(ch) >= CLAN_SARGEANT))
    {
      send_to_char("&RYou have insufficent rank in your clan to do this.&w\r\n", ch);
      return;
    }

    if ((vict = get_player_vis(ch, argument, 0)))
    {
      if (CLANRANK(vict) != CLAN_APPLY)
      {
	send_to_char("&RBut that person isn't applying to any clan!&w\r\n", ch);
	return;
      }

      if (PLAYERCLAN(vict) != PLAYERCLAN(ch))
      {
        send_to_char("&RBut that person isn't applying to your clan!&w\r\n", ch);
	return;
      }

      if (GET_LEVEL(vict) < CLAN_LIMIT_DOWN(clan_index[PLAYERCLANNUM(vict)]) ||
        GET_LEVEL(vict) > CLAN_LIMIT_UP(clan_index[PLAYERCLANNUM(vict)])) {
        send_to_char("That person is not proper level to join your clan!\r\n", ch);
        return;
      }

      CLANRANK(vict) = CLAN_SOLDIER;
      GET_HOME(vict) = CLAN_HOMETOWN(clan_index[PLAYERCLANNUM(vict)]);
      sprintf(buf, "&WYou have been enlisted into the ranks of the '&Y%s&W' clan.&w\r\n", 
		CLANNAME(clan_index[ccmd]));
      send_to_char(buf, vict);

      sprintf(buf, "&gYou have enlisted &c%s&g into your clan.&w\r\n", 
		GET_NAME(vict));
      send_to_char(buf, ch);

      SET_BIT(PRF_FLAGS(vict), PRF_CLANTALK);

      /* Update number of clan members */
	
      CLANPLAYERS(clan_index[ccmd])++;
	
    }
    else
    {
      send_to_char("&RBut there is noone here by that name!&w\r\n", ch);
      return;
    }
  }
  else
  {

    if ((vict = get_player_vis(ch, argument, 0)))
    {
      if (CLANRANK(vict) != CLAN_APPLY)
      {
	send_to_char("&RBut that person isn't applying to any clan!&w\r\n", ch);
	return;
      }

    if (strcasecmp(GET_NAME(vict), 
		CLANOWNER(clan_index[PLAYERCLANNUM(vict)])) == 0)
      CLANRANK(vict) = CLAN_RULER;
    else
      CLANRANK(vict) = CLAN_SOLDIER;
    log("(GC) %s forced enlistment of %s.", GET_NAME(ch), GET_NAME(vict));

    sprintf(buf, "&gYou have forced the enlistment of &c%s&g into the clan '&m%s&g'&w.\r\n", 
		GET_NAME(vict), CLANNAME(clan_index[PLAYERCLANNUM(vict)]));
    send_to_char(buf, ch);

    sprintf(buf, "&WYou have been enlisted into the clan '&M%s&w'.\r\n", 
		CLANNAME(clan_index[PLAYERCLANNUM(vict)]));
    send_to_char(buf, vict);

    /* Update number of clan members */
    CLANPLAYERS(clan_index[PLAYERCLANNUM(vict)])++;
    }
    else
    {
      send_to_char("&RUnable to find a character of that name.&w\r\n", ch);
      return;
    }

  }
  return;
}

ACMD(do_clan_promote)
{
  /* Clan promotions */
  int players_clannum;
  struct char_data *vict;
  if ((vict = get_player_vis(ch, argument, 0)))
  {
    if (vict == ch && GET_LEVEL(ch) < LVL_COIMPL)
    {
      send_to_char("&RYou can't promote yourself!&w\r\n", ch);
      return;
    }
    
    if (PLAYERCLAN(vict) == 0)
    {
      send_to_char("&RBut that person doesn't belong to ANY clan!&w\r\n", ch);
      return;
    }
      
    players_clannum = PLAYERCLANNUM(vict);

    if (((CLANRANK(vict) < CLAN_SOLDIER) || 
	(PLAYERCLAN(vict) != PLAYERCLAN(ch))) 
	&& GET_LEVEL(ch) < LVL_COIMPL)
    {
      send_to_char("&RYou can't promote someone who doesn't belong to your clan!&w\r\n", ch);
      return;
    }

    if (CLANRANK(vict) >= CLAN_RULER)
    {
      send_to_char("&RYou can't promote this person any higher in rank!&w\r\n", ch);
      return;
    }
      
    if (CLANRANK(vict)+1 >= CLANRANK(ch) && !(CLANRANK(ch) >= CLAN_RULER)
      && GET_LEVEL(ch) < CLAN_EDIT_GOD_LEVEL)
    {
      send_to_char("&RYou have insufficient rank to promote that person!&w\r\n", ch);
      return;
    }

    CLANRANK(vict)++;
    sprintf(buf, "&WYou have been promoted to &C%s&W in your clan.&w\r\n", 
      CLANRANKTITLE(clan_index[players_clannum], CLANRANK(vict)));
    send_to_char(buf, vict);

    sprintf(buf, "&gYou have promoted &c%s&g to the rank of &M%s&g in your clan.&w\r\n", 
		GET_NAME(vict), CLANRANKTITLE(clan_index[players_clannum], CLANRANK(vict)));
    send_to_char(buf, ch);
  }
  else
  {
    send_to_char("Promote who?!\r\n", ch);
    return;
  }
  return;
}

ACMD(do_clan_demote)
{
  /* Clan demotions */
  int players_clannum;
  struct char_data *vict;
  
  if (CLANRANK(ch) == 0 && GET_LEVEL(ch) < LVL_COIMPL)
  {
    send_to_char("&RBut you don't belong to any clan!\r\n&w", ch);
    return;
  }

  if ((vict = get_player_vis(ch, argument, 0)))
  {   
    if (PLAYERCLAN(vict) == 0)
    {
      send_to_char("&RBut that person doesn't belong to ANY clan!&w\r\n", ch);
      return;
    }  
    if (vict == ch && GET_LEVEL(ch) < LVL_COIMPL)
    {
      send_to_char("&RYou can't demote yourself!&w\r\n", ch);
      return;
    }
    players_clannum = PLAYERCLANNUM(vict);
    if (CLANRANK(vict) >= CLANRANK(ch) && GET_LEVEL(ch) < LVL_COIMPL)
    {
      send_to_char("&RYou can't demote someone who ranks higher or equal as you!&w\r\n", ch);
      return;
    }  
    if (((CLANRANK(vict) < CLAN_SOLDIER) || 
		(PLAYERCLAN(vict) != PLAYERCLAN(ch))) 
		&& GET_LEVEL(ch) < LVL_COIMPL)
    {
      send_to_char("&RYou can't demote someone who doesn't belong to your clan!&w\r\n", ch);
      return;
    }

    if (CLANRANK(vict) == CLAN_SOLDIER)
    {
      send_to_char("&RYou can't demote this person any lower! Try clan kick <person>!&w\r\n", ch);
      return;
    }

    if (strcasecmp(GET_NAME(vict), CLANOWNER(clan_index[players_clannum])) == 0)
    {
      send_to_char("&RYou can't demote the clan owner!&w\r\n", ch);
      return;
    }

    CLANRANK(vict)--;
    sprintf(buf, "&WYou have been demoted to &M%s&W in your clan.&w\r\n",
      CLANRANKTITLE(clan_index[players_clannum], CLANRANK(vict)));
    send_to_char(buf, vict);

    sprintf(buf, "&gYou have demoted &c%s&g to the rank of &M%s&g in your clan.&w\r\n", 
		GET_NAME(vict), CLANRANKTITLE(clan_index[players_clannum], CLANRANK(vict)));
    send_to_char(buf, ch);
  }
  else
  {
    send_to_char("Demote who?!\r\n", ch);
    return;
  }

  return;
}

ACMD(do_clan_who)
{
  int players_clannum;
  struct descriptor_data *pt;
  /* List all clan members online, along with their clan rank */
  if ((PLAYERCLAN(ch) == 0) || (CLANRANK(ch) == 0))
  {
    send_to_char("&RBut you don't belong to any clan!&w\r\n", ch);
    return;
  }
  players_clannum = PLAYERCLANNUM(ch);
  sprintf(buf, "&YMembers of the clan '&C%s&Y' currently online:\r\n", 
    CLANNAME(clan_index[players_clannum]));

  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && CAN_SEE_ONETIME(ch, pt->character) &&
      PLAYERCLAN(pt->character) == PLAYERCLAN(ch))
      { 
        sprintf(buf + strlen(buf), "&w[%s%3d %s %-10s&w] &C%s &W%s&w\r\n",  
            (GET_LEVEL(pt->character) >= LVL_IMMORT ? "&M" : "&g"),
            GET_LEVEL(pt->character), CLASS_ABBR(pt->character),
            CLANRANKTITLE(GET_CLAN(pt->character), CLANRANK(pt->character)),
            GET_NAME(pt->character), GET_TITLE(pt->character));	
      }
  send_to_char(buf, ch);
  return;
}

ACMD(do_clan_areas)
{
  int i;
  sprintf(buf,"[&W # Area/Zone                      Zone Guardian    Clan Hegemony     &w]\r\n");  
    
  for (i = 0; i <= top_of_zone_table; i++) {
    if (!IS_SET(zone_table[i].zon_flags, ZON_NO_OWNER)) {
      if (zone_table[i].owner && !IS_SET(zone_table[i].zon_flags, ZON_NO_OWNER)
           && real_clan(zone_table[i].owner) != -1) {
          sprintf(buf1, "%s%s", CLANNAME(clan_index[real_clan(zone_table[i].owner)]),
            CLANCOLOUR(real_clan(zone_table[i].owner)));
          sprintf(buf2, "%d", CLANNUM(clan_index[real_clan(zone_table[i].owner)]));
        } else {
          *buf1 = '\0';
          *buf2 = '\0';
        }
        
        sprintf(buf + strlen(buf),"&C%3d &c%-30s &y%-16s %-15s&w\n\r",
        zone_table[i].number,
        zone_table[i].name, 
        zone_table[i].master_mob ? 
          (real_mobile(zone_table[i].master_mob) < 0 ? "" : 
          mob_proto[real_mobile(zone_table[i].master_mob)].player.short_descr) : 
          "",
          buf1);      
      } 
    }
    page_string(ch->desc, buf, 1);
    return;
}

ACMD(do_clan_zoneown)
{
  int cn, zn, ccmd;
  if (!argument || !*argument) {
    send_to_char("Syntax: clan zoneown <clan_num> <zone_vnum>\r\n", ch);
    return;
  }
  cn = atoi(argument);
  if (cn != 0) {
    for (ccmd = 0; ccmd <= clan_top; ccmd++)
      if (CLANNUM(clan_index[ccmd]) == cn) {
	  
        half_chop(buf, argument, buf);
        zn = atoi(buf);
	  
        if (zn != 0 && real_zone(100*zn) >= 0) {
        if (zone_table[real_zone(100*zn)].zon_flags & ZON_NO_OWNER) {
          send_to_char("&RZone is ZON_NO_OWNER!&w\r\n", ch);
          return;
        }
        if (zone_table[real_zone(100*zn)].zon_flags & ZON_FIXED_OWNER) {
          send_to_char("&RZone is ZON_FIXED_OWNER!&w\r\n", ch);
          return;
        }
        zone_table[real_zone(100*zn)].owner = cn;
        sprintf(buf, "&gChown zone &c%d&g to clan &G%d&g (%s)\r\n", zn, cn, 
          CLANNAME(clan_index[ccmd]));
        send_to_char(buf, ch);
        sprintf(buf, "(GC) %s changed ownership of zone %d to clan #%d", 
          GET_NAME(ch), zn, cn);
        mudlog(buf, CMP, LVL_GRGOD, TRUE);
        save_zone_owners();
        return;
      }
      send_to_char("&RSuch zone does not exist.&w\r\n", ch);
      return;
    }
  }
  send_to_char("&RSuch clan does not exist.&w\r\n", ch);
  return;
}

ACMD(do_clan_say)
{
  /* Clan say */
  struct descriptor_data *pt;
  
  if (CLANRANK(ch) == 0)
  {
    send_to_char("&RBut you don't belong to any clan!&w\r\n", ch);
    return;
  }

  if (!PRF_FLAGGED(ch, PRF_CLANTALK))
  {
    send_to_char("&WTry turning your clan talk channel on first, dork!&w\r\n", ch);
    return;
  }    

  sprintf(buf, "&c%s&g says to the clan, &w'%s'&w\r\n", GET_NAME(ch), argument);
  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && 
        PLAYERCLAN(pt->character) == PLAYERCLAN(ch) &&
        pt->character != ch && PRF_FLAGGED(pt->character, PRF_CLANTALK)) 
	  send_to_char(buf, pt->character);
        
  sprintf(buf, "&gYou say to the clan, &w'%s'\r\n", argument);
  send_to_char(buf, ch);

  return;
}

ACMD(do_clan_channel)
{
  if (PRF_FLAGGED(ch, PRF_CLANTALK))
  {
    TOGGLE_BIT(PRF_FLAGS(ch), PRF_CLANTALK);
    send_to_char("&WClan talk channel turned &ROFF&W.&w\r\n", ch);
  }
  else
  {
    TOGGLE_BIT(PRF_FLAGS(ch), PRF_CLANTALK);
    send_to_char("&WClan talk channel turned &GON&W.&w\r\n", ch);
  }
  return;
}

ACMD(do_clan_deposit)
{
  int value;
  int players_clannum = PLAYERCLANNUM(ch);
  /* This handles the process of depositing gold into the clan account */
  /* This may only be done in the clan room */
    
  if (ch->in_room != real_room(CLANROOM(clan_index[players_clannum])))
  {
    send_to_char("&RYou can only do that in clan rooms!&w\r\n", ch);
    return;
  }

  value = atoi(argument);
  if (value != 0)
  {
    if (ch->points.gold < value)
    {
      send_to_char("&RBut you don't have that much gold!&w\r\n", ch);
      return;
    }

    ch->points.gold = ch->points.gold - value;

    CLANGOLD(clan_index[players_clannum]) = CLANGOLD(clan_index[players_clannum]) + value;      

    sprintf(buf, "&Y%d&g gold deposited into clan account.&w\r\n", value);
    send_to_char(buf, ch);

  }
  else
  {
    send_to_char("How much do you wish to deposit to clan account?\r\n", ch);
  }

  return;
}

ACMD(do_clan_withdraw)
{
  /* This handles withdrawals from the clan account */
  int players_clannum = PLAYERCLANNUM(ch);

  int value = atoi(argument);
  if (value > -1)
  {
    if (ch->in_room != real_room(CLANROOM(clan_index[players_clannum])))
    {
      send_to_char("&RBut you can only do that in your clan room!&w\r\n", ch);
      return;
    }

    if (CLANGOLD(clan_index[players_clannum]) < value)
    {
      send_to_char("&RBut your clan doesn't have that much money!&w\r\n", ch);
      return;
    }
    else
    {
      CLANGOLD(clan_index[players_clannum]) = CLANGOLD(clan_index[players_clannum]) - value;    
      ch->points.gold = ch->points.gold + value;
      sprintf(buf, "&Y%d&g gold coins withdrawn from the clan account.&w\r\n",  value);
      send_to_char(buf, ch);
    }

  }
  else
  {
    send_to_char("How much do you want to withdraw?\r\n", ch);
    return;
  }
  return;
}

ACMD(do_clan_kick)
{
  int players_clannum = PLAYERCLANNUM(ch);
  struct char_data *vict;
  if ((vict = get_player_vis(ch, argument, 0)))
  {
    if (PLAYERCLAN(vict) == 0)
    {
      send_to_char("&RBut that person doesn't belong to ANY clan!&w\r\n", ch);
      return;
    }
      
    if (PLAYERCLAN(vict) != PLAYERCLAN(ch) && GET_LEVEL(ch) < LVL_COIMPL)
    {
      send_to_char("&RBut that person doesn't belong to your clan!&w\r\n", ch);
      return;
    }

    if (strcasecmp(CLANOWNER(clan_index[players_clannum]), GET_NAME(vict)) == 0)
    {
      send_to_char("&RYou can't kick the clan owner out!&w\r\n", ch);
      return;
    }
      
    /* Update number of members in clan */
    if (CLANRANK(vict) != CLAN_APPLY)
      CLANPLAYERS(clan_index[players_clannum])--;

    CLANRANK(vict) = 0;
    PLAYERCLAN(vict) = 0;
    GET_HOMETOWN(ch) = DEF_HOMETOWN(ch);

    sprintf(buf, "&gYou have kicked &c%s&g from your clan.&w\r\n", GET_NAME(vict));
    send_to_char(buf, ch);
    sprintf(buf, "&WYou have been kicked from the clan, '&M%s&W'.&w\r\n", CLANNAME(clan_index[players_clannum]));
    send_to_char(buf, vict);
  }
  else
  {
    send_to_char("Can't find that person.\r\n", ch);
    return;
  }
  return;
}

ACMD(do_clan_help)
{
  if (!argument || !*argument) {
    send_to_char("Usage: clan help <topic>\r\n", ch);
    return;
  }
  strcpy(buf, "clan ");
  strcat(buf, argument);
  do_help(ch, buf, 0, 0);
}

ACMD(do_clan_members)
{
  ACMD(do_prgexec);
  int clan = 0, rnum;
  FILE *f;
  char *fname;

  if (!argument || !*argument)
    clan = PLAYERCLAN(ch);
  else clan = atoi(argument); 
  if ((rnum = real_clan(clan)) < 0) {
    send_to_char("Such clan does not exist,\r\n", ch);
    return;
  }
  fname = tmpnam(NULL);
  if ((f = fopen(fname, "wb")) == NULL) {
    log("SYSERR: Cannot create/overwrite %s.",MEMBERS_FILE);
    return;
  }
  fprintf(f, "%d\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n", clan,
  CLANNAME(clan_index[rnum]),
  CLANRANKTITLE(clan_index[rnum], CLAN_APPLY),
  CLANRANKTITLE(clan_index[rnum], CLAN_SOLDIER),
  CLANRANKTITLE(clan_index[rnum], CLAN_SARGEANT),
  CLANRANKTITLE(clan_index[rnum], CLAN_CAPTAIN),
  CLANRANKTITLE(clan_index[rnum], CLAN_BUILDER),
  CLANRANKTITLE(clan_index[rnum], CLAN_RULER),
  CLANRANKTITLE(clan_index[rnum], CLAN_RETIRED)
  );
  fclose(f);
  do_prgexec(ch, fname, 0, SCMD_MEMBERS);
}

ACMD(do_olc);
ACMD(do_liblist);
/*
ACMD(do_clan_olc)
{
  do_olc(ch, argument, 0, subcmd);
}
*/

#define NUM_TOP_X	5
#define SORT_ASCEND	0
#define SORT_DESCEND	1

/* Sorry, this is only bubblesort :( */
inline void clan_sort(int reff[], float val[])
{
  int i, tmp;
  bool chflag = TRUE;
  
  while (chflag) {
    chflag = FALSE;
    for (i = 0; i < clan_top; i++) {
      if (val[reff[i]] < val[reff[i+1]]) {
        tmp = reff[i];
        reff[i] = reff[i+1];
        reff[i+1] = tmp;
        chflag = TRUE;
      } 
    }
  }
}


#define CLAN_REF(i)	(clan_index[reff[i]])

inline void get_num_short(float value, char *buffer)
{
  if (value < 10)
    sprintf(buffer, "%G ", value);
  if (value < 10000)
    sprintf(buffer, "%ld ", (long)value);
  else if (value < 10000000)
    sprintf(buffer, "%ldk", (long)(value / 1000));
  else sprintf(buffer, "%ldM", (long)(value / 1000 / 1000));
}

inline void bar_graph(float value, float max, int width, char *buffer)
{
  int n, i = 0;
  if (max != 0 && value != 0) {
    n = (int) (((float) width / max)* value);
    for ( ; i <= n; i++) buffer[i] = '>';
  }
  buffer[i] = '\000';
}

inline void clan_standings(int reff[], float val[], char *title, 
  int num_top_x, int direction)					
{	
  int i = 0, j = 0;
  float max = -999999;								
  
  for (i = 0; i <= clan_top; i++)
    if (!CLAN_FLAGGED(CLAN_REF(i), CF_GODCLAN) && val[reff[i]] > max) 
      max = val[reff[i]];			
  sprintf(buf + strlen(buf), "&cStandings according to &C%s:\r\n", title);
  if (direction == SORT_DESCEND) i = 0; else i = clan_top;
  while (j < num_top_x && ((direction == SORT_DESCEND && i <= clan_top)
  ||  (direction == SORT_ASCEND && i >= 0))) {			
    if (!CLAN_FLAGGED(CLAN_REF(i), CF_GODCLAN)) {			
      get_num_short(val[reff[i]], buf1);				
      bar_graph(val[reff[i]], max, 42, buf2);			
      sprintf(buf + strlen(buf), 					
      "&Y%1d. %s%-20s|%7s | %s\r\n", j+1, clan_colour[CLANCOLORNUM(CLAN_REF(i))].colour_string,	
      CLANNAME(CLAN_REF(i)), buf1, buf2);
      
      j++;								
    }				
    if (direction == SORT_DESCEND) i++; else i--;								
  }									
  strcat(buf, "\r\n");															
}

#define CLAN_TOP(test, title, criteria, direction)	{		\
  if (show_all || test)	{						\
    for (i = 0; i <= clan_top; i++) { reff[i] = i; val[i] = criteria; }	\
    clan_sort(reff, val);					\
    clan_standings(reff, val, title, num_top_x, direction);					\
    found = TRUE;							\
  }									\
}

ACMD(do_clan_top)
{
  int *reff;
  float *val;
  int i;
  char *ptr = argument;
  bool show_members = FALSE, show_gold = FALSE, show_all = FALSE, found = FALSE;
  bool show_rips = FALSE, show_kills = FALSE;
  int num_top_x = NUM_TOP_X;
 
  while (ptr && *ptr) {
    ptr = one_argument(ptr, buf1);
    if (atoi(buf1) > 0) num_top_x = atoi(buf1);
    else if (!str_cmp(buf1, "all")) show_all = TRUE;
    else if (!str_cmp(buf1, "members")) show_members = TRUE;
    else if (!str_cmp(buf1, "gold")) show_gold = TRUE;
    else if (!str_cmp(buf1, "kills")) show_kills = TRUE;
    else if (!str_cmp(buf1, "rips")) show_rips = TRUE;
    else {
      sprintf(buf, "Unknown: %s", buf1);
      send_to_char(buf, ch);
      return;
    }
  }
  CREATE(reff, int, clan_top+1);
  CREATE(val, float, clan_top+1);
  *buf = '\000';
  CLAN_TOP(show_members, "Number Of Members", CLANPLAYERS(CLAN_REF(i)), SORT_DESCEND);
  CLAN_TOP(show_gold, "Gold", CLANGOLD(CLAN_REF(i)), SORT_DESCEND);
  CLAN_TOP(show_kills, "Kills", CLANKILLS(CLAN_REF(i)), SORT_DESCEND);
  CLAN_TOP(show_rips, "Rips", CLANRIPS(CLAN_REF(i)), SORT_ASCEND);
  if (found) 
    page_string(ch->desc, buf, 1);
  else
    send_to_char("Usage: clan top [ all | members | gold | kills | rips | <number> ]\r\n", ch);
  free(reff);
  free(val);
}

ACMD(do_zreset);

ACMD(do_clan_zreset)
{
  char local_buf[20];
  if (PLAYERCLAN(ch) == 0 || CLANZONE(GET_CLAN((ch))) == 0) return;
  sprintf(local_buf, "%d", CLANZONE(GET_CLAN((ch))));
  do_zreset(ch, local_buf, 0, 0); 
}

/* Clan Command Table */
struct clan_command_info clan_command_table[] = {
  { "apply",	do_clan_apply,	CLAN_NORANK,	0,	0 },
  { "areas",	do_clan_areas,	CLAN_NOBODY,	LVL_COIMPL,	0 },
  { "channel",	do_clan_channel,CLAN_SOLDIER,	LVL_IMPL+1,	0 },
  { "demote",	do_clan_demote,	CLAN_CAPTAIN,	LVL_COIMPL,	0 },
  { "deposit",	do_clan_deposit,CLAN_SOLDIER,	LVL_IMPL+1,	0 },
  { "edit",	do_clan_edit,	CLAN_RULER,	LVL_GRGOD, 	0 },
  { "enlist",   do_clan_enlist, CLAN_SARGEANT,	LVL_GRGOD,	0 },
  { "help",	do_clan_help,	CLAN_NORANK,	0,	0 },
  { "info",	do_clan_info,	CLAN_NORANK,	0,	0 },
  { "kick",	do_clan_kick,	CLAN_CAPTAIN,	LVL_GRGOD,	0 },
  { "list",	do_clan_list,	CLAN_NORANK,	0, 	0 },
  { "medit",	do_olc,		CLAN_BUILDER,	LVL_COIMPL,	SCMD_OLC_MEDIT+SCMD_OLC_CLAN_EDIT },
  { "members",  do_clan_members,CLAN_SOLDIER,	LVL_IMMORT,	0 },
  { "mlist",	do_liblist,	CLAN_BUILDER,	LVL_IMPL+1,	SCMD_MLIST },
  { "motd",	do_gen_ps,	CLAN_SOLDIER,	LVL_IMPL+1,	SCMD_CLANMOTD },
  { "oedit",	do_olc,		CLAN_BUILDER,	LVL_COIMPL,	SCMD_OLC_OEDIT+SCMD_OLC_CLAN_EDIT },
  { "olc",	do_olc,		CLAN_BUILDER,	LVL_IMPL+1,	SCMD_OLC_SAVEINFO+SCMD_OLC_CLAN_EDIT },
  { "olist",	do_liblist,	CLAN_BUILDER,	LVL_IMPL+1,	SCMD_OLIST },
  { "promote",  do_clan_promote,CLAN_CAPTAIN,	LVL_COIMPL,	0 },
  { "redit",	do_olc,		CLAN_BUILDER,	LVL_COIMPL,	SCMD_OLC_REDIT+SCMD_OLC_CLAN_EDIT },
  { "resign",	do_clan_resign,	CLAN_APPLY,	LVL_IMPL+1,	0 },
  { "rlist",	do_liblist,	CLAN_BUILDER,	LVL_IMPL+1,	SCMD_RLIST },
  { "save",	do_clan_save,	CLAN_NOBODY,	LVL_GRGOD,	0 },
  { "say",	do_clan_say,	CLAN_SOLDIER,	LVL_IMPL+1,	0 },
  { "sedit",	do_olc,		CLAN_BUILDER,	LVL_COIMPL,	SCMD_OLC_SEDIT+SCMD_OLC_CLAN_EDIT },
  { "show",	do_gen_tog,	CLAN_APPLY,	0,	SCMD_SHOWCLAN },
  { "slist",	do_liblist,	CLAN_BUILDER,	LVL_IMPL+1,	SCMD_SLIST },
  { "top",	do_clan_top,	CLAN_APPLY,	0,	0 },
  { "withdraw",	do_clan_withdraw,	CLAN_BUILDER,	LVL_IMPL+1,	0 },
  { "who",	do_clan_who,	CLAN_APPLY,	LVL_IMPL+1,	0 },
  { "zedit",	do_olc,		CLAN_BUILDER,	LVL_COIMPL,	SCMD_OLC_ZEDIT+SCMD_OLC_CLAN_EDIT },
  { "zoneown",	do_clan_zoneown,CLAN_NOBODY,	LVL_COIMPL,	0 },
  { "zreset",	do_clan_zreset,	CLAN_BUILDER,	LVL_COIMPL,	0 },
  { NULL, 0, 0, 0, 0 } 
};




/*
 * The general player commands go here
*/
ACMD(do_clan)
{
  int cmd_index = 0, counter = 0;

  if (IS_NPC(ch))
    return;

  half_chop(argument, buf1, buf2);
  
  if (buf1 && *buf1)
    while(clan_command_table[cmd_index].command != NULL) {
      if (GET_LEVEL(ch) >= clan_command_table[cmd_index].min_level ||
        (PLAYERCLAN(ch) != 0 && CLANRANK(ch) >= 
          clan_command_table[cmd_index].min_clan_level))
      if (is_abbrev(buf1, clan_command_table[cmd_index].command)) {
        prepare_colour_table(ch);
        ((*clan_command_table[cmd_index].command_pointer)(ch, buf2, cmd, 
        clan_command_table[cmd_index].subcmd));
        return;
      }
      cmd_index++;
    }
  
  if (PLAYERCLAN(ch) != 0)
      sprintf(buf,"You are currently member of &W%s&w and your rank is &G%s&w\r\n",
        CLANNAME(clan_index[PLAYERCLANNUM(ch)]), 
        get_clan_rank_str(PLAYERCLANNUM(ch), CLANRANK(ch)));
    else
      sprintf(buf,"You are not member of any clan!\r\n");
  
  sprintf(buf + strlen(buf), "\r\n&cAvailable &CCLAN&c commands:&w\r\n");
  
  cmd_index = 0;
  while(clan_command_table[cmd_index].command != NULL) {
      if (GET_LEVEL(ch) >= clan_command_table[cmd_index].min_level ||
        (PLAYERCLAN(ch) != 0 && CLANRANK(ch) >= 
          clan_command_table[cmd_index].min_clan_level)) {
        sprintf(buf+strlen(buf), "%-9s", clan_command_table[cmd_index].command);
        if (counter++ % 8 == 7) strcat(buf, "\r\n");
      }
      cmd_index++;
    }
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
  
  return;
};

/*
 * The following procedures attempt to fit into the db.c load procedures
 * to allow the dynamic creation, editing, and removal of clans whilst
 * in game play.
 *
 * Most of these functions will be almost direct copies of the db.c
 * procedures.
 *
*/

void parse_clan(FILE *fl, int virtual_nr)
{
  static int clan_nr = 0;
  int t[10];
  char line[256];
  int i;

  sprintf(buf2, "Parsing clan #%d", virtual_nr);
  log(buf2);
  
  clan_index[clan_nr].number = virtual_nr;
  clan_index[clan_nr].title = fread_string(fl, buf2);
  if (!get_line(fl,line) || sscanf(line, " %d", t) != 1) {
    fprintf(stderr, "Format error in clan #%d (first 1-const line)\n", virtual_nr);
    exit(1);
  }
  clan_index[clan_nr].zone_vnum = t[0];
  clan_index[clan_nr].description = fread_string(fl, buf2);
  clan_index[clan_nr].motd = fread_string(fl, buf2);
  clan_index[clan_nr].owner = fread_string(fl, buf2);
  for (i = CLAN_SOLDIER; i <= CLAN_RETIRED; i++)
    CLANRANKTITLE(clan_index[clan_nr], i) = fread_string(fl, buf2);
  CLANRANKTITLE(clan_index[clan_nr], 0) = str_dup(notitle);
  CLANRANKTITLE(clan_index[clan_nr], 1) = str_dup(applytitle);

  if (!get_line(fl,line) || sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
    fprintf(stderr, "Format error in clan #%d (first 4-const line)\n", virtual_nr);
    exit(1);
  }

  clan_index[clan_nr].clanroom = t[0];
  clan_index[clan_nr].clangold = t[1];
//  clan_index[clan_nr].nummembers = t[2];
  clan_index[clan_nr].colour = t[3];
//  clan_index[clan_nr].status = CLAN_OLD;
  
  if (!get_line(fl,line) || sscanf(line, " %d %d %d %d %d %d", t, t + 1, t + 2, t + 3, t+4, t+5) != 6) {
    fprintf(stderr, "Format error in (6-const line) clan #%d\n", virtual_nr);
    exit(1);
  }
  
  clan_index[clan_nr].clantown = t[0];
  clan_index[clan_nr].flags    = t[1];
  clan_index[clan_nr].kills    = t[2];
  clan_index[clan_nr].rips     = t[3];
  clan_index[clan_nr].limit_down     = t[4];
  clan_index[clan_nr].limit_up     = t[5];
  
  clan_index[clan_nr].nummembers = 0;
  
  clan_nr++;
}

void prepare_colour_table(struct char_data *ch)
{
  clan_colour[0].colour_string = "&W";
  clan_colour[1].colour_string = "&G";
  clan_colour[2].colour_string = "&R";
  clan_colour[3].colour_string = "&B";
  clan_colour[4].colour_string = "&M";
  clan_colour[5].colour_string = "&Y";
  clan_colour[6].colour_string = "&C";
  clan_colour[7].colour_string = "&w";
}

/*
void update_clan_cross_reference()
{
  int i = 0;

  for (i = 0; i < 9999; i++)
    cross_reference[i] = -1;

  for (i = 0; i <= clan_top; i++)
    cross_reference[CLANNUM(clan_index[i])] = i;

}
*/

void clan_save(int system, struct char_data *ch)
{
  int i = 0, found = 0, last = 0, rec_count = 0, files = 0;
  FILE *fl, *index;
  char *index_file;
  char line[256];

  if (mini_mud)
    index_file = MINDEX_FILE;
  else
    index_file = INDEX_FILE;

  sprintf(buf, "%s/%s", CLAN_PREFIX, index_file);

  if (!(index = fopen(buf, "r")))
  {
    log("FUNCTION(clan_save): Unable to open clan index file");
    if (system == FALSE)
      send_to_char("ClanSave: Unable to open index file (ERROR)\r\n", ch);
    return;
  }

  fscanf(index, "%s\n", buf);
  while (*buf != '$')
  {
    sprintf(buf2, "%s/%s", CLAN_PREFIX, buf);
    if (!(fl = fopen(buf2, "r")))
    {
      log("FUNCTION(clan_save): Unable to open sub file from index file");
      if (system == FALSE)
      {
	sprintf(buf, "ClanSave: Unable to open '%s' (ERROR)\r\n", buf2);
        send_to_char(buf, ch);
      }
      return;
    }
    else
    {
      rec_count = 0;

      while (found == 0)
      {
        if (!get_line(fl, line))
        {
          log("FUNCTION(clan_save): Format error encountered");
          if (system == FALSE);
          {
  	    sprintf(buf, "ClanSave: Format error after clan #%d (ERROR)\r\n", i);
            send_to_char(buf, ch);
          }
          exit(1);
        }

        if (*line == '$')
	  found = 1;

	if (*line == '#')
	{
	  last = i;

	  if (sscanf(line, "#%d", &i) != 1)
	  {
	    log("FUNCTION(clan_save): Format error in clan file");
	    if (system == FALSE)
	    {
	      sprintf(buf, "ClanSave: Format error after clan #%d\r\n", last);
	      send_to_char(buf, ch);
	    }
	    exit(1);
	  }
	  
	  if (i >= 99999)
	    return;

	}	  
      }

      /* Now that we have a list of the clan #'s in this clan file */
      /* We replace them with the updated records */
      fclose(fl);
      
      sprintf(buf2, "%s/%s", CLAN_PREFIX, buf);
      if (!(fl = fopen(buf2, "w")))
      {
        log("FUNCTION(clan_save): Error truncuating clan file for rewrite");
	if (system == FALSE)
	{
	  sprintf(buf, "ClanSave: Unable to truncuate '%s' (ERROR)\r\n", buf2);
	  send_to_char(buf, ch);
	}
        return;
      }
      log("Writing clans.");
      for (i = 0; i <= clan_top; i++)
	write_clan(fl, i);

      fputs("$~\n", fl);
      fclose(fl);
    }  

  fscanf(index, "%s\n", buf);
  files++;
  }

  log("Successfully saved clan details");

  if (system == FALSE)
    send_to_char("Clans saved.\r\n", ch);

}

/* cnum is clan virtual number */
void write_clan(FILE *fl, int cnum)
{
  int i, j = cnum;

//  log("Whew! Made it to the 'write_clan' function");

//  update_clan_cross_reference();

  if (j == -1) {
    log("SYSERR: write_clan: Attempt to save nonexisting clan");
    return;
  }

  
//  fprintf(stderr, "Writing clan #%d which is '%s'\n", cnum, clan_index[j].title);


  fprintf(fl, "#%d\n%s~\n%d\n", clan_index[j].number, clan_index[j].title,
    clan_index[j].zone_vnum);
  strcpy(buf, clan_index[j].description);
  strip_string(buf);
  fprintf(fl, "%s~\n", buf);
  strcpy(buf, clan_index[j].motd);
  strip_string(buf);
  fprintf(fl, "%s~\n", buf);
  fprintf(fl, "%s~\n", clan_index[j].owner);
  for (i = CLAN_SOLDIER; i <= CLAN_RETIRED; i++)
    fprintf(fl, "%s~\n", CLANRANKTITLE(clan_index[j], i));
  fprintf(fl, "%d %ld %d %d\n", clan_index[j].clanroom, 
          clan_index[j].clangold, clan_index[j].nummembers, 
	  clan_index[j].colour);
  fprintf(fl, "%d %ld %d %d %d %d\n", clan_index[j].clantown, 
          clan_index[j].flags, clan_index[j].kills, clan_index[j].rips,
          clan_index[j].limit_down, clan_index[j].limit_up);

}

void cedit_display_menu(struct descriptor_data *d)
{
  sprintbit(CLANFLAGS(*OLC_CLAN(d)), clan_flags, buf1);
  sprintf(buf, 
    "[H[J"
    "&W[Editing clan record - CLANS ][ beta written by Yilard]\r\n"
    "&w-- Clan Vnum: %d\r\n"
    "&g1&w) Clan zone: &c#%d\r\n"
    "&g2&w) Clanname: %s%s\r\n"
    "&g3&w) Clan Owner : &y%s\r\n"
    "&g4&w) Change clan description\r\n"
    "&w%s\r\n"
    "&g5&w) Change clan colour\r\n"
    "&g6&w) Clan Gold: &y%-8ld	&g7&w) Clan Room : &y%d\r\n"
    "&g8&w) Change clan ranks	&g9&w) Clan hometown: &y%s\r\n"
    "&gA&w) Clan kills: &y%-8d	&gB&w) Clan rips: &y%d\r\n"
    "&gC&w) Clan levels: &y%d-%d\r\n"
    "&gE&w) Clan flags: &c%s\r\n"
    "&gF&w) Clan motd:\r\n&w%s\r\n"
    "&gQ&w) Quit editing clan\r\n"
    "&gD&w) Delete clan\r\n"
    "&wCommand: ",
    CLANNUM(*OLC_CLAN(d)),
    CLANZONE(*OLC_CLAN(d)),
    clan_colour[CLANCOLORNUM(*OLC_CLAN(d))].colour_string,
    CLANNAME(*OLC_CLAN(d)),
    CLANOWNER(*OLC_CLAN(d)),
    CLANDESC(*OLC_CLAN(d)),
    CLANGOLD(*OLC_CLAN(d)),
    CLANROOM(*OLC_CLAN(d)),
    hometowns[CLANTOWN(*OLC_CLAN(d))].homename,
    CLANKILLS(*OLC_CLAN(d)),
    CLANRIPS(*OLC_CLAN(d)),
    CLAN_LIMIT_DOWN(*OLC_CLAN(d)), CLAN_LIMIT_UP(*OLC_CLAN(d)),
    buf1,
    CLANMOTD(*OLC_CLAN(d))
    
    );

  send_to_char(buf, d->character);
  OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
}

/* Some generic tests */
int is_clannum_already_used(int cnum)
{
  if (real_clan(cnum) >= 0)
    return TRUE;
  else
    return FALSE;
}

int already_being_edited(struct char_data *ch, int cnum)
{
  struct descriptor_data *d;
  
  for (d = descriptor_list; d; d = d->next)
    if (d->connected == CON_CLAN_EDIT)
      if (d->olc && OLC_NUM(d) == cnum)
      { sprintf(buf, "That clan number  is currently being edited by %s.\r\n",
                GET_NAME(d->character));
        send_to_char(buf, ch);
        return TRUE;
      }
  return FALSE;
}

/* cnum is the new clan virtual number */
void cedit_setup_new(struct descriptor_data *d, int cnum)
{
  struct clan_info *clan;
  
  CREATE(clan, struct clan_info, 1);
  OLC_CLAN(d) = clan;
  
  CLANNUM(*clan) = cnum;
  CLANNAME(*clan) = str_dup("New Clan");
  CLANOWNER(*clan) = str_dup(GET_NAME(d->character));
  CLANDESC(*clan) = str_dup("A new clan");
  CLANGOLD(*clan) = 0;
  CLANFLAGS(*clan) = 0;
  CLANROOM(*clan) = 0;
  CLANKILLS(*clan) = 0;
  CLANRIPS(*clan) = 0;
  CLAN_LIMIT_DOWN(*clan) = 1;
  CLAN_LIMIT_UP(*clan) = 40;
  CLAN_HOMETOWN(*clan) = 0;
  CLANMOTD(*clan) = strdup("CLAN MOTD\r\n");
  CLANCOLORNUM(*clan) = CLAN_NORMAL;
  
  CLANRANKTITLE(*clan, CLAN_NORANK) = str_dup(notitle);
  CLANRANKTITLE(*clan, CLAN_APPLY) = str_dup(applytitle);
  CLANRANKTITLE(*clan, CLAN_SOLDIER) = str_dup("Soldier");
  CLANRANKTITLE(*clan, CLAN_SARGEANT) = str_dup("Sargeant");
  CLANRANKTITLE(*clan, CLAN_CAPTAIN) = str_dup("Captain");
  CLANRANKTITLE(*clan, CLAN_BUILDER) = str_dup("Builder");
  CLANRANKTITLE(*clan, CLAN_RULER) = str_dup("Ruler");
  CLANRANKTITLE(*clan, CLAN_RETIRED) = str_dup("Retired");
//  CLANSTATUS(*clan) = CLAN_NEW;

  // clan_top++;

  // update_clan_cross_reference();

  OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
  cedit_display_menu(d);

}


/* cnum if clan real number */
void cedit_setup_existing(struct descriptor_data *d, int cnum)
{
  struct clan_info *clan;
  int i;
  
  CREATE(clan, struct clan_info, 1);
  memcpy(clan, clan_index + cnum, sizeof(struct clan_info));
  OLC_CLAN(d) = clan;
  CLANNAME(*OLC_CLAN(d)) = str_dup(CLANNAME(clan_index[cnum]));
  CLANOWNER(*OLC_CLAN(d)) = str_dup(CLANOWNER(clan_index[cnum]));
  CLANDESC(*OLC_CLAN(d)) = str_dup(CLANDESC(clan_index[cnum]));
  CLANMOTD(*OLC_CLAN(d)) = str_dup(CLANMOTD(clan_index[cnum]));
  for (i = 0; i < CLAN_NUM_RANKS; i++)
    if (CLANRANKTITLE(clan_index[cnum], i))
      CLANRANKTITLE(*OLC_CLAN(d), i) = str_dup(CLANRANKTITLE(clan_index[cnum], i));
    else CLANRANKTITLE(*OLC_CLAN(d), i) = NULL;
  
  OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
  
  cedit_display_menu(d);
}



void cleanup_clan_edit(struct descriptor_data *d)
{
  
  if (d->character)
  {
    STATE(d) = CON_PLAYING;
    REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING);
      
    if (d->olc) {
      free_clan(OLC_CLAN(d));
      free(d->olc);
    }
    act("$n stops editing clans.", TRUE, d->character, 0, 0, TO_ROOM);
  }
  fix_illegal_cnum();
}

void fix_illegal_cnum()
{
  /* Search out and destroy cnum's in player's records which no longer */
  /* exist in the clan_index array */
  int i = 0, valid = FALSE;
  struct descriptor_data *pt;

  for (pt = descriptor_list; pt; pt = pt->next)
  {
    for (i = 0; i <= clan_top; i++)
      if (PLAYERCLAN(pt->character) == 0 || 
        real_clan(PLAYERCLAN(pt->character)) >= 0) valid = TRUE;
    if (valid == FALSE)
    {
      log("(ILLEGAL CLAN NUMBER FOUND) belonging to '%s' - #%d\n",
	GET_NAME(pt->character), PLAYERCLAN(pt->character));
      PLAYERCLAN(pt->character) = 0;
      CLANRANK(pt->character) = 0;
    }
    valid = FALSE;
  }
  log("Done fixing illegal clannum links");
}

void free_clan(struct clan_info *clan)
{
  int i;
  if (CLANNAME(*clan))
    free(CLANNAME(*clan));
  if (CLANOWNER(*clan))
    free(CLANOWNER(*clan));
  if (CLANDESC(*clan))
    free(CLANDESC(*clan));
  if (CLANMOTD(*clan))
    free(CLANMOTD(*clan));
  for (i = 0; i < CLAN_NUM_RANKS; i++)
    if (CLANRANKTITLE(*clan, i) != NULL)
      free(CLANRANKTITLE(*clan, i));
  free(clan);
  
}

void cedit_save_internally(struct descriptor_data *d)
{
  
  int i, j, num;
  struct clan_info *new_index;
  
  if ((num = (real_clan(OLC_NUM(d)))) != -1) {
    free(CLANNAME(clan_index[num]));
    free(CLANOWNER(clan_index[num]));
    free(CLANDESC(clan_index[num]));
    free(CLANMOTD(clan_index[num]));
    CLANNAME(clan_index[num]) = str_dup(CLANNAME(*OLC_CLAN(d)));
    CLANOWNER(clan_index[num]) = str_dup(CLANOWNER(*OLC_CLAN(d)));
    CLANDESC(clan_index[num]) = str_dup(CLANDESC(*OLC_CLAN(d)));
    CLANMOTD(clan_index[num]) = str_dup(CLANMOTD(*OLC_CLAN(d)));
    CLAN_LIMIT_DOWN(clan_index[num]) = CLAN_LIMIT_DOWN(*OLC_CLAN(d));
    CLAN_LIMIT_UP(clan_index[num]) = CLAN_LIMIT_UP(*OLC_CLAN(d));
    CLAN_HOMETOWN(clan_index[num]) = CLAN_HOMETOWN(*OLC_CLAN(d));
    CLANCOLORNUM(clan_index[num]) = CLANCOLORNUM(*OLC_CLAN(d));
    CLANFLAGS(clan_index[num]) = CLANFLAGS(*OLC_CLAN(d));
    CLANZONE(clan_index[num]) = CLANZONE(*OLC_CLAN(d));
    for (i = 0; i < CLAN_NUM_RANKS; i++) {
      free(CLANRANKTITLE(clan_index[num], i));
      CLANRANKTITLE(clan_index[num], i) = str_dup(CLANRANKTITLE(*OLC_CLAN(d), i));
    }
    if (OLC_VAL(d) | 2) {
      CLANGOLD(clan_index[num]) = CLANGOLD(*OLC_CLAN(d));
      CLANKILLS(clan_index[num]) = CLANKILLS(*OLC_CLAN(d));
      CLANRIPS(clan_index[num]) = CLANRIPS(*OLC_CLAN(d));
    }
    // free(OLC_CLAN(d));
  } else {
    CREATE(new_index, struct clan_info, clan_top + 2);
    for (i = 0; i <= clan_top; i++)
      if (CLANNUM(clan_index[i]) <= CLANNUM(*OLC_CLAN(d)))
        memcpy(new_index + i, clan_index + i, sizeof(struct clan_info));
      else break;
      
    memcpy(new_index + i, OLC_CLAN(d), sizeof(struct clan_info));
    CLANNAME(new_index[i]) = str_dup(CLANNAME(*OLC_CLAN(d)));
    CLANOWNER(new_index[i]) = str_dup(CLANOWNER(*OLC_CLAN(d)));
    CLANDESC(new_index[i]) = str_dup(CLANDESC(*OLC_CLAN(d)));
    CLANMOTD(new_index[i]) = str_dup(CLANMOTD(*OLC_CLAN(d)));
    for (j = 0; j < CLAN_NUM_RANKS; j++) {
      // free(CLANRANKTITLE(clan_index[num], j));
      CLANRANKTITLE(new_index[i], j) = str_dup(CLANRANKTITLE(*OLC_CLAN(d), j));
    }
    clan_top++;
    for (i = i+1; i <= clan_top; i++)
    memcpy(new_index + i, clan_index + i - 1, sizeof(struct clan_info));
    free(clan_index);
    
    clan_index = new_index;
    // free(OLC_CLAN(d)); 
  }
}

void cedit_disp_flags_menu(struct descriptor_data *d)
{
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  menubit(&CLANFLAGS(*OLC_CLAN(d)), clan_flags, buf1, NULL);
  send_to_char(buf1, d->character);
  sprintbit(CLANFLAGS(*OLC_CLAN(d)), clan_flags, buf1);
  sprintf(buf, "\r\n"
	"Current flags : %s%s%s\r\n"
	"Enter clan flags (0 to quit) : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}

void parse_clan_edit(struct descriptor_data *d, char *arg)
{
  int i = 0;
  int num = -1;

  switch (OLC_MODE(d))
  {
    case CLAN_EDIT_MAIN_MENU:
      switch (*arg)
      {
        case 'd':
	case 'D':
	  OLC_MODE(d) = CLAN_EDIT_CONFIRM_DELETE;
	  send_to_char("\r\nAre you sure you wish to delete this clan? [Yy/Nn]", d->character);
	  break;
	case 'q':
	case 'Q':
	  if (OLC_VAL(d) == 0) {
	    send_to_char("\r\nNo changes.\r\n", d->character);
	    cleanup_clan_edit(d);
	    return;
	  } else {
	     OLC_MODE(d) = CLAN_EDIT_CONFIRM_SAVE;
	     send_to_char("\r\nDo you wish to save this clan? [Yy/Nn]", d->character);
	  }
	  break;
	case '1':
	  if (GET_LEVEL(d->character) < CLAN_EDIT_PRIVILEGED) {
            send_to_char(no_permission, d->character);
            OLC_MODE(d) = CLAN_EDIT_WAITKEY;
            break;
          }
	  OLC_MODE(d) = CLAN_EDIT_NUMBER;
	  send_to_char("\r\nEnter new clan zone number: ", d->character);
	  break;
	case '2':
	  OLC_MODE(d) = CLAN_EDIT_NAME;
	  send_to_char("\r\nEnter clan name: \r\n", d->character);
	  break;
	case '3':
	  if (GET_LEVEL(d->character) < CLAN_EDIT_PRIVILEGED &&
	    str_cmp(CLANOWNER(*OLC_CLAN(d)), GET_NAME(d->character))) {
            send_to_char(no_permission, d->character);
            OLC_MODE(d) = CLAN_EDIT_WAITKEY;
            break;
          }
	  OLC_MODE(d) = CLAN_EDIT_OWNER;
	  send_to_char("\r\nEnter clan owners name: \r\n", d->character);
	  break;
	case '4':
	  OLC_MODE(d) = CLAN_EDIT_DESCRIPTION;
	  SEND_TO_Q("\x1B[H\x1B[J", d);
	  /*
	   * Attempting the impossible - Using the inbuilt text editor */
	  d->backstr = str_dup(CLANDESC(*OLC_CLAN(d)));
          SEND_TO_Q("Old clan description:\r\n\r\n", d);
	  SEND_TO_Q(CLANDESC(*OLC_CLAN(d)), d);
	  SEND_TO_Q("\r\n\r\nEnter new description: \r\n\r\n", d);
	  d->str = &(CLANDESC(*OLC_CLAN(d)));
	  d->max_str = 1000;
	  d->mail_to = 0;
	  break;
	case '5':
	  OLC_MODE(d) = CLAN_EDIT_COLOUR;
	  display_colour_menu(d);
	  break;
	case '6':
	  if (GET_LEVEL(d->character) < CLAN_EDIT_PRIVILEGED) {
            send_to_char(no_permission, d->character);
            OLC_MODE(d) = CLAN_EDIT_WAITKEY;
            break;
          }
	  OLC_MODE(d) = CLAN_EDIT_GOLD;
	  send_to_char("\r\nEnter new gold value: ", d->character);
	  break;
	case '7':
	  OLC_MODE(d) = CLAN_EDIT_CLANROOM;
	  send_to_char("\r\nEnter new clan room: ", d->character);
	  break;
	case '8':
	  OLC_MODE(d) = CLAN_EDIT_RANK_MENU;
	  display_rank_menu(d, num);
	  break;
	case '9':
	  if (GET_LEVEL(d->character) < CLAN_EDIT_PRIVILEGED) {
            send_to_char(no_permission, d->character);
            OLC_MODE(d) = CLAN_EDIT_WAITKEY;
            break;
          }
	  OLC_MODE(d) = CLAN_EDIT_HOMETOWN;
	  sprintf(buf, "\r\nEnter new clan hometown index (0-%d): ", NUM_HOMETOWNS-1);
	  send_to_char(buf, d->character);
	  break;
	case 'a':
	case 'A':
	  if (GET_LEVEL(d->character) < CLAN_EDIT_PRIVILEGED) {
            send_to_char(no_permission, d->character);
            OLC_MODE(d) = CLAN_EDIT_WAITKEY;
            break;
          }
	  OLC_MODE(d) = CLAN_EDIT_KILLS;
	  send_to_char("Enter number of kills: ", d->character);
	  break;
	case 'b':
	case 'B':
	  if (GET_LEVEL(d->character) < CLAN_EDIT_PRIVILEGED) {
            send_to_char(no_permission, d->character);
            OLC_MODE(d) = CLAN_EDIT_WAITKEY;
            break;
          }
	  OLC_MODE(d) = CLAN_EDIT_RIPS;
	  send_to_char("Enter number of rips: ", d->character);
	  break;
	case 'c':
	case 'C':
	  OLC_MODE(d) = CLAN_EDIT_LEVEL_DOWN;
	  send_to_char("Enter bottom clan level limit: ", d->character);
	  break;
	case 'e':
	case 'E':
	  if (GET_LEVEL(d->character) < CLAN_EDIT_PRIVILEGED) {
            send_to_char(no_permission, d->character);
            OLC_MODE(d) = CLAN_EDIT_WAITKEY;
            break;
          }
	  OLC_MODE(d) = CLAN_EDIT_FLAGS_MENU;
	  cedit_disp_flags_menu(d);
	  break;
	case 'f':
	case 'F':
	  OLC_MODE(d) = CLAN_EDIT_MOTD;
	  SEND_TO_Q("\x1B[H\x1B[J", d);
	  /*
	   * Attempting the impossible - Using the inbuilt text editor */
	  d->backstr = str_dup(CLANMOTD(*OLC_CLAN(d)));
          SEND_TO_Q("Old clan motd:\r\n\r\n", d);
	  SEND_TO_Q(CLANMOTD(*OLC_CLAN(d)), d);
	  SEND_TO_Q("\r\n\r\nEnter new motd: \r\n\r\n", d);
	  d->str = &(CLANMOTD(*OLC_CLAN(d)));
	  d->max_str = 2000;
	  d->mail_to = 0;
	  break;
	default:
	  send_to_char("r\nInvalid command!", d->character);
	  cedit_display_menu(d);
	  break;
      }
      return;

    case CLAN_EDIT_CONFIRM_DELETE:
      switch (*arg)
      {
	case 'Y':
	case 'y':
	  send_to_char("\r\nClan deleted.\r\n", d->character);
	  OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  cedit_display_menu(d);
	  break;
	case 'N':
	case 'n':
	  send_to_char("\r\nNOT deleted.\r\n", d->character);
	  OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  cedit_display_menu(d);
	  break;
      }
      break;

    case CLAN_EDIT_CONFIRM_SAVE:
      switch (*arg)
      {
	case 'Y':
	case 'y':
	  cedit_save_internally(d);
	  send_to_char("\r\nSaved.\r\n", d->character);
	  break;
	case 'N':
	case 'n':
	  send_to_char("\r\nNOT saved.\r\n", d->character);
	  break;
	default:
	  send_to_char("\r\nInvalid command!", d->character);
	  cedit_display_menu(d);
	  break;
      }
      cleanup_clan_edit(d);
      return;

    case CLAN_EDIT_NUMBER:
      i = 0;
      i = atoi(arg);
      if ((i > 0))
      {
        CLANZONE(*OLC_CLAN(d)) = i;
        OLC_VAL(d) |= 1;
	cedit_display_menu(d);
      }
      else
      {
        send_to_char("\r\nInvalid number!\r\n", d->character);
	OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	cedit_display_menu(d);
      }
      break;

    case CLAN_EDIT_NAME:
      if (arg && *arg) {
        free(CLANNAME(*OLC_CLAN(d)));
        CLANNAME(*OLC_CLAN(d)) = str_dup(arg);
        OLC_VAL(d) |= 1;
      }
      OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
      cedit_display_menu(d);
      break;

    case CLAN_EDIT_OWNER:
      /* No error checking here */
      if (arg && *arg) {
        free(CLANOWNER(*OLC_CLAN(d)));
        CLANOWNER(*OLC_CLAN(d)) = str_dup(arg);
        OLC_VAL(d) |= 1;
      }
      OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
      cedit_display_menu(d);
      break;

    case CLAN_EDIT_GOLD:
      i = 0;
      i = atoi(arg);
      if (!(i <= 0))
      {
	CLANGOLD(*OLC_CLAN(d)) = i;
	OLC_VAL(d) |= 2;
	OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
        cedit_display_menu(d);
      }
      else
      {
	send_to_char("\r\nInvalid number!\r\n", d->character);
	OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	cedit_display_menu(d);
      }
      break;
      
      
    case CLAN_EDIT_HOMETOWN:
      i = atoi(arg);
      if (i >= 0 && i < NUM_HOMETOWNS)
      {
	CLANTOWN(*OLC_CLAN(d)) = i;
	OLC_VAL(d) |= 1;
	OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
        cedit_display_menu(d);
      }
      else
      {
	send_to_char("\r\nInvalid number!\r\n", d->character);
	OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	cedit_display_menu(d);
      }
      break;

    case CLAN_EDIT_CLANROOM:
	/* No error checking */
      i = 0;
      i = atoi(arg);
      if (!(i <= 0))
      {
	CLANROOM(*OLC_CLAN(d)) = i;
	OLC_VAL(d) |= 1;
	OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
        cedit_display_menu(d);
      }
      else
      {
	send_to_char("\r\nInvalid number!\r\n", d->character);
	OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	cedit_display_menu(d);
      }
      break;

    case CLAN_EDIT_LEVEL_DOWN:
      CLAN_LIMIT_DOWN(*OLC_CLAN(d)) = atoi(arg);
      OLC_VAL(d) |= 1;
      OLC_MODE(d) = CLAN_EDIT_LEVEL_UP;
      send_to_char("Enter top clan level limit: ", d->character);
      break;
    
    case CLAN_EDIT_LEVEL_UP:
      
      CLAN_LIMIT_UP(*OLC_CLAN(d)) = atoi(arg);
      if (CLAN_LIMIT_DOWN(*OLC_CLAN(d)) > CLAN_LIMIT_UP(*OLC_CLAN(d))) {
        CLAN_LIMIT_UP(*OLC_CLAN(d)) = CLAN_LIMIT_DOWN(*OLC_CLAN(d));
        CLAN_LIMIT_DOWN(*OLC_CLAN(d)) = atoi(arg);
      }
      OLC_VAL(d) |= 1;
      cedit_display_menu(d);
      break;
      
    case CLAN_EDIT_KILLS:
      if (atoi(arg) > 0) {
        CLANKILLS(*OLC_CLAN(d)) = atoi(arg);
        OLC_VAL(d) |= 2;
      }
      cedit_display_menu(d);
      break;
      
    case CLAN_EDIT_RIPS:
      if (atoi(arg) > 0) {
        CLANRIPS(*OLC_CLAN(d)) = atoi(arg);
        OLC_VAL(d) |= 2;
      }
      cedit_display_menu(d);
      break;

    case CLAN_EDIT_COLOUR:
      switch (*arg)
      {
	case 'w':
	case 'W':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_WHITE;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'n':
	case 'N':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_NORMAL;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'r':
	case 'R':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_RED;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'g':
	case 'G':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_GREEN;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'b':
	case 'B':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_BLUE;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'y':
	case 'Y':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_YELLOW;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'm':
	case 'M':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_MAGENTA;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	case 'c':
	case 'C':
	  CLANCOLORNUM(*OLC_CLAN(d)) = CLAN_CYAN;
	  OLC_VAL(d) |= 1;
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
	default:
	  send_to_char("Invalid colour!\r\n", d->character);
	  cedit_display_menu(d);
          OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  break;
      }
      cedit_display_menu(d);
      OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
      break;

    case CLAN_EDIT_WAITKEY:
      cedit_display_menu(d);
      break;

    case CLAN_EDIT_SOLDIER:
      SOLDIERTITLE(*OLC_CLAN(d)) = str_dup(arg);
      OLC_VAL(d) |= 1;
      OLC_MODE(d) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;

    case CLAN_EDIT_SARGEANT:
      SARGEANTTITLE(*OLC_CLAN(d)) = str_dup(arg);
      OLC_VAL(d) |= 1;
      OLC_MODE(d) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;
  
    case CLAN_EDIT_CAPTAIN:
      CAPTAINTITLE(*OLC_CLAN(d)) = str_dup(arg);
      OLC_VAL(d) |= 1;
      OLC_MODE(d) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;

    case CLAN_EDIT_RULER:
      RULERTITLE(*OLC_CLAN(d)) = str_dup(arg);
      OLC_VAL(d) |= 1;
      OLC_MODE(d) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;
      
    case CLAN_EDIT_BUILDER:
      CLANRANKTITLE(*OLC_CLAN(d), CLAN_BUILDER) = str_dup(arg);
      OLC_VAL(d) |= 1;
      OLC_MODE(d) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;
      
    case CLAN_EDIT_RETIRED:
      CLANRANKTITLE(*OLC_CLAN(d), CLAN_RETIRED) = str_dup(arg);
      OLC_VAL(d) |= 1;
      OLC_MODE(d) = CLAN_EDIT_RANK_MENU;
      display_rank_menu(d, num);
      break;
      
    case CLAN_EDIT_RANK_MENU:
      switch (*arg)
      {
	case '1':
	  OLC_MODE(d) = CLAN_EDIT_SOLDIER;
	  send_to_char("\r\nEnter new soldier rank: \r\n", d->character);
	  break;
	case '2':
	  OLC_MODE(d) = CLAN_EDIT_SARGEANT;
	  send_to_char("\r\nEnter new sargeant rank: \r\n", d->character);
	  break;
	case '3':
	  OLC_MODE(d) = CLAN_EDIT_CAPTAIN;
	  send_to_char("\r\nEnter new captain rank: \r\n", d->character);
	  break;
	case '4':
	  OLC_MODE(d) = CLAN_EDIT_BUILDER;
	  send_to_char("\r\nEnter new builder rank: \r\n", d->character);
	  break;
	case '5':
	  OLC_MODE(d) = CLAN_EDIT_RULER;
	  send_to_char("\r\nEnter new ruler rank: \r\n", d->character);
	  break;
	case '6':
	  OLC_MODE(d) = CLAN_EDIT_RETIRED;
	  send_to_char("\r\nEnter new retired rank: \r\n", d->character);
	  break;
	case 'q':
        case 'Q':
	  OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
	  cedit_display_menu(d);
	  break;
	default:
	  send_to_char("\r\nInvalid command!\r\n", d->character);
	  display_rank_menu(d, num);
	  break;
      }
      break;

    case CLAN_EDIT_FLAGS_MENU:
      if (atoi(arg) > 0) {
        OLC_VAL(d) |= 1;
        updatebit(&CLANFLAGS(*OLC_CLAN(d)), NUM_CF_FLAGS, atoi(arg), NULL);
        cedit_disp_flags_menu(d);
        return;
      }
      OLC_MODE(d) = CLAN_EDIT_MAIN_MENU;
      cedit_display_menu(d);
      break;
    }
}

void display_colour_menu(struct descriptor_data *d)
{
  struct char_data *ch;

  ch = d->character;

  send_to_char("\r\n\x1B[H\x1B[J", ch);
  sprintf(buf, "%s(W)hite\r\n", CCWHT(ch, C_NRM));
  sprintf(buf, "%s%s(R)ed\r\n", buf, CCRED(ch, C_NRM));
  sprintf(buf, "%s%s(G)reen\r\n", buf, CCGRN(ch, C_NRM));
  sprintf(buf, "%s%s(B)lue\r\n", buf, CCBLU(ch, C_NRM));
  sprintf(buf, "%s%s(Y)ellow\r\n", buf, CCYEL(ch, C_NRM));
  sprintf(buf, "%s%s(M)agenta\r\n", buf, CCMAG(ch, C_NRM));
  sprintf(buf, "%s%s(C)yan\r\n", buf, CCCYN(ch, C_NRM));
  sprintf(buf, "%s%s(N)ormal\r\n", buf, CCNRM(ch, C_NRM));
  sprintf(buf, "%s\r\nSelect a colour: ", buf);
  send_to_char(buf, ch);

}

void display_rank_menu(struct descriptor_data *d, int num)
{
  struct char_data *ch;

  ch = d->character;

  sprintf(buf, "\x1B[H\x1B[J\r\n%sSoldier Rank : %s%s\r\n", CCYEL(ch, C_NRM), 
		CCGRN(ch, C_NRM), 
	SOLDIERTITLE(*OLC_CLAN(d)));
  sprintf(buf + strlen(buf), "%sSargeant Rank: %s%s\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	SARGEANTTITLE(*OLC_CLAN(d)));
  sprintf(buf + strlen(buf), "%sCaptain Rank : %s%s\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	CAPTAINTITLE(*OLC_CLAN(d)));
  sprintf(buf + strlen(buf), "%sBuilder Rank : %s%s\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	CLANRANKTITLE(*OLC_CLAN(d), CLAN_BUILDER));
  sprintf(buf + strlen(buf), "%sRuler Rank: %s%s\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	RULERTITLE(*OLC_CLAN(d)));
  sprintf(buf + strlen(buf), "%sRetired Rank : %s%s\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM), 
	CLANRANKTITLE(*OLC_CLAN(d), CLAN_RETIRED));
  sprintf(buf + strlen(buf), "\r\n%s1. %sEdit Soldier Rank\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf + strlen(buf), "%s2. %sEdit Sargeant Rank\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf + strlen(buf), "%s3. %sEdit Captain Rank\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf + strlen(buf), "%s4. %sEdit Builder Rank\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf + strlen(buf), "%s5. %sEdit Ruler Rank\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf + strlen(buf), "%s6. %sEdit Retired Rank\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  sprintf(buf + strlen(buf), "\r\n%sQ. %sReturn to main menu\r\n", CCYEL(ch, C_NRM),
		CCGRN(ch, C_NRM));
  send_to_char(buf, ch);


}

void save_zone_owners(void)
{
  FILE *fl;
  int i;
  if (!(fl = fopen(HEGEMONY_FILE,"wt")))  {
    log("SYSERR: Cannot write zone owners.");
  } else {
    for (i = 0; i<=top_of_zone_table; i++) {
      fprintf(fl,"%d %d\n", zone_table[i].number, zone_table[i].owner);
    }
    fprintf(fl,"-1");
    fclose(fl);
  }
  return;
}

void load_zone_owners(void)
{
  FILE *fl;
  int zn, cn;
  if (!(fl = fopen(HEGEMONY_FILE,"rt")))  {
    log("SYSERR: Cannot read zone owners. Setting defaults.");
  } else {
    zn = 0;
    while (zn != -1) {
      fscanf(fl,"%d %d", &zn, &cn);
      if (zn != -1 && real_zone(100*zn) != -1) {
        zone_table[real_zone(100*zn)].owner = cn;
 //       sprintf(buf,"%d %d %d", zn, real_zone(100*zn), cn);
 //       log(buf);
      }
    }
    fclose(fl);
  }
  
}

