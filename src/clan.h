/* *************************************************************************
*   File: clan.h				     Addition to CircleMUD *
*  Usage: Contains structure definitions for clan.c                        *
*                                                                          *
*  Written by Daniel Muller                                                *
*                                                                          *
************************************************************************* */

#ifndef __CLAN_H__
#define __CLAN_H__

/* Clan flags */
#define CF_GODCLAN	(1 << 0)

#define NUM_CF_FLAGS	1

/*
 * Clan levels
*/
#define CLAN_NORANK	0
#define CLAN_APPLY 	1
#define CLAN_SOLDIER 	2
#define CLAN_SARGEANT 	3
#define CLAN_CAPTAIN 	4
#define CLAN_BUILDER	5
#define CLAN_RULER  	6
#define CLAN_RETIRED	7
#define CLAN_NOBODY	99

#define CLAN_NUM_RANKS	8

#define CLAN_WHITE		0
#define CLAN_GREEN		1
#define CLAN_RED		2
#define CLAN_BLUE		3
#define CLAN_MAGENTA		4
#define CLAN_YELLOW		5
#define CLAN_CYAN		6
#define CLAN_NORMAL		7

#define CLAN_EDIT_GOD_LEVEL	LVL_CHBUILD
#define CLAN_EDIT_LEVEL		CLAN_RULER
#define CLAN_EDIT_PRIVILEGED	LVL_COIMPL

struct clan_info {
   char *title;
   ubyte colour;
   char *owner;
   char *description;
   char *ranks[CLAN_NUM_RANKS];
   int clanroom;
   long clangold;
   int nummembers;
   int number;
   long flags;
   ubyte clantown;
   int rips;
   int kills;
   ubyte limit_down;
   ubyte limit_up;
   char *motd;
   int zone_vnum;
};

struct clan_colours {
  char *colour_string;
};

struct clan_command_info {
  const char *command;
  void	(*command_pointer)
    (struct char_data *ch, char * argument, int cmd, int subcmd);
  byte min_clan_level;
  byte min_level;
  int  subcmd;
};

#define CLAN_EDIT_CONFIRM_SAVE	0
#define CLAN_EDIT_MAIN_MENU	1
#define CLAN_EDIT_NAME		2
#define CLAN_EDIT_NUMBER	3
#define CLAN_EDIT_GOLD		4
#define CLAN_EDIT_OWNER		5
#define CLAN_EDIT_DESCRIPTION	6
#define CLAN_EDIT_CLANROOM	7
#define CLAN_EDIT_COLOUR	8
#define CLAN_EDIT_RANK_MENU	9
#define CLAN_EDIT_SELECT_COLOUR 10
#define CLAN_EDIT_SOLDIER	11
#define CLAN_EDIT_SARGEANT	12
#define CLAN_EDIT_CAPTAIN	13
#define CLAN_EDIT_RULER		14
#define CLAN_EDIT_DESCRIPTION_FINISHED		15
#define CLAN_EDIT_CONFIRM_DELETE		16
#define CLAN_EDIT_HOMETOWN	17
#define CLAN_EDIT_LEVEL_DOWN	18
#define CLAN_EDIT_LEVEL_UP	19
#define CLAN_EDIT_KILLS		20
#define CLAN_EDIT_RIPS		21
#define CLAN_EDIT_WAITKEY	22
#define CLAN_EDIT_MOTD		23
#define CLAN_EDIT_BUILDER	24
#define CLAN_EDIT_RETIRED	25
#define CLAN_EDIT_HELP		26
#define CLAN_EDIT_FLAGS_MENU	27

#define PLAYERCLAN(ch)		((ch)->player_specials->saved.clannum)
#define PLAYERCLANNUM(ch)	(real_clan(PLAYERCLAN(ch)))
#define	CLANNUM(clan)		((clan).number)
#define	CLANZONE(clan)		((clan).zone_vnum)
#define	CLANNAME(clan)		((clan).title)
#define	CLANRANK(ch)		((ch)->player_specials->saved.clanrank)
#define CLANPLAYERS(clan) 	((clan).nummembers)	
#define CLANOWNER(clan)   	((clan).owner)
#define CLANGOLD(clan)		((clan).clangold)
#define CLANROOM(clan)		((clan).clanroom)
#define CLANDESC(clan)		((clan).description)
#define CLANCOLOUR(num)	(clan_colour[clan_index[(num)].colour].colour_string)
#define CLANCOLOR(num)		(clan_index[(num)].colour)
#define CLANCOLORNUM(clan)	((clan).colour)
#define IN_WAR(ch)		(PLAYERCLAN(ch))
#define CLANTOWN(clan)		((clan).clantown)
#define CLANFLAGS(clan)		((clan).flags)
#define CLANRIPS(clan)		((clan).rips)
#define CLANKILLS(clan)		((clan).kills)
#define CLANMOTD(clan)		((clan).motd)
#define CLAN_LIMIT_DOWN(clan)		((clan).limit_down)
#define CLAN_LIMIT_UP(clan)		((clan).limit_up)
#define CLAN_HOMETOWN(clan)		((clan).clantown)
#define CLANRANKTITLE(clan, num)	((clan).ranks[num])

#define CLAN_FLAGGED(clan, flag) (IS_SET(CLANFLAGS(clan), (flag)))

#define SOLDIERTITLE(clan)	((clan).ranks[CLAN_SOLDIER])
#define SARGEANTTITLE(clan)	((clan).ranks[CLAN_SARGEANT])
#define CAPTAINTITLE(clan)	((clan).ranks[CLAN_CAPTAIN])
#define BUILDERTITLE(clan)	((clan).ranks[CLAN_BUILDER])
#define RULERTITLE(clan)	((clan).ranks[CLAN_RULER])
#define RETIREDTITLE(clan)	((clan).ranks[CLAN_RETIRED])

#define GET_CLAN(ch)		(clan_index[PLAYERCLANNUM(ch)])

/* Public functions */
void clan_save(int system, struct char_data *ch);
void cleanup_clan_edit(struct descriptor_data *d);
void update_clan_cross_reference();

/* Externals from clan.c */
#ifndef __CLAN_C__

#endif

#endif /* __CLAN_H__ */
