/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - olc.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define __OLC_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "olc.h"
#include "screen.h"
#include "guild.h"
#include "shop.h"
#include "handler.h"
#include "constants.h"
#include "clan.h"

/*. External data structures .*/
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct room_data *world;
extern struct index_data *mob_index;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;
extern struct guild_master_data *gm_index;
extern struct clan_info *clan_index;
extern int top_of_world;

/*. External functions .*/

/*. Internal function prototypes .*/

void olc_saveinfo(struct char_data *ch);
void olc_saveall(struct char_data *ch);
int can_edit_zone(struct char_data *ch, int number);
int can_view_zone(struct char_data *ch, int number);

/*. Internal data .*/

struct olc_scmd_data {
  char *text;
  int con_type;
};

struct olc_scmd_data olc_scmd_info[6] =
{ {"room", 	CON_REDIT},
  {"object", 	CON_OEDIT},
  {"room",	CON_ZEDIT},
  {"mobile", 	CON_MEDIT},
  {"shop", 	CON_SEDIT},
  {"gm",	CON_GEDIT}
};

#define CHECK_OLC_PERM(ch, zone) \
  { if (!can_edit_zone(ch, real_zone(zone))) { \
    send_to_char("You have not permission to edit that zone.\r\n", ch); \
    return; \
    } \
  }

/* Can they edit a zone?  This takes a zone's rnum.   */
int can_edit_zone(struct char_data *ch, int number)
{
  if (GET_LEVEL(ch) >= LVL_CHBUILD)
    return TRUE;
    
  if (clan_olc_allowed && PLAYERCLAN(ch) != 0 && CLANRANK(ch) >= CLAN_BUILDER 
    && CLANZONE(GET_CLAN(ch)) > 0 
    && real_zone(CLANZONE(GET_CLAN(ch))*100) == number)
    return TRUE;
 
  if (GET_LEVEL(ch) < LVL_BUILDER)
    return FALSE;
    
  if (!olc_allowed || !zone_table[number].builders || !isname(GET_NAME(ch), zone_table[number].builders))
    return FALSE;

  return TRUE;
}

/* Can they VIEW (Read-only OLC editor) a zone?  This takes a zone's rnum.   */
int can_view_zone(struct char_data *ch, int number)
{
  if (can_edit_zone(ch, number))
    return TRUE;
  else if (GET_LEVEL(ch) >= LVL_CHBUILD || ((GET_LEVEL(ch) == LVL_BUILDER && 
    !ZON_FLAGGED(&zone_table[number], ZON_PRIVATE)) && olc_allowed))
      return TRUE;
  else
    if (PLAYERCLAN(ch) != 0 && CLANRANK(ch) >= CLAN_BUILDER &&
    ZON_FLAGGED(&zone_table[number], ZON_PUBLIC) && clan_olc_allowed)
      return TRUE;
  else
    return FALSE;
}

/*------------------------------------------------------------*\
 Eported ACMD do_olc function

 This function is the OLC interface.  It deals with all the 
 generic OLC stuff, then passes control to the sub-olc sections.
\*------------------------------------------------------------*/

ACMD(do_olc)
{
  int number = -1, save = 0, real_num;
  bool clan_edit; 
  struct descriptor_data *d;

  if (IS_NPC(ch))
    /*. No screwing arround .*/
    return;

  if (subcmd >= SCMD_OLC_CLAN_EDIT) {
    clan_edit = TRUE;
    subcmd -= SCMD_OLC_CLAN_EDIT;
  } else clan_edit = FALSE;
  
  if (subcmd == SCMD_OLC_SAVEINFO)
  { 
    one_argument(argument, buf1);
    if (strncmp("save", buf1, 4) == 0) olc_saveall(ch);
    else olc_saveinfo(ch);
    return;
  }
  
  if (GET_LEVEL(ch) < LVL_IMMORT && GET_POS(ch) < POS_STANDING)
    return;
/*  
  if (subcmd == SCMD_OLC_SAVEALL)
  { olc_saveall(ch);
    return;
  }
*/
  /*. Parse any arguments .*/
  two_arguments(argument, buf1, buf2);
  if (!*buf1)
  { /* No argument given .*/
    switch(subcmd)
    { case SCMD_OLC_ZEDIT:
      case SCMD_OLC_REDIT:
        number = world[IN_ROOM(ch)].number;
        break;
      case SCMD_OLC_OEDIT:
      case SCMD_OLC_MEDIT:
      case SCMD_OLC_SEDIT:
      case SCMD_OLC_GEDIT:
        sprintf(buf, "Specify a %s VNUM to edit.\r\n", olc_scmd_info[subcmd].text);
        send_to_char (buf, ch);
        return;
      
    }
  } else if (!isdigit (*buf1))
  {
    if (strncmp("save", buf1, 4) == 0)
    { if (!*buf2)
      { send_to_char("Save which zone?\r\n", ch);
        return;
      } else 
      { save = 1;
        number = atoi(buf2) * 100;
      }
    } else if (subcmd == SCMD_OLC_ZEDIT && GET_LEVEL(ch) >= LVL_CHBUILD)
    { if ((strncmp("new", buf1, 3) == 0) && *buf2)
        zedit_new_zone(ch, atoi(buf2));
      else
        send_to_char("Specify a new zone number.\r\n", ch);
      return;
    } else
    { send_to_char ("Yikes!  Stop that, someone will get hurt!\r\n", ch);
      return;
    }
  }

  

  /*. If a numeric argument was given, get it .*/
  if (number == -1)
    number = atoi(buf1);

  /*. Check whatever it is isn't already being edited .*/
  for (d = descriptor_list; d; d = d->next)
    if (d->connected == olc_scmd_info[subcmd].con_type)
      if (d->olc && OLC_NUM(d) == number)
      { sprintf(buf, "That %s is currently being edited by %s.\r\n",
                olc_scmd_info[subcmd].text, GET_NAME(d->character));
        send_to_char(buf, ch);
        return;
      }

  d = ch->desc; 

  /*. Give descriptor an OLC struct .*/
  CREATE(d->olc, struct olc_data, 1);
  OLC_CLAN_EDIT(d) = clan_edit;
  
  /*. Find the zone .*/
  OLC_ZNUM(d) = real_zone(number);
  if (OLC_ZNUM(d) == -1)
  { send_to_char ("Sorry, there is no zone for that number!\r\n", ch); 
    free(d->olc);
    return;
  }

  /*. Every BUILDER can only edit zones they have been assigned .*/
  /* Here's only view check performed (YIL) */
  if ((GET_LEVEL(ch) < LVL_CHBUILD && !PLR_FLAGGED(ch, PLR_TRUST)) && 
      !can_view_zone(ch, OLC_ZNUM(d)) && 
      (zone_table[OLC_ZNUM(d)].number != GET_OLC_ZONE(ch)))
  { send_to_char("You do not have permission to edit/view this zone.\r\n", ch); 
    free(d->olc);
    return;
  }
  
  if (can_edit_zone(ch, OLC_ZNUM(d)))
    OLC_READONLY(d) = FALSE;
  else
    OLC_READONLY(d) = TRUE;
  
    
  if(save)
  { 
    if (OLC_READONLY(d)) {
      send_to_char("You do not have permission to save this zone.\r\n", 
        d->character);
      return;
    
    }
    switch(subcmd)
    { case SCMD_OLC_REDIT: 
        send_to_char("Saving all rooms in zone.\r\n", ch);
        sprintf(buf, "OLC: %s saves rooms for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        redit_save_to_disk(d); 
        break;
      case SCMD_OLC_ZEDIT:
        send_to_char("Saving all zone information.\r\n", ch);
        sprintf(buf, "OLC: %s saves zone info for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        zedit_save_to_disk(d); 
        break;
      case SCMD_OLC_OEDIT:
        send_to_char("Saving all objects in zone.\r\n", ch);
        sprintf(buf, "OLC: %s saves objects for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        oedit_save_to_disk(d); 
        break;
      case SCMD_OLC_MEDIT:
        send_to_char("Saving all mobiles in zone.\r\n", ch);
        sprintf(buf, "OLC: %s saves mobs for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        medit_save_to_disk(d); 
        break;
      case SCMD_OLC_SEDIT:
        send_to_char("Saving all shops in zone.\r\n", ch);
        sprintf(buf, "OLC: %s saves shops for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        sedit_save_to_disk(d); 
        break;
      case SCMD_OLC_GEDIT:
        send_to_char("Saving all gms in zone.\r\n", ch);
        sprintf(buf, "OLC: %s saves gms for zone %d",
               GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        gedit_save_to_disk(d);
        break;
    }
    free(d->olc);
    return;
  }
 
  OLC_NUM(d) = number;

  /*. Steal players descriptor start up subcommands .*/
  switch(subcmd)
  { case SCMD_OLC_REDIT:
      real_num = real_room(number);
      if (real_num >= 0)
        redit_setup_existing(d, real_num);
      else
        redit_setup_new(d);
      STATE(d) = CON_REDIT;
      break;
    case SCMD_OLC_ZEDIT:
      real_num = real_room(number);
      if (real_num < 0)
      {  send_to_char("That room does not exist.\r\n", ch); 
         free(d->olc);
         return;
      }
      zedit_setup(d, real_num);
      STATE(d) = CON_ZEDIT;
      break;
    case SCMD_OLC_MEDIT:
      real_num = real_mobile(number);
      if (real_num < 0) {
        OLC_FPROG(d) = NULL;
        medit_setup_new(d);
        
      }
      else {
        OLC_FPROG(d) = dup_mobprograms(mob_index[real_num].mobprogs);
        medit_setup_existing(d, real_num);
        
      }
      STATE(d) = CON_MEDIT;
      break;
    case SCMD_OLC_OEDIT:
      real_num = real_object(number);
      if (real_num >= 0)
        oedit_setup_existing(d, real_num);
      else
        oedit_setup_new(d);
      STATE(d) = CON_OEDIT;
      break;
    case SCMD_OLC_SEDIT:
      real_num = real_shop(number);
      if (real_num >= 0)
        sedit_setup_existing(d, real_num);
      else
        sedit_setup_new(d);
      STATE(d) = CON_SEDIT;
      break;
    case SCMD_OLC_GEDIT:
    real_num = real_gm(number);
    if (real_num >= 0)
      gedit_setup_existing(d, real_num);
    else
      gedit_setup_new(d);
    STATE(d) = CON_GEDIT;
    break;
  }
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT(PLR_FLAGS (ch), PLR_WRITING);
}
/*------------------------------------------------------------*\
 Internal utlities 
\*------------------------------------------------------------*/

void olc_saveinfo(struct char_data *ch)
{ struct olc_save_info *entry;
  static char *save_info_msg[6] = { "Rooms", "Objects", "Zone info", "Mobiles", 
    	"Shops", "Gms" };
  int i, found;

  send_to_char("&MDLMUD&mOLC&Y by Mayhem.\r\n\r\n", ch);
  if (GET_LEVEL(ch) >= LVL_CHBUILD || (GET_LEVEL(ch) >= LVL_BUILDER &&
    PLR_FLAGGED(ch, PLR_TRUST)))
    send_to_char("You are permitted to edit any zone in the MUD.\r\n", ch);
  else if (GET_LEVEL(ch) >= LVL_BUILDER || (PLAYERCLAN(ch) != 0 && CLANRANK(ch) >= CLAN_BUILDER)) {
    sprintf(buf, "You have granted permission to edit following zones:\r\n");
    for (found = 0, i = 0; i <= top_of_zone_table; i++)
      if (can_edit_zone(ch, i))
        sprintf(buf + strlen(buf), "%2d.  #%3d  %s\r\n", ++found, 
          zone_table[i].number, zone_table[i].name);
      if (!found)
        sprintf(buf + strlen(buf), "None!\r\n");
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
  } else send_to_char("You have no permission to edit any zone.\r\n", ch); 
  if (olc_save_list)
    send_to_char("The following OLC components need saving:-\r\n", ch);
  else
    send_to_char("The database is up to date.\r\n", ch);

  for (entry = olc_save_list; entry; entry = entry->next)
  { sprintf(buf, " - %s for zone %d.\r\n", 
	save_info_msg[(int)entry->type],
	entry->zone 
    );
    send_to_char(buf, ch);
  }
}


void olc_saveall(struct char_data *ch)
{ struct olc_save_info *entry;
  static char *save_info_msg[6] = { "Rooms", "Objects", "Zone info", "Mobiles", "Shops", "Guildmasters" };
  struct descriptor_data *d;
  int zsave[512];
  int ztype[512];
  int i = 0;
  int j;
  if (GET_LEVEL(ch)<LVL_CHBUILD) {
    send_to_char("Sorry. You have insufficient priority to do this.\r\n", ch);
    return;
  }
  if (olc_save_list)
    send_to_char("The following OLC components will be saved:-\r\n", ch);
  else {
    send_to_char("Nothing to save.\r\n", ch);
    return;}
  for (entry = olc_save_list; entry; entry = entry->next)
    { if (i >= 512) send_to_char("Cannot save - internal error",ch);
      zsave[i] = entry->zone; ztype[i] = entry->type; i++;}
    
    
  for (j = 0; j < i; j++)
  { d = ch->desc; 
    CREATE(d->olc, struct olc_data, 1);
    OLC_ZNUM(d) = real_zone(100 * zsave[j]);
    sprintf(buf, " - %s for zone %d - real_zone %d.\r\n", 
	save_info_msg[ztype[j]],
	zsave[j], OLC_ZNUM(d));
    send_to_char(buf, ch);
    
    switch(ztype[j])
    { case OLC_SAVE_ROOM: 
        sprintf(buf, "OLC: %s saves rooms for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        redit_save_to_disk(d); 
        break;
      case OLC_SAVE_ZONE:
        sprintf(buf, "OLC: %s saves zone info for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        zedit_save_to_disk(d); 
        break;
      case OLC_SAVE_OBJ:
        sprintf(buf, "OLC: %s saves objects for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        oedit_save_to_disk(d); 
        break;
      case OLC_SAVE_MOB:
        sprintf(buf, "OLC: %s saves mobs for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        medit_save_to_disk(d); 
        break;
      case OLC_SAVE_SHOP:
        sprintf(buf, "OLC: %s saves shops for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        sedit_save_to_disk(d); 
        break;
      case OLC_SAVE_GM:
        sprintf(buf, "OLC: %s saves guildmasters for zone %d",
		 GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
        mudlog(buf, CMP, LVL_BUILDER, TRUE);
        gedit_save_to_disk(d); 
        break;
    }
    free(d->olc);
  }
  send_to_char("Done.\r\n",ch);
}


int real_zone(int number)
{ int counter;
  for (counter = 0; counter <= top_of_zone_table; counter++)
    if ((number >= (zone_table[counter].number * 100)) &&
        (number <= (zone_table[counter].top)))
      return counter;

  return -1;
}

/*------------------------------------------------------------*\
 Exported utlities 
\*------------------------------------------------------------*/

/*. Add an entry to the 'to be saved' list .*/

void olc_add_to_save_list(int zone, byte type)
{ struct olc_save_info *new;

  /*. Return if it's already in the list .*/
  for(new = olc_save_list; new; new = new->next)
    if ((new->zone == zone) && (new->type == type))
      return;

  CREATE(new, struct olc_save_info, 1);
  new->zone = zone;
  new->type = type;
  new->next = olc_save_list;
  olc_save_list = new;
}

/*. Remove an entry from the 'to be saved' list .*/

void olc_remove_from_save_list(int zone, byte type)
{ struct olc_save_info **entry;
  struct olc_save_info *temp;

  for(entry = &olc_save_list; *entry; entry = &(*entry)->next)
    if (((*entry)->zone == zone) && ((*entry)->type == type))
    { temp = *entry;
      *entry = temp->next;
      free(temp);
      return;
    }
}

/*. Set the colour string pointers for that which this char will
    see at color level NRM.  Changing the entries here will change 
    the colour scheme throught the OLC.*/

void get_char_cols(struct char_data *ch)
{ nrm = CCNRM(ch, C_NRM);
  grn = CCGRN(ch, C_NRM);
  cyn = CCCYN(ch, C_NRM);
  yel = CCYEL(ch, C_NRM);
}


/*. This procedure removes the '\r\n' from a string so that it may be
    saved to a file.  Use it only on buffers, not on the oringinal
    strings.*/

void strip_string(char *buffer)
{ register char *ptr, *str;

  ptr = buffer;
  str = ptr;

  while((*str = *ptr))
  { str++;
    ptr++;
    if (*ptr == '\r')
      ptr++;
  }
}


/*. This procdure frees up the strings and/or the structures
    attatched to a descriptor, sets all flags back to how they
    should be .*/

void cleanup_olc(struct descriptor_data *d, byte cleanup_type)
{ 
  if (d->olc)
  {
    /*. Check for room .*/
    if(OLC_ROOM(d))
    { /*. free_room performs no sanity checks, must be carefull here .*/
      switch(cleanup_type)
      { case CLEANUP_ALL:
          free_room(OLC_ROOM(d));
          break;
        case CLEANUP_STRUCTS:
          free(OLC_ROOM(d));
          break;
        default:
          /*. Caller has screwed up .*/
          break;
      }
    }
  
    /*. Check for object .*/
    if(OLC_OBJ(d))
    { /*. free_obj checks strings arn't part of proto .*/
      free_obj(OLC_OBJ(d));
    }

    /*. Check for mob .*/
    if(OLC_MOB(d))
    { /*. free_char checks strings arn't part of proto .*/
/*      free_char(OLC_MOB(d));*/
      medit_free_mobile(OLC_MOB(d));
      medit_free_progs(OLC_FPROG(d));
    }
  
    /*. Check for zone .*/
    if(OLC_ZONE(d))
    { /*. cleanup_type is irrelivent here, free everything .*/
      free(OLC_ZONE(d)->name);
      free(OLC_ZONE(d)->cmd);
      free(OLC_ZONE(d));
    }

    /*. Check for shop .*/
    if(OLC_SHOP(d))
    { /*. free_shop performs no sanity checks, must be carefull here .*/
      switch(cleanup_type)
      { case CLEANUP_ALL:
          free_shop(OLC_SHOP(d));
          break;
        case CLEANUP_STRUCTS:
          free(OLC_SHOP(d));
          break;
        default:
          /*. Caller has screwed up .*/
          break;
      }
      /*. Check for gm . */
      if (OLC_GUILD(d)) {              
       switch (cleanup_type) {
        case CLEANUP_ALL:
         free_gm(OLC_GUILD(d));
         break;
        case CLEANUP_STRUCTS:
         free(OLC_GUILD(d));
          break;
        }
      }
    }

    /*. Restore desciptor playing status .*/
    if (d->character)
    { REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING);
      STATE(d)=CON_PLAYING;
      act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
    }
    free(d->olc);
  }
}

#define MAX_FOUND	(160)
#define IN_NEXT_ZONE(number)	(number > last_in_zone && ((number / 100) > (last_in_zone / 100)))
#define CHECK_ZONE_VIEW_PERM(num)	\
	if (IN_NEXT_ZONE(num) || last_in_zone == -1) { 				\
	    zone_rnum = real_zone(num);						\
            last_in_zone = zone_table[zone_rnum].top;				\
            if (!can_view_zone(ch, zone_rnum)) {				\
              sprintf(buf + strlen(buf), "&RZone #%d: Access denied.&w\r\n", 	\
                zone_table[zone_rnum].number);					\
              first = last_in_zone + 1;						\
              first += 100 - (first % 100);					\
              continue;								\
            };									\
          }

ACMD(do_liblist)
{
  extern struct room_data *world;
  extern struct index_data *mob_index;
  extern struct char_data *mob_proto;
  extern struct index_data *obj_index;
  extern struct obj_data *obj_proto;
  extern struct shop_data *shop_index;
  extern int top_of_objt;
  extern int top_of_mobt;
  extern int top_of_world;
  extern int top_shop;
  extern int top_guild;

  int first, last, nr, found = 0, last_in_zone = -1, zone_rnum;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2) {
    switch (subcmd) {
      case SCMD_RLIST:
        send_to_char("Usage: rlist <begining number> <ending number>\r\n", ch);
        break;
      case SCMD_OLIST:
        send_to_char("Usage: olist <begining number> <ending number>\r\n", ch);
        break;
      case SCMD_MLIST:
        send_to_char("Usage: mlist <begining number> <ending number>\r\n", ch);
        break;
      case SCMD_GLIST:
        send_to_char("Usage: glist <begining number> <ending number>\r\n", ch);
        break;
      case SCMD_SLIST:
        send_to_char("Usage: slist <begining number> <ending number>\r\n", ch);
        break;
      default:
        sprintf(buf, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
        mudlog(buf, BRF, LVL_GOD, TRUE);
        break;
    } 
    return;
  }

  first = atoi(buf);
  last = atoi(buf2);

  if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
    send_to_char("Values must be between 0 and 99999.\n\r", ch);
    return;
  }

  if (first >= last) {
    send_to_char("Second value must be greater than first.\n\r", ch);
    return;
  }

  *buf = '\000';
  switch (subcmd) {
    case SCMD_RLIST:
      sprintf(buf, "&WRoom List From Vnum &C%d&W to &C%d&w\r\n", first, last);
      for (nr = 0; nr <= top_of_world && (world[nr].number <= last); nr++) {
        if (world[nr].number >= first) {
          CHECK_ZONE_VIEW_PERM(world[nr].number);
          
          
          if (found > MAX_FOUND) {
            strcat(buf, "&R**OVERFLOW**&w\r\n");
            break;
          }
            
          sprintf(buf + strlen(buf), "%5d. %s[%5d] (%3d) %s\r\n", ++found,
                  ROOM_FLAGGED(nr, ROOM_DELETED) ? "&R*&w" : " ",
                  world[nr].number, world[nr].zone,
                  world[nr].name);
        }
      }
      break;
    case SCMD_OLIST:
      sprintf(buf, "&WObject List From Vnum &C%d&W to &C%d&w\r\n", first, last);
      for (nr = 0; nr <= top_of_objt && (obj_index[nr].vnum <= last); nr++) {
        if (obj_index[nr].vnum >= first) {
          CHECK_ZONE_VIEW_PERM(obj_index[nr].vnum);
          if (found > MAX_FOUND) {
            strcat(buf, "&R**OVERFLOW**&w\r\n");
            break;
          }
          sprintf(buf + strlen(buf), "%5d. %s[%5d] %s\r\n", ++found,
                  IS_OBJ_STAT2(obj_proto + nr, ITEM2_DELETED) ? "&R*&w" : " ",
                  obj_index[nr].vnum,
                  obj_proto[nr].short_description);
        }
      }
      break;
    case SCMD_MLIST:
      sprintf(buf, "&WMob List From Vnum &C%d&W to &C%d&w\r\n", first, last);
      for (nr = 0; nr <= top_of_mobt && (mob_index[nr].vnum <= last); nr++) {
        if (mob_index[nr].vnum >= first) {
          CHECK_ZONE_VIEW_PERM(mob_index[nr].vnum);
          if (found > MAX_FOUND) {
            strcat(buf, "&R**OVERFLOW**&w\r\n");
            break;
          }
          sprintf(buf + strlen(buf), "%5d. %s[%5d] %s\r\n", ++found,
                  MOB2_FLAGGED(&mob_proto[nr], MOB2_DELETED) ? "&R*&w" : " ",
                  mob_index[nr].vnum,
                  mob_proto[nr].player.short_descr);
        }
      }
      break;
    case SCMD_GLIST:
      sprintf(buf, "&WGuild List From Vnum &C%d&W to &C%d&w\r\n", first, last);
      for (nr = 0; nr <= top_guild && (gm_index[nr].num <= last); nr++) {
        if (gm_index[nr].num >= first) {
          CHECK_ZONE_VIEW_PERM(gm_index[nr].num);
          if (found > MAX_FOUND) {
            strcat(buf, "&R**OVERFLOW**&w\r\n");
            break;
          }
          sprintf(buf + strlen(buf), "%5d. [%5d] gm:[%5d] %s\r\n", ++found,
                  gm_index[nr].num, mob_index[gm_index[nr].gm].vnum, 
                  mob_proto[gm_index[nr].gm].player.short_descr);
        }
      }
      break;
    case SCMD_SLIST:
      sprintf(buf, "&WShop List From Vnum &C%d&W to &C%d&w\r\n", first, last);
      for (nr = 0; nr <= top_shop && (shop_index[nr].vnum <= last); nr++) {
        if (shop_index[nr].vnum >= first) {
          CHECK_ZONE_VIEW_PERM(shop_index[nr].vnum);
          if (found > MAX_FOUND) {
            strcat(buf, "&R**OVERFLOW**&w\r\n");
            break;
          }
          sprintf(buf + strlen(buf), "%5d. [&c%5d&w] keeper:[&g%5d&w] %-25s room:[%5d]\r\n", 
          	  ++found, shop_index[nr].vnum, 
                  mob_index[shop_index[nr].keeper].vnum, 
                  mob_proto[shop_index[nr].keeper].player.short_descr, 
                  shop_index[nr].in_room ? *shop_index[nr].in_room : -1);
        }
      }
      break;
    default:
      sprintf(buf, "SYSERR:: invalid SCMD passed to ACMD(do_liblist)!");
      mudlog(buf, BRF, LVL_GOD, TRUE);
      return;
  }

  if (!found) {
    switch (subcmd) {
      case SCMD_RLIST:
        strcat(buf, "No rooms found within those parameters.\r\n");
        break;
      case SCMD_OLIST:
        strcat(buf, "No objects found within those parameters.\r\n");
        break;
      case SCMD_MLIST:
        strcat(buf, "No mobiles found within those parameters.\r\n");
        break;
      case SCMD_GLIST:
        strcat(buf, "No guilds found within those parameters.\r\n");
        break;
      case SCMD_SLIST:
        strcat(buf, "No shops found within those parameters.\r\n");
        break;
      default:
        sprintf(buf, "SYSERR:: invalid SCMD passed to do_build_list!");
        mudlog(buf, BRF, LVL_GOD, TRUE);
        return;
    } 
  }

  page_string(ch->desc, buf, 1);
}

int make_help(struct char_data* ch, char *arg, char *help_str)
{
   ACMD(do_help);
   if (strcmp(arg, "?")==0) {
     send_to_char("\r\n", ch);
     do_help(ch, help_str, 0, 0);
     send_to_char("&gPress a key to continue...&w\r\n", ch);
     return 1;
   } else return 0;
}

void olc_disp_spec_proc_menu(struct descriptor_data * d,
  struct specproc_info table[])
{
  send_to_char("[H[J", d->character);
  show_spec_proc_table(table, GET_LEVEL(d->character), buf);
  send_to_char(buf, d->character);
  send_to_char("\r\n&wSelect spec proc: ", d->character);
}

ACMD(do_mex)
{
/* Only works if you have Oasis OLC */
extern void olc_add_to_save_list(int zone, byte type);

  int iroom = 0, rroom = 0;
  int dir = 0;
  struct room_data *room;

  two_arguments(argument, buf2, buf); 

  if (!*buf2 || !*buf) {
    send_to_char("Format: mexit <dir> <room number>\r\n", ch); 
    return; }

  iroom = atoi(buf);
  rroom = real_room(iroom);

 
   CHECK_OLC_PERM(ch, GET_ROOM_ZONE(ch->in_room) * 100);
   CHECK_OLC_PERM(ch, iroom);
   if (rroom <= 0) {
     sprintf(buf, "There is no room with the number %d", iroom);
     send_to_char(buf, ch);
     return;
   }
/* Main stuff */
    switch (*buf2) {
    case 'n':
    case 'N':
      dir = NORTH;
      break;
    case 'e':
    case 'E':
      dir = EAST;
      break;
    case 's':
    case 'S':
      dir = SOUTH;
      break;
    case 'w':
    case 'W':
      dir = WEST;
      break;
    case 'u':
    case 'U':
      dir = UP;
      break;
    case 'd':
    case 'D':
      dir = DOWN;
      break; }

  room = &world[rroom];
  if (room->dir_option[rev_dir[dir]]) {
    if (room->dir_option[rev_dir[dir]]->general_description)
      free(room->dir_option[rev_dir[dir]]->general_description);
    if (room->dir_option[rev_dir[dir]]->keyword)
      free(room->dir_option[rev_dir[dir]]->keyword);

  }
  CREATE(world[rroom].dir_option[rev_dir[dir]], struct room_direction_data,1); 
  world[rroom].dir_option[rev_dir[dir]]->general_description = NULL;
  world[rroom].dir_option[rev_dir[dir]]->keyword = NULL;
  world[rroom].dir_option[rev_dir[dir]]->to_room = ch->in_room; 


  room = &world[ch->in_room];
  if (room->dir_option[rev_dir[dir]]) {
    if (room->dir_option[rev_dir[dir]]->general_description)
      free(room->dir_option[rev_dir[dir]]->general_description);
    if (room->dir_option[rev_dir[dir]]->keyword)
      free(room->dir_option[rev_dir[dir]]->keyword);
  }

  CREATE(world[ch->in_room].dir_option[dir], struct room_direction_data,1); 
  world[ch->in_room].dir_option[dir]->general_description = NULL;
  world[ch->in_room].dir_option[dir]->keyword = NULL;
  world[ch->in_room].dir_option[dir]->to_room = rroom; 

/* Only works if you have Oasis OLC */
  olc_add_to_save_list((iroom/100), OLC_SAVE_ROOM);

 sprintf(buf, "You make an exit %s to room %d.\r\n", buf2, iroom);
 send_to_char(buf, ch);
}

ACMD(do_delete)
{
  int i, r;
  
  two_arguments(argument, buf2, buf);
  if (!*buf2 || !*buf) {
    if (subcmd == SCMD_DELETE)
      send_to_char("Format: delete <room | mob | obj> <virtual number>\r\n", ch); 
    else
      send_to_char("Format: undelete <room | mob | obj> <virtual number>\r\n", ch); 
    return; 
  }
  if (is_abbrev(buf2, "room")) {
    i = atoi(buf);  
    r = real_room(i);
    if (r <= 0) {
      send_to_char("There is no room with that number.\r\n", ch);
      return;
    }
    CHECK_OLC_PERM(ch, i);
    if (subcmd == SCMD_DELETE) {
      if (!IS_SET(ROOM_FLAGS(r), ROOM_DELETED)) {
        SET_BIT(ROOM_FLAGS(r), ROOM_DELETED);
        sprintf(buf, "Room %d deleted.\r\n", i);
        send_to_char(buf, ch);
        olc_add_to_save_list(i/100, OLC_SAVE_ROOM);
      } else send_to_char("That room is already marked for deletion.\r\n", ch);
    } else
    if (subcmd == SCMD_UNDELETE) {
      if (IS_SET(ROOM_FLAGS(r), ROOM_DELETED)) {
        REMOVE_BIT(ROOM_FLAGS(r), ROOM_DELETED);
        sprintf(buf, "Room %d UNdeleted.\r\n", i);
        send_to_char(buf, ch);
        olc_add_to_save_list(i/100, OLC_SAVE_ROOM);
      } else send_to_char("That room is NOT marked for deletion.\r\n", ch);
    }
  }
  else if (is_abbrev(buf2, "mob")) {
    i = atoi(buf);  
    r = real_mobile(i);
    if (r <= 0) {
      send_to_char("There is no mob with that number.\r\n", ch);
      return;
    }
    CHECK_OLC_PERM(ch, i);
    if (subcmd == SCMD_DELETE) {
      if (!MOB2_FLAGGED(mob_proto + r, MOB2_DELETED)) {
        SET_BIT(MOB2_FLAGS(mob_proto + r), MOB2_DELETED);
        sprintf(buf, "Mob %d deleted.\r\n", i);
        send_to_char(buf, ch);
        olc_add_to_save_list(i/100, OLC_SAVE_MOB);
      } else send_to_char("That mob is already marked for deletion.\r\n", ch);
    } else
    if (subcmd == SCMD_UNDELETE) {
      if (MOB2_FLAGGED(mob_proto +r, MOB2_DELETED)) {
        REMOVE_BIT(MOB2_FLAGS(mob_proto + r), MOB2_DELETED);
        sprintf(buf, "Mob %d UNdeleted.\r\n", i);
        send_to_char(buf, ch);
        olc_add_to_save_list(i/100, OLC_SAVE_MOB);
      } else send_to_char("That mob is NOT marked for deletion.\r\n", ch);
    }
  }
  else if (is_abbrev(buf2, "obj")) {
    i = atoi(buf);  
    r = real_object(i);
    if (r <= 0) {
      send_to_char("There is no obj with that number.\r\n", ch);
      return;
    }
    CHECK_OLC_PERM(ch, i);
    if (subcmd == SCMD_DELETE) {
      if (!IS_OBJ_STAT2(obj_proto + r, ITEM2_DELETED)) {
        SET_BIT(GET_OBJ_EXTRA2(obj_proto +r), ITEM2_DELETED);
        sprintf(buf, "Object %d deleted.\r\n", i);
        send_to_char(buf, ch);
        olc_add_to_save_list(i/100, OLC_SAVE_OBJ);
      } else send_to_char("That obj is already marked for deletion.\r\n", ch);
    } else
    if (subcmd == SCMD_UNDELETE) {
      if (IS_OBJ_STAT2(obj_proto +r, ITEM2_DELETED)) {
        REMOVE_BIT(GET_OBJ_EXTRA2(obj_proto +r), ITEM2_DELETED);
        sprintf(buf, "Object %d UNdeleted.\r\n", i);
        send_to_char(buf, ch);
        olc_add_to_save_list(i/100, OLC_SAVE_OBJ);
      } else send_to_char("That obj is NOT marked for deletion.\r\n", ch);
    }
  }
}

ACMD(do_roomlink)         /* Shows all exits to a given room */
{
  int door, i, room_num;

  one_argument(argument, buf);
  
  if (!buf || !*buf)
    room_num = ch->in_room;
  else
    room_num = real_room(atoi(buf));

  if (room_num == NOWHERE) {
    send_to_char("There is no room with that number.\r\n", ch);
    return;
  } else {
    for (i = 0; i <= top_of_world; i++) {
      for (door = 0; door < NUM_OF_DIRS; door++) {
        if (world[i].dir_option[door] &&
	    world[i].dir_option[door]->to_room == room_num) {
          sprintf(buf2, "Exit %s from room %d.\r\n", dirs[door], 
		world[i].number);
          send_to_char(buf2, ch);
        }
      }
    }
  }
}

long mob_account(struct char_data *mob)
{
  long cost = 0;
  assert(mob);
  cost += 10;
  cost += GET_LEVEL(mob)*1000 + (GET_HIT(mob)*GET_MANA(mob)+GET_MOVE(mob))
    + GET_HITROLL(mob)*1000 + GET_DAMROLL(mob)*1000 + 50*(100 - GET_AC(mob));
  // GET_MOB_PRICE(mob) = cost;
  return cost;
}

int mob_exp(struct char_data *mob)
{
  int exp = 0;
  exp += 5*(GET_HIT(mob)*GET_MANA(mob)+GET_MOVE(mob))+GET_HITROLL(mob)*1000
    + GET_DAMROLL(mob)*1000 + (100 - GET_AC(mob))*50;
  GET_EXP(mob) = exp;
  return exp;
}

long room_account(struct room_data *room)
{
  long cost = 0;
  assert(room);
  cost += 10000;
  return cost;
}

void perm_range_message(struct descriptor_data *d, int bottom, int top)
{
  sprintf(buf, "You may only enter values from range [%d-%d]\r\n"
  		"Press any key to continue...", bottom, top);
  send_to_char(buf, d->character);
  OLC_LAST(d) = MEDIT_MAIN_MENU;
  OLC_MODE(d) = MEDIT_HELP;
}
