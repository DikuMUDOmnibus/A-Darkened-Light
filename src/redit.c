/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - redit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*. Original author: Levork .*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "boards.h"
#include "olc.h"
#include "teleport.h"
#include "interpreter.h"
#include "constants.h"

/*------------------------------------------------------------------------*/
/*. External data .*/

extern int      top_of_world;
extern struct room_data *world;
extern struct obj_data *obj_proto;
extern struct char_data *mob_proto;
extern struct zone_data *zone_table;
extern sh_int r_mortal_start_room;
extern sh_int r_immort_start_room;
extern sh_int r_frozen_start_room;
extern sh_int mortal_start_room;
extern sh_int immort_start_room;
extern sh_int frozen_start_room;
extern int top_of_zone_table;
extern struct specproc_info room_procs[];


/* extern fnncts */
void reload_room_descs(sh_int real_nr, int lock);
void olc_disp_spec_proc_menu(struct descriptor_data * d,
  struct specproc_info table[]);
ACMD(do_help);
/*------------------------------------------------------------------------*/
/* function protos */

void redit_disp_extradesc_menu(struct descriptor_data * d);
void redit_disp_exit_menu(struct descriptor_data * d);
void redit_disp_exit_flag_menu(struct descriptor_data * d);
void redit_disp_flag_menu(struct descriptor_data * d);
void redit_disp_sector_menu(struct descriptor_data * d);
void redit_disp_menu(struct descriptor_data * d);
void redit_parse(struct descriptor_data * d, char *arg);
void redit_setup_new(struct descriptor_data *d);
void redit_setup_existing(struct descriptor_data *d, int real_num);
void redit_save_to_disk(struct descriptor_data *d);
void redit_save_internally(struct descriptor_data *d);
void free_room(struct room_data *room);
void redit_disp_teleport_menu(struct descriptor_data *d);

/*------------------------------------------------------------------------*/

#define  W_EXIT(room, num) (world[(room)].dir_option[(num)])

/*-------------------------------------------------------------------*\
  permission checks 
\*-------------------------------------------------------------------*/

/* Here define flags which lower level builders (and clan builders) cannot */
/* change */
long_bitv room_flags_mask =
{
  ROOM_PRIVATE + ROOM_GODROOM + ROOM_HOUSE + ROOM_HOUSE_CRASH + ROOM_ATRIUM
  +ROOM_OLC + ROOM_BFS_MARK +ROOM_TELEPORT + ROOM_OWNER_ONLY + ROOM_DELETED
  +ROOM_WASTED,
  0,
  0, 
  0
};

long_bitv room_aff_flags_mask =
{
  0,
  0,
  0, 
  0
};

/*------------------------------------------------------------------------*\
  Utils and exported functions.
\*------------------------------------------------------------------------*/

void redit_setup_new(struct descriptor_data *d)
{ 
  struct teleport_data *new_tele;
  
  CREATE(OLC_ROOM(d), struct room_data, 1);
  /* set up teleporter */
  CREATE(new_tele, struct teleport_data, 1);
  OLC_ROOM(d)->tele = new_tele;
  OLC_ROOM(d)->tele->time = MIN_TELEPORT_FREQ;
  OLC_ROOM(d)->name = str_dup("An unfinished room");
  OLC_ROOM(d)->description = str_dup("You are in an unfinished room.\r\n");
  OLC_SPEC(d) = 0;
  redit_disp_menu(d);
  OLC_VAL(d) = 0;
}

/*------------------------------------------------------------------------*/

void redit_setup_existing(struct descriptor_data *d, int real_num)
{ struct room_data *room;
  struct teleport_data *new_tele;
  int counter;
  
  if (zone_table[world[real_num].zone].unloaded)
          reload_room_descs(real_num, FALSE);
  /*. Build a copy of the room .*/
  CREATE (room, struct room_data, 1);
  *room = world[real_num];
  /* allocate space for all strings  */
  if (world[real_num].name)
    room->name = str_dup (world[real_num].name);
  if (world[real_num].description)
    room->description = str_dup (world[real_num].description);

  /* exits - alloc only if necessary */
  for (counter = 0; counter < NUM_OF_DIRS; counter++)
  { if (world[real_num].dir_option[counter])
    { CREATE(room->dir_option[counter], struct room_direction_data, 1);
      /* copy numbers over */
      *room->dir_option[counter] = *world[real_num].dir_option[counter];
      /* malloc strings */
      if (world[real_num].dir_option[counter]->general_description)
        room->dir_option[counter]->general_description =
          str_dup(world[real_num].dir_option[counter]->general_description);
      if (world[real_num].dir_option[counter]->keyword)
        room->dir_option[counter]->keyword =
          str_dup(world[real_num].dir_option[counter]->keyword);
    }
  }
  
  /*. Extra descriptions if necessary .*/ 
  if (world[real_num].ex_description) 
  { struct extra_descr_data *this, *temp, *temp2;
    CREATE (temp, struct extra_descr_data, 1);
    room->ex_description = temp;
    for (this = world[real_num].ex_description; this; this = this->next)
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
  /* setup teleporter */
    CREATE(new_tele, struct teleport_data, 1);
    room->tele = new_tele;
    if (world[real_num].tele != NULL) {
        room->tele->targ = world[real_num].tele->targ;
        room->tele->time = world[real_num].tele->time;
        room->tele->mask = world[real_num].tele->mask;
        room->tele->cnt = world[real_num].tele->cnt;
        room->tele->obj = world[real_num].tele->obj;
    } else {
        room->tele->time = MIN_TELEPORT_FREQ;
    }
 
  room->orig_affections = world[real_num].orig_affections;
  room->room_affections = world[real_num].orig_affections;
  /*. Attatch room copy to players descriptor .*/
  OLC_ROOM(d) = room;
  OLC_VAL(d) = 0;
  OLC_SPEC(d) = get_spec_name(room_procs, room->func);
  redit_disp_menu(d);
}

/*------------------------------------------------------------------------*/
      
#define ZCMD (zone_table[zone].cmd[cmd_no])

void redit_save_internally(struct descriptor_data *d)
{ int i, j, room_num, found = 0, zone, cmd_no;
  struct room_data *new_world;
  struct char_data *temp_ch;
  struct obj_data *temp_obj;

  room_num = real_room(OLC_NUM(d));
  
  if (OLC_ROOM(d)->tele->targ == 0) {
        /* this is a null teleport! Remove it! */
        REMOVE_BIT(OLC_ROOM(d)->room_flags[0], ROOM_TELEPORT);
        free(OLC_ROOM(d)->tele);
        OLC_ROOM(d)->tele = NULL;
    }
  if (room_num > 0) 
  { /*. Room exits: move contents over then free and replace it .*/
    reload_room_descs(room_num, TRUE);
    OLC_ROOM(d)->contents = world[room_num].contents;
    OLC_ROOM(d)->people = world[room_num].people;
    free_room(world + room_num);
    world[room_num] = *OLC_ROOM(d);
  } else 
  { /*. Room doesn't exist, hafta add it .*/

    CREATE(new_world, struct room_data, top_of_world + 2);

    /* count thru world tables */
    for (i = 0; i <= top_of_world; i++) 
    { if (!found) {
        /*. Is this the place? .*/
        if (world[i].number > OLC_NUM(d)) 
	{ found = 1;

	  new_world[i] = *(OLC_ROOM(d));
	  new_world[i].number = OLC_NUM(d);
	  new_world[i].func = NULL;
          room_num  = i;
	
	  /* copy from world to new_world + 1 */
          new_world[i + 1] = world[i];
          /* people in this room must have their numbers moved */
	  for (temp_ch = world[i].people; temp_ch; temp_ch = temp_ch->next_in_room)
	    if (temp_ch->in_room != -1)
	      temp_ch->in_room = i + 1;

	  /* move objects */
	  for (temp_obj = world[i].contents; temp_obj; temp_obj = temp_obj->next_content)
	    if (temp_obj->in_room != -1)
	      temp_obj->in_room = i + 1;
        } else 
        { /*.   Not yet placed, copy straight over .*/
	  new_world[i] = world[i];
        }
      } else 
      { /*. Already been found  .*/
 
        /* people in this room must have their in_rooms moved */
        for (temp_ch = world[i].people; temp_ch; temp_ch = temp_ch->next_in_room)
	  if (temp_ch->in_room != -1)
	    temp_ch->in_room = i + 1;

        /* move objects */
        for (temp_obj = world[i].contents; temp_obj; temp_obj = temp_obj->next_content)
  	  if (temp_obj->in_room != -1)
	    temp_obj->in_room = i + 1;

        new_world[i + 1] = world[i];
      }
    }
    if (!found)
    { /*. Still not found, insert at top of table .*/
      new_world[i] = *(OLC_ROOM(d));
      new_world[i].number = OLC_NUM(d);
      new_world[i].func = NULL;
      room_num  = i;
    }

    /* copy world table over */
    free(world);
    world = new_world;
    top_of_world++;
    
    world[room_num].zone = OLC_ZNUM(d);
    reload_room_descs(room_num, TRUE);
    
    /*. Update zone table .*/
    for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
        switch (ZCMD.command)
        {
          case 'M':
          case 'O':
            if (ZCMD.arg3 >= room_num)
              ZCMD.arg3++;
 	    break;
          case 'D':
          case 'R':
            if (ZCMD.arg1 >= room_num)
              ZCMD.arg1++;
          case 'G':
          case 'P':
          case 'E':
          case '*':
            break;
          default:
            mudlog("SYSERR: OLC: redit_save_internally: Unknown comand", BRF, LVL_BUILDER, TRUE);
        }

    /* update load rooms, to fix creep	ing load room problem */
    if (room_num <= r_mortal_start_room)
      r_mortal_start_room++;
    if (room_num <= r_immort_start_room)
      r_immort_start_room++;
    if (room_num <= r_frozen_start_room)
      r_frozen_start_room++;

    /*. Update world exits .*/
    for (i = 0; i < top_of_world + 1; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
        if (W_EXIT(i, j))
          if (W_EXIT(i, j)->to_room >= room_num)
	    W_EXIT(i, j)->to_room++;

  }
  
  world[room_num].func = room_procs[OLC_SPEC(d)].sp_pointer;
  
  world[room_num].room_affections = OLC_ROOM(d)->orig_affections;
  world[room_num].orig_affections = OLC_ROOM(d)->orig_affections;
  
 
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ROOM);
}


/*------------------------------------------------------------------------*/

void redit_save_to_disk(struct descriptor_data *d)
{
  int counter, counter2, realcounter;
  FILE *fp;
  struct room_data *room;
  struct extra_descr_data *ex_desc;

  sprintf(buf, "%s/%d.wld", WLD_PREFIX, zone_table[OLC_ZNUM(d)].number);
  if (!(fp = fopen(buf, "w+")))
  { mudlog("SYSERR: OLC: Cannot open room file!", BRF, LVL_BUILDER, TRUE);
    return;
  }

  for (counter = zone_table[OLC_ZNUM(d)].number * 100;
       counter <= zone_table[OLC_ZNUM(d)].top;
       counter++) 
  { realcounter = real_room(counter);
    if (realcounter >= 0 && !ROOM_FLAGGED(realcounter, ROOM_DELETED)) 
    { 
      room = (world + realcounter);

      /*. Remove the '\r\n' sequences from description .*/
      strcpy(buf1, room->description ? room->description : "Empty");
      strip_string(buf1);

      /*. Build a buffer ready to save .*/
      sprintf(buf, "#%d\n%s~\n%s~\n%d %ld %ld %ld %ld %d\n",
      		counter, room->name ? room->name : "undefined", buf1,
      		zone_table[room->zone].number,
      		room->room_flags[0], room->room_flags[1], 
      		room->room_flags[2], room->room_flags[3], 
      		room->sector_type
      );
      /*. Save this section .*/
      fputs(buf, fp);

      /*. Handle exits .*/
      for (counter2 = 0; counter2 < NUM_OF_DIRS; counter2++) 
      { if (room->dir_option[counter2]) 
        { int temp_door_flag;

          /*. Again, strip out the crap .*/
          if (room->dir_option[counter2]->general_description)
          { strcpy(buf1, room->dir_option[counter2]->general_description);
            strip_string(buf1);
          } else
          *buf1 = 0;

          /*. Figure out door flag .*/
          if (IS_SET(room->dir_option[counter2]->exit_info, EX_ISDOOR)) 
          { if (IS_SET(room->dir_option[counter2]->exit_info, EX_PICKPROOF))
	      temp_door_flag = 2;
	    else
	      temp_door_flag = 1;
	  } else
	      temp_door_flag = 0;

          /*. Check for keywords .*/
          if(room->dir_option[counter2]->keyword)
            strcpy(buf2, room->dir_option[counter2]->keyword);
          else
            *buf2 = 0;
               
          /*. Ok, now build a buffer to output to file .*/
          sprintf(buf, "D%d\n%s~\n%s~\n%d %d %d\n",
                        counter2, buf1, buf2, temp_door_flag,
			room->dir_option[counter2]->key,
			room->dir_option[counter2]->to_room != -1 ? 
			world[room->dir_option[counter2]->to_room].number : -1
          );
          /*. Save this door .*/
	  fputs(buf, fp);
        }
      }
      if (room->ex_description) 
      { for (ex_desc = room->ex_description; ex_desc; ex_desc = ex_desc->next) 
        { /*. Home straight, just deal with extras descriptions..*/
          strcpy(buf1, ex_desc->description);
          strip_string(buf1);
          sprintf(buf, "E\n%s~\n%s~\n", ex_desc->keyword,buf1);
          fputs(buf, fp);
	}
      }
      /* save teleport info */
      if ((room->tele != NULL) && (room->tele->targ > 0)) {
//        sprintbit(room->tele->mask, teleport_bits, buf1);
        sprintf(buf, "T\n%d %ld %d %d\n",
                room->tele->targ,
                room->tele->mask,
                room->tele->time,
                room->tele->obj);
        fputs(buf, fp);
        
      
      }
      
      /* save room affections */    
      if (room->orig_affections != 0)        
        fprintf(fp, "A\n%ld\n", room->orig_affections);
      
      /* save spec proc */
      if (room->func != NULL)
          fprintf(fp, "P\n%s\n", 
            room_procs[get_spec_name(room_procs, room->func)].name);
  
      fprintf(fp, "S\n");
    }
  }
  /* write final line and close */
  fprintf(fp, "$~\n");
  fclose(fp);
  olc_remove_from_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_ROOM);
  unlock_room_descs(OLC_ZNUM(d));
}

/*------------------------------------------------------------------------*/

void free_room(struct room_data *room)
{ int i;
  struct extra_descr_data *this, *next;

  if (room->name)
    free(room->name);
  if (room->description)
    free(room->description);
  if (room->tele) free(room->tele);
  /*. Free exits .*/
  for (i = 0; i < NUM_OF_DIRS; i++)
  { if (room->dir_option[i])
    { if (room->dir_option[i]->general_description)
        free(room->dir_option[i]->general_description);
      if (room->dir_option[i]->keyword)
        free(room->dir_option[i]->keyword);
    }
    free(room->dir_option[i]);
  }

  /*. Free extra descriptions .*/
  for (this = room->ex_description; this; this = next)
  { next = this->next;
    if (this->keyword)
      free(this->keyword);
    if (this->description)
      free(this->description);
    free(this);
  }
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/* For extra descriptions */
void redit_disp_extradesc_menu(struct descriptor_data * d)
{
  struct extra_descr_data *extra_desc = OLC_DESC(d);
  
  sprintf(buf, "[H[J"
  	"%s1%s) Keyword: %s%s\r\n"
  	"%s2%s) Description:\r\n%s%s\r\n"
        "%s3%s) Goto next description: ",

	grn, nrm, yel, extra_desc->keyword ? extra_desc->keyword : "<NONE>",
	grn, nrm, yel, extra_desc->description ?  extra_desc->description : "<NONE>",
        grn, nrm
  );
  
  if (!extra_desc->next)
    strcat(buf, "<NOT SET>\r\n");
  else
    strcat(buf, "Set.\r\n");
  strcat(buf, "Enter choice (0 to quit, ? to help) : ");
  send_to_char(buf, d->character);
  OLC_MODE(d) = REDIT_EXTRADESC_MENU;
}

/* For exits */
void redit_disp_exit_menu(struct descriptor_data * d)
{
  /* if exit doesn't exist, alloc/create it */
  if(!OLC_EXIT(d)) {
    CREATE(OLC_EXIT(d), struct room_direction_data, 1);
    OLC_EXIT(d)->to_room = -1;
  }

  /* weird door handling! */
  if (IS_SET(OLC_EXIT(d)->exit_info, EX_ISDOOR)) {
    if (IS_SET(OLC_EXIT(d)->exit_info, EX_PICKPROOF))
      strcpy(buf2, "Pickproof");
    else
      strcpy(buf2, "Is a door");
  } else
    strcpy(buf2, "No door");

  get_char_cols(d->character);
  sprintf(buf, "[H[J"
	"%s1%s) Exit to     : %s%d\r\n"
	"%s2%s) Description :-\r\n%s%s\r\n"
  	"%s3%s) Door name   : %s%s\r\n"
  	"%s4%s) Key         : %s%d\r\n"
  	"%s5%s) Door flags  : %s%s\r\n"
  	"%s6%s) Purge exit.\r\n"
	"Enter choice (0 to quit, ? to help) : ",

	grn, nrm, cyn, OLC_EXIT(d)->to_room != -1 ? 
	  world[OLC_EXIT(d)->to_room].number : -1,
        grn, nrm, yel, OLC_EXIT(d)->general_description ? OLC_EXIT(d)->general_description : "<NONE>",
	grn, nrm, yel, OLC_EXIT(d)->keyword ? OLC_EXIT(d)->keyword : "<NONE>",
	grn, nrm, cyn, OLC_EXIT(d)->key,
	grn, nrm, cyn, buf2, grn, nrm
  );

  send_to_char(buf, d->character);
  OLC_MODE(d) = REDIT_EXIT_MENU;
}

/* For exit flags */
void redit_disp_exit_flag_menu(struct descriptor_data * d)
{
  get_char_cols(d->character);
  sprintf(buf,  "%s0%s) No door\r\n"
  		"%s1%s) Closeable door\r\n"
		"%s2%s) Pickproof\r\n"
  		"Enter choice (? to help): ",
		grn, nrm, grn, nrm, grn, nrm
  );
  send_to_char(buf, d->character);
}

/* For room flags */
void redit_disp_flag_menu(struct descriptor_data * d)
{ 

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  menubit(OLC_ROOM(d)->room_flags, room_bits, buf, 
    CHECK_MASK_PERM(d, room_flags_mask));
  send_to_char(buf, d->character);
  sprintbit_multi(OLC_ROOM(d)->room_flags, room_bits, buf1);
  sprintf(buf, 
	"\r\nRoom flags: %s%s%s\r\n"
  	"Enter room flags (0 to quit, ? to help) : ",
	cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
  OLC_MODE(d) = REDIT_FLAGS;
}

/* for sector type */
void redit_disp_sector_menu(struct descriptor_data * d)
{ int counter, columns = 0;

  send_to_char("[H[J", d->character);
  for (counter = 0; counter < NUM_ROOM_SECTORS; counter++) {
    sprintf(buf, "%s%2d%s) %-20.20s ",
	    grn, counter, nrm, sector_types[counter]);
    if(!(++columns % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  send_to_char("\r\nEnter sector type (? to help): ", d->character);
  OLC_MODE(d) = REDIT_SECTOR;
}

/* For teleport flags */
void redit_disp_teleport_menu(struct descriptor_data *d) {
        int counter, columns = 0;

        get_char_cols(d->character);
        send_to_char("[H[J", d->character);
        for (counter = 0; counter < NUM_TELEPORT; counter ++) {
          sprintf(buf, "%s%2d%s) %-20.20s ",
                  grn, counter + 1, nrm, teleport_bits[counter]);
          if(!(++columns % 2))
                strcat(buf, "\r\n");
          send_to_char(buf, d->character);
        }
        sprintbit(OLC_ROOM(d)->tele->mask, teleport_bits, buf1);
        sprintf(buf,
                "\r\nTeleport flags, %s%s%s\r\n"
                "Enter teleport flags (0 to quit, ? to help) : ",
                cyn, buf1, nrm );
        send_to_char(buf, d->character);
        OLC_MODE(d) = REDIT_TELEPORT_MENU;
}

/* room affect menu */
void redit_disp_room_aff_menu(struct descriptor_data * d)
{
  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  menubit(&OLC_ROOM(d)->orig_affections, room_affections, buf, 
    CHECK_MASK_PERM(d, room_aff_flags_mask));
  send_to_char(buf, d->character);
  sprintbit(OLC_ROOM(d)->orig_affections, room_affections, buf1);
  sprintf(buf,
                "\r\nRoom affection flags, %s%s%s\r\n"
                "Enter room affection flags (0 to quit, ? to help) : ",
                cyn, buf1, nrm );
  send_to_char(buf, d->character);
  OLC_MODE(d) = REDIT_ROOM_AFFECT;
}

void redit_disp_tele_main_menu(struct descriptor_data * d)
{
  sprintbit((long) OLC_ROOM(d)->tele->mask, teleport_bits, buf1);
  sprintf(buf,
  	"[H[J"
  	"&WTeleport editor (Room: &g%d&W)&w\r\n"
  	"%s1%s) Tele-Target : %s%s (Set to '0' (The Void) to disable)\r\n"
        "%s2%s) Tele-Freq   : %s%d*10 seconds\r\n"
        "%s3%s) Tele-Flags  : %s%s\r\n"
        "%s4%s) Tele-Obj    : %s%d (%s flag is set)\r\n"
        "%sEnter choice (0 to quit, ? for help) : ",
        OLC_NUM(d),
        grn, nrm, cyn, world[real_room(OLC_ROOM(d)->tele->targ)].name,
        grn, nrm, cyn, OLC_ROOM(d)->tele->time,
        grn, nrm, cyn, buf1,
        grn, nrm, cyn, OLC_ROOM(d)->tele->obj,
          (IS_SET(OLC_ROOM(d)->tele->mask, TELE_NOOBJ) ? 
            "if not have Obj" : "if has Obj"),
        nrm
  );
  send_to_char(buf, d->character);
  OLC_MODE(d) = REDIT_TELE_MAIN_MENU;
}

/* the main menu */
void redit_disp_menu(struct descriptor_data * d)
{ struct room_data *room;
  char b1[32], b2[32], b3[32], b4[32], b5[32], b6[32];
  char buf4[MAX_STRING_LENGTH];
  char local_buf[80];
  get_char_cols(d->character);
  room = OLC_ROOM(d);
  
  if (OLC_CLAN_EDIT(d)) {
    sprintf(local_buf, " &Y(%ld gold/mudmonth)&w", room_account(OLC_ROOM(d)));
  }
  
  if (room->dir_option[NORTH] != 0) 
    sprintf(b1, "%d", room->dir_option[NORTH]->to_room == -1 ? 
      -1 : world[room->dir_option[NORTH]->to_room].number);
  else
    strcpy(b1, NONESTR);
    
  if (room->dir_option[EAST] != 0) 
    sprintf(b2, "%d", room->dir_option[EAST]->to_room == -1 ? 
      -1 : world[room->dir_option[EAST]->to_room].number);
  else
    strcpy(b2, NONESTR);
    
  if (room->dir_option[SOUTH] != 0) 
    sprintf(b3, "%d", room->dir_option[SOUTH]->to_room == -1 ? 
      -1 : world[room->dir_option[SOUTH]->to_room].number);
  else
    strcpy(b3, NONESTR);  	
    
  if (room->dir_option[WEST] != 0) 
    sprintf(b4, "%d", room->dir_option[WEST]->to_room == -1 ? 
      -1 : world[room->dir_option[WEST]->to_room].number);
  else
    strcpy(b4, NONESTR);
   
  if (room->dir_option[UP]) 
    sprintf(b5, "%d", room->dir_option[UP]->to_room == -1 ? 
      -1 : world[room->dir_option[UP]->to_room].number);
  else
    strcpy(b5, NONESTR);
  
  if (room->dir_option[DOWN]) 
    sprintf(b6, "%d", room->dir_option[DOWN]->to_room == -1 ? 
      -1 : world[room->dir_option[DOWN]->to_room].number);
  else
    strcpy(b6, NONESTR);
    
  sprintbit_multi((long *) room->room_flags, room_bits, buf1);
  sprinttype(room->sector_type, sector_types, buf2);
  
  sprintbit((long) room->orig_affections, room_affections, buf4);
  
  sprintf(buf,
  	"[H[J"
	"-- Room number : [%s%d%s]  	Room zone: [%s%d%s]\r\n"
	"%s1%s) Name        : %s%s\r\n"
	"%s2%s) Description :\r\n%s%s"
  	"%s3%s) Room flags  : %s%s\r\n"
	"%s4%s) Sector type : %s%s\r\n"
  	"%s5%s) Exit north  : %s%-10s %s6%s) Exit east   : %s%s\r\n"
  	"%s7%s) Exit south  : %s%-10s %s8%s) Exit west   : %s%s\r\n"
  	"%s9%s) Exit up     : %s%-10s %sA%s) Exit down   : %s%s\r\n"
  	"%sB%s) Extra descriptions menu\r\n"
  	"%sC%s) Teleport: %s%s\r\n"
        "%sD%s) Spec-proc   : %s%s\r\n"
        "%sE%s) Room affect : %s%s\r\n"
  	"%sQ%s) Quit%s%s\r\n"
  	"Enter choice (? to help) : ",

	cyn, OLC_NUM(d), nrm,
	cyn, zone_table[OLC_ZNUM(d)].number, nrm,
	grn, nrm, yel, room->name,
	grn, nrm, yel, room->description,
	grn, nrm, cyn, buf1,
	grn, nrm, cyn, buf2,
  	grn, nrm, cyn, b1,
	grn, nrm, cyn, b2,
  	grn, nrm, cyn, b3,
  	grn, nrm, cyn, b4,
  	grn, nrm, cyn, b5,
  	grn, nrm, cyn, b6,
        grn, nrm, 
        grn, nrm, cyn, (room->tele->targ > 0) ? "YES" : "NO",
        grn, nrm, cyn, room_procs[OLC_SPEC(d)].name,
        grn, nrm, cyn, buf4,
        grn, nrm,
        OLC_CLAN_EDIT(d) ? local_buf : "",
        OLC_READONLY(d) ? " &R(Read-Only)&w" : ""
  );
  send_to_char(buf, d->character);

  OLC_MODE(d) = REDIT_MAIN_MENU;
}



/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse(struct descriptor_data * d, char *arg)
{ //extern struct room_data *world;
  int number;
  /* First check help */
  if (arg && *arg) {
    two_arguments(arg, buf, buf1);
    if (str_cmp(buf, "?") == 0) {
      OLC_LAST(d) = OLC_MODE(d);
      OLC_MODE(d) = REDIT_HELP;
      if (*buf1 != '\0')
        do_help(d->character, buf1, 0, 0);
      else
      switch (OLC_LAST(d)) {  /* Here define help invokations for input modes */
        case REDIT_MAIN_MENU:
          do_help(d->character, "REDIT MAIN MENU", 0, 0);      
          break;
        case REDIT_FLAGS:
          do_help(d->character, "ROOM FLAGS", 0, 0);      
          break;
        case REDIT_SECTOR:
          do_help(d->character, "ROOM SECTOR", 0, 0);      
          break;
        case REDIT_EXIT_MENU:
          do_help(d->character, "ROOM EXIT", 0, 0);      
          break;
        case REDIT_EXIT_DOORFLAGS:
          do_help(d->character, "DOOR FLAGS", 0, 0);      
          break;
        case REDIT_TELE_MAIN_MENU:
          do_help(d->character, "REDIT TELEPORT", 0, 0);      
          break;
        case REDIT_TELEPORT_MENU:
          do_help(d->character, "TELEPORT FLAGS", 0, 0);      
          break;
        case REDIT_ROOM_AFFECT:
          do_help(d->character, "ROOM AFFECT", 0, 0);      
          break;
        case REDIT_EXTRADESC_MENU:
          do_help(d->character, "EXTRA DESCRIPTION", 0, 0);
          break;      
        default:  /* Do nothing */
          OLC_MODE(d) = OLC_LAST(d);
          break;
      }  
    }
    if (OLC_MODE(d) == REDIT_HELP) return;
  }

  switch (OLC_MODE(d)) {
  case REDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      redit_save_internally(d);
      sprintf(buf, "OLC: %s edits room %d", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, CMP, LVL_BUILDER, TRUE);
      /*. Do NOT free strings! just the room structure .*/
      cleanup_olc(d, CLEANUP_STRUCTS);
      send_to_char("Room saved to memory.\r\n", d->character);
      break;
    case 'n':
    case 'N':
      /* free everything up, including strings etc */
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save this room internally? : ", d->character);
      break;
    }
    return;

  case REDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (!OLC_READONLY(d) && OLC_VAL(d))
      { /*. Something has been modified .*/
        send_to_char("Do you wish to save this room internally? : ", d->character);
        OLC_MODE(d) = REDIT_CONFIRM_SAVESTRING;
      } else
        cleanup_olc(d, CLEANUP_ALL);
      return;
    case '1':
      send_to_char("Enter room name:-\r\n| ", d->character);
      OLC_MODE(d) = REDIT_NAME;
      break;
    case '2':
      OLC_MODE(d) = REDIT_DESC;
	 SEND_TO_Q("\x1B[H\x1B[J", d);
	 SEND_TO_Q("Enter room description: (/s saves /h for help)\r\n\r\n", d);
	 d->backstr = NULL;
	 if (OLC_ROOM(d)->description) {
	    SEND_TO_Q(OLC_ROOM(d)->description, d);
	    d->backstr = str_dup(OLC_ROOM(d)->description);
	 }
	 d->str = &OLC_ROOM(d)->description;
      d->max_str = MAX_ROOM_DESC;
      d->mail_to = 0;
      OLC_VAL(d) = 1;
      break;
    case '3':
      redit_disp_flag_menu(d);
      break;
    case '4':
      redit_disp_sector_menu(d);
      break;
    case '5':
      OLC_VAL(d) = NORTH;
      redit_disp_exit_menu(d);
      break;
    case '6':
      OLC_VAL(d) = EAST;
      redit_disp_exit_menu(d);
      break;
    case '7':
      OLC_VAL(d) = SOUTH;
      redit_disp_exit_menu(d);
      break;
    case '8':
      OLC_VAL(d) = WEST;
      redit_disp_exit_menu(d);
      break;
    case '9':
      OLC_VAL(d) = UP;
      redit_disp_exit_menu(d);
      break;
    case 'a':
    case 'A':
      OLC_VAL(d) = DOWN;
      redit_disp_exit_menu(d);
      break;
    case 'b':
    case 'B':
      /* if extra desc doesn't exist . */
      if (!OLC_ROOM(d)->ex_description) {
	CREATE(OLC_ROOM(d)->ex_description, struct extra_descr_data, 1);
	OLC_ROOM(d)->ex_description->next = NULL;
      }
      OLC_DESC(d) = OLC_ROOM(d)->ex_description;
      redit_disp_extradesc_menu(d);
      break;
    case 'c':
    case 'C':
      if (!OLC_CLAN_EDIT(d)) {
        redit_disp_tele_main_menu(d);
        break;
      } else {
        redit_disp_menu(d);
        return;
      }
    
    case 'd':
    case 'D':
      if (!OLC_CLAN_EDIT(d)) {
        OLC_MODE(d) = REDIT_SPEC_PROC;
        olc_disp_spec_proc_menu(d, room_procs);
        return;
      }
      redit_disp_menu(d);
      return;
    case 'e':
    case 'E':
      redit_disp_room_aff_menu(d);
      return;
    default:
      send_to_char("Invalid choice!", d->character);
      redit_disp_menu(d);
      break;
    }
    return;

  case REDIT_NAME:
    if (OLC_ROOM(d)->name)
      free(OLC_ROOM(d)->name);
    if (strlen(arg) > MAX_ROOM_NAME)
      arg[MAX_ROOM_NAME -1] = 0;
    OLC_ROOM(d)->name = str_dup(arg);
    break;
  case REDIT_DESC:
    /* we will NEVER get here */
    mudlog("SYSERR: Reached REDIT_DESC case in parse_redit",BRF,LVL_BUILDER,TRUE);
    break;

  case REDIT_FLAGS:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ROOM_FLAGS)) {
      send_to_char("That's not a valid choice!\r\n", d->character);
      redit_disp_flag_menu(d);
    } else {
      if (number == 0)
        break;
      else {
        updatebit(OLC_ROOM(d)->room_flags, NUM_ROOM_FLAGS, number, 
          
          CHECK_MASK_PERM(d, room_flags_mask));
	redit_disp_flag_menu(d);
      }
    }
    return;
    
  case REDIT_ROOM_AFFECT:
    number = atoi(arg);
    if ((number < 0) || (number > NUM_ROOM_AFF)) {
      send_to_char("That's not a valid choice!\r\n", d->character);
      redit_disp_room_aff_menu(d);
    } else {
      if (number == 0)
        break;
      else {
        updatebit(&(OLC_ROOM(d)->orig_affections), NUM_ROOM_AFF, number,
          CHECK_MASK_PERM(d, room_aff_flags_mask));
//        TOGGLE_BIT(OLC_ROOM(d)->orig_affections, 1 << (number - 1));
	redit_disp_room_aff_menu(d);
      }
    }
    return;

  case REDIT_SECTOR:
    if (*arg == '\0') break;
    number = atoi(arg);
    if (number < 0 || number >= NUM_ROOM_SECTORS) {
      send_to_char("Invalid choice!", d->character);
      redit_disp_sector_menu(d);
      return;
    } else 
      OLC_ROOM(d)->sector_type = number;
    break;

  case REDIT_EXIT_MENU:
    switch (*arg) {
    case '0':
      break;
    case '1':
      OLC_MODE(d) = REDIT_EXIT_NUMBER;
      send_to_char("Exit to room number : ", d->character);
      return;
    case '2':
      OLC_MODE(d) = REDIT_EXIT_DESCRIPTION;
       SEND_TO_Q("Enter exit description: (/s saves /h for help)\r\n\r\n", d);
       d->backstr = NULL;
       if (OLC_EXIT(d)->general_description) {
	  SEND_TO_Q(OLC_EXIT(d)->general_description, d);
	  d->backstr = str_dup(OLC_EXIT(d)->general_description);
       }
       d->str = &OLC_EXIT(d)->general_description;
      d->max_str = MAX_EXIT_DESC;
      d->mail_to = 0;
      return;
    case '3':
      OLC_MODE(d) = REDIT_EXIT_KEYWORD;
      send_to_char("Enter keywords : ", d->character);
      return;
    case '4':
      OLC_MODE(d) = REDIT_EXIT_KEY;
      send_to_char("Enter key number : ", d->character);
      return;
    case '5':
      redit_disp_exit_flag_menu(d);
      OLC_MODE(d) = REDIT_EXIT_DOORFLAGS;
      return;
    case '6':
      /* delete exit */
      if (OLC_EXIT(d)->keyword)
	free(OLC_EXIT(d)->keyword);
      if (OLC_EXIT(d)->general_description)
	free(OLC_EXIT(d)->general_description);
      free(OLC_EXIT(d));
      OLC_EXIT(d) = NULL;
      break;
    default:
      send_to_char("Try again : ", d->character);
      return;
    }
    break;

  case REDIT_EXIT_NUMBER:
    number = (atoi(arg));
    if (number != -1)
    { number = real_room(number);
      if (number < 0)
      { send_to_char("That room does not exist, try again : ", d->character);
        return;
      }
    }
    OLC_EXIT(d)->to_room = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DESCRIPTION:
    /* we should NEVER get here */
    mudlog("SYSERR: Reached REDIT_EXIT_DESC case in parse_redit",BRF,LVL_BUILDER,TRUE);
    break;

  case REDIT_EXIT_KEYWORD:
    if (OLC_EXIT(d)->keyword)
      free(OLC_EXIT(d)->keyword);
    OLC_EXIT(d)->keyword = str_dup(arg);
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_KEY:
    number = atoi(arg);
    OLC_EXIT(d)->key = number;
    redit_disp_exit_menu(d);
    return;

  case REDIT_EXIT_DOORFLAGS:
    number = atoi(arg);
    if ((number < 0) || (number > 2)) {
      send_to_char("That's not a valid choice!\r\n", d->character);
      redit_disp_exit_flag_menu(d);
    } else {
      /* doors are a bit idiotic, don't you think? :) */
      if (number == 0)
	OLC_EXIT(d)->exit_info = 0;
      else if (number == 1)
	OLC_EXIT(d)->exit_info = EX_ISDOOR;
      else if (number == 2)
	OLC_EXIT(d)->exit_info = EX_ISDOOR | EX_PICKPROOF;
      /* jump out to menu */
      redit_disp_exit_menu(d);
    }
    return;

  case REDIT_EXTRADESC_KEY:
    OLC_DESC(d)->keyword = str_dup(arg);
    redit_disp_extradesc_menu(d);
    return;

  case REDIT_EXTRADESC_MENU:
    number = atoi(arg);
    switch (number) {
    case 0:
      {
	/* if something got left out, delete the extra desc
	 when backing out to menu */
	if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) 
        { struct extra_descr_data **tmp_desc;

	  if (OLC_DESC(d)->keyword)
	    free(OLC_DESC(d)->keyword);
	  if (OLC_DESC(d)->description)
	    free(OLC_DESC(d)->description);

	  /*. Clean up pointers .*/
	  for(tmp_desc = &(OLC_ROOM(d)->ex_description); *tmp_desc;
	      tmp_desc = &((*tmp_desc)->next))
          { if(*tmp_desc == OLC_DESC(d))
	    { *tmp_desc = NULL;
              break;
	    }
	  }
	  free(OLC_DESC(d));
	}
      }
      break;
    
    case 1:
      OLC_MODE(d) = REDIT_EXTRADESC_KEY;
      send_to_char("Enter keywords, separated by spaces : ", d->character);
      return;
    case 2:
      OLC_MODE(d) = REDIT_EXTRADESC_DESCRIPTION;
	 SEND_TO_Q("Enter extra description: (/s saves /h for help)\r\n\r\n", d);
	 d->backstr = NULL;
	 if (OLC_DESC(d)->description) {
	    SEND_TO_Q(OLC_DESC(d)->description, d);
	    d->backstr = str_dup(OLC_DESC(d)->description);
	 }
	 d->str = &OLC_DESC(d)->description;
	 d->max_str = MAX_MESSAGE_LENGTH;
      d->mail_to = 0;
      return;

    case 3:
      if (!OLC_DESC(d)->keyword || !OLC_DESC(d)->description) {
	send_to_char("You can't edit the next extra desc without completing this one.\r\n", d->character);
	redit_disp_extradesc_menu(d);
      } else {
	struct extra_descr_data *new_extra;

	if (OLC_DESC(d)->next)
	  OLC_DESC(d) = OLC_DESC(d)->next;
	else {
	  /* make new extra, attach at end */
	  CREATE(new_extra, struct extra_descr_data, 1);
	  OLC_DESC(d)->next = new_extra;
	  OLC_DESC(d) = new_extra;
	}
	redit_disp_extradesc_menu(d);
      }
      return;
    }
    break;
  case REDIT_TELE_MAIN_MENU:
    number = atoi(arg);
    switch (number) {
      case 1:
        send_to_char("Enter target vnum: ", d->character);
        OLC_MODE(d) = REDIT_TELEPORT_TARGET;
        return;
      case 2:
        send_to_char("Enter teleport frequency: ", d->character);
        OLC_MODE(d) = REDIT_TELEPORT_FREQ;
        return;
      case 3:
        redit_disp_teleport_menu(d);
        return;
      case 4:
        send_to_char("Enter Object's Vnum: ", d->character);
        OLC_MODE(d) = REDIT_TELEPORT_OBJ;
        return;
    }
    break;
    
  case REDIT_TELEPORT_TARGET:
    if (isdigit(*arg)) {
        number = atoi(arg);
        if (real_room(number) != -1) {
                if (number == 0) {
                        /* remove TELEPORT flag from room */
                        REMOVE_BIT(OLC_ROOM(d)->room_flags[0], ROOM_TELEPORT);
                } else {
                        SET_BIT(OLC_ROOM(d)->room_flags[0], ROOM_TELEPORT);
                }
                OLC_ROOM(d)->tele->targ = number;
                redit_disp_tele_main_menu(d);
                return;
        }
    }
    send_to_char("That was not a valid room, please try again: ", d->character);
    return;
  case REDIT_TELEPORT_FREQ:
    if (isdigit(*arg)) {
        number = atoi(arg);
        if (number >= MIN_TELEPORT_FREQ && number <= MAX_TELEPORT_FREQ) {
                OLC_ROOM(d)->tele->time = number;
                redit_disp_tele_main_menu(d);
                return;
        }
    }
    sprintf(buf, "That was not a valid number.\r\n"
            "Please enter a frequency between %d and %d: ",
            MIN_TELEPORT_FREQ, MAX_TELEPORT_FREQ);
    send_to_char(buf, d->character);
    return;
  case REDIT_TELEPORT_OBJ:
    if (isdigit(*arg)) {
        number = atoi(arg);
        if (real_object(number) != -1) {
                OLC_ROOM(d)->tele->obj = number;
                redit_disp_tele_main_menu(d);
                return;
        }
    }
    send_to_char("That was not a valid number, please enter the object's VNUM: ", d->character);
    return;
  case REDIT_TELEPORT_MENU:
    number = atoi(arg);
    if (number < 0 || number > NUM_TELEPORT) {
                redit_disp_teleport_menu(d);
    } else {
          if (number == 0) {
                redit_disp_tele_main_menu(d);
                return;
          } else {
                if (IS_SET(OLC_ROOM(d)->tele->mask, 1 << (number - 1)))
                        REMOVE_BIT(OLC_ROOM(d)->tele->mask, 1 << (number - 1));
                else
                        SET_BIT(OLC_ROOM(d)->tele->mask, 1 << (number - 1));
                redit_disp_teleport_menu(d);
          }
    }
    return;
  case REDIT_SPEC_PROC:
    if (*arg == '\0') break;
    OLC_SPEC(d) = 
      get_spec_proc_index(room_procs, GET_LEVEL(d->character), atoi(arg));
    break;
  /*-------------------------------------------------------------------*/
  /* This is return from help */
  case REDIT_HELP:
    OLC_MODE(d) = OLC_LAST(d);
    switch (OLC_MODE(d)) {
      case REDIT_MAIN_MENU:
        redit_disp_menu(d);
        break;
      case REDIT_FLAGS:
        redit_disp_flag_menu(d);
        break;
      case REDIT_SECTOR:
        redit_disp_sector_menu(d);
        break;
      case REDIT_EXIT_MENU:
        redit_disp_exit_menu(d);
        break;
      case REDIT_EXIT_DOORFLAGS:
        redit_disp_exit_flag_menu(d);
        break;
      case REDIT_TELE_MAIN_MENU:
        redit_disp_tele_main_menu(d);
        break;
      case REDIT_TELEPORT_MENU:
        redit_disp_teleport_menu(d);
        break;
      case REDIT_ROOM_AFFECT:
        redit_disp_room_aff_menu(d);
        break;
      default:
        log("SYSERR: Help topic with undefined return in redit (mode = %d)", OLC_MODE(d));
        break;
    }
    return;
  default:
    /* we should never get here */
    mudlog("SYSERR: Reached default case in parse_redit",BRF,LVL_BUILDER,TRUE);
    break;
  }
  /*. If we get this far, something has be changed .*/
  OLC_VAL(d) = 1;
  redit_disp_menu(d);
}

