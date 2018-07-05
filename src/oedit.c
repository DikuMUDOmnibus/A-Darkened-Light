/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - oedit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*. Original author: Levork .*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "spells.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "shop.h"
#include "olc.h"
#include "constants.h"
#include "interpreter.h"

/*------------------------------------------------------------------------*/
/* external variables */

extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern int top_of_objt;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct shop_data *shop_index;			/*. shop.c	.*/
extern int top_shop;					/*. shop.c	.*/
extern struct attack_hit_type attack_hit_text[]; 	/*. fight.c 	.*/
extern struct board_info_type board_info[];
extern struct descriptor_data *descriptor_list;		/*. comm.c	.*/
extern struct specproc_info obj_procs[];		/*. spec_assign.c*/
extern const char *spells[];				/*. spell_parser.c */

/*------------------------------------------------------------------------*/
/*. Macros .*/

#define S_PRODUCT(s, i) ((s)->producing[(i)])

/*------------------------------------------------------------------------*/
/* Function externs */
void olc_disp_spec_proc_menu(struct descriptor_data * d, 
  struct specproc_info table[]);
ACMD(do_help);

/* Function prototypes */
void oedit_disp_container_flags_menu(struct descriptor_data * d);
void oedit_disp_extradesc_menu(struct descriptor_data * d);
void oedit_disp_weapon_menu(struct descriptor_data * d);
void oedit_disp_val1_menu(struct descriptor_data * d);
void oedit_disp_val2_menu(struct descriptor_data * d);
void oedit_disp_val3_menu(struct descriptor_data * d);
void oedit_disp_val4_menu(struct descriptor_data * d);
void oedit_disp_type_menu(struct descriptor_data * d);
void oedit_disp_extra_menu(struct descriptor_data * d);
void oedit_disp_wear_menu(struct descriptor_data * d);
void oedit_disp_menu(struct descriptor_data * d);

void oedit_parse(struct descriptor_data * d, char *arg);
void oedit_disp_spells_menu(struct descriptor_data * d);
void oedit_liquid_type(struct descriptor_data * d);
void oedit_setup_new(struct descriptor_data *d);
void oedit_setup_existing(struct descriptor_data *d, int real_num);
void oedit_save_to_disk(struct descriptor_data *d);
void oedit_save_internally(struct descriptor_data *d);

/*------------------------------------------------------------------------*\
  Utility and exported functions
\*------------------------------------------------------------------------*/

void oedit_setup_new(struct descriptor_data *d)
{
  CREATE (OLC_OBJ(d), struct obj_data, 1);
  clear_object(OLC_OBJ(d));
  OLC_OBJ(d)->name = str_dup("unfinished object");
  OLC_OBJ(d)->description = str_dup("An unfinished object is lying here.");
  OLC_OBJ(d)->short_description = str_dup("an unfinished object");
  GET_OBJ_WEAR(OLC_OBJ(d)) = ITEM_WEAR_TAKE;
  OLC_SPEC(d) = 0;
  OLC_VAL(d) = 0;
  oedit_disp_menu(d);
}
/*------------------------------------------------------------------------*/

void oedit_setup_existing(struct descriptor_data *d, int real_num)
{ struct extra_descr_data *this, *temp, *temp2;
  struct obj_data *obj;

  /* allocate object */
  CREATE (obj, struct obj_data, 1);
  clear_object (obj);
  *obj = obj_proto[real_num];
 
  /* copy all strings over */
  if (obj_proto[real_num].name)
    obj->name = str_dup (obj_proto[real_num].name);
  if (obj_proto[real_num].short_description)
    obj->short_description = str_dup (obj_proto[real_num].short_description);
  if (obj_proto[real_num].description)
    obj->description = str_dup (obj_proto[real_num].description);
  if (obj_proto[real_num].action_description)
    obj->action_description = str_dup (obj_proto[real_num].action_description);

  /*. Extra descriptions if necessary .*/
  if (obj_proto[real_num].ex_description)
  { /* temp is for obj being edited */
    CREATE (temp, struct extra_descr_data, 1);
    obj->ex_description = temp;
    for (this = obj_proto[real_num].ex_description; this; this = this->next)
    { if (this->keyword)
	temp->keyword = str_dup (this->keyword);
      if (this->description)
	temp->description = str_dup (this->description);
      if (this->next)
      { CREATE (temp2, struct extra_descr_data, 1);
        temp->next = temp2;
        temp = temp2;
      } else
	temp->next = NULL;
    }
  }
  OLC_SPEC(d) = get_spec_name(obj_procs, obj_index[real_num].func);

  /*. Attatch new obj to players descriptor .*/
  OLC_OBJ(d) = obj;
  OLC_VAL(d) = 0;
  oedit_disp_menu(d);
}
/*------------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

void oedit_save_internally(struct descriptor_data *d)
{ int i, shop, robj_num, found = FALSE, zone, cmd_no;
  struct extra_descr_data *this, *next_one;
  struct obj_data *obj, *swap, *new_obj_proto;
  struct index_data *new_obj_index;
  struct descriptor_data *dsc;

  /* write to internal tables */
  robj_num = real_object(OLC_NUM(d));
  if (robj_num > 0) {
    /* we need to run through each and every object currently in the
     * game to see which ones are pointing to this prototype */
  
    /* if object is pointing to this prototype, then we need to replace
     * with the new one */
    CREATE(swap, struct obj_data, 1);
    for (obj = object_list; obj; obj = obj->next) {
      if (obj->item_number == robj_num) {
        *swap = *obj;
        *obj = *OLC_OBJ(d);
        /* copy game-time dependent vars over */
        obj->in_room = swap->in_room;
        obj->item_number = robj_num;
        obj->carried_by = swap->carried_by;
        obj->worn_by = swap->worn_by;
        obj->worn_on = swap->worn_on;
        obj->in_obj = swap->in_obj;
        obj->contains = swap->contains;
        obj->next_content = swap->next_content;
        obj->next = swap->next;
        COPY_LONG_BITV(swap->obj_flags.bitvector, obj->obj_flags.bitvector);
/*        for (i=0; i< NUM_AFF; i++)
          obj->obj_flags.bitvector[i] = swap->obj_flags.bitvector[i]; */
      }
    }
    free_obj(swap);
    /* now safe to free old proto and write over */
    if (obj_proto[robj_num].name)
    free(obj_proto[robj_num].name);
    if (obj_proto[robj_num].description)
      free(obj_proto[robj_num].description);
    if (obj_proto[robj_num].short_description)
      free(obj_proto[robj_num].short_description);
    if (obj_proto[robj_num].action_description)
      free(obj_proto[robj_num].action_description);
    if (obj_proto[robj_num].ex_description)
      for (this = obj_proto[robj_num].ex_description;
  	 this; this = next_one) 
      { next_one = this->next;
        if (this->keyword)
  	  free(this->keyword);
        if (this->description)
  	  free(this->description);
        free(this);
      }
    obj_proto[robj_num] = *OLC_OBJ(d);
    obj_proto[robj_num].item_number = robj_num;
    /*. Update spec procs .*/
    obj_index[robj_num].func = obj_procs[OLC_SPEC(d)].sp_pointer;
  } else {
    /*. It's a new object, we must build new tables to contain it .*/

    CREATE(new_obj_index, struct index_data, top_of_objt + 2);
    CREATE(new_obj_proto, struct obj_data, top_of_objt + 2);
    /* start counting through both tables */
    for (i = 0; i <= top_of_objt; i++) {
      /* if we haven't found it */
      if (!found) {
        /* check if current virtual is bigger than our virtual */
        if (obj_index[i].vnum > OLC_NUM(d)) 
        { found = TRUE;
          robj_num = i;
	  OLC_OBJ(d)->item_number = robj_num;
	  new_obj_index[robj_num].vnum = OLC_NUM(d);
	  new_obj_index[robj_num].number = 0;
	  new_obj_index[robj_num].func = NULL;
	  new_obj_proto[robj_num] = *(OLC_OBJ(d));
	  new_obj_proto[robj_num].in_room = NOWHERE;
          /*. Copy over the mob that should be here .*/
          new_obj_index[robj_num + 1] = obj_index[robj_num];
	  new_obj_proto[robj_num + 1] = obj_proto[robj_num];
	  new_obj_proto[robj_num + 1].item_number = robj_num + 1;
        } else {
	  /* just copy from old to new, no num change */
	  new_obj_proto[i] = obj_proto[i];
  	  new_obj_index[i] = obj_index[i];
        }
      } else {
        /* we HAVE already found it.. therefore copy to object + 1 */
        new_obj_index[i + 1] = obj_index[i];
        new_obj_proto[i + 1] = obj_proto[i];
        new_obj_proto[i + 1].item_number = i + 1;
      }
    }
    if (!found)
    { robj_num = i;
      OLC_OBJ(d)->item_number = robj_num;
      new_obj_index[robj_num].vnum = OLC_NUM(d);
      new_obj_index[robj_num].number = 0;
      new_obj_index[robj_num].func = NULL;
      new_obj_proto[robj_num] = *(OLC_OBJ(d));
      new_obj_proto[robj_num].in_room = NOWHERE;
    }

    /* free and replace old tables */
    free (obj_proto);
    free (obj_index);
    obj_proto = new_obj_proto;
    obj_index = new_obj_index;
    top_of_objt++;

    /*. Update spec procs .*/
    obj_index[robj_num].func = obj_procs[OLC_SPEC(d)].sp_pointer;

    /*. Renumber live objects .*/
    for (obj = object_list; obj; obj = obj->next)
      if (GET_OBJ_RNUM (obj) >= robj_num)
        GET_OBJ_RNUM (obj)++;

    /*. Renumber zone table .*/
    for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
        switch (ZCMD.command)
	{ case 'P':
	    if(ZCMD.arg3 >= robj_num)
              ZCMD.arg3++;
            /*. No break here - drop into next case .*/
	  case 'O':
	  case 'G':
	  case 'E':
	    if(ZCMD.arg1 >= robj_num)
              ZCMD.arg1++;
	    break;
	  case 'R':
	    if(ZCMD.arg2 >= robj_num)
              ZCMD.arg2++;
	    break;
        }

    /*. Renumber notice boards */
    for (i = 0; i < NUM_OF_BOARDS; i++)
      if (BOARD_RNUM(i) >= robj_num)
	BOARD_RNUM(i) = BOARD_RNUM(i) + 1;

    /*. Renumber shop produce .*/
    for(shop = 0; shop < top_shop; shop++)
      for(i = 0; SHOP_PRODUCT(shop, i) != -1; i++)
        if (SHOP_PRODUCT(shop, i) >= robj_num)
          SHOP_PRODUCT(shop, i)++;

    /*. Renumber produce in shops being edited .*/
    for(dsc = descriptor_list; dsc; dsc = dsc->next)
      if(dsc->connected == CON_SEDIT)
        for(i = 0; S_PRODUCT(OLC_SHOP(dsc), i) != -1; i++)
          if (S_PRODUCT(OLC_SHOP(dsc), i) >= robj_num)
            S_PRODUCT(OLC_SHOP(dsc), i)++;
      
  } 
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ);
}
/*------------------------------------------------------------------------*/

void oedit_save_to_disk(struct descriptor_data *d)
{ int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;

  sprintf(buf, "%s/%d.obj", OBJ_PREFIX, zone_table[OLC_ZNUM(d)].number);
  if (!(fp = fopen(buf, "w+")))
  { mudlog("SYSERR: OLC: Cannot open objects file!", BRF, LVL_BUILDER, TRUE);
    return;
  }

  /* start running through all objects in this zone */
  for (counter = zone_table[OLC_ZNUM(d)].number * 100;
       counter <= zone_table[OLC_ZNUM(d)].top;
       counter++) 
  { /* write object to disk */
    realcounter = real_object(counter);
    if (realcounter >= 0 && !IS_OBJ_STAT2(&obj_proto[realcounter], ITEM2_DELETED)) 
    { obj = (obj_proto + realcounter);

      if (obj->action_description)
      { strcpy(buf1, obj->action_description);
        strip_string(buf1);
      } else
	*buf1 = 0;

      fprintf(fp, 
        "#%d\n"
	"%s~\n"
	"%s~\n"
	"%s~\n"
	"%s~\n"
	"%d %ld %ld %ld %ld %d\n"
	"%d %d %d %d\n"
	"%d %d %d\n",

 	GET_OBJ_VNUM(obj),
	obj->name ? obj->name : "undefined",
	obj->short_description ? obj->short_description : "undefined",
	obj->description ? obj->description : "undefined",
	buf1,
	GET_OBJ_TYPE(obj),
	GET_OBJ_EXTRA(obj),
	GET_OBJ_EXTRA2(obj),
	GET_OBJ_EXTRA3(obj),
	GET_OBJ_EXTRA4(obj),
	GET_OBJ_WEAR(obj),
	GET_OBJ_VAL(obj, 0),
	GET_OBJ_VAL(obj, 1),
	GET_OBJ_VAL(obj, 2),
	GET_OBJ_VAL(obj, 3),
	GET_OBJ_WEIGHT(obj),
	GET_OBJ_COST(obj),
	GET_OBJ_RENT(obj)
/*.     GET_OBJ_LEVEL(obj) -- Level flags for objects .*/
      );

      /* Do we have extra descriptions? */
      if (obj->ex_description)
      { /*. Yep, save them too .*/
	for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) 
        { /*. Sanity check to prevent nasty protection faults .*/
          if (!*ex_desc->keyword || !*ex_desc->description)
          { mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!", BRF, LVL_BUILDER, TRUE);
            continue;
          }
          strcpy(buf1, ex_desc->description);
          strip_string(buf1);
	  fprintf(fp,   "E\n"
	  		"%s~\n"
	 		"%s~\n",
			ex_desc->keyword,
			buf1
          );
	}
      }
      
      /* Do we have affects? */
      
      
      for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
        if (obj->affected[counter2].modifier) 
	  fprintf(fp,   "A\n"
	  		"%d %d\n", 
 			obj->affected[counter2].location,
			obj->affected[counter2].modifier
          );
          
      /* Now let's do charaffects */
      if (is_setbit_multi(OBJ_AFF_FLAGS_BITV(obj), LONG_BITV_WIDTH)) {
        writebit_multi(OBJ_AFF_FLAGS_BITV(obj), LONG_BITV_WIDTH, buf);
        fprintf(fp, "C\n"
                    "%s\n", buf);
      }
 		
      /* And now the spec-proc */
        if (obj_index[realcounter].func != NULL)
          fprintf(fp, "S\n%s\n", 
            obj_procs[get_spec_name(obj_procs, obj_index[realcounter].func)].name);
            
      /* Save level restriction if needed */
        if (GET_OBJ_TOP_LEV(obj) != 0 &&
          (GET_OBJ_BOTTOM_LEV(obj) != 0 || GET_OBJ_TOP_LEV(obj) != LVL_IMPL))
          fprintf(fp, "L\n%d %d\n", 
            GET_OBJ_BOTTOM_LEV(obj), GET_OBJ_TOP_LEV(obj));
    }
  }

  /* write final line, close */
  fprintf(fp, "$~\n");
  fclose(fp);
  olc_remove_from_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_OBJ);
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/* For container flags */
void oedit_disp_container_flags_menu(struct descriptor_data * d)
{ get_char_cols(d->character);
  sprintbit(GET_OBJ_VAL(OLC_OBJ(d), 1), container_bits, buf1);
  send_to_char("[H[J", d->character);
  sprintf(buf,
	"%s1%s) CLOSEABLE\r\n"
	"%s2%s) PICKPROOF\r\n"
	"%s3%s) CLOSED\r\n"
	"%s4%s) LOCKED\r\n"
	"Container flags: %s%s%s\r\n"
        "Enter flag (0 to quit, ? to help) : ",
        grn, nrm, grn, nrm, grn, nrm, grn, nrm, cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}

/* For extra descriptions */
void oedit_disp_extradesc_menu(struct descriptor_data * d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);
  
  if (!extra_desc->next)
    strcpy(buf1, "<Not set>\r\n");
  else
    strcpy(buf1, "Set.");

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  sprintf(buf, 
	"Extra desc menu\r\n"
  	"%s1%s) Keyword: %s%s\r\n"
  	"%s2%s) Description:\r\n%s%s\r\n"
        "%s3%s) Goto next description: %s\r\n"
  	"%s0%s) Quit\r\n"
        "Enter choice (? to help): ",

	grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>",
	grn, nrm, yel, extra_desc->description ? extra_desc->description : "<NONE>",
        grn, nrm, buf1,
        grn, nrm
        
  );
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_EXTRADESC_MENU;
}

/* Ask for *which* apply to edit */
void oedit_disp_prompt_apply_menu(struct descriptor_data * d)
{
  int counter;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
    if (OLC_OBJ(d)->affected[counter].modifier) {
      sprinttype(OLC_OBJ(d)->affected[counter].location, apply_types, buf2);
      sprintf(buf, " %s%d%s) %+d to %s\r\n", 
      	  	grn, counter + 1, nrm,
		OLC_OBJ(d)->affected[counter].modifier, buf2
      );
      send_to_char(buf, d->character);
    } else {
      sprintf(buf, " %s%d%s) None.\r\n", grn, counter + 1, nrm);
      send_to_char(buf, d->character);
    }
  }
  send_to_char("\r\nEnter affection to modify (0 to quit, ? to help) : ", d->character);
  OLC_MODE(d) = OEDIT_PROMPT_APPLY;
}

/*. Ask for liquid type .*/
void oedit_liquid_type(struct descriptor_data * d)
{ int counter, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (counter = 0; counter < NUM_LIQ_TYPES; counter++) 
  { sprintf(buf, " %s%2d%s) %s%-20.20s ", 
      	  	grn, counter, nrm, yel,
		drinks[counter]
    );
    if (!(++columns % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintf(buf, "\r\n%sEnter drink type : ", nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_VALUE_3;
}

/* The actual apply to set */
void oedit_disp_apply_menu(struct descriptor_data * d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (counter = 0; counter < NUM_APPLIES; counter++) 
  { sprintf(buf, "%s%2d%s) %-20.20s ",
	    grn, counter, nrm, apply_types[counter]
    );
    if (!(++columns % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  send_to_char("\r\nEnter apply type (0 is no apply, ? to help) : ", d->character);
  OLC_MODE(d) = OEDIT_APPLY;
}


/* weapon type */
void oedit_disp_weapon_menu(struct descriptor_data * d)
{ int counter, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (counter = 0; counter < NUM_ATTACK_TYPES; counter++) 
  { sprintf(buf, "%s%2d%s) %-20.20s ",
	    grn, counter, nrm, attack_hit_text[counter].singular
    );
    if(!(++columns % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  send_to_char("\r\nEnter weapon type : ", d->character);
}

/* spell type */
void oedit_disp_spells_menu(struct descriptor_data * d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (counter = 0; counter < NUM_SPELLS; counter++) {
    sprintf(buf, "&g%3d&w) &y%-20.20s ",
	    counter, spells[counter]
    );
    if (!(++columns % 3))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  for (counter = START_NON_PLAYER_SPELLS; 
       counter < NUM_NON_PLAYER_SPELLS+START_NON_PLAYER_SPELLS; counter++) {
    sprintf(buf, "&g%3d&w) &y%-20.20s ",
	    counter, spells[counter]
    );
    if (!(++columns % 3))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintf(buf, "\r\n%sEnter spell choice (0 for none, ? <spellname> for help) : ", nrm);
  send_to_char(buf, d->character);
}

/* object value 1 */
void oedit_disp_val1_menu(struct descriptor_data * d)
{
  OLC_MODE(d) = OEDIT_VALUE_1;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_LIGHT:
    /* values 0 and 1 are unused.. jump to 2 */
    oedit_disp_val3_menu(d);
    break;
  case ITEM_BOW:
  case ITEM_CROSSBOW:
  case ITEM_SLING:
    send_to_char("Range (1 - 3) = ", d->character);
    break;
  case ITEM_ROCK:
  case ITEM_BOLT:
  case ITEM_ARROW:
  case ITEM_THROW:
    oedit_disp_val2_menu(d);
    break;
  case ITEM_SCROLL:
  case ITEM_WAND:
  case ITEM_STAFF:
  case ITEM_POTION:
    send_to_char("Spell level : ", d->character);
    break;
  case ITEM_WEAPON:
//    /* this seems to be a circleism.. not part of normal diku? */
//    send_to_char("Modifier to Hitroll : ", d->character);
    oedit_disp_val2_menu(d);
    break;
  case ITEM_ARMOR:
    send_to_char("Apply to AC : ", d->character);
    break;
  case ITEM_CONTAINER:
    send_to_char("Max weight to contain : ", d->character);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    send_to_char("Max drink units : ", d->character);
    break;
  case ITEM_FOOD:
    send_to_char("Hours to fill stomach : ", d->character);
    break;
  case ITEM_MONEY:
    send_to_char("Number of gold coins : ", d->character);
    break;
  case ITEM_NOTE:
    /* this is supposed to be language, but it's unused */
    break;
  case ITEM_GRENADE:
    send_to_char("Ticks to countdown to explosion : ", d->character);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/* object value 2 */
void oedit_disp_val2_menu(struct descriptor_data * d)
{
  OLC_MODE(d) = OEDIT_VALUE_2;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char("Max number of charges : ", d->character);
    break;
  case ITEM_ROCK:
  case ITEM_BOLT:
  case ITEM_ARROW:
  case ITEM_THROW:
  case ITEM_GRENADE:
  case ITEM_WEAPON:
    send_to_char("Number of damage dice : ", d->character);
    break;
  case ITEM_FOOD:
    /* values 2 and 3 are unused, jump to 4. how odd */
    oedit_disp_val4_menu(d);
    break;
  case ITEM_CONTAINER:
    /* these are flags, needs a bit of special handling */
    oedit_disp_container_flags_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    send_to_char("Initial drink units : ", d->character);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/* object value 3 */
void oedit_disp_val3_menu(struct descriptor_data * d)
{
  OLC_MODE(d) = OEDIT_VALUE_3;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_LIGHT:
    send_to_char("Number of hours (0 = burnt, -1 is infinite) : ", d->character);
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char("Number of charges remaining : ", d->character);
    break;
  case ITEM_ROCK:
  case ITEM_BOLT:
  case ITEM_ARROW:
  case ITEM_THROW:
  case ITEM_GRENADE:
  case ITEM_WEAPON:
    send_to_char("Size of damage dice : ", d->character);
    break;
  case ITEM_CONTAINER:
    send_to_char("Vnum of key to open container (-1 for no key) : ", d->character);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    oedit_liquid_type(d);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/* object value 4 */
void oedit_disp_val4_menu(struct descriptor_data * d)
{
  OLC_MODE(d) = OEDIT_VALUE_4;
  switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
  case ITEM_SCROLL:
  case ITEM_POTION:
  case ITEM_WAND:
  case ITEM_STAFF:
    oedit_disp_spells_menu(d);
    break;
  case ITEM_WEAPON:
    oedit_disp_weapon_menu(d);
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
  case ITEM_FOOD:
    send_to_char("Poisoned (0 = not poison) : ", d->character);
    break;
  default:
    oedit_disp_menu(d);
  }
}

/* object type */
void oedit_disp_type_menu(struct descriptor_data * d)
{ int counter, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (counter = 1; counter < NUM_ITEM_TYPES; counter++) {
    sprintf(buf, "%s%2d%s) %-20.20s ",
	    grn, counter, nrm, item_types[counter]
    );
    if (!(++columns % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  send_to_char("\r\nEnter object type (0 to exit, ? to help): ", d->character);
}

/* object extra flags */
void oedit_disp_extra_menu(struct descriptor_data * d)
{

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  menubit(GET_OBJ_EXTRA_BITV(OLC_OBJ(d)), extra_bits, buf1, NULL);
  send_to_char(buf1, d->character);
  /*
  for (counter = 0; counter < NUM_ITEM_FLAGS; counter++) 
  { sprintf(buf, "%s%2d%s) %-20.20s ",
	    grn, counter + 1, nrm, extra_bits[counter]
    );
    if (!(++columns % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  */
  sprintbit_multi(GET_OBJ_EXTRA_BITV(OLC_OBJ(d)), extra_bits, buf1);
  sprintf(buf,  "\r\nObject flags: %s%s%s\r\n"
		"Enter object extra flag (0 to quit, ? to help) : ", 
		cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}

/* object wear flags */
void
oedit_disp_wear_menu(struct descriptor_data * d)
{
  int counter, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (counter = 0; counter < NUM_ITEM_WEARS; counter++) {
    sprintf(buf, "%s%2d%s) %-20.20s ",
	    grn, counter + 1, nrm, wear_bits[counter]
    );
    if (!(++columns % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintbit(GET_OBJ_WEAR(OLC_OBJ(d)), wear_bits, buf1);
  sprintf(buf,  "\r\nWear flags: %s%s%s\r\n"
  		"Enter wear flag, 0 to quit : ",
 		cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}


/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void oedit_disp_aff_flags(struct descriptor_data *d)
{ 
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  menubit(OBJ_AFF_FLAGS_BITV(OLC_OBJ(d)), affected_bits, buf1, NULL);
  send_to_char(buf1, d->character);
  /*
  for (i = 0; i < NUM_AFF_FLAGS; i++)
  {  sprintf(buf, "%s%2d%s) %-20.20s  ", 
	grn, i+1, nrm, affected_bits[i]
     );
     if(!(++columns % 2))
       strcat(buf, "\r\n");
     send_to_char(buf, d->character);
  }
  */
  sprintbit_multi(OBJ_AFF_FLAGS_BITV(OLC_OBJ(d)), affected_bits, buf1);
  sprintf(buf, "\r\n"
	"Current flags   : %s%s%s\r\n"
	"Enter aff flags (0 to quit, ? to help) : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
}

/* display main menu */
void oedit_disp_menu(struct descriptor_data * d)
{ struct obj_data *obj;

  obj = OLC_OBJ(d);
  get_char_cols(d->character);

  /*. Build buffers for first part of menu .*/
  sprinttype(GET_OBJ_TYPE(obj), item_types, buf1);
  sprintbit_multi(GET_OBJ_EXTRA_BITV(obj), extra_bits, buf2);

  /*. Build first hallf of menu .*/
  sprintf(buf, "[H[J"
  	"-- Item number : [%s%d%s]\r\n"
  	"%s1%s) Namelist : %s%s\r\n"
  	"%s2%s) S-Desc   : %s%s\r\n"
  	"%s3%s) L-Desc   :-\r\n%s%s\r\n"
  	"%s4%s) A-Desc   :-\r\n%s%s"
  	"%s5%s) Type        : %s%s\r\n"
  	"%s6%s) Extra flags : %s%s\r\n",

	cyn, OLC_NUM(d), nrm, 
	grn, nrm, yel, obj->name,
 	grn, nrm, yel, obj->short_description,
	grn, nrm, yel, obj->description,
	grn, nrm, yel, obj->action_description ?  obj->action_description : "<not set>\r\n",
	grn, nrm, cyn, buf1,
 	grn, nrm, cyn, buf2
  );
  /*. Send first half .*/
  send_to_char(buf, d->character);

  /*. Build second half of menu .*/
  sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf1);
  sprintbit_multi(OBJ_AFF_FLAGS_BITV(obj), affected_bits, buf2);
  sprintf(buf,
  	"%s7%s) Wear flags  : %s%s\r\n"
  	"%s8%s) Weight      : %s%d\r\n"
  	"%s9%s) Cost        : %s%d\r\n"
  	"%sA%s) Cost/Day    : %s%d\r\n"
  	"%sB%s) Timer       : %s%d\r\n"
  	"%sC%s) Levels      : %s%d-%d\r\n"
  	"%sD%s) Values      : %s%d %d %d %d\r\n"
  	"%sE%s) Applies menu\r\n"
  	"%sF%s) Extra descriptions menu\r\n"
  	"%sG%s) AFF flags: %s%s\r\n"
  	"%sH%s) Spec-proc: %s%s\r\n"
  	"%sQ%s) Quit%s\r\n"
  	"Enter choice (? to help): ",

 	grn, nrm, cyn, buf1,
 	grn, nrm, cyn, GET_OBJ_WEIGHT(obj),
 	grn, nrm, cyn, GET_OBJ_COST(obj),
 	grn, nrm, cyn, GET_OBJ_RENT(obj),
 	grn, nrm, cyn, GET_OBJ_TIMER(obj),
	grn, nrm, cyn, GET_OBJ_BOTTOM_LEV(obj), GET_OBJ_TOP_LEV(obj),
	grn, nrm, cyn, GET_OBJ_VAL(obj, 0), 
        	  	GET_OBJ_VAL(obj, 1), 
		  	GET_OBJ_VAL(obj, 2),
        	  	GET_OBJ_VAL(obj, 3),
        grn, nrm, grn, nrm, grn, nrm, cyn, buf2,
        grn, nrm, cyn, obj_procs[OLC_SPEC(d)].name,
        
        grn, nrm,
        OLC_READONLY(d) ? " &R(Read-Only)&w" : ""
  );
  send_to_char(buf, d->character);
  OLC_MODE(d) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 main loop (of sorts).. basically interpreter throws all input to here
 ***************************************************************************/


void oedit_parse(struct descriptor_data * d, char *arg)
{
  int number, max_val, min_val;
  
  /* First check help */
  if (arg && *arg) {
    two_arguments(arg, buf, buf1);
    if (str_cmp(buf, "?") == 0) {
      OLC_LAST(d) = OLC_MODE(d);
      OLC_MODE(d) = OEDIT_HELP;
      if (*buf1 != '\0')
        do_help(d->character, buf1, 0, 0);
      else
      switch (OLC_LAST(d)) {  /* Here define help invokations for input modes */
        case OEDIT_MAIN_MENU:
          do_help(d->character, "OEDIT MAIN MENU", 0, 0);      
          break;
        case OEDIT_TYPE:
          do_help(d->character, "ITEMS", 0, 0);      
          break;
        case OEDIT_AFF_FLAGS:
          do_help(d->character, "AFF FLAGS", 0, 0);      
          break;
        case OEDIT_EXTRADESC_MENU:
          do_help(d->character, "EXTRA DESCRIPTION", 0, 0);
          break;
        case OEDIT_EXTRAS:
          do_help(d->character, "EFF FLAGS", 0, 0);
          break;
        case OEDIT_PROMPT_APPLY:
          do_help(d->character, "OBJECT APPLY", 0, 0);
          break;
        case OEDIT_APPLY:
          do_help(d->character, "OBJECT APPLIES", 0, 0);
          break;
          
        default:  /* Do nothing */
          OLC_MODE(d) = OLC_LAST(d);
          break;
      }  
    }
    if (OLC_MODE(d) == OEDIT_HELP) return;
  } 
  
  
  switch (OLC_MODE(d)) {
  case OEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      send_to_char("Saving object to memory.\r\n", d->character);
      oedit_save_internally(d);
      sprintf(buf, "OLC: %s edits obj %d", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, LVL_BUILDER, TRUE);
      cleanup_olc(d, CLEANUP_STRUCTS);
      return;
    case 'n':
    case 'N':
      /*. Cleanup all .*/
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save this object internally?\r\n", d->character);
      return;
    }

  case OEDIT_MAIN_MENU:
    /* throw us out to whichever edit mode based on user input */
    switch (*arg) {
    case 'q':
    case 'Q':
      if (!OLC_READONLY(d) && OLC_VAL(d)) 
      { /*. Something has been modified .*/
        send_to_char("Do you wish to save this object internally? : ", d->character);
        OLC_MODE(d) = OEDIT_CONFIRM_SAVESTRING;
      } else 
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      send_to_char("Enter namelist : ", d->character);
      OLC_MODE(d) = OEDIT_EDIT_NAMELIST;
      break;
    case '2':
      send_to_char("Enter short desc : ", d->character);
      OLC_MODE(d) = OEDIT_SHORTDESC;
      break;
    case '3':
      send_to_char("Enter long desc :-\r\n| ", d->character);
      OLC_MODE(d) = OEDIT_LONGDESC;
      break;
    case '4':
      /* let's go out to modify.c */
      send_to_char("Enter action desc :-\r\n", d->character);
      OLC_MODE(d) = OEDIT_ACTDESC;
//      OLC_DESC(d)->description = OLC_OBJ(d)->action_description;
      d->backstr = NULL;
	 if (OLC_OBJ(d)->action_description) {
	    SEND_TO_Q(OLC_OBJ(d)->action_description, d);
	    d->backstr = str_dup(OLC_OBJ(d)->action_description);
	 }
      d->str = &OLC_OBJ(d)->action_description;
      /*
      d->str = (char **) malloc(sizeof(char *));

      *(d->str) = NULL;
      */
      d->max_str = MAX_OBJ_DESC;
      d->mail_to = 0;
      OLC_VAL(d) = 1;
      break;
    case '5':
      oedit_disp_type_menu(d);
      OLC_MODE(d) = OEDIT_TYPE;
      break;
    case '6':
      oedit_disp_extra_menu(d);
      OLC_MODE(d) = OEDIT_EXTRAS;
      break;
    case '7':
      oedit_disp_wear_menu(d);
      OLC_MODE(d) = OEDIT_WEAR;
      break;
    case '8':
      send_to_char("Enter weight : ", d->character);
      OLC_MODE(d) = OEDIT_WEIGHT;
      break;
    case '9':
      send_to_char("Enter cost : ", d->character);
      OLC_MODE(d) = OEDIT_COST;
      break;
    case 'a':
    case 'A':
      send_to_char("Enter cost per day : ", d->character);
      OLC_MODE(d) = OEDIT_COSTPERDAY;
      break;
    case 'b':
    case 'B':
      send_to_char("Enter timer : ", d->character);
      OLC_MODE(d) = OEDIT_TIMER;
      break;
    case 'c':
    case 'C':
      // oedit_disp_menu(d);
      send_to_char("Enter lower bound of levels: ", d->character);
      OLC_MODE(d) = OEDIT_BOTTOM_LEV;
      /*. Object level flags in my mud...
      send_to_char("Enter level : ", d->character);
      OLC_MODE(d) = OEDIT_LEVEL;
      .*/
      break;
    case 'd':
    case 'D':
      /*. Clear any old values .*/
      GET_OBJ_VAL(OLC_OBJ(d), 0) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 1) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 2) = 0;
      GET_OBJ_VAL(OLC_OBJ(d), 3) = 0;
      oedit_disp_val1_menu(d);
      break;
    case 'e':
    case 'E':
      oedit_disp_prompt_apply_menu(d);
      break;
    case 'f':
    case 'F':
      /* if extra desc doesn't exist . */
      if (!OLC_OBJ(d)->ex_description) {
	CREATE(OLC_OBJ(d)->ex_description, struct extra_descr_data, 1);
	OLC_OBJ(d)->ex_description->next = NULL;
      }
      OLC_DESC(d) = OLC_OBJ(d)->ex_description;
      oedit_disp_extradesc_menu(d);
      break;
    case 'G':
    case 'g':
      OLC_MODE(d) = OEDIT_AFF_FLAGS;
      oedit_disp_aff_flags(d);
      return;
    case 'H':
    case 'h':
      OLC_MODE(d) = OEDIT_SPEC_PROC;
      olc_disp_spec_proc_menu(d, obj_procs);
      return;
    default:
      oedit_disp_menu(d);
      break;
    }
    return;			/* end of OEDIT_MAIN_MENU */

  case OEDIT_EDIT_NAMELIST:
    if (OLC_OBJ(d)->name)
      free(OLC_OBJ(d)->name);
    OLC_OBJ(d)->name = str_dup(arg);
    break;

  case OEDIT_SHORTDESC:
    if (OLC_OBJ(d)->short_description)
      free(OLC_OBJ(d)->short_description);
    OLC_OBJ(d)->short_description = str_dup(arg);
    break;

  case OEDIT_LONGDESC:
    if (OLC_OBJ(d)->description)
      free(OLC_OBJ(d)->description);
    OLC_OBJ(d)->description = str_dup(arg);
    break;

  case OEDIT_TYPE:
    number = atoi(arg);
    if (number == 0)
      break;
    if ((number < 1) || (number >= NUM_ITEM_TYPES))
    { send_to_char("Invalid choice, try again : ", d->character);
      return;
    } else 
    { GET_OBJ_TYPE(OLC_OBJ(d)) = number;
    }
    break;

  case OEDIT_EXTRAS:
    number = atoi(arg);
    if (number == 0) break;
    updatebit(GET_OBJ_EXTRA_BITV(OLC_OBJ(d)), NUM_ITEM_FLAGS, number, NULL);
    oedit_disp_extra_menu(d);
    return;
    
    if ((number < 0) || (number > NUM_ITEM_FLAGS))
    { oedit_disp_extra_menu(d);
      return;
    } else {
      /* if 0, quit */
      if (number == 0)
	break;
      else 
      { /* if already set.. remove */
	if (IS_SET(GET_OBJ_EXTRA(OLC_OBJ(d)), 1 << (number - 1)))  
          REMOVE_BIT(GET_OBJ_EXTRA(OLC_OBJ(d)), 1 << (number - 1));
	else
	  /* set */
	  SET_BIT(GET_OBJ_EXTRA(OLC_OBJ(d)), 1 << (number - 1));
	oedit_disp_extra_menu(d);
        return;
      }
    }
    

  case OEDIT_WEAR:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ITEM_WEARS)) {
      send_to_char("That's not a valid choice!\r\n", d->character);
      oedit_disp_wear_menu(d);
      return;
    } else 
    { /* if 0, quit */
      if (number == 0)
	break;
      else 
      { /* if already set.. remove */
	if (IS_SET(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1)))
	  REMOVE_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1));
	else
	  SET_BIT(GET_OBJ_WEAR(OLC_OBJ(d)), 1 << (number - 1));
	oedit_disp_wear_menu(d);
        return;
      }
    }

  case OEDIT_WEIGHT:
    number = atoi(arg);
    GET_OBJ_WEIGHT(OLC_OBJ(d)) = number;
    break;

  case OEDIT_COST:
    number = atoi(arg);
    GET_OBJ_COST(OLC_OBJ(d)) = number;
    break;

  case OEDIT_COSTPERDAY:
    number = atoi(arg);
    GET_OBJ_RENT(OLC_OBJ(d)) = number;
    break;

  case OEDIT_TIMER:
    number = atoi(arg);
    GET_OBJ_TIMER(OLC_OBJ(d)) = number;
    break;

  case OEDIT_BOTTOM_LEV:
    number = atoi(arg);
    GET_OBJ_BOTTOM_LEV(OLC_OBJ(d)) = number;
    send_to_char("Enter higher bound of levels: ", d->character);
    OLC_MODE(d) = OEDIT_TOP_LEV;
    return;
    
  case OEDIT_TOP_LEV:
    number = atoi(arg);
    if (number == 0) number = LVL_IMPL;
    if (number >= GET_OBJ_BOTTOM_LEV(OLC_OBJ(d))) {
      GET_OBJ_TOP_LEV(OLC_OBJ(d)) = number;
    } else {
      GET_OBJ_TOP_LEV(OLC_OBJ(d)) = GET_OBJ_BOTTOM_LEV(OLC_OBJ(d));
      GET_OBJ_BOTTOM_LEV(OLC_OBJ(d)) = number;
    }
  break;
    
  case OEDIT_SPEC_PROC:
    if (*arg == '\0') break;
    OLC_SPEC(d) = get_spec_proc_index(obj_procs, GET_LEVEL(d->character), atoi(arg));;
    break;

  case OEDIT_VALUE_1:
    /* lucky, I don't need to check any of these for outofrange values */
    /*. Hmm, I'm not so sure - Rv .*/
    number = atoi(arg);
    GET_OBJ_VAL(OLC_OBJ(d), 0) = number;
    /* proceed to menu 2 */
    oedit_disp_val2_menu(d);
    return;
  case OEDIT_VALUE_2:
    /* here, I do need to check for outofrange values */
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
    { case ITEM_SCROLL:
      case ITEM_POTION:
	if (number < 0 || number >= TOP_SPELLS)
	  oedit_disp_val2_menu(d);
	else {
	  GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
	  oedit_disp_val3_menu(d);
	}
	break;
      case ITEM_CONTAINER:
	/* needs some special handling since we are dealing with flag values
	 * here */
	number = atoi(arg);
	if (number < 0 || number > 4)
	  oedit_disp_container_flags_menu(d);
	else 
        { /* if 0, quit */
	  if (number != 0)
	  { number = 1 << (number - 1);
            if (IS_SET(GET_OBJ_VAL(OLC_OBJ(d), 1), number))
              REMOVE_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), number);
	    else
              SET_BIT(GET_OBJ_VAL(OLC_OBJ(d), 1), number);
	    oedit_disp_val2_menu(d);
	  } else
	    oedit_disp_val3_menu(d);
	}
	break;
      default:
	GET_OBJ_VAL(OLC_OBJ(d), 1) = number;
	oedit_disp_val3_menu(d);
    }
    return;

  case OEDIT_VALUE_3:
    number = atoi(arg);
    /*. Quick'n'easy error checking .*/
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) 
    { case ITEM_SCROLL:
      case ITEM_POTION:
        min_val = 0;
	max_val = TOP_SPELLS;
        break;
      case ITEM_WEAPON:
        min_val = 1;
        max_val = 50;
      case ITEM_WAND:
      case ITEM_STAFF:
        min_val = 0;
	max_val = TOP_SPELLS;
        break;
      case ITEM_DRINKCON:
      case ITEM_FOUNTAIN:
        min_val = 0;
        max_val = NUM_LIQ_TYPES -1;
        break;
      default:
        min_val = -32000;
        max_val = 32000;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 2) = MAX(min_val, MIN(number, max_val));
    oedit_disp_val4_menu(d);
    return;

  case OEDIT_VALUE_4:
    number = atoi(arg);
    switch (GET_OBJ_TYPE(OLC_OBJ(d))) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      min_val = 0;
      max_val = TOP_SPELLS;
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      min_val = 1;
      max_val = TOP_SPELLS;
      break;
    case ITEM_WEAPON:
      min_val = 0;
      max_val = NUM_ATTACK_TYPES -1;
      break;
    default:
      min_val = -32000;
      max_val = 32000;
      break;
    }
    GET_OBJ_VAL(OLC_OBJ(d), 3) = MAX(min_val, MIN(number, max_val));
    break;

  case OEDIT_PROMPT_APPLY:
    number = atoi(arg);
    if (number == 0)
      break;
    else if (number < 0 || number > MAX_OBJ_AFFECT) 
    { oedit_disp_prompt_apply_menu(d);
      return;
    }
    OLC_VAL(d) = number - 1;
    OLC_MODE(d) = OEDIT_APPLY;
    oedit_disp_apply_menu(d);
    return;

  case OEDIT_APPLY:
    number = atoi(arg);
    if (number == 0) 
    { OLC_OBJ(d)->affected[OLC_VAL(d)].location = 0;
      OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = 0;
      oedit_disp_prompt_apply_menu(d);
    } else if (number < 0 || number >= NUM_APPLIES) 
      oedit_disp_apply_menu(d);
    else 
    { OLC_OBJ(d)->affected[OLC_VAL(d)].location = number;
      send_to_char("Modifier : ", d->character);
      OLC_MODE(d) = OEDIT_APPLYMOD;
    }
    return;

  case OEDIT_APPLYMOD:
    number = atoi(arg);
    OLC_OBJ(d)->affected[OLC_VAL(d)].modifier = number;
    oedit_disp_prompt_apply_menu(d);
    return;
    
  

/*-------------------------------------------------------------------*/
  case OEDIT_AFF_FLAGS:
    number = atoi(arg);
    if (number == 0)
      break;
    updatebit(OBJ_AFF_FLAGS_BITV(OLC_OBJ(d)), NUM_AFF_FLAGS, number, NULL);
    oedit_disp_aff_flags(d);
    return;
   
/*-------------------------------------------------------------------*/

  case OEDIT_EXTRADESC_KEY:
    if (OLC_DESC(d)->keyword)
      free(OLC_DESC(d)->keyword);
    OLC_DESC(d)->keyword = str_dup(arg);
    oedit_disp_extradesc_menu(d);
    return;

  case OEDIT_EXTRADESC_MENU:
    number = atoi(arg);
    switch (number) {
    case 0:
      { /* if something got left out */
	if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) 
        { struct extra_descr_data **tmp_desc;

	  if (OLC_DESC(d)->keyword)
	    free(OLC_DESC(d)->keyword);
	  if (OLC_DESC(d)->description)
	    free(OLC_DESC(d)->description);

          /*. Clean up pointers .*/
	  for(tmp_desc = &(OLC_OBJ(d)->ex_description); *tmp_desc;
	      tmp_desc = &((*tmp_desc)->next))
          { if (*tmp_desc == OLC_DESC(d))
            { *tmp_desc = NULL;
              break;
            }
          }
	  free(OLC_DESC(d));
	}
      }
      break;

    case 1:
      OLC_MODE(d) = OEDIT_EXTRADESC_KEY;
      send_to_char("Enter keywords, separated by spaces :-\r\n| ", d->character);
      return;

    case 2:
      OLC_MODE(d) = OEDIT_EXTRADESC_DESCRIPTION;
      send_to_char("Enter description :-\r\n", d->character);
      /* send out to modify.c */
      d->backstr = NULL;
	 if (OLC_DESC(d)->description) {
	    SEND_TO_Q(OLC_DESC(d)->description, d);
	    d->backstr = str_dup(OLC_DESC(d)->description);
	 }
      d->str = &OLC_DESC(d)->description;
      /*
      d->str = (char **) malloc(sizeof(char *));

      *(d->str) = NULL;
      */
      d->max_str = MAX_OBJ_DESC;
      d->mail_to = 0;
      OLC_VAL(d) = 1;
      return;

    case 3:
      /*. Only go to the next descr if this one is finished .*/
      if (OLC_DESC(d)->keyword && OLC_DESC(d)->description)
      { struct extra_descr_data *new_extra;

	if (OLC_DESC(d)->next)
	  OLC_DESC(d) = OLC_DESC(d)->next;
	else 
        { /* make new extra, attach at end */
	  CREATE(new_extra, struct extra_descr_data, 1);

	  OLC_DESC(d)->next = new_extra;
	  OLC_DESC(d) = OLC_DESC(d)->next;
	}
      }
      /*. No break - drop into default case .*/
    default:
      oedit_disp_extradesc_menu(d);
      return;
    }
    break;
  /*-------------------------------------------------------------------*/
  /* This is return from help */
  case OEDIT_HELP:
    OLC_MODE(d) = OLC_LAST(d);
    switch (OLC_MODE(d)) {
      case OEDIT_MAIN_MENU:
        oedit_disp_menu(d);
        break;
      case OEDIT_TYPE:
        oedit_disp_type_menu(d);
        break;
      case OEDIT_AFF_FLAGS:
        oedit_disp_aff_flags(d);
        break;        
      case OEDIT_EXTRADESC_MENU:
        oedit_disp_extradesc_menu(d);
        break;
      case OEDIT_EXTRAS:
        oedit_disp_extra_menu(d);
        break;
      case OEDIT_APPLY:
        oedit_disp_apply_menu(d);
        break;        
      case OEDIT_PROMPT_APPLY:
        oedit_disp_prompt_apply_menu(d);
        break;        
      default:
        log("SYSERR: Help topic with undefined return in oedit (mode = %d)", OLC_MODE(d));
        break;
    }
    return;
  default:
    mudlog("SYSERR: OLC: Reached default case in oedit_parse()!", BRF, LVL_BUILDER, TRUE);
    break;
  }

  /*. If we get here, we have changed something .*/
  OLC_VAL(d) = 1; /*. Has changed flag .*/
  oedit_disp_menu(d);
}
