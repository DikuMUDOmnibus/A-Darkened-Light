/* ************************************************************************
*   File: trains.c                                                        *
*  Usage: Transporter handling functions for CircleMUD                    *
*     By: Carl Tashian [Guru Meditation]                                  *
*         [tashiacm@ctrvax.vanderbilt.edu]                                *
*  Modif: Vladimir Marko [Yilard] 26.2.1998				  *
*         [yilard@earthling.net]                                          *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

/* Trains are currently hard-coded and you need to reboot to set up new train */
/* Maybe sometimes I will make it possible to edit using VisionOLC ;) */ 

#define NUM_TRAINS 	3
#define MAX_STATIONS	10

#define MOVE_FORWARD	0
#define MOVE_BACKWARD	1
#define MOVE_ABSOLUTE	2

#define PHASE_LEAVE	1
#define PHASE_ENTER	2

/*   external vars  */
extern struct room_data *world;
extern int mini_mud;
int transpath = 0;
char curstat1[100], curstat2[100];

void enter_stat(int station, char wherefrom[100], int which);
void leave_stat(int station, char whereto[100], int which);

char *symbol_wagon = "&R*&w";
char *symbol_line = "&c|&w";
char *symbol_end = "&cV&w";

typedef struct {
  char *name;
  int  room;
} STATION;

typedef struct {
  int  trans_room;	
  char *name;
  char *in_entermsg;
  char *in_leavemsg;
  char *out_entermsg;
  char *out_leavemsg;
  int numstat;
  signed char pos;
  int  last_pos;
  int  phase;
  int  enter_dir;
  int  leave_dir;
  int  speed;
  int  counter;
  STATION stations[MAX_STATIONS];
} TRAIN;

TRAIN train_list[NUM_TRAINS] = 
{
  {
    3070,
    "Midgaard Railway System - Line 1",
  
    "The transporter slows to a halt, and the doors to your west open..\n\r"
    "The conductor announces, 'Welcome to %s'\n\r",
  
    "The conductor announces, 'Next stop: %s'\n\r"
    "The transporter doors close, and the transporter starts it's journey.\n\r",
  
    "Without a sound, the transporter from %s pulls into\n\r"
    "the station, and the doors to your east open.\n\r",

    "The transporter doors close, and it instantly disappears into the tunnel..\n\r",
    
    7, 0, 6, PHASE_ENTER, EAST, WEST, 1, 0,
    {
      { "Midgaard Central", 3069 },
      { "Midgaard South", 3080 },
      { "New Thalos", 9000 },
      { "Rome", 9001 },
      { "Drow City", 5151 },
      { "New Sparta", 21130 },
      { "Sundhaven", 6786 }
    
    }
  },
  {
  
    3072,
    "Midgaard Railway System - Line 2",
  
    "The transporter slows to a halt, and the doors to your west open..\n\r"
    "The conductor announces, 'Welcome to %s'\n\r",
  
    "The conductor announces, 'Next stop: %s'\n\r"
    "The transporter doors close, and the transporter starts it's journey.\n\r",
  
    "Without a sound, the transporter from %s pulls into\n\r"
    "the station, and the doors to your east open.\n\r",

    "The transporter doors close, and it instantly disappears into the tunnel..\n\r",
    
    7, 3, 2, PHASE_ENTER, EAST, WEST, 1, 0,
    {
      { "Midgaard Central", 3069 },
      { "Midgaard South", 3080 },
      { "New Thalos", 9000 },
      { "Rome", 9001 },
      { "Drow City", 5151 },
      { "New Sparta", 21130 },
      { "Sundhaven", 6786 }
    
    }
  },
  {
    1561,
    "Space Shuttle Galeleo",
    
    "You feel an overdrive, pushing you into the seat. When the engines stop\r\n"
    "You can hear an artificial voice saying: '%s. Landed. System status: 34%%'\r\n", 
    
    "The word 'Launch' appears on the screen and an artificial voice says:\r\n" 
    "'Destination: %s. Takeoff.'\r\n"
    "You couldn't hear anything else because you've lost your consciousness.\r\n",
    
    "You suddenly hear an air splitting noise. You feel the heat and see the large\r\n"
    "Space shuttle coming obviously from %s.\r\n",
    
    "Your skin is burnt by the engines of the space shuttle as it takes off.\r\n",
    
    2, 1, 0, PHASE_ENTER, UP, DOWN, 6, 5,
    {
      { "New Sparta Space Centre", 21151 },
      { "Space Station Alpha", 1516 }
    }
  }
};

void move_train(TRAIN *train, char move_type, char move_arg)
{
  int troom, sroom, lroom;
  
  if (++(train->counter) < train->speed) return;
  train->counter = 0;
  troom = real_room(train->trans_room);
  sroom = real_room(train->stations[train->pos].room);
  lroom = real_room(train->stations[train->last_pos].room);
  switch (train->phase) {
    case PHASE_ENTER:
//      log("TRAIN - enter");
      world[troom].dir_option[train->leave_dir]->to_room 
        = sroom;
      world[sroom].dir_option[train->enter_dir]->to_room 
        = troom; 
      sprintf(buf, train->in_entermsg, train->stations[train->pos].name);
      send_to_room(buf, troom); 
      sprintf(buf, train->out_entermsg, train->stations[train->last_pos].name);
      send_to_room(buf, sroom);
      train->phase = PHASE_LEAVE;
      break;
      
    case PHASE_LEAVE:
//      log("TRAIN - leave");
      
      world[troom].dir_option[train->leave_dir]->to_room 
        = NOWHERE;
      world[sroom].dir_option[train->enter_dir]->to_room 
        = NOWHERE; 
      
      train->last_pos = train->pos;
      sroom = real_room(train->stations[train->pos].room);
      lroom = real_room(train->stations[train->last_pos].room);
      switch (move_type) {
        case MOVE_FORWARD:
          train->pos += move_arg;
          break;
        case MOVE_BACKWARD:
          train->pos -= move_arg;
          break;
        case MOVE_ABSOLUTE:
          train->pos = move_arg;
          break;
        default: 
          log("Transporter is not moving!");
          break;
      }
      if (train->pos >= train->numstat) train->pos = 0;
      else if (train->pos < 0) train->pos = train->numstat;
      sprintf(buf, train->in_leavemsg, train->stations[train->pos].name);
      send_to_room(buf, troom);
      sprintf(buf, train->out_leavemsg, train->stations[train->pos].name);
      send_to_room(buf, lroom); 
      train->phase = PHASE_ENTER;
      break;
      
    default: 
      log("SYSERR: Big problem with the transporter!");
      break;
  } 
//  sprintf(buf, "Phase: %d", train->phase);
//  log(buf);
}

void train_upd(void)
{
  int i;
  if (mini_mud) return;
  for (i = 0; i< NUM_TRAINS; i++)
    move_train(train_list + i, MOVE_FORWARD, 1);
  return;
} 

int list_train(TRAIN *train, struct char_data *ch, char *name)
{
  return 0;
}

TRAIN *get_train(int room_vnum)
{
  int i;
  if (room_vnum < 0) return NULL;
  for (i = 0; i < NUM_TRAINS; i++)
    if (train_list[i].trans_room == room_vnum)
      return train_list + i;
  return NULL;
}

void list_stations_wagon(struct char_data *ch, int room_vnum)
{
  TRAIN *t = NULL;
  
  char *str;
  int i = 0;
  
  t = get_train(room_vnum);
  if (t == NULL) {
    send_to_char("ERROR: This is not transporter but the spec-proc is set!\r\nPlease report to Implementor.\r\n", ch);
    return;
  }
  
  sprintf(buf, "&C%s Timetable&w\r\n"
               "---------------------------------\r\n", t->name);
  
  
  for (i = 0; i < t->numstat; i++) {
    if (t->pos == i)
      str = symbol_wagon;
    else if (i + 1 == t->numstat)
      str = symbol_end;
    else str = symbol_line; 
    sprintf(buf + strlen(buf), "%s %s\r\n", str, t->stations[i].name);
    
  }
  send_to_char(buf, ch);
}

/* Returns vnum of transporter room, room_rnum is rnum of station */
int get_transroom(int room_rnum, int dir)
{
  int i;
  
  
  if (!world[room_rnum].dir_option[dir]) return -1;
  
  for (i = 0; i < NUM_TRAINS; i++)
    if (real_room(train_list[i].trans_room) == 
      world[room_rnum].dir_option[dir]->to_room)
      return (train_list[i].trans_room);
      
  return -1;
}

/* Returns vnum of transporter room, room_rnum is rnum of station */
int get_station_train_index(int room_rnum, int *last_search)
{
  int i, j;
  
  if (!last_search || *last_search >= NUM_TRAINS) return -1;
  
  for (i = last_search ? *last_search : 0; i < NUM_TRAINS; i++)
    for (j = 0; j < train_list[i].numstat; j++)
      if (real_room(train_list[i].stations[j].room) == room_rnum) {
        *last_search = i + 1;
        return i;
      }
      
  return -1;
}


void list_stations(struct char_data *ch)
{
  int found = 0;
  int pos = 0;
  int train_index;
  
  while ((train_index = get_station_train_index(ch->in_room, &pos)) >= 0) {
    if (!found++)
      send_to_char("&GLines going through this station:&w\r\n", ch);
    list_stations_wagon(ch, train_list[train_index].trans_room);
  }
  if (!found) 
    send_to_char("This is not a station of any mean of transport.\r\n", ch);
}


