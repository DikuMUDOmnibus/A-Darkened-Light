/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - medit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "olc.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"

/*-------------------------------------------------------------------*/
/* external variables */
extern struct index_data *mob_index;			/*. db.c    	.*/
extern struct char_data *mob_proto;			/*. db.c    	.*/
extern struct char_data *character_list;		/*. db.c    	.*/
extern int top_of_mobt;					/*. db.c    	.*/
extern struct zone_data *zone_table;			/*. db.c    	.*/
extern int top_of_zone_table;				/*. db.c    	.*/
extern struct player_special_data dummy_mob;		/*. db.c    	.*/	
extern struct attack_hit_type attack_hit_text[]; 	/*. fight.c 	.*/
extern int top_shop;					/*. shop.c	.*/
extern struct shop_data *shop_index;			/*. shop.c	.*/
extern struct descriptor_data *descriptor_list;		/*. comm.c	.*/
extern struct specproc_info mob_procs[];

/*-------------------------------------------------------------------*/
/*. Handy  macros .*/

#define GET_NDD(mob) ((mob)->mob_specials.damnodice)
#define GET_SDD(mob) ((mob)->mob_specials.damsizedice)
#define GET_ALIAS(mob) ((mob)->player.name)
#define GET_SDESC(mob) ((mob)->player.short_descr)
#define GET_LDESC(mob) ((mob)->player.long_descr)
#define GET_DDESC(mob) ((mob)->player.description)
#define GET_ATTACK(mob) ((mob)->mob_specials.attack_type)
#define S_KEEPER(shop) ((shop)->keeper)

/* Function externs */
char *mprog_type_to_name(int type);
char *mprog_get_argument_str(int type);
void olc_disp_spec_proc_menu(struct descriptor_data * d,
  struct specproc_info table[]);
ACMD(do_help);
/*-------------------------------------------------------------------*/
/*. Function prototypes .*/

void medit_parse(struct descriptor_data * d, char *arg);
void medit_disp_menu(struct descriptor_data * d);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(struct descriptor_data *d);
void init_mobile(struct char_data *mob);
void copy_mobile(struct char_data *tmob, struct char_data *fmob);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);


/*-------------------------------------------------------------------*\
  permission checks 
\*-------------------------------------------------------------------*/

/* Here define flags which lower level builders (and clan builders) cannot */
/* change */
long_bitv npc_flags_mask =
{
  MOB_SPEC+MOB_ISNPC+MOB_STAY_ZONE, 
  MOB2_DELETED+MOB2_GAZEPETRIFY, 
  0, 
  0
};

long_bitv aff_flags_mask =
{
  AFF_BLIND + AFF_GROUP + AFF_POISON + AFF_SLEEP + AFF_UNUSED17 + AFF_UNUSED20 
  + AFF_CHARM + AFF_UNUSED22,
  AFF2_PLUSONE + AFF2_PLUSTWO + AFF2_PLUSTHREE + AFF2_PLUSFOUR + AFF2_PLUSFIVE
  + AFF2_SILVER + AFF2_FARSEE + AFF2_CRIT_HIT + AFF2_BURNING + AFF2_FREEZING
  + AFF2_ACIDED + AFF2_UNUSED17 + AFF2_BLOCK,
  0, 
  0
};

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

void medit_free_progs(MPROG_DATA *a)
{
  MPROG_DATA *b;
  
//  log("Freeing mobprog duplicates.");
  while (a) {
    b = a->next;
    free(a->arglist);
    free(a->comlist);
    free(a->filename);
    free(a);
    a = b;
  }
}

int get_prog_types(MPROG_DATA *first)
{
  MPROG_DATA *t;
  int i = 0;
  t = first;
  while (t) {
    i |= t->type;
    t = t->next;
  }
  return i;
}

/* * * * *
 * Free a mobile structure that has been edited.
 * Take care of existing mobiles and their mob_proto!
 * * * * */
 
void medit_free_mobile(struct char_data * mob)
{
  int i;
  
  if (GET_MOB_RNUM(mob) == -1)	/* Non prototyped mobile */
  {
    if (mob->player.name)
      free(mob->player.name);
    if (mob->player.title)
      free(mob->player.title);
    if (mob->player.short_descr)
      free(mob->player.short_descr);
    if (mob->player.long_descr)
      free(mob->player.long_descr);
    if (mob->player.description)
      free(mob->player.description);
  }
  else if ((i = GET_MOB_RNUM(mob)) > -1) /* Prototyped mobile */
  {
    if (mob->player.name && mob->player.name != mob_proto[i].player.name)
      free(mob->player.name);
    if (mob->player.title && mob->player.title != mob_proto[i].player.title)
      free(mob->player.title);
    if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr)
      free(mob->player.short_descr);
    if (mob->player.long_descr && mob->player.long_descr != mob_proto[i].player.long_descr)
      free(mob->player.long_descr);
    if (mob->player.description && mob->player.description != mob_proto[i].player.description)
      free(mob->player.description);
  }

  while (mob->affected)
    affect_remove(mob, mob->affected);

  free(mob);
}

void medit_setup_new(struct descriptor_data *d)
{ struct char_data *mob;

  /*. Alloc some mob shaped space .*/
  CREATE(mob, struct char_data, 1);
  init_mobile(mob);
  
  GET_MOB_RNUM(mob) = -1;
  /*. default strings .*/
  GET_ALIAS(mob) = str_dup("mob unfinished");
  GET_SDESC(mob) = str_dup("the unfinished mob");
  GET_LDESC(mob) = str_dup("An unfinished mob stands here.\r\n");
  GET_DDESC(mob) = str_dup("It looks, err, unfinished.\r\n");

  OLC_MOB(d) = mob;
  OLC_VAL(d) = 0;   /*. Has changed flag .*/
  
  OLC_FPROG(d) = NULL;
  OLC_PROG(d) = NULL;
  
  OLC_SPEC(d) = 0;
  
  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/
MPROG_DATA *dup_mobprograms(MPROG_DATA *first)
{
  MPROG_DATA *a;
  MPROG_DATA *b;
  MPROG_DATA *c;
  MPROG_DATA *d = NULL;
    
//  log("Duplicating mobrogram structures.");
  for (a = first; (a); a = a->next) {
    if (a && a->arglist && (a->comlist || a->type == IN_FILE_PROG) && a->type >= 0) {
      CREATE(b, MPROG_DATA, 1);
      if (d) {
        c->next = b;
      } else {
        d = b;
      }
      b->type = a->type;
      if (a->arglist)
        b->arglist = strdup(a->arglist);
      if (a->comlist)
        b->comlist = strdup(a->comlist);
      if (b->filename)
        b->filename = strdup(a->filename);
      b->next = NULL;
//      log("... Record created.");
    } else { /*log("Dumb record skipped."); */ }
      c = b;
    
  }
//  log("Duplicating complete.");
  return d;
  
}
/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num)
{ struct char_data *mob;

  /*. Alloc some mob shaped space .*/
  CREATE(mob, struct char_data, 1);
  copy_mobile(mob, mob_proto + rmob_num);
//  OLC_FPROG(d) = dup_mobprograms(rmob_num);
  OLC_MOB(d) = mob;
  OLC_SPEC(d) = get_spec_name(mob_procs, mob_index[rmob_num].func);
  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/
/*. Copy one mob struct to another .*/

void copy_mobile(struct char_data *tmob, struct char_data *fmob)
{
  /*. Free up any used strings .*/
  if (GET_ALIAS(tmob))
    free(GET_ALIAS(tmob));
  if (GET_SDESC(tmob))
    free(GET_SDESC(tmob));
  if (GET_LDESC(tmob))
    free(GET_LDESC(tmob));
  if (GET_DDESC(tmob))
    free(GET_DDESC(tmob));
  
  /*.Copy mob .*/
  *tmob = *fmob;
 
  /*. Realloc strings .*/
  if (GET_ALIAS(fmob))
    GET_ALIAS(tmob) = str_dup(GET_ALIAS(fmob));

  if (GET_SDESC(fmob))
    GET_SDESC(tmob) = str_dup(GET_SDESC(fmob));

  if (GET_LDESC(fmob))
    GET_LDESC(tmob) = str_dup(GET_LDESC(fmob));

  if (GET_DDESC(fmob))
    GET_DDESC(tmob) = str_dup(GET_DDESC(fmob));
}


/*-------------------------------------------------------------------*/
/*. Ideally, this function should be in db.c, but I'll put it here for
    portability.*/

void init_mobile(struct char_data *mob)
{
  clear_char(mob);

  GET_HIT(mob) = 1;
  GET_MANA(mob) = 1;
  GET_MAX_MANA(mob) = 100;
  GET_MAX_MOVE(mob) = 100;
  GET_NDD(mob) = 1;
  GET_SDD(mob) = 1;
  GET_WEIGHT(mob) = 200;
  GET_HEIGHT(mob) = 198;

  mob->real_abils.str = 11;
  mob->real_abils.intel = 11;
  mob->real_abils.wis = 11;
  mob->real_abils.dex = 11;
  mob->real_abils.con = 11;
  mob->real_abils.cha = 11;
  mob->aff_abils = mob->real_abils;

  SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
  mob->player_specials = &dummy_mob;
}

/*-------------------------------------------------------------------*/
/*. Save new/edited mob to memory .*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void medit_save_internally(struct descriptor_data *d)
{ int rmob_num, found = 0, new_mob_num = 0, zone, cmd_no, shop;
  struct char_data *new_proto;
  struct index_data *new_index;
  struct char_data *live_mob;
  struct descriptor_data *dsc;

  rmob_num = real_mobile(OLC_NUM(d));

  /*. Mob exists? Just update it .*/
  if (rmob_num != -1)
  { copy_mobile((mob_proto + rmob_num), OLC_MOB(d));
  
    /*. Free old mob-programs and duplicate new .*/
    medit_free_progs(mob_index[rmob_num].mobprogs);
    mob_index[rmob_num].mobprogs = dup_mobprograms(OLC_FPROG(d));
    mob_index[rmob_num].progtypes = get_prog_types(mob_index[rmob_num].mobprogs);
    
    /*. Update spec procs .*/
    mob_index[rmob_num].func = mob_procs[OLC_SPEC(d)].sp_pointer;
    
    /*. Update live mobiles .*/
    for(live_mob = character_list; live_mob; live_mob = live_mob->next)
      if(IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num)
      { /*. Only really need update the strings, since these can cause
            protection faults.  The rest can wait till a reset/reboot .*/
        GET_ALIAS(live_mob) = GET_ALIAS(mob_proto + rmob_num);
        GET_SDESC(live_mob) = GET_SDESC(mob_proto + rmob_num);
        GET_LDESC(live_mob) = GET_LDESC(mob_proto + rmob_num);
        GET_DDESC(live_mob) = GET_DDESC(mob_proto + rmob_num);
      }
  } 
  /*. Mob does not exist, hafta add it .*/
  else
  { CREATE(new_proto, struct char_data, top_of_mobt + 2);
    CREATE(new_index, struct index_data, top_of_mobt + 2);

    for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++)
    { if (!found)
      { /*. Is this the place?  .*/
        if ((rmob_num > top_of_mobt) ||
            (mob_index[rmob_num].vnum > OLC_NUM(d)))
        { /*. Yep, stick it here .*/
          found = 1;
          new_index[rmob_num].vnum = OLC_NUM(d);
          new_index[rmob_num].number = 0;
          new_index[rmob_num].func = NULL;
          new_mob_num = rmob_num;
          GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
          copy_mobile((new_proto + rmob_num), OLC_MOB(d));
          /*. Copy the mob that should be here on top .*/
          new_index[rmob_num + 1] = mob_index[rmob_num];
          new_proto[rmob_num + 1] = mob_proto[rmob_num];
          GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
        } else
        { /*. Nope, copy over as normal.*/
          new_index[rmob_num] = mob_index[rmob_num];
          new_proto[rmob_num] = mob_proto[rmob_num];
        }
      } else
      { /*. We've already found it, copy the rest over .*/
        new_index[rmob_num + 1] = mob_index[rmob_num];
        new_proto[rmob_num + 1] = mob_proto[rmob_num];
        GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
      }
    }
    if (!found)
    { /*. Still not found, must add it to the top of the table .*/
      new_index[rmob_num].vnum = OLC_NUM(d);
      new_index[rmob_num].number = 0;
      new_index[rmob_num].func = NULL;
      new_mob_num = rmob_num;
      GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
      copy_mobile((new_proto + rmob_num), OLC_MOB(d));
    }

    /*. Replace tables .*/
    free(mob_index);
    free(mob_proto);
    mob_index = new_index;
    mob_proto = new_proto;
    top_of_mobt++;

    

    /*. Update live mobile rnums .*/
    for(live_mob = character_list; live_mob; live_mob = live_mob->next)
      if(GET_MOB_RNUM(live_mob) > new_mob_num)
        GET_MOB_RNUM(live_mob)++;
    
    /*. Update zone table .*/
    for (zone = 0; zone <= top_of_zone_table; zone++)
      if (!zone_table[zone].cmd)
        log("SYSERR: Error ");
      else 
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) 
        if (ZCMD.command == 'M')
          if (ZCMD.arg1 > new_mob_num)
            ZCMD.arg1++;

    /*. Update mob programs .*/
    mob_index[new_mob_num].mobprogs = dup_mobprograms(OLC_FPROG(d));
    mob_index[new_mob_num].progtypes = get_prog_types(mob_index[new_mob_num].mobprogs);

    /*. Update spec procs .*/
    mob_index[new_mob_num].func = mob_procs[OLC_SPEC(d)].sp_pointer;

    /*. Update shop keepers .*/
    for(shop = 0; shop <= top_shop; shop++)
      if(SHOP_KEEPER(shop) > new_mob_num)
        SHOP_KEEPER(shop)++;

    /*. Update keepers in shops being edited .*/
    for(dsc = descriptor_list; dsc; dsc = dsc->next)
      if(dsc->connected == CON_SEDIT)
        if(S_KEEPER(OLC_SHOP(dsc)) >= new_mob_num)
          S_KEEPER(OLC_SHOP(dsc))++;
  }
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_MOB);
}


/*-------------------------------------------------------------------*/
/*. Save ALL mobiles for a zone to their .mob file, mobs are all 
    saved in Extended format, regardless of whether they have any
    extended fields.  Thanks to Samedi for ideas on this bit of code.*/

void str_remove_cr(char *src, char *dest)
{
  char *ptr1, *ptr2;
  ptr1 = src;
  ptr2 = dest;
  while (*ptr1 != '\0') {
    if (*ptr1 != '\r') { 
      *ptr2 = *ptr1; 
      ptr2++;
    }  
    ptr1++;
  }
  *ptr2 = '\0';
}

void save_mprog(MPROG_DATA *data, FILE *f) {
  fprintf(f, ">%s %s~\n", mprog_type_to_name(data->type),
    data->arglist);
  str_remove_cr(data->comlist, buf);
  if (data->comlist) fprintf(f, "%s~", buf); // removed CR
  fprintf(f,"\n");
}

void save_mob_progs_to_disk(int real_num, FILE *f) {
  MPROG_DATA *m;
  m = mob_index[real_num].mobprogs;
  if (!m) return;
  while (m) {
    if (m->filename) {
      log("Saving INFILE mprog.");
      log("SYSERR: INFILE mobrogs not implemented.");
    } else {
      log("Saving INLINE mprog.");
      save_mprog(m, f);
      m = m->next;
    }
  }
  fprintf(f, "|\n");
}

void medit_save_to_disk(struct descriptor_data *d)
{ 
  int i, rmob_num, zone, top;
  FILE *mob_file;
  char fname[64];
  struct char_data *mob;

  zone = zone_table[OLC_ZNUM(d)].number; 
  top = zone_table[OLC_ZNUM(d)].top; 

  sprintf(fname, "%s/%i.mob", MOB_PREFIX, zone);

  if(!(mob_file = fopen(fname, "w")))
  { mudlog("SYSERR: OLC: Cannot open mob file!", BRF, LVL_BUILDER, TRUE);
    return;
  }

  /*. Seach database for mobs in this zone and save em .*/
  for(i = zone * 100; i <= top; i++)
  { rmob_num = real_mobile(i);
    
    if(rmob_num != -1 && !MOB2_FLAGGED(&mob_proto[rmob_num], MOB2_DELETED)) 
    { if(fprintf(mob_file, "#%d\n", i) < 0)
      { mudlog("SYSERR: OLC: Cannot write mob file!\r\n", BRF, LVL_BUILDER, TRUE);
        fclose(mob_file);
        return;
      }
      mob = (mob_proto + rmob_num);

      /*. Clean up strings .*/
      strcpy (buf1, GET_LDESC(mob) ? GET_LDESC(mob) : "undefined");
      strip_string(buf1);
      strcpy(buf2, GET_DDESC(mob) ? GET_DDESC(mob) : "undefined");
      strip_string(buf2);

      fprintf(mob_file, 
	"%s~\n"
	"%s~\n"
	"%s~\n"
	"%s~\n"
	"%ld %ld %ld %ld %ld %ld %ld %ld %i E\n" 
	"%d %d %i %dd%d+%d %dd%d+%d\n" 
	"%ld %ld\n"  /*. Gold & Exp are longs in my mud, ignore any warning .*/
	"%d %d %d\n",
	GET_ALIAS(mob) ? GET_ALIAS(mob) : "undefined",
	GET_SDESC(mob) ? GET_SDESC(mob) : "undefined",
	buf1,
	buf2,
	MOB_FLAGS(mob), MOB2_FLAGS(mob), MOB3_FLAGS(mob), MOB4_FLAGS(mob),
	AFF_FLAGS(mob), AFF2_FLAGS(mob), AFF3_FLAGS(mob), AFF4_FLAGS(mob), 
  	GET_ALIGNMENT(mob), 
  	GET_LEVEL(mob),
  	20 - GET_HITROLL(mob), /*. Convert hitroll to thac0 .*/
        GET_AC(mob) / 10,
	GET_HIT(mob),
	GET_MANA(mob),
	GET_MOVE(mob),
	GET_NDD(mob),
	GET_SDD(mob),
	GET_DAMROLL(mob),
	(long)GET_GOLD(mob),
	(long)GET_EXP(mob),
	GET_POS(mob),
	GET_DEFAULT_POS(mob),
	GET_SEX(mob)
      );

      /*. Deal with Extra stats in case they are there .*/
      if(GET_ATTACK(mob) != 0)
	fprintf(mob_file, "BareHandAttack: %d\n", GET_ATTACK(mob));
      if(GET_STR(mob) != 11)
        fprintf(mob_file, "Str: %d\n", GET_STR(mob));
      if(GET_ADD(mob) != 0)
        fprintf(mob_file, "StrAdd: %d\n", GET_ADD(mob));
      if(GET_DEX(mob) != 11)
        fprintf(mob_file, "Dex: %d\n", GET_DEX(mob));
      if(GET_INT(mob) != 11)
        fprintf(mob_file, "Int: %d\n", GET_INT(mob));
      if(GET_WIS(mob) != 11)
        fprintf(mob_file, "Wis: %d\n", GET_WIS(mob));
      if(GET_CON(mob) != 11)
        fprintf(mob_file, "Con: %d\n", GET_CON(mob));
      if(GET_CHA(mob) != 11)
        fprintf(mob_file, "Cha: %d\n", GET_CHA(mob));
      if(GET_MAX_MOVE(mob) != 100)
        fprintf(mob_file, "MaxMove: %d\n", GET_MAX_MOVE(mob));
      if(GET_MOB_SPEC_INDEX(mob) != 0)
        fprintf(mob_file, "SpecProc: %s\n", 
          mob_procs[GET_MOB_SPEC_INDEX(mob)].name);

      /*. Add E-mob handlers here .*/

      fprintf(mob_file, "E\n");
      
      /*. Now save the mobprograms .*/
      
      save_mob_progs_to_disk(GET_MOB_RNUM(mob),mob_file);
    }
  }
  fprintf(mob_file, "$\n");
  fclose(mob_file);
  olc_remove_from_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_MOB);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/
/*. Display poistions (sitting, standing etc) .*/

void medit_disp_positions(struct descriptor_data *d)
{ int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; *position_types[i] != '\n'; i++)
  {  sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
     send_to_char(buf, d->character);
  }
  send_to_char("Enter position number : ", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display sex (Oooh-err).*/

void medit_disp_sex(struct descriptor_data *d)
{ int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_GENDERS; i++)
  {  sprintf(buf, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
     send_to_char(buf, d->character);
  }
  send_to_char("Enter gender number : ", d->character);
}

/*-------------------------------------------------------------------*/
/*. Display attack types menu .*/

void medit_disp_attack_types(struct descriptor_data *d)
{ int i;
  
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_ATTACK_TYPES; i++)
  {  sprintf(buf, "%s%2d%s) %s\r\n", 
	grn, i, nrm, attack_hit_text[i].singular
     );
     send_to_char(buf, d->character);
  }
  send_to_char("Enter attack type : ", d->character);
}
 

/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/

void medit_disp_mob_flags(struct descriptor_data *d)
{ 
  
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  menubit(MOB_FLAGS_BITV(OLC_MOB(d)), action_bits, buf1, 
    CHECK_MASK_PERM(d, npc_flags_mask));
  send_to_char(buf1, d->character);
  sprintbit_multi(MOB_FLAGS_BITV(OLC_MOB(d)), action_bits, buf1);
  sprintf(buf, "\r\n"
	"Current flags : %s%s%s\r\n"
	"Enter mob flags (0 to quit, ? for help) : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void medit_disp_aff_flags(struct descriptor_data *d)
{ 
  
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  menubit(AFF_FLAGS_BITV(OLC_MOB(d)), affected_bits, buf1, 
    CHECK_MASK_PERM(d, aff_flags_mask));
  send_to_char(buf1, d->character);
  sprintbit_multi(AFF_FLAGS_BITV(OLC_MOB(d)), affected_bits, buf1);
  sprintf(buf, "\r\n"
	"Current flags   : %s%s%s\r\n"
	"Enter aff flags (0 to quit, ? to help) : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}
  
/*-------------------------------------------------------------------*/
/*. Display main menu .*/

void medit_disp_menu(struct descriptor_data * d)
{ struct char_data *mob;
  char local_buf[80];

  mob = OLC_MOB(d);
  get_char_cols(d->character);

  if (OLC_CLAN_EDIT(d)) {
    sprintf(local_buf, " &Y(%ld gold/mudmonth)&w", mob_account(OLC_MOB(d)));
    mob_exp(OLC_MOB(d));
  }

  sprintf(buf, "[H[J"
	"-- Mob Number:  [%s%d%s]\r\n"
	"%s1%s) Sex: %s%-7.7s%s	         %s2%s) Alias: %s%s\r\n"
        "%s3%s) S-Desc: %s%s\r\n"
	"%s4%s) L-Desc:-\r\n%s%s"
	"%s5%s) D-Desc:-\r\n%s%s"
	"%s6%s) Level:       [%s%4d%s],  %s7%s) Alignment:    [%s%4d%s]\r\n"
        "%s8%s) Hitroll:     [%s%4d%s],  %s9%s) Damroll:      [%s%4d%s]\r\n"
        "%sA%s) NumDamDice:  [%s%4d%s],  %sB%s) SizeDamDice:  [%s%4d%s]\r\n"
	"%sC%s) Num HP Dice: [%s%4d%s],  %sD%s) Size HP Dice: [%s%4d%s],  %sE%s) HP Bonus: [%s%5d%s]\r\n"
	"%sF%s) Armor Class: [%s%4d%s],  %sG%s) Exp:     [%s%9ld%s],  %sH%s) Gold:  [%s%8ld%s]\r\n",

	cyn, OLC_NUM(d), nrm,
	grn, nrm, yel, genders[(int)GET_SEX(mob)], nrm,
	grn, nrm, yel, GET_ALIAS(mob),
	grn, nrm, yel, GET_SDESC(mob),
	grn, nrm, yel, GET_LDESC(mob),
	grn, nrm, yel, GET_DDESC(mob),
	grn, nrm, cyn, GET_LEVEL(mob), nrm,
	grn, nrm, cyn, GET_ALIGNMENT(mob), nrm,
	grn, nrm, cyn, GET_HITROLL(mob), nrm,
	grn, nrm, cyn, GET_DAMROLL(mob), nrm,
	grn, nrm, cyn, GET_NDD(mob), nrm,
	grn, nrm, cyn, GET_SDD(mob), nrm,
	grn, nrm, cyn, GET_HIT(mob), nrm,
	grn, nrm, cyn, GET_MANA(mob), nrm,
	grn, nrm, cyn, GET_MOVE(mob), nrm,
	grn, nrm, cyn, GET_AC(mob), nrm, 
        /*. Gold & Exp are longs in my mud, ignore any warnings .*/
	grn, nrm, cyn, (long)GET_EXP(mob), nrm,
	grn, nrm, cyn, (long)GET_GOLD(mob), nrm
  );
  send_to_char(buf, d->character);

  sprintbit_multi(MOB_FLAGS_BITV(mob), action_bits, buf1);
  sprintbit_multi(AFF_FLAGS_BITV(mob), affected_bits, buf2);
  sprintf(buf,
	"%sI%s) Position  : %s%s\r\n"
        "%sJ%s) Default   : %s%s\r\n"
        "%sK%s) Attack    : %s%s\r\n"
        "%sL%s) NPC Flags : %s%s\r\n"
        "%sM%s) AFF Flags : %s%s\r\n"
        "%sN%s) Mob Programs: %s%s\r\n"
        "%sO%s) Spec proc: %s%s\r\n"
        "%sQ%s) Quit%s%s\r\n"
        "Enter choice (? for help) : ",

	grn, nrm, yel, position_types[(int)GET_POS(mob)],
	grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)],
        grn, nrm, yel, attack_hit_text[GET_ATTACK(mob)].singular,
	grn, nrm, cyn, buf1, 
	grn, nrm, cyn, buf2,
	grn, nrm, cyn, 
	  /* mob_index[GET_MOB_RNUM(OLC_MOB(d))].mobprogs */
	  OLC_FPROG(d) != NULL && OLC_FPROG(d)->type != NO_PROG ? "SET" : "NONE",
	grn, nrm, cyn, mob_procs[OLC_SPEC(d)].name,
        grn, nrm,
        OLC_CLAN_EDIT(d) ? local_buf : "",
        OLC_READONLY(d) ? " &R(Read-Only)&w" : ""
  );
  send_to_char(buf, d->character);

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

MPROG_DATA *create_blank_mob_prog(struct descriptor_data * d)
{
  MPROG_DATA *t;
  if (OLC_PROG(d))
    CREATE(t, MPROG_DATA, 1);
  else 
    CREATE(t, MPROG_DATA, 1);
  t->type = NO_PROG;
  t->next = NULL;
  t->arglist = NULL;
  t->comlist = NULL;
  t->filename = NULL;
//  log("Created blank olc program");
  return t;
}

void delete_mob_prog(struct descriptor_data * d)
{
  MPROG_DATA *n, *m;
  
  if (OLC_PROG(d)) {
    if (OLC_FPROG(d) == OLC_PROG(d)) {
      OLC_FPROG(d) = OLC_FPROG(d)->next;
      free(OLC_PROG(d)->arglist);
      free(OLC_PROG(d)->comlist);
      free(OLC_PROG(d)->filename);
      free(OLC_PROG(d));
      OLC_PROG(d) = OLC_FPROG(d);
    } else {
      n = OLC_FPROG(d);
      while (n) {
        if (n->next == OLC_PROG(d)) {
          m = n->next->next;
          free(n->next->arglist);
          free(n->next->comlist);
          free(n->next->filename);
          free(n->next);
          n->next = m;
          break;
        }
        n = n->next;
      }
      OLC_PROG(d) = n;
    }
  }  
}

void clean_mob_prog(struct descriptor_data * d)
{
  if (OLC_PROG(d) && (OLC_PROG(d)->type == NO_PROG || !OLC_PROG(d)->arglist || 
    !OLC_PROG(d)->comlist)) {
    delete_mob_prog(d);
  }
}

void medit_disp_mob_prog_menu(struct descriptor_data * d)
{
  
  MPROG_DATA *mp_data = OLC_PROG(d);
  if (!mp_data) log("Error - trying to modify nonexisting mob prog.");
  sprintf(buf, "[H[J"
  	"&YMobProg online editor by Yilard\r\n"
  	"&w-- &yLocation: &r%s\r\n"
  	"&G1&w) &yType: &C%s\r\n"
  	"&G2&w) &yArgument (%s): &c%s\r\n"
  	"&G3&w) &yProgram:\r\n&g",
  	
        mp_data->filename ? mp_data->filename : "&BINLINE&w",
	mprog_type_to_name(mp_data->type),
	mprog_get_argument_str(mp_data->type),
	mp_data->arglist ? mp_data->arglist : NONESTR
  );
  send_to_char(buf, d->character);
  show_color_codes = TRUE;
  sprintf(buf, mp_data->comlist ? mp_data->comlist : NONESTR);
  send_to_char(buf, d->character);
  show_color_codes = FALSE;
  sprintf(buf, "\r\n"
  	"&G4&w) &yDelete this program\r\n"
        "&G5&w) &yGoto next program: ");
        
  if (!mp_data->next)
    strcat(buf, "<NOT SET>\r\n");
  else
    sprintf(buf, "%sSet. (%s)\r\n", buf, mprog_type_to_name(mp_data->next->type));
  strcat(buf, "&yEnter choice (0 to quit) :&w ");
  send_to_char(buf, d->character);
  OLC_MODE(d) = MEDIT_MPROG_MENU;
}

void medit_disp_mprog_types(struct descriptor_data * d)
{
  
  int i;
  sprintf(buf, "[H[J&MMPROG type:\r\n&w");
  for (i = 0; i < NUM_MPROG_TYPES; i++)
    sprintf(buf, "%s&C%2d&w)&c %s\r\n", buf, i, 
      mprog_type_to_name(i ? 1 << (i-1) : 0));
  sprintf(buf, "%s&gEnter (0-%d, ? for help):", buf, NUM_MPROG_TYPES-1); 
  send_to_char(buf, d->character);
  OLC_MODE(d) = MEDIT_MPROG_TYPE;
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void medit_parse(struct descriptor_data * d, char *arg)
{ int i;

  /* First check help */
  if (arg && *arg) {
    two_arguments(arg, buf, buf1);
    if (str_cmp(buf, "?") == 0) {
      OLC_LAST(d) = OLC_MODE(d);
      OLC_MODE(d) = MEDIT_HELP;
      if (*buf1 != '\0')
        do_help(d->character, buf1, 0, 0);
      else
      switch (OLC_LAST(d)) {  /* Here define help invokations for input modes */
        case MEDIT_MAIN_MENU:
          do_help(d->character, "MEDIT MAIN MENU", 0, 0);      
          break;
        case MEDIT_NPC_FLAGS:
          do_help(d->character, "NPC FLAGS", 0, 0);      
          break;
        case MEDIT_AFF_FLAGS:
          do_help(d->character, "AFF FLAGS", 0, 0);      
          break;
        case MEDIT_MPROG_TYPE:
          do_help(d->character, "MPTRIGGERS", 0, 0);      
          break;
        default:  /* Do nothing */
          OLC_MODE(d) = OLC_LAST(d);
          break;
      }  
    }
    if (OLC_MODE(d) == MEDIT_HELP) return;
  } 
  
  else if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE)
  { if(!*arg || (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1])))))
    { send_to_char("Field must be numerical, try again : ", d->character);
      return;
    }
  }

  switch (OLC_MODE(d)) 
  {
/*-------------------------------------------------------------------*/
  case MEDIT_CONFIRM_SAVESTRING:
    /*. Ensure mob has MOB_ISNPC set or things will go pair shaped .*/
    SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    /*. Ensure to not allow clan defined monsters escape from their zone .*/
    if (OLC_CLAN_EDIT(d))
      SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_STAY_ZONE);
    switch (*arg) {
    case 'y':
    case 'Y':
      /*. Save the mob in memory and to disk  .*/
      send_to_char("Saving mobile to memory.\r\n", d->character);
      medit_save_internally(d);
      sprintf(buf, "OLC: %s edits mob %d", GET_NAME(d->character),
               OLC_NUM(d));	      
      mudlog(buf, CMP, LVL_BUILDER, TRUE);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save the mobile? : ", d->character);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) 
    { case 'q':
      case 'Q':
        if (!OLC_READONLY(d) && OLC_VAL(d)) /*. Anything been changed? .*/
        { send_to_char("Do you wish to save the changes to the mobile? (y/n) : ", d->character);
          OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
        } else
          cleanup_olc(d, CLEANUP_ALL);
        return;
      case '1':
        OLC_MODE(d) = MEDIT_SEX;
        medit_disp_sex(d);
        return;
      case '2':
        OLC_MODE(d) = MEDIT_ALIAS;
        i--;
        break;
      case '3':
        OLC_MODE(d) = MEDIT_S_DESC;
        i--;
        break;
      case '4':
        OLC_MODE(d) = MEDIT_L_DESC;
        i--;
        break;
      case '5':
        OLC_MODE(d) = MEDIT_D_DESC;
	  SEND_TO_Q("Enter mob description: (/s saves /h for help)\r\n\r\n", d);
	  d->backstr = NULL;
	  if (OLC_MOB(d)->player.description) {
	     SEND_TO_Q(OLC_MOB(d)->player.description, d);
	     d->backstr = str_dup(OLC_MOB(d)->player.description);
	  }
	  d->str = &OLC_MOB(d)->player.description;
        d->max_str = MAX_MOB_DESC;
        d->mail_to = 0;
        return;
      case '6':
        OLC_MODE(d) = MEDIT_LEVEL;
        i++;
        break;
      case '7':
        OLC_MODE(d) = MEDIT_ALIGNMENT;
        i++;
        break;
      case '8':
        OLC_MODE(d) = MEDIT_HITROLL;
        i++;
        break;
      case '9':
        OLC_MODE(d) = MEDIT_DAMROLL;
        i++;
        break;
      case 'a':
      case 'A':
        OLC_MODE(d) = MEDIT_NDD;
        i++;
        break;
      case 'b':
      case 'B':
        OLC_MODE(d) = MEDIT_SDD;
        i++;
        break;
      case 'c':
      case 'C':
        OLC_MODE(d) = MEDIT_NUM_HP_DICE;
        i++;
        break;
      case 'd':
      case 'D':
        OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
        i++;
        break;
      case 'e':
      case 'E':
        OLC_MODE(d) = MEDIT_ADD_HP;
        i++;
        break;
      case 'f':
      case 'F':
        OLC_MODE(d) = MEDIT_AC;
        i++;
        break;
      case 'g':
      case 'G':
        OLC_MODE(d) = MEDIT_EXP;
        i++;
        break;
      case 'h':
      case 'H':
        OLC_MODE(d) = MEDIT_GOLD;
        i++;
        break;
      case 'i':
      case 'I':
        OLC_MODE(d) = MEDIT_POS;
        medit_disp_positions(d);
        return;
      case 'j':
      case 'J':
        OLC_MODE(d) = MEDIT_DEFAULT_POS;
        medit_disp_positions(d);
        return;
      case 'k':
      case 'K':
        OLC_MODE(d) = MEDIT_ATTACK;
        medit_disp_attack_types(d);
        return;
      case 'l':
      case 'L':
        OLC_MODE(d) = MEDIT_NPC_FLAGS;
        medit_disp_mob_flags(d);
        return;
      case 'm':
      case 'M':
        OLC_MODE(d) = MEDIT_AFF_FLAGS;
        medit_disp_aff_flags(d);
        return;
      case 'n':
      case 'N':
        if (!OLC_CLAN_EDIT(d)) {
          if (!OLC_FPROG(d)) 
            OLC_FPROG(d) = create_blank_mob_prog(d);
          OLC_PROG(d) = OLC_FPROG(d);
          medit_disp_mob_prog_menu(d);
          return;
        }
        medit_disp_menu(d);
        return;
      case 'o':
      case 'O':
        if (!OLC_CLAN_EDIT(d)) {
          OLC_MODE(d) = MEDIT_SPEC_PROC;
          olc_disp_spec_proc_menu(d, mob_procs);
          return;
        }
        medit_disp_menu(d);
        return;
      default:
        medit_disp_menu(d);
	return;
    }
    if (i==1)
    {  send_to_char("\r\nEnter new value : ", d->character);
       return;
    }
    if (i==-1)
    {  send_to_char("\r\nEnter new text :\r\n| ", d->character);
       return;
    }
    break; 
  case MEDIT_MPROG_MENU:
    switch (*arg) {
     case '0': 
//       clean_mob_prog(d);  	    
       medit_disp_menu(d);
       return;
     case '1':
//       log("Edit mprog types.");	
       medit_disp_mprog_types(d);
       OLC_VAL(d) = 1;
       return;
     case '2':
//       log("Edit mprog argument.");
       OLC_MODE(d) = MEDIT_MPROG_ARG;
       OLC_VAL(d) = 1;
       send_to_char("&yEnter new mprog argument:\r\n| &w", d->character);
       return;
     case '3':
       OLC_MODE(d) = MEDIT_MPROG_CMDS;
          SET_BIT(PRF_FLAGS(d->character), PRF_EDIT_BW);
	  SEND_TO_Q("Enter mob program: (/s saves /h for help)\r\n\r\n", d);
	  d->backstr = NULL;
	  if (OLC_PROG(d)->comlist) {
	     show_color_codes = TRUE;
	     SEND_TO_Q(OLC_PROG(d)->comlist, d);
	     show_color_codes = FALSE;
	     d->backstr = str_dup(OLC_PROG(d)->comlist);
	  }
	  d->str = &OLC_PROG(d)->comlist;
        d->max_str = MAX_STRING_LENGTH;
        d->mail_to = 0;
        OLC_VAL(d) = 1;
        return;
      case '4':
//        log("Deleting mob program.");
        delete_mob_prog(d);
        if (!OLC_FPROG(d)) {
          OLC_FPROG(d) = create_blank_mob_prog(d);
          OLC_PROG(d) = OLC_FPROG(d);
        }
        medit_disp_mob_prog_menu(d);
        OLC_VAL(d) = 1;
        return;
      case '5':
        if (!OLC_PROG(d)->next) 
          OLC_PROG(d)->next = create_blank_mob_prog(d);
        OLC_PROG(d) = OLC_PROG(d)->next;
        medit_disp_mob_prog_menu(d);
        OLC_VAL(d) = 1;
        return;
    }
    break;
  case MEDIT_MPROG_TYPE:
    i = atoi(arg);
    if (isdigit(*arg) && i >= 0 && i < NUM_MPROG_TYPES) {
      OLC_PROG(d)->type = i ? 1 << (i-1) : 0;
    medit_disp_mob_prog_menu(d);
    return;
    }
    medit_disp_mprog_types(d);
    return;
  case MEDIT_MPROG_ARG:
    if (OLC_PROG(d)->arglist)
      free(OLC_PROG(d)->arglist);
    OLC_PROG(d)->arglist = strdup(arg);
    medit_disp_mob_prog_menu(d);
    return;
/*-------------------------------------------------------------------*/
  case MEDIT_ALIAS:
    if(GET_ALIAS(OLC_MOB(d)))
      free(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = str_dup(arg); 
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_S_DESC:
    if(GET_SDESC(OLC_MOB(d)))
      free(GET_SDESC(OLC_MOB(d)));
    GET_SDESC(OLC_MOB(d)) = str_dup(arg); 
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_L_DESC:
    if(GET_LDESC(OLC_MOB(d)))
      free(GET_LDESC(OLC_MOB(d)));
    strcpy(buf, arg);
    strcat(buf, "\r\n");
    GET_LDESC(OLC_MOB(d)) = str_dup(buf); 
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_D_DESC:
    /*. We should never get here .*/
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!",BRF,LVL_BUILDER,TRUE);
    break;
/*-------------------------------------------------------------------*/
  case MEDIT_NPC_FLAGS:
    i = atoi(arg);
    if (i==0)
      break;
    updatebit(MOB_FLAGS_BITV(OLC_MOB(d)), NUM_MOB_FLAGS, i, 
      CHECK_MASK_PERM(d, npc_flags_mask));
    medit_disp_mob_flags(d);
    return;
/*-------------------------------------------------------------------*/
  case MEDIT_AFF_FLAGS:
    i = atoi(arg);
    if (i==0)
      break;
    updatebit(AFF_FLAGS_BITV(OLC_MOB(d)), NUM_AFF_FLAGS, i, 
      CHECK_MASK_PERM(d, aff_flags_mask));
    medit_disp_aff_flags(d);
    return;
/*-------------------------------------------------------------------*/
/*. Numerical responses .*/

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = MAX(0, MIN(NUM_GENDERS -1, atoi(arg)));
    break;

  case MEDIT_HITROLL:
    GET_HITROLL(OLC_MOB(d)) = atoi(arg);
    CHECK_RANGE(LVL_GRGOD, GET_HITROLL(OLC_MOB(d)), 0, 5); 
    CHECK_RANGE(LVL_BUILDER, GET_HITROLL(OLC_MOB(d)), 0, 50);
    CHECK_RANGE(LVL_IMPL, GET_HITROLL(OLC_MOB(d)), 0, 50);
    break;

  case MEDIT_DAMROLL:
    GET_DAMROLL(OLC_MOB(d)) = atoi(arg);
    CHECK_RANGE(LVL_GRGOD, GET_DAMROLL(OLC_MOB(d)), 0, 5); 
    CHECK_RANGE(LVL_BUILDER, GET_DAMROLL(OLC_MOB(d)), 0, 50);
    CHECK_RANGE(LVL_IMPL, GET_DAMROLL(OLC_MOB(d)), 0, 50); 
    break;

  case MEDIT_NDD:
    GET_NDD(OLC_MOB(d)) =  atoi(arg);
    CHECK_RANGE(LVL_GRGOD, GET_NDD(OLC_MOB(d)), 0, 10); 
    CHECK_RANGE(LVL_BUILDER, GET_NDD(OLC_MOB(d)), 0, 30);
    CHECK_RANGE(LVL_IMPL, GET_NDD(OLC_MOB(d)), 0, 30);
    break;

  case MEDIT_SDD:
    GET_SDD(OLC_MOB(d)) = atoi(arg);
    CHECK_RANGE(LVL_GRGOD, GET_SDD(OLC_MOB(d)), 0, 10); 
    CHECK_RANGE(LVL_BUILDER, GET_SDD(OLC_MOB(d)), 0, 30);
    CHECK_RANGE(LVL_IMPL, GET_SDD(OLC_MOB(d)), 0, 30);
    break;

  case MEDIT_NUM_HP_DICE:
    GET_HIT(OLC_MOB(d)) = MAX(0, MIN(30, atoi(arg)));
    break;

  case MEDIT_SIZE_HP_DICE:
    GET_MANA(OLC_MOB(d)) = MAX(0, MIN(1000, atoi(arg)));
    break;

  case MEDIT_ADD_HP:
    GET_MOVE(OLC_MOB(d)) = MAX(0, MIN(30000, atoi(arg)));
    break;

  case MEDIT_AC:
    GET_AC(OLC_MOB(d)) = MAX(-200, MIN(200, atoi(arg)));
    break;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = MAX(0, atol(arg));
    break;

  case MEDIT_GOLD:
    GET_GOLD(OLC_MOB(d)) = MAX(0, atol(arg));
    break;

  case MEDIT_POS:
    GET_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS-1, atoi(arg)));
    break;

  case MEDIT_DEFAULT_POS:
    GET_DEFAULT_POS(OLC_MOB(d)) = MAX(0, MIN(NUM_POSITIONS-1, atoi(arg)));
    break;

  case MEDIT_ATTACK:
    GET_ATTACK(OLC_MOB(d)) = MAX(0, MIN(NUM_ATTACK_TYPES-1, atoi(arg)));
    break;

  case MEDIT_LEVEL:
    GET_LEVEL(OLC_MOB(d)) = MAX(1, MIN(99, atoi(arg)));
    break;

  case MEDIT_ALIGNMENT:
    GET_ALIGNMENT(OLC_MOB(d)) = MAX(-1000, MIN(1000, atoi(arg)));
    break;
    
  case MEDIT_SPEC_PROC:
    if (*arg == '\0') break;
    OLC_SPEC(d) = 
      get_spec_proc_index(mob_procs, GET_LEVEL(d->character), atoi(arg));
    if (OLC_SPEC(d)) SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_SPEC);
      else REMOVE_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_SPEC);
    break;

/*-------------------------------------------------------------------*/
/* This is return from help */
  case MEDIT_HELP:
    OLC_MODE(d) = OLC_LAST(d);
    switch (OLC_MODE(d)) {
      case MEDIT_MAIN_MENU:
        medit_disp_menu(d);
        break;
      case MEDIT_NPC_FLAGS:
        medit_disp_mob_flags(d);
        break;
      case MEDIT_AFF_FLAGS:
        medit_disp_aff_flags(d);
        break;
      case MEDIT_MPROG_TYPE:
        medit_disp_mprog_types(d);
        break;
      default:
        log("SYSERR: Help topic with undefined return in medit (mode = %d)", OLC_MODE(d));
        break;
    }
    return;
/*-------------------------------------------------------------------*/
  default:
    /*. We should never get here .*/
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached default case!",BRF,LVL_BUILDER,TRUE);
    break;
  }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
    If we get here, we have probably changed something, and now want to
    return to main menu.  Use OLC_VAL as a 'has changed' flag .*/

  OLC_VAL(d) = 1;
  medit_disp_menu(d);
}
/*. End of medit_parse() .*/

