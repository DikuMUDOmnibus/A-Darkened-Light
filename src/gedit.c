/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  gedit.c:  Olc written for shoplike guildmasters, code by             *
 *             Jason Goodwin                                             *
 *    Made for Circle3.0 bpl11, its copyright applies                    *
 *                                                                       *
 *  Made for Oasis OLC                                                   *
 *  Copyright 1996 Harvey Gilpin.                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "guild.h"
#include "olc.h"
#include "shop.h"

/*-------------------------------------------------------------------*/
/* external variables */
extern struct guild_master_data *gm_index;	/*. guild.c      . */
extern int top_guild;		/*. guild.c      . */
extern struct char_data *mob_proto;	/*. db.c        . */
extern struct obj_data *obj_proto;	/*. db.c        . */
extern struct room_data *world;	/*. db.c        . */
extern struct zone_data *zone_table;	/*. db.c        . */
extern struct index_data *mob_index;	/*. db.c        . */
extern struct index_data *obj_index;	/*. db.c        . */
extern const char *trade_letters[];	/*. shop.h      . */
extern char *spells[];		/*. spell_parser.c */
/*-------------------------------------------------------------------*/
/*. Handy  macros . */

#define G_NUM(i)	 ((i)->num)
#define G_SK_AND_SP(i,j) ((i)->skills_and_spells[j])
#define G_CHARGE(i)	 ((i)->charge)
#define G_NO_SKILL(i)	 ((i)->no_such_skill)
#define G_NO_GOLD(i)	 ((i)->not_enough_gold)
#define G_TRAINER(i)	 ((i)->gm)
#define G_WITH_WHO(i)	 ((i)->with_who)
#define G_OPEN(i)	 ((i)->open)
#define G_CLOSE(i)	 ((i)->close)
#define G_FUNC(i)	 ((i)->func)
#define G_TYPE(i)	 ((i)->type)

/*-------------------------------------------------------------------*/
/*. Function prototypes . */

int real_gm(int vgm_num);
void gedit_setup_new(struct descriptor_data *d);
void gedit_setup_existing(struct descriptor_data *d, int rgm_num);
void gedit_parse(struct descriptor_data *d, char *arg);
void gedit_disp_menu(struct descriptor_data *d);
void gedit_no_train_menu(struct descriptor_data *d);
void gedit_save_internally(struct descriptor_data *d);
void gedit_save_to_disk(struct descriptor_data *d);
void copy_gm(struct guild_master_data *tgm, struct guild_master_data *fgm);
void free_gm_strings(struct guild_master_data *guild);
void free_gm(struct guild_master_data *guild);
void gedit_modify_string(char **str, char *new);

/*. External . */
SPECIAL(guild);

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

void gedit_setup_new(struct descriptor_data *d)
{
  int i;
  struct guild_master_data *guild;

  /*. Alloc some gm shaped space . */
  CREATE(guild, struct guild_master_data, 1);

  /*. Some default values . */
  G_TRAINER(guild) = -1;
  G_OPEN(guild) = 28;
  G_CLOSE(guild) = 28;
  G_CHARGE(guild) = 1.0;
  G_WITH_WHO(guild) = 0;
  G_FUNC(guild) = NULL;
  G_TYPE(guild) = 0;

  /*. Some default strings . */
  G_NO_SKILL(guild) = str_dup("%s Sorry, but I don't know that one.");
  G_NO_GOLD(guild) = str_dup("%s Sorry, but I'm gonna need more gold first.");

  /* init the wasteful skills and spells table */

  for (i = 0; i < MAX_SKILLS + 2; i++)
    G_SK_AND_SP(guild, i) = 0;

  OLC_GUILD(d) = guild;
  gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void gedit_setup_existing(struct descriptor_data *d, int rgm_num)
{
  /*. Alloc some gm shaped space . */
  CREATE(OLC_GUILD(d), struct guild_master_data, 1);
  copy_gm(OLC_GUILD(d), gm_index + rgm_num);
  gedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void copy_gm(struct guild_master_data *tgm, struct guild_master_data *fgm)
{
  int i;

  /*. Copy basic info over . */
  G_NUM(tgm) = G_NUM(fgm);
  G_CHARGE(tgm) = G_CHARGE(fgm);
  G_TRAINER(tgm) = G_TRAINER(fgm);
  G_WITH_WHO(tgm) = G_WITH_WHO(fgm);
  G_OPEN(tgm) = G_OPEN(fgm);
  G_CLOSE(tgm) = G_CLOSE(fgm);
  G_TYPE(tgm) = G_TYPE(fgm);

  /*. Copy the strings over . */
  free_gm_strings(tgm);
  G_NO_SKILL(tgm) = str_dup(G_NO_SKILL(fgm));
  G_NO_GOLD(tgm) = str_dup(G_NO_GOLD(fgm));

  /* copy the wasteful skills and spells table over */
  for (i = 0; i < MAX_SKILLS + 2; i++)
    G_SK_AND_SP(tgm, i) = G_SK_AND_SP(fgm, i);

}


/*-------------------------------------------------------------------*/
/*. Free all the character strings in a gm structure . */

void free_gm_strings(struct guild_master_data *guild)
{
  if (G_NO_SKILL(guild)) {
    free(G_NO_SKILL(guild));
    G_NO_SKILL(guild) = NULL;
  }
  if (G_NO_GOLD(guild)) {
    free(G_NO_GOLD(guild));
    G_NO_GOLD(guild) = NULL;
  }
}

/*-------------------------------------------------------------------*/
/*. Free up the whole guild structure and its contents . */

void free_gm(struct guild_master_data *guild)
{
  free_gm_strings(guild);
  free(guild);
}

/*-------------------------------------------------------------------*/

int real_gm(int vgm_num)
{
  int rgm_num;

  for (rgm_num = 0; rgm_num < top_guild; rgm_num++)
    if (GM_NUM(rgm_num) == vgm_num)
      return rgm_num;

  return -1;
}

/*-------------------------------------------------------------------*/
/*. Generic string modifyer for guild master messages . */

void gedit_modify_string(char **str, char *new)
{
  char *pointer;

  /*. Check the '%s' is present, if not, add it . */
  if (*new != '%') {
    strcpy(buf, "%s ");
    strcat(buf, new);
    pointer = buf;
  } else
    pointer = new;

  if (*str)
    free(*str);
  *str = str_dup(pointer);
}

/*-------------------------------------------------------------------*/

void gedit_save_internally(struct descriptor_data *d)
{
  int rgm, found = 0;
  struct guild_master_data *guild;
  struct guild_master_data *new_index;

  rgm = real_gm(OLC_NUM(d));
  guild = OLC_GUILD(d);
  G_NUM(guild) = OLC_NUM(d);

  if (rgm > -1) {		/*. The GM already exists, just update it. */
    copy_gm((gm_index + rgm), guild);
  } else {			/*. Doesn't exist - hafta insert it . */
    CREATE(new_index, struct guild_master_data, top_guild + 1);
    for (rgm = 0; rgm < top_guild; rgm++) {
      if (!found) {		/*. Is this the place ?. */
	if (GM_NUM(rgm) > OLC_NUM(d)) {		/*. Yep, stick it in here . */
	  found = 1;
	  copy_gm(&(new_index[rgm]), guild);
	  /*. Move the entry that used to go here up a place . */
	  new_index[rgm + 1] = gm_index[rgm];
	} else {		/*. This isn't the place, copy over info . */
	  new_index[rgm] = gm_index[rgm];
	}
      } else {			/*. GM's already inserted, copy rest over . */
	new_index[rgm + 1] = gm_index[rgm];
      }
    }
    if (!found)
      copy_gm(&(new_index[rgm]), guild);

    /*. Switch index in . */
    free(gm_index);
    gm_index = new_index;
    top_guild++;
  }
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_GM);
}


/*-------------------------------------------------------------------*/

void gedit_save_to_disk(struct descriptor_data *d)
{
  int i, j, rgm, zone, top;
  FILE *gm_file;
  char fname[64];
  struct guild_master_data *guild;

  log("Begin saving guildmasters");
  zone = zone_table[OLC_ZNUM(d)].number;
  top = zone_table[OLC_ZNUM(d)].top;

  sprintf(fname, "%s/%i.gld", GLD_PREFIX, zone);

  if (!(gm_file = fopen(fname, "w"))) {
    mudlog("SYSERR: OLC: Cannot open GM file!", BRF, LVL_BUILDER, TRUE);
    return;
  }
  /*. Search database for gms in this zone . */
  for (i = zone * 100; i <= top; i++) {
    rgm = real_gm(i);
    if (rgm != -1) {
      fprintf(gm_file, "#%d~\n", i);
      guild = gm_index + rgm;

      /* Write which skills and spells the gm knows */
      for (j = 0; j < MAX_SKILLS + 2; j++)
	if (G_SK_AND_SP(guild, j))
	  fprintf(gm_file, "%d\n", j);

      fprintf(gm_file, "-1\n");

      /* Write what the GM teaches */
      fprintf(gm_file, "%d\n", G_TYPE(guild));

      /*. Save charge . */
      fprintf(gm_file, "%4.1f\n", G_CHARGE(guild));

      /*. Save messages . */
      fprintf(gm_file,
	      "%s~\n%s~\n",
      /*. Added some small'n'silly defaults as sanity checks . */
	      (G_NO_SKILL(guild) ? G_NO_SKILL(guild) : "%s ERROR"),
	      (G_NO_GOLD(guild) ? G_NO_GOLD(guild) : "%s ERROR")
	  );


      /*. Save the rest . */
      fprintf(gm_file, "%d\n%d\n%d\n%d\n",
	      mob_index[G_TRAINER(guild)].vnum,
	      G_WITH_WHO(guild),
	      G_OPEN(guild),
	      G_CLOSE(guild));
    }
  }
  fprintf(gm_file, "$~\n");
  fclose(gm_file);
  olc_remove_from_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_GM);
}


/**************************************************************************
 Menu functions 
 **************************************************************************/

/*-------------------------------------------------------------------*/

char *cut_string(char *s, int max_width)
{
   static char buf[128];
   strcpy(buf, s);
   buf[max_width] = '\0';
   return buf;
}

void gedit_select_skills_menu(struct descriptor_data *d)
{
  int i, j = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);

  sprintf(buf, "Skills known:\r\n");

  for (i = MAX_SPELLS + 1; i <= MAX_SPELLS + NUM_SKILLS; i++) {
    sprintf(buf, "%s[%-7s] &C%2d&w %-15s", buf,
	    YESNO(G_SK_AND_SP(OLC_GUILD(d), i)),
	    i, cut_string(spells[i],15));
    if (!(++j % 3))
      strcat(buf, "\r\n");
    else
      strcat(buf, "  ");
  }
  send_to_char(buf, d->character);
  send_to_char("\r\nEnter skill num (0 = exit):  ", d->character);
}

/*-------------------------------------------------------------------*/

void gedit_select_spells_menu(struct descriptor_data *d)
{
  int i, j = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  sprintf(buf, "Spells known:\r\n");

  for (i = 1; i <= NUM_SPELLS; i++) {
    sprintf(buf, "%s[%-7s] &C%2d&w %-15s", buf,
	    YESNO(G_SK_AND_SP(OLC_GUILD(d), i)),
	    i,cut_string(spells[i], 15));
    if (!(++j % 3))
      strcat(buf, "\r\n");
    else
      strcat(buf, "  ");
  }
  send_to_char(buf, d->character);
  send_to_char("\r\nEnter spell num (0 = exit):  ", d->character);
}

/*-------------------------------------------------------------------*/

void gedit_no_train_menu(struct descriptor_data *d)
{
  int i, count = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_TRADERS; i++) {
    sprintf(buf,
	    "%s%2d%s) %-20.20s   ",
	    grn, i + 1, nrm, trade_letters[i]
	);
    if (!(++count % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintbit(G_WITH_WHO(OLC_GUILD(d)), trade_letters, buf1);
  sprintf(buf,
	  "\r\nCurrently won't train: %s%s%s\r\n"
	  "Enter choice : ",
	  cyn, buf1, nrm
      );
  send_to_char(buf, d->character);
  OLC_MODE(d) = GEDIT_NO_TRAIN;
}

/*-------------------------------------------------------------------*/
/*. Display main menu . */

void gedit_disp_menu(struct descriptor_data *d)
{
  struct guild_master_data *guild;

  guild = OLC_GUILD(d);
  get_char_cols(d->character);

  sprintbit(G_WITH_WHO(guild), trade_letters, buf1);

  sprintf(buf, "[H[J"
	  "-- Guild Number: [%s%d%s]\r\n"
	  "%s0%s) Guild Master	: [%s%d%s] %s%s\r\n"
	  "%s1%s) Doesn't know skill:\r\n %s%s\r\n"
	  "%s2%s) Player no gold:\r\n %s%s\r\n"
	  "%s3%s) Open:  [%s%d%s]	%s4%s) Close:  [%s%d%s]	%s5%s) Charge:  [%s%3.1f]\r\n"
	  "%s6%s) Don't Train:  %s%s\r\n"
	  "%sT%s) Teaches: %s%s%s\r\n"
	  "%s7%s) Spells Menu\r\n"
	  "%s8%s) Skills Menu\r\n"
	  "%sQ%s) Quit%s\r\n"
	  "Enter Choice : ",


	  cyn, OLC_NUM(d), nrm,
	  grn, nrm, cyn,
	  (G_TRAINER(guild) == -1) ?
	  -1 : mob_index[G_TRAINER(guild)].vnum, nrm,
	  yel, (G_TRAINER(guild) == -1) ?
	  "none" : mob_proto[G_TRAINER(guild)].player.short_descr,
	  grn, nrm, yel, G_NO_SKILL(guild),
	  grn, nrm, yel, G_NO_GOLD(guild),
	  grn, nrm, cyn, G_OPEN(guild), nrm,
	  grn, nrm, cyn, G_CLOSE(guild), nrm,
	  grn, nrm, cyn, G_CHARGE(guild),
	  grn, nrm, cyn, buf1,
          grn, nrm, cyn, (G_TYPE(guild) ? "spells" : "skills"), nrm,
	  grn, nrm, grn, nrm, grn, nrm,
	  OLC_READONLY(d) ? " &R(Read-Only)&w" : ""
  );

  send_to_char(buf, d->character);

  OLC_MODE(d) = GEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
 **************************************************************************/

void gedit_parse(struct descriptor_data *d, char *arg)
{
  int i;

  if (OLC_MODE(d) > GEDIT_NUMERICAL_RESPONSE) {
    if (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))) {
      send_to_char("Field must be numerical, try again : ", d->character);
      return;
    }
  }
  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case GEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      send_to_char("Saving GM to memory.\r\n", d->character);
      gedit_save_internally(d);
      sprintf(buf, "OLC: %s edits GM %d", GET_NAME(d->character),
	      OLC_NUM(d));
      mudlog(buf, CMP, LVL_BUILDER, TRUE);
      cleanup_olc(d, CLEANUP_STRUCTS);
      return;
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save the GM? : ", d->character);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case GEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) {
    case 'q':
    case 'Q':
      if (!OLC_READONLY(d) && OLC_VAL(d)) {		/*. Anything been changed? . */
	send_to_char("Do you wish to save the changes to the GM? (y/n) : ", d->character);
	OLC_MODE(d) = GEDIT_CONFIRM_SAVESTRING;
      } else
	cleanup_olc(d, CLEANUP_ALL);
      return;
    case '0':
      OLC_MODE(d) = GEDIT_TRAINER;
      send_to_char("Enter virtual number of guild master : ", d->character);
      return;
    case '1':
      OLC_MODE(d) = GEDIT_NO_SKILL;
      i--;
      break;
    case '2':
      OLC_MODE(d) = GEDIT_NO_CASH;
      i--;
      break;
    case '3':
      OLC_MODE(d) = GEDIT_OPEN;
      i++;
      break;
    case '4':
      OLC_MODE(d) = GEDIT_CLOSE;
      i++;
      break;
    case '5':
      OLC_MODE(d) = GEDIT_CHARGE;
      i++;
      break;
    case '6':
      OLC_MODE(d) = GEDIT_NO_TRAIN;
      gedit_no_train_menu(d);
      return;
    case '7':
      OLC_MODE(d) = GEDIT_SELECT_SPELLS;
      gedit_select_spells_menu(d);
      return;
    case '8':
      OLC_MODE(d) = GEDIT_SELECT_SKILLS;
      gedit_select_skills_menu(d);
      return;
    case 't':
      G_TYPE(OLC_GUILD(d)) = !(G_TYPE(OLC_GUILD(d)));
      gedit_disp_menu(d);
      return;
    default:
      gedit_disp_menu(d);
      return;
    }

    if (i == 1) {
      send_to_char("\r\nEnter new value : ", d->character);
      return;
    }
    if (i == -1) {
      send_to_char("\r\nEnter new text :\r\n| ", d->character);
      return;
    }
    break;
/*-------------------------------------------------------------------*/
    /*. String edits . */
  case GEDIT_NO_SKILL:
    gedit_modify_string(&G_NO_SKILL(OLC_GUILD(d)), arg);
    break;
  case GEDIT_NO_CASH:
    gedit_modify_string(&G_NO_GOLD(OLC_GUILD(d)), arg);
    break;

/*-------------------------------------------------------------------*/
    /*. Numerical responses . */

  case GEDIT_TRAINER:
    i = atoi(arg);
    if (i != -1) {
      i = real_mobile(i);
      if (i < 0) {
	send_to_char("That mobile does not exist, try again : ", d->character);
	return;
      }
    }
    G_TRAINER(OLC_GUILD(d)) = i;
    if (i == -1)
      break;
    /*. Fiddle with special procs . */
    G_FUNC(OLC_GUILD(d)) = mob_index[i].func;
    mob_index[i].func = guild;
    break;
  case GEDIT_OPEN:
    G_OPEN(OLC_GUILD(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case GEDIT_CLOSE:
    G_CLOSE(OLC_GUILD(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case GEDIT_CHARGE:
    sscanf(arg, "%f", &G_CHARGE(OLC_GUILD(d)));
    break;
  case GEDIT_NO_TRAIN:
    i = MAX(0, MIN(NUM_TRADERS, atoi(arg)));
    if (i > 0) {		/*. Toggle bit . */
      i = 1 << (i - 1);
      if (IS_SET(G_WITH_WHO(OLC_GUILD(d)), i))
	REMOVE_BIT(G_WITH_WHO(OLC_GUILD(d)), i);
      else
	SET_BIT(G_WITH_WHO(OLC_GUILD(d)), i);
      gedit_no_train_menu(d);
      return;
    }
    break;

  case GEDIT_SELECT_SPELLS:
    i = atoi(arg);
    if (i == 0)
      break;
//    i = MAX(1, MIN(i, NUM_SPELLS));
    if (i > 1 && i <= NUM_SPELLS)
      G_SK_AND_SP(OLC_GUILD(d), i) = !G_SK_AND_SP(OLC_GUILD(d), i);
    gedit_select_spells_menu(d);
    return;

  case GEDIT_SELECT_SKILLS:
    i = atoi(arg);
    if (i == 0)
      break;
//  i = MAX(MAX_SPELLS + 1, MIN(i, NUM_SKILLS));
    if ((i >= (MAX_SPELLS + 1)) && (i < (MAX_SPELLS +1 +NUM_SKILLS)))
      G_SK_AND_SP(OLC_GUILD(d), i) = !G_SK_AND_SP(OLC_GUILD(d), i);
    gedit_select_skills_menu(d);
    return;

/*-------------------------------------------------------------------*/
  default:
    /*. We should never get here . */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: gedit_parse(): Reached default case!", BRF, LVL_BUILDER, TRUE);
    break;
  }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
   If we get here, we have probably changed something, and now want to
   return to main menu.  Use OLC_VAL as a 'has changed' flag . */

  OLC_VAL(d) = 1;
  gedit_disp_menu(d);
}

