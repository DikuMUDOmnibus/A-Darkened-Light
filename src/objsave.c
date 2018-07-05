/* ************************************************************************
*   File: objsave.c                                     Part of CircleMUD *
*  Usage: loading/saving player objects & aliases for rent and crash-save *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*  Written by Yilard/VisionMUD  (Code cleanup 09/02/1998)                 *
************************************************************************ */

/* #define DEBUG_SAVE */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

/* these factors should be unique integers */
#define RENT_FACTOR 	1
#define CRYO_FACTOR 	4

extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct obj_data *obj_proto;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int min_rent_cost;
extern struct obj_data *obj_proto;		/* in db.c		 */
extern int rent_file_timeout, crash_file_timeout;
extern int free_rent;
extern int max_obj_save;	/* change in config.c */
int read_obj = FALSE;

/* (YIL) Added allowed extern variables here for load_world_settings() */
extern int circle_restrict;

/* Extern functions */
ACMD(do_tell);
ACMD(do_action);
SPECIAL(receptionist);
SPECIAL(cryogenicist);

/* Global variables */
int num_objs;

/* local functions */
int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist, int display, int factor);
int Crash_report_unrentables(struct char_data * ch, struct char_data * recep, struct obj_data * obj);
void Crash_report_rent(struct char_data * ch, struct char_data * recep, struct obj_data * obj, long *cost, long *nitems, int display, int factor);
struct obj_data *Obj_from_store(struct obj_file_elem object);
int Obj_to_store(struct obj_data * obj, FILE * fl);
void update_obj_file(void);
int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct rent_info * rent);
int gen_receptionist(struct char_data * ch, struct char_data * recep, int cmd, char *arg, int mode);
int Crash_save(struct obj_data * obj, FILE * fp);
void Crash_rent_deadline(struct char_data * ch, struct char_data * recep, long cost);
void Crash_restore_weight(struct obj_data * obj);
void Crash_extract_objs(struct obj_data * obj);
int Crash_is_unrentable(struct obj_data * obj);
void Crash_extract_norents(struct obj_data * obj);
void Crash_extract_expensive(struct obj_data * obj);
void Crash_calculate_rent(struct obj_data * obj, int *cost);
void Crash_rentsave(struct char_data * ch, int cost);
void Crash_cryosave(struct char_data * ch, int cost);

/* Function protos */
int Advanced_Load(struct char_data * ch);
int Advanced_Save(struct char_data * ch, FILE * fp, int save_norents,
  struct rent_info *rent);

#define OBJ_TERMINATE	100	/* Must be different from any WEAR position */
#define OBJ_INVENTORY	101	/* Must be different from any WEAR position */
#define OBJ_CONTENT	102	/* Must be different from any WEAR position */


void report_items(struct char_data *ch, struct char_data *mailman, struct obj_data * obj, 
  int *items, int *uitems)
{
  char local_buf[256];
  if (obj) {
    (*items)++;
    if (Crash_is_unrentable(obj)) {
      sprintf(local_buf, "$n tells you, 'You cannot mail %s.'", OBJS(obj, ch));
      act(local_buf, FALSE, mailman, 0, ch, TO_VICT);      
      (*uitems)++;
    }
    report_items(ch, mailman, obj->contains, items, uitems);
    if (obj->in_obj)
      report_items(ch, mailman, obj->next_content, items, uitems);
  }
}

long read_long(char **read_buf)
{
  long long_buf;
  
  while (!isdigit(**read_buf)) (*read_buf)++;
  long_buf = atol(*read_buf);
  while (isdigit(**read_buf)) (*read_buf)++;
  return long_buf;
}

struct obj_data *obj_from_string(char **str_buf)
{
  struct obj_data *obj;
  int j;
  long num;

  num = read_long(str_buf);
  
  if (real_object(num) > -1) {
    obj = read_object(num, VIRTUAL);
    GET_OBJ_VAL(obj, 0) = read_long(str_buf);
    GET_OBJ_VAL(obj, 1) = read_long(str_buf);
    GET_OBJ_VAL(obj, 2) = read_long(str_buf);
    GET_OBJ_VAL(obj, 3) = read_long(str_buf);
    GET_OBJ_EXTRA(obj) = read_long(str_buf);
    GET_OBJ_EXTRA2(obj) = read_long(str_buf);
    GET_OBJ_EXTRA3(obj) = read_long(str_buf);
    GET_OBJ_EXTRA4(obj) = read_long(str_buf);
    GET_OBJ_WEIGHT(obj) = read_long(str_buf);
    GET_OBJ_TIMER(obj) = read_long(str_buf);
    GET_OBJ_OWNER_ID(obj) = read_long(str_buf);
    for (j = 0; j<LONG_BITV_WIDTH; j++)
      obj->obj_flags.bitvector[j] = read_long(str_buf);

    for (j = 0; j < MAX_OBJ_AFFECT; j++) {
      obj->affected[j].location = read_long(str_buf);
      obj->affected[j].modifier = read_long(str_buf);
    }
    return obj;
  } else {
    log("SYSERR: Attempt to load object with nonexisting vnum (obj_from_string)");
    return NULL;
  }
}


void obj_to_string(struct obj_data *obj, char *str_buf)
{
  struct obj_data *tmp;
  int weight, j;
  
  weight = GET_OBJ_WEIGHT(obj);
  
  for (tmp = obj->contains; tmp; tmp = tmp->next_content)
    weight -= GET_OBJ_WEIGHT(tmp);  
  sprintf(str_buf,"%s %d %d %d %d %d %ld %ld %ld %ld %d %d %ld", str_buf,
    GET_OBJ_VNUM(obj), 
    GET_OBJ_VAL(obj, 0),  GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
    GET_OBJ_VAL(obj, 3),
    GET_OBJ_EXTRA(obj), GET_OBJ_EXTRA2(obj), 
    GET_OBJ_EXTRA3(obj), GET_OBJ_EXTRA4(obj), 
    weight, GET_OBJ_TIMER(obj), GET_OBJ_OWNER_ID(obj));
  for (j = 0; j < LONG_BITV_WIDTH; j++)
    sprintf(str_buf, "%s %ld", str_buf, obj->obj_flags.bitvector[j]);
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    sprintf(str_buf, "%s %d %d", str_buf, 
      obj->affected[j].location, obj->affected[j].modifier);
}




struct obj_data *Obj_from_store(struct obj_file_elem object)
{
  struct obj_data *obj;
  int j;

  if (real_object(object.item_number) > -1) {
    obj = read_object(object.item_number, VIRTUAL);
    GET_OBJ_VAL(obj, 0) = object.value[0];
    GET_OBJ_VAL(obj, 1) = object.value[1];
    GET_OBJ_VAL(obj, 2) = object.value[2];
    GET_OBJ_VAL(obj, 3) = object.value[3];
    GET_OBJ_EXTRA(obj) = object.extra_flags;
    GET_OBJ_WEIGHT(obj) = object.weight;
    GET_OBJ_TIMER(obj) = object.timer;
    obj->obj_flags.bitvector[0] = object.bitvector[0];
    obj->obj_flags.bitvector[1] = object.bitvector[1];
    obj->obj_flags.bitvector[2] = object.bitvector[2];
    obj->obj_flags.bitvector[3] = object.bitvector[3];

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      obj->affected[j] = object.affected[j];

    return obj;
  } else
    return NULL;
}



int Obj_to_store(struct obj_data * obj, FILE * fl)
{
  int j;
  struct obj_file_elem object;

  object.item_number = GET_OBJ_VNUM(obj);
  object.value[0] = GET_OBJ_VAL(obj, 0);
  object.value[1] = GET_OBJ_VAL(obj, 1);
  object.value[2] = GET_OBJ_VAL(obj, 2);
  object.value[3] = GET_OBJ_VAL(obj, 3);
  object.extra_flags = GET_OBJ_EXTRA(obj);
  object.weight = GET_OBJ_WEIGHT(obj);
  object.timer = GET_OBJ_TIMER(obj);
  
  COPY_LONG_BITV(obj->obj_flags.bitvector, object.bitvector);
/*  for (j=0; j< LONG_BITV_WIDTH; j++)
    object.bitvector[j] = obj->obj_flags.bitvector[j]; */
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    object.affected[j] = obj->affected[j];

  if (fwrite(&object, sizeof(struct obj_file_elem), 1, fl) < 1) {
    perror("SYSERR: error writing object in Obj_to_store");
    return 0;
  }
  return 1;
}



int Crash_delete_file(char *name)
{
  char filename[50];
  FILE *fl;

  if (!get_filename(name, filename, SAVE_FILE))
    return 0;
  
  if (!(fl = fopen(filename, "rb"))) {
    if (errno != ENOENT) {	/* if it fails but NOT because of no file */
      sprintf(buf1, "SYSERR: deleting crash file %s (1)", filename);
      perror(buf1);
    }
    return 0;
  }
  fclose(fl);

  if (unlink(filename) < 0) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: deleting crash file %s (2)", filename);
      perror(buf1);
    }
  }
  return (1);
}


int Crash_delete_crashfile(struct char_data * ch)
{
  char fname[MAX_INPUT_LENGTH];
  struct rent_info rent;
  FILE *fl;

  if (!get_filename(GET_NAME(ch), fname, SAVE_FILE))
    return 0;
  if (!(fl = fopen(fname, "rb"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: checking for crash file %s (3)", fname);
      perror(buf1);
    }
    return 0;
  }
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  fclose(fl);

  if (rent.rentcode == RENT_CRASH)
    Crash_delete_file(GET_NAME(ch));

  return 1;
}

int Crash_delete_objects(char *name)
{
  char fname[MAX_INPUT_LENGTH];
  struct rent_info rent;
  FILE *fl;

  if (!get_filename(name, fname, SAVE_FILE))
    return 0;
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: checking for crash file %s (3)", fname);
      perror(buf1);
    }
    return 0;
  }
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
    
  if (rent.rentcode == RENT_CRASH || rent.rentcode == RENT_TIMEDOUT ||
    rent.rentcode == RENT_FORCED) {
    rewind(fl);
    rent.item_offset = 0;
    Crash_write_rentcode(NULL, fl, &rent);
  }
  fclose(fl);
  return 1;
}

int Crash_delete_crashfile_objects(struct char_data * ch)
{
  return Crash_delete_objects(GET_NAME(ch));
}


int Crash_clean_file(char *name)
{
  char fname[MAX_STRING_LENGTH], filetype[20];
  struct rent_info rent;
  FILE *fl;

  if (!get_filename(name, fname, SAVE_FILE))
    return 0;
  /*
   * open for write so that permission problems will be flagged now, at boot
   * time.
   */
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: OPENING OBJECT FILE %s (4)", fname);
      perror(buf1);
    }
    return 0;
  }
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  fclose(fl);

  if (rent.item_offset == 0) return 0;
  if (((rent.rentcode == RENT_CRASH) ||
      (rent.rentcode == RENT_FORCED) || (rent.rentcode == RENT_TIMEDOUT))) {
    if (rent.time < time(0) - (crash_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_objects(name);
      switch (rent.rentcode) {
      case RENT_CRASH:
	strcpy(filetype, "crash");
	break;
      case RENT_FORCED:
	strcpy(filetype, "forced rent");
	break;
      case RENT_TIMEDOUT:
	strcpy(filetype, "idlesave");
	break;
      default:
	strcpy(filetype, "UNKNOWN!");
	break;
      }
      log("    Deleting objects from %s's %s file.", name, filetype);
      return 1;
    }
    /* Must retrieve rented items w/in 30 days */
  } else if (rent.rentcode == RENT_RENTED)
    if (rent.time < time(0) - (rent_file_timeout * SECS_PER_REAL_DAY)) {
      Crash_delete_objects(name);
      log("    Deleting objects from %s's rent file.", name);
      return 1;
    }
  return (0);
}



void update_obj_file(void)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    Crash_clean_file((player_table + i)->name);
  return;
}



void Crash_listrent(struct char_data * ch, char *name)
{
  FILE *fl;
  char fname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
//  struct obj_data *obj;
  struct rent_info rent;
  byte prefix;
  int nesting;

  if (!get_filename(name, fname, SAVE_FILE))
    return;
  if (!(fl = fopen(fname, "rb"))) {
    sprintf(buf, "%s has no rent file.\r\n", name);
    send_to_char(buf, ch);
    return;
  }
  sprintf(buf, "%s\r\n", fname);
  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  switch (rent.rentcode) {
  case RENT_RENTED:
    strcat(buf, "Rent\r\n");
    break;
  case RENT_DEATH:
    strcat(buf, "Death\r\n");
    break;
  case RENT_CRASH:
    strcat(buf, "Crash\r\n");
    break;
  case RENT_CRYO:
    strcat(buf, "Cryo\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
    strcat(buf, "TimedOut\r\n");
    break;
  default:
    strcat(buf, "Undef\r\n");
    break;
  }
  if (rent.item_offset == 0) {
    send_to_char(buf, ch);
    fclose(fl);
    return;
  }
  fseek(fl, rent.item_offset, SEEK_SET);
  nesting = 1;
  do {
    fread(&prefix, sizeof(byte), 1, fl);
    if (prefix != OBJ_TERMINATE) {
      fread(&object, sizeof(struct obj_file_elem), 1, fl);
      if (real_object(object.item_number) >= 0)
        sprintf(buf, "%s[%d] %s\r\n", buf, object.item_number,
          obj_proto[real_object(object.item_number)].short_description);
      else sprintf(buf, "%s[%d] &rError - nonexisting object!&w\r\n", buf, object.item_number);
      if (obj_proto[real_object(object.item_number)].obj_flags.type_flag == ITEM_CONTAINER) {
	nesting++;      
      };
    } else
       nesting--;
  } while (nesting != 0 && !feof(fl));
  send_to_char(buf, ch);
  fclose(fl);
}



int Crash_write_rentcode(struct char_data * ch, FILE * fl, struct rent_info * rent)
{
  if (fwrite(rent, sizeof(struct rent_info), 1, fl) < 1) {
    perror("SYSERR: writing rent code");
    return 0;
  }
  return 1;
}



int Crash_load(struct char_data * ch)
/* return values:
	0 - successful load, keep char in rent room.
	1 - load failure or load of crash items -- put char in temple.
	2 - rented equipment lost (no $)
*/
{
  return Advanced_Load(ch);  
}



int Crash_save(struct obj_data * obj, FILE * fp)
{
  struct obj_data *tmp;
  int result;

  if (obj) {
    Crash_save(obj->contains, fp);
    Crash_save(obj->next_content, fp);
    result = Obj_to_store(obj, fp);

    for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
      GET_OBJ_WEIGHT(tmp) -= GET_OBJ_WEIGHT(obj);

    if (!result)
      return 0;
  }
  return TRUE;
}


void Crash_restore_weight(struct obj_data * obj)
{
  if (obj) {
    Crash_restore_weight(obj->contains);
    Crash_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}



void Crash_extract_objs(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_objs(obj->contains);
    Crash_extract_objs(obj->next_content);
    extract_obj(obj);
  }
}


int Crash_is_unrentable(struct obj_data * obj)
{
  if (!obj)
    return 0;

  if (IS_OBJ_STAT(obj, ITEM_NORENT) || IS_OBJ_STAT2(obj, ITEM2_DELETED) ||    
    GET_OBJ_RENT(obj) < 0 || GET_OBJ_RNUM(obj) <= NOTHING || 
    GET_OBJ_TYPE(obj) == ITEM_KEY || IS_OBJ_STAT2(obj_proto + GET_OBJ_RNUM(obj), ITEM2_DELETED) ||
    (GET_OBJ_TYPE(obj) == ITEM_GRENADE && IS_SET(GET_OBJ_EXTRA(obj), ITEM_LIVE_GRENADE)))
    return 1;

  return 0;
}


void Crash_extract_norents(struct obj_data * obj)
{
  if (obj) {
    Crash_extract_norents(obj->contains);
    Crash_extract_norents(obj->next_content);
    if (Crash_is_unrentable(obj))
      extract_obj(obj);
  }
}


void Crash_extract_expensive(struct obj_data * obj)
{
  struct obj_data *tobj, *max;

  max = obj;
  for (tobj = obj; tobj; tobj = tobj->next_content)
    if (GET_OBJ_RENT(tobj) > GET_OBJ_RENT(max))
      max = tobj;
  extract_obj(max);
}



void Crash_calculate_rent(struct obj_data * obj, int *cost)
{
  if (obj) {
    *cost += MAX(0, GET_OBJ_RENT(obj));
    Crash_calculate_rent(obj->contains, cost);
    Crash_calculate_rent(obj->next_content, cost);
  }
}


void Crash_crashsave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  FILE *fp;

  if (IS_NPC(ch))
    return;
    
  
  
  if (!get_filename(GET_NAME(ch), buf, SAVE_FILE))
    return;
    
  if (!(fp = fopen(buf, "wb")))
    return;

  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  Advanced_Save(ch, fp, TRUE, &rent);
  
  fclose(fp);
  
  REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}

void Crash_deathsave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  FILE *fp;

  if (IS_NPC(ch))
    return;
    
  if (!get_filename(GET_NAME(ch), buf, SAVE_FILE))
    return;
    
  if (!(fp = fopen(buf, "wb")))
    return;

  rent.rentcode = RENT_DEATH;
  rent.time = time(0);
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  Advanced_Save(ch, fp, FALSE, &rent);
  fclose(fp);
  
}


void Crash_idlesave(struct char_data * ch)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  int cost;
  FILE *fp;

  if (IS_NPC(ch))
    return;
    
  if (!get_filename(GET_NAME(ch), buf, SAVE_FILE))
    return;
    
  if (!(fp = fopen(buf, "wb")))
    return;
/*
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j))
      obj_to_char(unequip_char(ch, j), ch);

  Crash_extract_norents(ch->carrying);
*/
  cost = 0;
  Crash_calculate_rent(ch->carrying, &cost);
  cost *= 2;			/* forcerent cost is 2x normal rent */
  while ((cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) && ch->carrying) {
    Crash_extract_expensive(ch->carrying);
    cost = 0;
    Crash_calculate_rent(ch->carrying, &cost);
    cost *= 2;
  }

  if (!ch->carrying) {
    fclose(fp);
    Crash_delete_file(GET_NAME(ch));
    return;
  }
  rent.net_cost_per_diem = cost;

  rent.rentcode = RENT_TIMEDOUT;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  Advanced_Save(ch, fp, FALSE, &rent);
  /* if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }*/
  fclose(fp);
  
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j))
      obj_to_char(unequip_char(ch, j), ch);
  Crash_extract_objs(ch->carrying);
  
}


void Crash_rentsave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;
  
  
  if (!get_filename(GET_NAME(ch), buf, SAVE_FILE))
    return;
    
  if (!(fp = fopen(buf, "wb")))
    return;
/*
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j))
      obj_to_char(unequip_char(ch, j), ch);

  Crash_extract_norents(ch->carrying);
*/
  rent.net_cost_per_diem = cost;
  rent.rentcode = RENT_RENTED;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  /*
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  } */
  Advanced_Save(ch, fp, FALSE, &rent);
  fclose(fp);

  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j))
      obj_to_char(unequip_char(ch, j), ch);
  Crash_extract_objs(ch->carrying);

}


void Crash_cryosave(struct char_data * ch, int cost)
{
  char buf[MAX_INPUT_LENGTH];
  struct rent_info rent;
  int j;
  FILE *fp;

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), buf, SAVE_FILE))
    return;
    
  if (!(fp = fopen(buf, "wb")))
    return;
/*
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j))
      obj_to_char(unequip_char(ch, j), ch);

  Crash_extract_norents(ch->carrying);
*/
  GET_GOLD(ch) = MAX(0, GET_GOLD(ch) - cost);

  rent.rentcode = RENT_CRYO;
  rent.time = time(0);
  rent.gold = GET_GOLD(ch);
  rent.account = GET_BANK_GOLD(ch);
  rent.net_cost_per_diem = 0;
  if (!Crash_write_rentcode(ch, fp, &rent)) {
    fclose(fp);
    return;
  }
  /*
  if (!Crash_save(ch->carrying, fp)) {
    fclose(fp);
    return;
  }*/
  Advanced_Save(ch, fp, FALSE, &rent);
  fclose(fp);
  
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j))
      obj_to_char(unequip_char(ch, j), ch);
  Crash_extract_objs(ch->carrying);
  
  SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
}


/* ************************************************************************
* Routines used for the receptionist					  *
************************************************************************* */

void Crash_rent_deadline(struct char_data * ch, struct char_data * recep,
			      long cost)
{
  long rent_deadline;

  if (!cost)
    return;

  rent_deadline = ((GET_GOLD(ch) + GET_BANK_GOLD(ch)) / cost);
  sprintf(buf,
      "$n tells you, 'You can rent for &W%ld&w day%s with the gold you have\r\n"
	  "on hand and in the bank.'\r\n",
	  rent_deadline, (rent_deadline > 1) ? "s" : "");
  act(buf, FALSE, recep, 0, ch, TO_VICT);
}

int Crash_report_unrentables(struct char_data * ch, struct char_data * recep,
			         struct obj_data * obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj) {
    if (Crash_is_unrentable(obj)) {
      has_norents = 1;
      sprintf(buf, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
      act(buf, FALSE, recep, 0, ch, TO_VICT);
    }
    has_norents += Crash_report_unrentables(ch, recep, obj->contains);
    has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
  }
  return (has_norents);
}



void Crash_report_rent(struct char_data * ch, struct char_data * recep,
		            struct obj_data * obj, long *cost, long *nitems, int display, int factor)
{
  static char buf[256];

  if (obj) {
    if (!Crash_is_unrentable(obj)) {
      (*nitems)++;
/*      *cost += MAX(0, (GET_OBJ_RENT(obj) * factor)); */
      *cost += 1;
      if (display) {
	sprintf(buf, "$n tells you, '%5d coins for %s..'",
/*		(GET_OBJ_RENT(obj) * factor), OBJS(obj, ch)); */
		1, OBJS(obj, ch));
	act(buf, FALSE, recep, 0, ch, TO_VICT);
      }
    }
    Crash_report_rent(ch, recep, obj->contains, cost, nitems, display, factor);
    Crash_report_rent(ch, recep, obj->next_content, cost, nitems, display, factor);
  }
}



int Crash_offer_rent(struct char_data * ch, struct char_data * receptionist,
		         int display, int factor)
{
  char buf[MAX_INPUT_LENGTH];
  int i;
  long totalcost = 0, numitems = 0, norent = 0;

  norent = Crash_report_unrentables(ch, receptionist, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    norent += Crash_report_unrentables(ch, receptionist, GET_EQ(ch, i));

  if (norent)
    return 0;

  totalcost = min_rent_cost * factor;

  Crash_report_rent(ch, receptionist, ch->carrying, &totalcost, &numitems, display, factor);

  for (i = 0; i < NUM_WEARS; i++)
    Crash_report_rent(ch, receptionist, GET_EQ(ch, i), &totalcost, &numitems, display, factor);

  if (!numitems) {
    act("$n tells you, 'But you are not carrying anything!  Just quit!'",
	FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (numitems > max_obj_save) {
    sprintf(buf, "$n tells you, 'Sorry, but I cannot store more than %d items.'",
	    max_obj_save);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    return (0);
  }
  if (display) {
    sprintf(buf, "$n tells you, 'Plus, my %d coin fee..'",
	    min_rent_cost * factor);
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    sprintf(buf, "$n tells you, 'For a total of %ld coins%s.'",
	    totalcost, (factor == RENT_FACTOR ? " per day" : ""));
    act(buf, FALSE, receptionist, 0, ch, TO_VICT);
    if (totalcost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, receptionist, 0, ch, TO_VICT);
      return (0);
    } else if (factor == RENT_FACTOR)
      Crash_rent_deadline(ch, receptionist, totalcost);
  }
  return (totalcost);
}



int gen_receptionist(struct char_data * ch, struct char_data * recep,
		         int cmd, char *arg, int mode)
{
  int cost = 0;
  sh_int save_room;
  const char *action_table[] = {"smile", "dance", "sigh", "blush", "burp",
  "cough", "fart", "twiddle", "yawn"};

  if (!ch->desc || IS_NPC(ch))
    return FALSE;

  save_room = world[ch->in_room].number;
  
  if (!cmd && !number(0, 5)) {
    do_action(recep, NULL, find_command(action_table[number(0, 8)]), 0);
    return FALSE;
  }
  if (!CMD_IS("offer") && !CMD_IS("rent"))
    return FALSE;
  if (!AWAKE(recep)) {
    send_to_char("She is unable to talk to you...\r\n", ch);
    return TRUE;
  }
  if (!CAN_SEE(recep, ch)) {
    act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
    return TRUE;
  }
  if (free_rent) {
    act("$n tells you, 'Rent is free here.  Just quit, and your objects will be saved!'",
	FALSE, recep, 0, ch, TO_VICT);
    return 1;
  }
  if (CMD_IS("rent")) {
    if (!(cost = Crash_offer_rent(ch, recep, FALSE, mode)))
      return TRUE;
    if (mode == RENT_FACTOR)
      sprintf(buf, "$n tells you, 'Rent will cost you %d gold coins per day.'", cost);
    else if (mode == CRYO_FACTOR)
      sprintf(buf, "$n tells you, 'It will cost you %d gold coins to be frozen.'", cost);
    act(buf, FALSE, recep, 0, ch, TO_VICT);
    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      act("$n tells you, '...which I see you can't afford.'",
	  FALSE, recep, 0, ch, TO_VICT);
      return TRUE;
    }
    if (cost && (mode == RENT_FACTOR))
      Crash_rent_deadline(ch, recep, cost);

    if (mode == RENT_FACTOR) {
      act("$n stores your belongings and helps you into your private chamber.",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_rentsave(ch, cost);
      sprintf(buf, "%s has rented (%d/day, %d tot.)", GET_NAME(ch),
	      cost, GET_GOLD(ch) + GET_BANK_GOLD(ch));
    } else {			/* cryo */
      act("$n stores your belongings and helps you into your private chamber.\r\n"
	  "A white mist appears in the room, chilling you to the bone...\r\n"
	  "You begin to lose consciousness...",
	  FALSE, recep, 0, ch, TO_VICT);
      Crash_cryosave(ch, cost);
      sprintf(buf, "%s has cryo-rented.", GET_NAME(ch));
      SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
    }

    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);
    
    extract_char(ch);
    SET_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
    save_char(ch, save_room);
  } else {
    Crash_offer_rent(ch, recep, TRUE, mode);
    act("$N gives $n an offer.", FALSE, ch, 0, recep, TO_ROOM);
  }
  return TRUE;
}


SPECIAL(receptionist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, RENT_FACTOR));
}


SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, (struct char_data *)me, cmd, argument, CRYO_FACTOR));
}


void Crash_save_all(void)
{
  struct descriptor_data *d;
  log("Saveing all players in game.");
  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
	Crash_crashsave(d->character);
	save_char(d->character, NOWHERE);
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}

struct obj_data *obj_from_store(struct obj_file_elem object)
{
  struct obj_data *obj;
  int j;

  if (real_object(object.item_number) > -1) {
    obj = read_object(object.item_number, VIRTUAL);
    GET_OBJ_VAL(obj, 0) = object.value[0];
    GET_OBJ_VAL(obj, 1) = object.value[1];
    GET_OBJ_VAL(obj, 2) = object.value[2];
    GET_OBJ_VAL(obj, 3) = object.value[3];
    GET_OBJ_EXTRA(obj) = object.extra_flags;
    GET_OBJ_WEIGHT(obj) = object.weight;
    GET_OBJ_TIMER(obj) = object.timer;
    GET_OBJ_OWNER_ID(obj) = object.engraved_id;
    COPY_LONG_BITV(object.bitvector, obj->obj_flags.bitvector);
/*    for (j = 0; j<NUM_AFF; j++)
      obj->obj_flags.bitvector[j] = object.bitvector[j]; */

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
      obj->affected[j] = object.affected[j];

    return obj;
  } else {
    log("SYSERR: Attempt to load object with nonexisting vnum");
    return NULL;
  }
}

void obj_to_store(struct obj_data *obj, struct obj_file_elem *object)
{
  struct obj_data *tmp;
  int weight, j;
  
  weight = GET_OBJ_WEIGHT(obj);
  
  for (tmp = obj->contains; tmp; tmp = tmp->next_content)
    weight -= GET_OBJ_WEIGHT(tmp);  
  object->item_number = GET_OBJ_VNUM(obj);
  object->value[0] = GET_OBJ_VAL(obj, 0);
  object->value[1] = GET_OBJ_VAL(obj, 1);
  object->value[2] = GET_OBJ_VAL(obj, 2);
  object->value[3] = GET_OBJ_VAL(obj, 3);
  object->extra_flags = GET_OBJ_EXTRA(obj);
  object->weight = weight;
  object->timer = GET_OBJ_TIMER(obj);
  object->engraved_id = GET_OBJ_OWNER_ID(obj);
  COPY_LONG_BITV(obj->obj_flags.bitvector, object->bitvector);
/*  for (j = 0; j < NUM_AFF; j++)
    object->bitvector[j] = obj->obj_flags.bitvector[j]; */
  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    object->affected[j] = obj->affected[j];
}

void save_aliases(FILE * fp, struct char_data * ch)
{
  struct alias *temp;
  sh_int length;
  char temp_buf[MAX_ALIAS_LENGTH+1];
  
  temp = GET_ALIASES(ch);
  while (temp && strlen(temp->alias))
  {
    if (strlen(temp->alias) > MAX_ALIAS_LENGTH) {
      strncpy(temp_buf, temp->alias, MAX_ALIAS_LENGTH);
      temp_buf[MAX_ALIAS_LENGTH] = '\0';
    } else strcpy(temp_buf, temp->alias);
    length = strlen(temp_buf);
    fwrite(&length, 1, sizeof(sh_int), fp);
    fwrite(temp_buf, 1, length, fp);
    
    if (strlen(temp->replacement) > MAX_ALIAS_LENGTH) {
      strncpy(temp_buf, temp->replacement, MAX_ALIAS_LENGTH);
      temp_buf[MAX_ALIAS_LENGTH] = '\0';
    } else strcpy(temp_buf, temp->replacement);
    length = strlen(temp_buf);
    fwrite(&length, 1, sizeof(sh_int), fp);
    fwrite(temp_buf, 1, length, fp);
    
    fwrite(&temp->type, 1, sizeof(int), fp);
    temp = temp->next;
  }
  /* Write terminator */
  length = -1;
  fwrite(&length, 1, sizeof(sh_int), fp);
}




void save_terminate(FILE *fp)
{
   byte prefix = OBJ_TERMINATE;
   fwrite(&prefix, 1, sizeof(byte), fp);
}

void save_object(FILE *fp, struct obj_data *obj, int position, int save_norents)
{
   byte	prefix;
   struct obj_file_elem object;
   struct obj_data *obj1;
   
   /* Sanity check */
   if (obj == NULL) {
     log("SYSERR: Attempt to save NULL obj");
     return;
   }
   /* Check if it is NO_RENT */
   if ((!save_norents && (IS_OBJ_STAT(obj, ITEM_NORENT) || GET_OBJ_RENT(obj) < 0
       || GET_OBJ_TYPE(obj) == ITEM_KEY)) || GET_OBJ_RNUM(obj) <= NOTHING)
      return;
      
   /* Okay, now write prefix */
   prefix = (byte) position;
   fwrite(&prefix, 1, sizeof(byte), fp);
   
   
   
   /* And now the obj_file_elem */
   obj_to_store(obj, &object);
   fwrite(&object, 1, sizeof(struct obj_file_elem), fp);
   
   /* If it is a container save the contents recursively */
   if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
    {
      for (obj1 = obj->contains; obj1; obj1 = obj1->next_content)
        save_object(fp, obj1, OBJ_CONTENT, save_norents);
      save_terminate(fp);
    }
   
   /* It's done */
   return;
}

int Advanced_Save(struct char_data * ch, FILE * fp, int save_norents, 
  struct rent_info *rent)
{
  int j;
  struct obj_data * obj;

  if (IS_NPC(ch))
    return 0;
  
  rent->alias_offset = 0;
  rent->item_offset = 0;
//  Crash_write_rentcode(ch, fp, rent);
  
  rent->item_offset = ftell(fp);
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j))
      save_object(fp, GET_EQ(ch, j), j, save_norents);
       
  for (obj = ch->carrying; (obj); obj = obj->next_content)
    save_object(fp, obj, OBJ_INVENTORY, save_norents);
   
  save_terminate(fp);
  
  rent->alias_offset = ftell(fp);
  save_aliases(fp, ch);
  
  rewind(fp);
  Crash_write_rentcode(ch, fp, rent);  
  return 1;
}

struct obj_data *load_object(struct char_data *ch, FILE * fl)
{
  int perform_wear(struct char_data *ch,struct obj_data *obj, int where);
  void perform_put(struct char_data *ch,struct obj_data *obj, struct obj_data *cont);

  byte prefix;
  struct obj_file_elem object;
  struct obj_data *obj, *obj1;

  /* Load Prefix */
  fread(&prefix, sizeof(byte), 1, fl);
  if (feof(fl)) {
    log("SYSERR: Unexpected end of plrobj file (obj prefix missing)");
    return NULL;
  }
  /* Is it OBJ_TERMINATE, if so, we are finished */
  if (prefix == OBJ_TERMINATE) return NULL;
  
  /* Read obj file elem */
  fread(&object, sizeof(struct obj_file_elem), 1, fl);
  if (ferror(fl) || feof(fl)) {
    log("SYSERR: Error reading save file (obj_file_elem corrupted"); 
    return NULL;
  }
  
//  obj = read_object(object.item_number, VIRTUAL);
  obj = obj_from_store(object);
  if (!obj) return NULL;

  /* If it's a container load the contents */
  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
    do {
      obj1 = load_object(ch, fl);
      if (obj1)
        obj_to_obj(obj1, obj);
    } while (obj1);
    /* then return */
  }
  
  switch (prefix) {
    case OBJ_CONTENT:
      /* If it is a content of container do not equip char with it */
      break;
    case OBJ_INVENTORY:
      /* If it's in inventory, so Give it to char */
      obj_to_char(obj, ch);
      break;
    default:
      /* From here I assume that the prefix is wear position and nothing else */
      equip_char(ch, obj, prefix);
      break;
  }
  num_objs++;
  return obj;
}

int load_aliases(FILE * fl, struct char_data *ch)
{
  sh_int length, length2;
  int type;
  char temp_buf1[MAX_ALIAS_LENGTH+1];
  char temp_buf2[MAX_ALIAS_LENGTH+1];
  struct alias *temp;
  
  GET_ALIASES(ch) = NULL;
  temp = NULL;
  
  do {
    if (feof(fl)) {
      log("SYSERR: Unexpected end of aliases.");
      return FALSE;
    }
  
    fread(&length, sizeof(sh_int), 1, fl);
  
    if (feof(fl)) {
      log("SYSERR: Unexpected end of aliases.");
      return FALSE;
    }
  
    if (length == -1) return FALSE;
   
    fread(&temp_buf1, length, 1, fl);
    temp_buf1[length] = '\0';
    
    fread(&length2, sizeof(sh_int), 1, fl);
    fread(&temp_buf2, length2, 1, fl);
    temp_buf2[length2] = '\0';
    
    fread(&type, sizeof(int), 1, fl);
   
    if (feof(fl)) {
      log("SYSERR: Unexpected end of aliases.");
      type = 1;
    }
   
    if (temp == NULL) {
      CREATE(temp, struct alias, 1);
      GET_ALIASES(ch) = temp;
    } else {
      CREATE(temp->next, struct alias, 1);
      temp = temp->next;
    }
    
    temp->next = NULL;
    temp->alias = str_dup(temp_buf1);
    temp->replacement = str_dup(temp_buf2);
    temp->type = type;
    
  } while (length != -1 && !feof(fl));
  return TRUE;
}

int Advanced_Load(struct char_data * ch)
/* return values:
	0 - successful load, keep char in rent room.
	1 - load failure or load of crash items -- put char in temple.
	2 - rented equipment lost (no $)
*/
{
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  
  struct rent_info rent;
  int cost, orig_rent_code;
  float num_of_days;
  
  if (!get_filename(GET_NAME(ch), fname, SAVE_FILE))
    return 1;
  if (!(fl = fopen(fname, "r+b"))) {
    if (errno != ENOENT) {	/* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING SAVE OBJECT FILE %s (5)", fname);
      perror(buf1);
      send_to_char("\r\n********************* NOTICE *********************\r\n"
		   "There was a problem loading your objects from disk.\r\n"
		   "Contact a God for assistance.\r\n", ch);
    }
    sprintf(buf, "%s entering game with no rented equipment.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    return 1;
  }
  
  if (feof(fl)) {
    log("SYSERR: Unexpected end of plrobj file before rent structure");
    return 1;
  }
  
  fread(&rent, sizeof(struct rent_info), 1, fl);

  if (feof(fl)) {
    log("SYSERR: Unexpected end of plrobj file in rent_structure");
    return 1;
  }

  if (rent.rentcode == RENT_RENTED || rent.rentcode == RENT_TIMEDOUT) {
    num_of_days = (float) (time(0) - rent.time) / SECS_PER_REAL_DAY;
    cost = (int) (rent.net_cost_per_diem * num_of_days);
    if (cost > GET_GOLD(ch) + GET_BANK_GOLD(ch)) {
      fclose(fl);
      sprintf(buf, "%s entering game, rented equipment lost (no $).",
	      GET_NAME(ch));
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
      Crash_crashsave(ch);
      return 2;
    } else {
      GET_BANK_GOLD(ch) -= MAX(cost - GET_GOLD(ch), 0);
      GET_GOLD(ch) = MAX(GET_GOLD(ch) - cost, 0);
      save_char(ch, GET_LOADROOM(ch));
    }
  }
  switch (orig_rent_code = rent.rentcode) {
  case RENT_DEATH:
    sprintf(buf, "%s entering game with no rented equipment.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_RENTED:
    sprintf(buf, "%s un-renting and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRASH:
    sprintf(buf, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_CRYO:
    sprintf(buf, "%s un-cryo'ing and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  case RENT_FORCED:
  case RENT_TIMEDOUT:
    sprintf(buf, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  default:
    sprintf(buf, "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
    mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
    break;
  }
  num_objs = 0;
  if (rent.item_offset != 0) {
    fseek(fl, rent.item_offset, SEEK_SET);
    read_obj = TRUE;
    num_objs = 0;
    while (load_object(ch, fl)) {}
    read_obj = FALSE;

    if (feof(fl)) {
      log("SYSERR: Unexpected end of plrobj file in objects");
      return 1;
    }
  }
  
  if (rent.alias_offset != 0) {
    fseek(fl, rent.alias_offset, SEEK_SET);
    load_aliases(fl,ch);
  }
  
  /* Little hoarding check. -gg 3/1/98 */
   sprintf(fname, "%s (level %d) has %d objects (max %d).",
     GET_NAME(ch), GET_LEVEL(ch), num_objs, max_obj_save);
   mudlog(fname, NRM, LVL_GOD, TRUE);
  
  /* turn this into a crash file by re-writing the control block */
  rent.rentcode = RENT_CRASH;
  rent.time = time(0);
  rewind(fl);
  Crash_write_rentcode(ch, fl, &rent);
  
  fclose(fl);
  if ((orig_rent_code == RENT_RENTED) || (orig_rent_code == RENT_CRYO))
    return 0;
  else
    return 1;
}

#define	ALLSTR(test)	(test ? "Allowed" : "Not Allowed")

void load_world_settings(void)
{
  FILE *fl;
  if (!(fl = fopen(WORLD_SET_FILE, "rt"))) {
    sprintf(buf2,"SYSERR: World settings not found (%s missing)", WORLD_SET_FILE);
    log(buf2);
    log("  Leaving defaults!");
  } else {
    fscanf(fl," %d \n %d \n %d \n %d \n %d \n %d \n %d \n %d \n %d \n %d \n", 
    &pk_allowed, &sleep_allowed, &charm_allowed, &summon_allowed, 
    &roomaffect_allowed, &circle_restrict, &pt_allowed,
    &max_level_diff, &olc_allowed, &clan_olc_allowed);
    fclose(fl);
  }
  sprintf(buf2,"   PKilling: %s", ALLSTR(pk_allowed));
  log(buf2);
  sprintf(buf2,"   PThieving: %s", ALLSTR(pt_allowed));
  log(buf2);
  sprintf(buf2,"   Casting sleep on PCs: %s", ALLSTR(sleep_allowed));
  log(buf2);
  sprintf(buf2,"   Charming PCs: %s", ALLSTR(charm_allowed));
  log(buf2);
  sprintf(buf2,"   Summoning PCs: %s", ALLSTR(summon_allowed));
  log(buf2);
  sprintf(buf2,"   Roomaffect PCs: %s", ALLSTR(roomaffect_allowed));
  log(buf2);
  sprintf(buf2,"   Wizlock: %d", circle_restrict);
  log(buf2);
  sprintf(buf2,"   Max-lev-diff: %d", max_level_diff);
  log(buf2);
  sprintf(buf2,"   God OLC: %s", ALLSTR(olc_allowed));
  log(buf2);
  sprintf(buf2,"   Clan OLC: %s", ALLSTR(clan_olc_allowed));
  log(buf2);
  return;
}

void save_world_settings(void)
{
  FILE * fl;
  if (!(fl = fopen(WORLD_SET_FILE, "wt"))) {
    log("SYSERR: Unable to write world settings");
    return;
  } else {
    fprintf(fl,	"%d\n%d\n%d\n%d\n%d\n"
    		"%d\n%d\n%d\n%d\n%d\n", 
    		pk_allowed, sleep_allowed, charm_allowed,
    summon_allowed, roomaffect_allowed, circle_restrict, pt_allowed,
    max_level_diff, olc_allowed, clan_olc_allowed);
    fclose(fl);
  }
}
