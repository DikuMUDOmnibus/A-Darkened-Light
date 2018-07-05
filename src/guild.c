
/* 
************************************************************************
*   File: Guild.c                                                         *
*  Usage: GuildMaster's: loading files, assigning spec_procs, and handling*
*                        practicing.                                      *
*                                                                         *
* Based on shop.c.  As such, the CircleMud License applies                *
* Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"
#include "guild.h"

/*extern declerations */
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


/* extern function prototypes */
ACMD(do_tell);
ACMD(do_say);


/* Local variables */
struct guild_master_data *gm_index;
int top_guild = 0;


char *how_good(int percent)
{
  if (percent == 0) return(" (not learned)");
  else if (percent <= 10) return(" (awful)");
  else if (percent <= 20) return(" (bad)");
  else if (percent <= 40) return(" (poor)");
  else if (percent <= 55) return(" (average)");
  else if (percent <= 70) return(" (fair)");
  else if (percent <= 80) return(" (good)");
  else if (percent <= 85) return(" (very good)");
  else return(" (superb)");
}

//char *prac_types[] = {
//  "spell",
//  "skill"
//};

int is_guild_open(struct char_data *keeper, int guild_nr, int msg)
{
  char buf[200];

  *buf = 0;
  if (GM_OPEN(guild_nr) > time_info.hours && GM_CLOSE(guild_nr) < time_info.hours)
    strcpy(buf, MSG_TRAINER_NOT_OPEN);

  if (!(*buf))
    return (TRUE);
  if (msg)
    do_say(keeper, buf, cmd_tell, 0);
  return (FALSE);
}

int is_gm_ok_char(struct char_data * keeper, struct char_data * ch, int guild_nr)
{

  char buf[200];

  if (!(CAN_SEE(keeper, ch))) {
    do_say(keeper, MSG_TRAINER_NO_SEE_CH, cmd_say, 0);
    return (FALSE);
  }
  /*
  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "%s %s", GET_NAME(ch), "Why would an immortal need to practice?");
    do_tell(keeper, buf, cmd_tell, 0); 
    return (FALSE);
	}
  */
  if ((IS_GOOD(ch) && NOTRAIN_GOOD(guild_nr)) ||
      (IS_EVIL(ch) && NOTRAIN_EVIL(guild_nr)) ||
      (IS_NEUTRAL(ch) && NOTRAIN_NEUTRAL(guild_nr))) {
    sprintf(buf, "%s %s", GET_NAME(ch), MSG_TRAINER_DISLIKE_ALIGN);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }

  if (IS_NPC(ch)) {
    sprintf(buf, "%s %s", GET_NAME(ch), "Why would a mob need to practice?");
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
	}

  if ((IS_MAGIC_USER(ch) && NOTRAIN_MAGE(guild_nr)) ||
      (IS_CLERIC(ch) && NOTRAIN_CLERIC(guild_nr)) ||
      (IS_THIEF(ch) && NOTRAIN_THIEF(guild_nr)) ||
      (IS_WARRIOR(ch) && NOTRAIN_WARRIOR(guild_nr)) ||
      (IS_PALADIN(ch) && NOTRAIN_PALADIN(guild_nr)) ||
      (IS_RANGER(ch) && NOTRAIN_RANGER(guild_nr)) ||
      (IS_WARLOCK(ch) && NOTRAIN_WARLOCK(guild_nr)) ||
      (IS_CYBORG(ch) && NOTRAIN_CYBORG(guild_nr)) ||
      (IS_NECROMANCER(ch) && NOTRAIN_NECROMANCER(guild_nr)) ||
      (IS_DRUID(ch) && NOTRAIN_DRUID(guild_nr)) ||
      (IS_ALCHEMIST(ch) && NOTRAIN_ALCHEMIST(guild_nr)) ||
      (IS_BARBARIAN(ch) && NOTRAIN_BARBARIAN(guild_nr))) {
    sprintf(buf, "%s %s", GET_NAME(ch), MSG_TRAINER_DISLIKE_CLASS);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }
  return (TRUE);
}

int is_gm_ok(struct char_data * keeper, struct char_data * ch, int guild_nr)
{
  if (is_guild_open(keeper, guild_nr, TRUE))
    return (is_gm_ok_char(keeper, ch, guild_nr));
  else
    return (FALSE);
}

int does_gm_know(int guild_nr, int i)
{
return ((int)(gm_index[guild_nr].skills_and_spells[i]));
}

/* this and list skills should probally be combined.
   perhaps in the next release?  */

void what_does_gm_know(int guild_nr, struct char_data * ch)
{
  extern char *spells[];
  extern struct spell_info_type spell_info[];
  int i, sortpos, found;

  if (!GET_PRACTICES(ch))
    strcpy(buf2, "&WYou have &Rno&W practice sessions remaining.&w\r\n");
  else
    sprintf(buf2, "&WYou have &G%d&W practice session%s remaining.&w\r\n",
	    GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));

//  sprintf(buf, "%sI can teach you the following %ss:\r\n", buf, GM_SPLSKL(guild_nr));

//  strcpy(buf2, buf);
  
  strcat(buf2, "&MI can teach you the following spells:\r\n");

  found = 0;
  for (sortpos = 1; sortpos < MAX_SKILLS; sortpos++) {
    i = spell_sort_info[sortpos];
    if (!IS_SKILL(i)) {
      if (strlen(buf2) >= MAX_STRING_LENGTH - 80) {
        strcat(buf2, "**OVERFLOW**\r\n");
        break;
      }
      if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] &&
          does_gm_know(guild_nr, i)) {
        sprintf(buf2 + strlen(buf2), "&c%-20s &g%-15s &Y%d&w gold\r\n", spells[i], how_good(GET_SKILL(ch, i)), 
                  GM_COST(guild_nr,i,ch));
        found++;
      }
    }
  }
  if (found == 0) strcat(buf2,"  None.\r\n");
  strcat(buf2, "\r\n");

  strcat(buf2, "&MAnd the following skills:\r\n");
  found = 0;
  for (sortpos = 1; sortpos < MAX_SKILLS; sortpos++) {
    i = spell_sort_info[sortpos];
    if (IS_SKILL(i)) {
      if (strlen(buf2) >= MAX_STRING_LENGTH - 80) {
        strcat(buf2, "**OVERFLOW**\r\n");
        break;
      }
      if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)] &&
          does_gm_know(guild_nr, i)) {
        sprintf(buf2 + strlen(buf2), "&c%-20s &g%-15s &Y%d&w gold\r\n", spells[i], how_good(GET_SKILL(ch, i)), 
                  GM_COST(guild_nr,i,ch));
        found++;
      }
    }
  }
  if (found == 0) strcat(buf2,"  None.\r\n");
  strcat(buf2, "\r\n");
  page_string(ch->desc, buf2, 1);
}


void list_skills(struct char_data * ch)
{
extern char *spells[];
extern struct spell_info_type spell_info[];
int i, x;

//sprintf(buf, "%s%s----------------------------------[ SKILLS 
//]----------------------------------\r\n", buf, x != 3 ? "\r\n" : "");
//x = 0;
//for (i = MAX_SPELLS ; i < MAX_SKILLS; i++) {
//if (GET_LEVEL(ch) >= spell_info[i].min_level[(int)GET_CLASS(ch)]) {
//x++;
//sprintf(buf, "%s%17.17s &w(&c%2d%%&w)&c%s", buf, spells[i], 
//GET_SKILL(ch, i), x == 3 ? "\r\n" : "");
//if (x == 3)
//x = 0;
// }
//}

sprintf(buf, "%s%s&c----------------------------------[ SPELL BOOK ]--------------------------------\r\n"
"--------------------------------------------------------------------------------\r\n", buf, x != 3 ? "\r\n" : "");
x = 0;
for (i = 1; i < MAX_SPELLS; i++) {

if (GET_LEVEL(ch) >= spell_info[i].min_level[(int)GET_CLASS(ch)]) {
x++;
sprintf(buf, "%s%17.17s &w(&c%2d%%&w)&c%s", buf, spells[i], GET_SKILL(ch, i), x == 3 ? "\r\n" : "");
if (x == 3)
x = 0;
 }
}
sprintf (buf, 
"%s\r\n------------------------------------[ COMBAT ]----------------------------------\r\n"
"--------------------------------------------------------------------------------\r\n",buf);

x = 0;
for (i = MAX_SPELLS ; i < MAX_SKILLS; i++) {
if (GET_LEVEL(ch) >= spell_info[i].min_level[(int)GET_CLASS(ch)]) {
x++;
sprintf(buf, "%s%17.17s &w(&c%2d%%&w)&c%s", buf, spells[i],
GET_SKILL(ch, i), x == 3 ? "\r\n" : "");
if (x == 3)
x = 0;
 }
}

if (x != 3)
sprintf(buf, 
"%s\r\n--------------------------------------------------------------------------------",buf);
strcat(buf,"\r\n");

if (!GET_PRACTICES(ch))
sprintf(buf, "%s		You have no rune stones to sacrafice remaining.",buf);
else
sprintf(buf, "%s        	You have %d rune stone%s remaining to sacrafice.", buf,
GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));
sprintf(buf, 
"%s\r\n--------------------------------------------------------------------------------\r\n",buf);
send_to_char(buf, ch);
}



SPECIAL(guild)
{
  int skill_num, percent;

  extern struct spell_info_type spell_info[];
  extern struct int_app_type int_app[];

/***  Let's see which guildmaster we're dealing with ***/

struct char_data *keeper = (struct char_data *) me;
int guild_nr;

for (guild_nr = 0; guild_nr < top_guild; guild_nr++)
 if (GM_TRAINER(guild_nr) == keeper->nr)
  break;

if (guild_nr >= top_guild)
   return (FALSE);

if (GM_FUNC(guild_nr))
   if ((GM_FUNC(guild_nr)) (ch, me, cmd, arg))
      return(TRUE);


 if (IS_NPC(ch) || !CMD_IS("practice"))
    return 0;

  skip_spaces(&argument);

/*** Is the GM able to train?    ****/

  if (!AWAKE(keeper))
   return (FALSE);

  if (!(is_gm_ok(keeper, ch, guild_nr)))
      return 1;

  if (!*argument) {
	what_does_gm_know(guild_nr, ch);
	return 1;
  }

  if (GET_PRACTICES(ch) <= 0) {
    send_to_char("You do not seem to be able to practice now.\r\n", ch);
    return 1;
  }

  skill_num = find_skill_num(argument);

/****  Does the GM know the skill the player wants to learn?  ****/

if (!(does_gm_know(guild_nr, skill_num))) {
    sprintf(buf2, gm_index[guild_nr].no_such_skill, GET_NAME(ch));
    do_tell(keeper, buf2, cmd_tell, 0);
	return 1;
	}

/**** Can the player learn the skill if the GM knows it?  ****/ 

if (skill_num < 1 ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
    sprintf(buf, "You do not know of that %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
    return 1;
  }

/****  Is the player maxxed out with the skill?  ****/

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You are already learned in that area.\r\n", ch);
    return 1;
  }

/****  Does the Player have enough gold to train?  ****/

  if (GET_GOLD(ch) < (GM_COST(guild_nr,skill_num,ch))) {
    sprintf(buf1, gm_index[guild_nr].not_enough_gold, GET_NAME(ch));
    do_tell(keeper, buf1, cmd_tell, 0);
	return 1;
}

/****  If we've made it this far, then its time to practice  ****/

  send_to_char("You practice for a while...\r\n", ch);
  GET_PRACTICES(ch)--;

  GET_GOLD(ch) -= GM_COST(guild_nr,skill_num,ch);

  percent = GET_SKILL(ch, skill_num);
  percent += MIN(MAXGAIN(ch, skill_num), MAX(MINGAIN(ch, skill_num), 
    int_app[GET_INT(ch)].learn));

  SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);
 

  return 1;
}

/**** This function is here just because I'm extremely paranoid.  Take it
      out if you aren't ;)  ****/
void clear_skills(int index)
{
int i;

for  (i = 0; i < MAX_SKILLS + 2; i++) 
   gm_index[index].skills_and_spells[i] = 0;
}

/****  This is ripped off of read_line from shop.c.  They could be combined. But why? ****/

void read_gm_line(FILE * gm_f, char *string, void *data)
{
  if (!get_line(gm_f, buf) || !sscanf(buf, string, data)) {
    fprintf(stderr, "Error in guild #%d\n", GM_NUM(top_guild));
    exit(1);
  }
}

void boot_the_guilds(FILE * gm_f, char *filename, int rec_count)
{
  char *buf, buf2[150];
  int temp;
  int done = 0;

  sprintf(buf2, "beginning of GM file %s", filename);

  while (!done) {
    buf = fread_string(gm_f, buf2);
    if (*buf == '#') {		/* New Trainer */
      sscanf(buf, "#%d\n", &temp);
      sprintf(buf2, "GM #%d in GM file %s", temp, filename);
      free(buf);		/* Plug memory leak! */
      if (!top_guild)
	CREATE(gm_index, struct guild_master_data, rec_count);

      GM_NUM(top_guild) = temp;
        clear_skills(top_guild);    
      read_gm_line(gm_f, "%d", &temp);
           while( temp > -1) {
                gm_index[top_guild].skills_and_spells[(int)temp] = 1;
                read_gm_line(gm_f, "%d", &temp);
           }
      read_gm_line(gm_f, "%d", &GM_TYPE(top_guild));                   
      read_gm_line(gm_f, "%f", &GM_CHARGE(top_guild));

      gm_index[top_guild].no_such_skill = fread_string(gm_f, buf2);
      gm_index[top_guild].not_enough_gold = fread_string(gm_f, buf2);
      
      read_gm_line(gm_f, "%d", &GM_TRAINER(top_guild));

      GM_TRAINER(top_guild) = real_mobile(GM_TRAINER(top_guild));
      read_gm_line(gm_f, "%d", &GM_WITH_WHO(top_guild));

      read_gm_line(gm_f, "%d", &GM_OPEN(top_guild));
      read_gm_line(gm_f, "%d", &GM_CLOSE(top_guild));

      GM_FUNC(top_guild) = 0;
      top_guild++;
    } else {
      if (*buf == '$')		/* EOF */
	done = TRUE;
           free(buf);		/* Plug memory leak! */
    }
  }
}


void assign_the_gms(void)
{
  int index;

  for (index = 0; index < top_guild; index++) {
    if (mob_index[GM_TRAINER(index)].func)
      GM_FUNC(index) = mob_index[GM_TRAINER(index)].func;
    mob_index[GM_TRAINER(index)].func = guild;
  }
}

/* yet another function that was already written in shop.c that
   didn't need to be re-invented ;)  */

char *customer_string2(int gm_nr, int detailed)
{
  extern char *trade_letters[];

  int index, cnt = 1;
  static char buf[256];

  *buf = 0;
  for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
    if (!(GM_WITH_WHO(gm_nr) & cnt)) {
      if (detailed) {
        if (*buf)
          strcat(buf, ", ");
        strcat(buf, trade_letters[index]);
      } else
        sprintf(END_OF(buf), "%c", *trade_letters[index]);
    } else if (!detailed) {
      strcat(buf, "_");
  }
  return (buf);
}


void list_all_gms(struct char_data *ch)
{
int gm_nr;

strcpy(buf, "\r\n");
for (gm_nr = 0; gm_nr < top_guild; gm_nr++) {
  if (!(gm_nr % 19)) {
    strcat(buf, "Virtual   G.Master    Charge     Members\n\r");
    strcat(buf, "----------------------------------------\n\r");
  }
   sprintf(buf2, " %6d    ", GM_NUM(gm_nr));
 if (GM_TRAINER(gm_nr) < 0)
      strcpy(buf1, "<NONE>");
    else
      sprintf(buf1, "%6d   ", mob_index[GM_TRAINER(gm_nr)].vnum);
    sprintf(END_OF(buf2), "%s   %5.2f    ", buf1,
          GM_CHARGE(gm_nr));
    strcat(buf2, customer_string2(gm_nr, FALSE));
    sprintf(END_OF(buf), "%s\n\r", buf2);
  }

  page_string(ch->desc, buf, 1);
}

void list_detailed_gm(struct char_data * ch, int gm_nr)
{
  int i;

    if (GM_TRAINER(gm_nr) < 0)
      strcpy(buf1, "<NONE>");
    else
      sprintf(buf1, "%6d   ", mob_index[GM_TRAINER(gm_nr)].vnum);

   sprintf(buf, " Guild Master: %s\r\n", buf1);
   sprintf(buf, "%s Hours: %4d to %4d,  Surcharge: %5.2f\r\n", buf,
      GM_OPEN(gm_nr), GM_CLOSE(gm_nr), GM_CHARGE(gm_nr));
   sprintf(buf, "%s Whom will train: %s\r\n", buf,
                  customer_string2(gm_nr, TRUE));
   /* now for the REAL reason why someone would want to see a GM :) */

   sprintf(buf, "%s The GM can teach the following %ss:\r\n", buf, GM_SPLSKL(gm_nr));

   *buf2 = '\0';
   for (i = 0; i <= MAX_SKILLS; i++) {
   if (does_gm_know(gm_nr, i))
    sprintf(buf2, "%s %s \r\n", buf2, spells[i]);
   }
 
  strcat(buf, buf2);

  page_string(ch->desc, buf, 1);
}

  

void show_gm(struct char_data * ch, char *arg)
{
  int gm_nr, gm_num;

  if (!*arg)
    list_all_gms(ch);
  else {
    if (is_number(arg))
      gm_num = atoi(arg);
    else
      gm_num = -1;

   if (!(gm_num < 0)) {
    for (gm_nr = 0; gm_nr < top_guild; gm_nr++) {
      if (gm_num == GM_NUM(gm_nr))
        break; 
    }

    if ((gm_num < 0) || (gm_nr >= top_guild)) {
      send_to_char("Illegal guild master number.\n\r", ch);
      return;
    }
    list_detailed_gm(ch, gm_nr);
  }
}
}

int real_guild(int guild_vnum)
{ 
  int i;
  for (i = 0; i<= top_guild; i++) 
    if (gm_index[i].num == guild_vnum) return i;
  return -1;
}
