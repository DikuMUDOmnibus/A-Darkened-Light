/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern struct room_data *world;
extern int top_of_world;
extern int mini_mud;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern int dts_are_dumps;

/* ********************************************************************
*  MOB SpecProc Externs                                               *
******************************************************************** */
  SPECIAL(postmaster);
  SPECIAL(cityguard);
  SPECIAL(receptionist);
  SPECIAL(cryogenicist);
  SPECIAL(guild_guard);
  SPECIAL(meta_physician);
  SPECIAL(guild);
  SPECIAL(puff);
  SPECIAL(fido);
  SPECIAL(janitor);
  SPECIAL(mayor);
  SPECIAL(snake);
  SPECIAL(thief);
  SPECIAL(magic_user);
  
  SPECIAL(sund_earl);
  SPECIAL(hangman);  
  SPECIAL(blinder);
  SPECIAL(silktrader);
  SPECIAL(butcher);   
  SPECIAL(idiot);  
  SPECIAL(athos);
  SPECIAL(stu);
  SPECIAL(cleric);
  SPECIAL(wiseman);
  SPECIAL(newbie_guide);
  
  SPECIAL(sea_serpent);
  SPECIAL(Leviathan);
  SPECIAL(master);
  SPECIAL(slave);
  SPECIAL(Priest_of_Fear);
  SPECIAL(Butcher);
  SPECIAL(tarbaby);
  SPECIAL(Grand_Inquisitor);
  SPECIAL(High_Priest_of_Terror);
  

  SPECIAL(dragon_fire);
  SPECIAL(dragon_gas);
  SPECIAL(dragon_frost);
  SPECIAL(dragon_acid);
  SPECIAL(dragon_lightning);
  SPECIAL(dragon_guard);
  /* grave */
  SPECIAL(grave_undertaker);
  SPECIAL(grave_demilich);
  SPECIAL(grave_ghoul);
  SPECIAL(grave_priest);
  
  /* King Welmar's Castle (previously in castle.c) */
  SPECIAL(CastleGuard);
  SPECIAL(James);
  SPECIAL(cleaning);
  SPECIAL(DicknDavid);
  SPECIAL(tim);
  SPECIAL(tom);
  SPECIAL(king_welmar);
  SPECIAL(training_master);
  SPECIAL(peter);
  SPECIAL(jerry);
  
  /* others */
  SPECIAL(engraver);
  SPECIAL(evil_cleric);
  SPECIAL(fighter);
  SPECIAL(alchemist);

/* ********************************************************************
*  OBJ SpecProc Externs                                               *
******************************************************************** */
  SPECIAL(bank);
  SPECIAL(gen_board);
  SPECIAL(tokens);
  SPECIAL(marbles);
  SPECIAL(portal);
  SPECIAL(recharger);
  SPECIAL(pop_dispenser);
  SPECIAL(blind_weapon);
  SPECIAL(fireball_weapon);
  SPECIAL(curse_weapon);
  SPECIAL(wand_of_wonder);

/* ********************************************************************
*  ROOM SpecProc Externs                                               *
******************************************************************** */
  SPECIAL(dump);
  SPECIAL(pet_shops);
//  SPECIAL(pray_for_items);
  SPECIAL(horse_shops);
  SPECIAL(hometown_room);
  SPECIAL(transporter);
  
/* global variables */
int num_mob_procs;
int num_obj_procs;
int num_room_procs;

/* ********************************************************************
*  Master MOB SpecProc Table                                          *
******************************************************************** */
const struct specproc_info mob_procs[] = {
  {"NONE", 		NULL, 			LVL_BUILDER},
  {"receptionist", 	receptionist, 		LVL_BUILDER},
  {"cryogenicist", 	cryogenicist, 		LVL_BUILDER},
  {"postmaster", 	postmaster, 		LVL_BUILDER},
  {"cityguard", 	cityguard, 		LVL_BUILDER},
  {"fido", 		fido,			LVL_BUILDER},
  {"janitor", 		janitor, 		LVL_BUILDER},
  {"mayor", 		mayor, 			LVL_BUILDER},
  {"puff",		puff,			LVL_BUILDER},
  {"newbie_guide", 	newbie_guide, 		LVL_CODER},
  {"wiseman", 		wiseman, 		LVL_BUILDER},
  {"meta_physician", 	meta_physician,		LVL_CODER},
  {"thief", 		thief, 			LVL_BUILDER},
  {"magic_user", 	magic_user,		LVL_BUILDER},
  {"evil_cleric",	evil_cleric,		LVL_BUILDER},
  {"fighter",		fighter,		LVL_BUILDER},
  {"snake", 		snake, 			LVL_BUILDER},
  {"cleric", 		cleric, 		LVL_BUILDER},
  {"dragon_fire", 	dragon_fire, 		LVL_BUILDER},
  {"dragon_gas", 	dragon_gas, 		LVL_BUILDER},
  {"dragon_frost", 	dragon_frost, 		LVL_BUILDER},
  {"dragon_acid", 	dragon_acid, 		LVL_BUILDER},
  {"dragon_lightning", 	dragon_lightning, 	LVL_BUILDER},
  {"dragon_guard", 	dragon_guard, 		LVL_BUILDER},
  {"sea_serpent", 	sea_serpent, 		LVL_BUILDER},
  {"Leviathan", 	Leviathan, 		LVL_BUILDER},
  {"master", 		master, 		LVL_BUILDER},
  {"slave", 		slave, 			LVL_BUILDER},
  {"priest_fear", 	Priest_of_Fear, 	LVL_BUILDER},
  {"priest_terror", 	High_Priest_of_Terror, 	LVL_BUILDER},
  {"inquisitor", 	Grand_Inquisitor, 	LVL_BUILDER},
  {"butcher", 		butcher, 		LVL_BUILDER},
  {"tarbaby", 		tarbaby, 		LVL_BUILDER},
  {"undertaker",	grave_undertaker,	LVL_BUILDER},
  {"ghoul",		grave_ghoul,		LVL_BUILDER},
  {"demilich",		grave_demilich,		LVL_BUILDER},
  {"grave_priest",	grave_priest,		LVL_BUILDER},
  {"earl",		sund_earl,		LVL_BUILDER},
  {"hangman",		hangman,		LVL_BUILDER},
  {"blinder",		blinder,		LVL_BUILDER},
  {"silktrader",	silktrader,		LVL_BUILDER},
  {"idiot",		idiot,			LVL_BUILDER},
  {"athos",		athos,			LVL_BUILDER},
  {"stu",		stu,			LVL_BUILDER},
  {"butcher", 		butcher, 		LVL_BUILDER},
  {"castle_guard",	CastleGuard,		LVL_BUILDER},
  {"James",		James,			LVL_BUILDER},
  {"DicknDavid",	DicknDavid,		LVL_BUILDER},
  {"Tim",		tim,			LVL_BUILDER},
  {"Tom",		tom,			LVL_BUILDER},
  {"King_Welmar",	king_welmar,		LVL_BUILDER},
  {"training_master",	training_master,	LVL_BUILDER},
  {"Peter",		peter,			LVL_BUILDER},
  {"Jerry",		jerry,			LVL_BUILDER},
  {"cleaning_woman",	cleaning,		LVL_BUILDER},
  {"engraver",		engraver,		LVL_BUILDER},
  {"alchemist",		alchemist,		LVL_BUILDER}, 
  {"\n",		0,			0}
};

/* ********************************************************************
*  Master OBJ SpecProc Table                                          *
******************************************************************** */
const struct specproc_info obj_procs[] = {
  {"NONE", 		NULL, 			LVL_BUILDER},
  {"bank", 		bank, 			LVL_BUILDER},
  {"gen_board", 	gen_board, 		LVL_CHBUILD},
  {"token_machine", 	tokens, 		LVL_BUILDER},
  {"marbles", 		marbles, 		LVL_BUILDER},
  {"portal", 		portal, 		LVL_COIMPL},
  {"recharger", 	recharger, 		LVL_BUILDER},
  {"pop_dispenser", 	pop_dispenser, 		LVL_BUILDER},
  {"blind_weapon",	blind_weapon,		LVL_CHBUILD},
  {"fireball_weapon",	fireball_weapon,	LVL_CHBUILD},
  {"curse_weapon",	curse_weapon,		LVL_CHBUILD},
  {"wand_of_wonder",	wand_of_wonder,		LVL_CHBUILD},
  {"\n",		0,			0}
};

/* ********************************************************************
*  Master ROOM SpecProc Table                                         *
******************************************************************** */
const struct specproc_info room_procs[] = {
  {"NONE", 		NULL, 			LVL_BUILDER},
  {"dump", 		dump, 			LVL_BUILDER},
  {"pet_shop", 		pet_shops,		LVL_BUILDER},
  {"horse_shop", 	horse_shops, 		LVL_BUILDER},
  {"hometown_room", 	hometown_room, 		LVL_CHBUILD},
  {"transporter",	transporter,		LVL_CHBUILD},
  {"\n",		0,			0}
};

/* local functions */
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(int room, SPECIAL(fname));
void ASSIGNMOB(int mob, SPECIAL(fname));
void ASSIGNOBJ(int obj, SPECIAL(fname));

/* functions to perform assignments */


void ASSIGNMOB(int mob, SPECIAL(fname))
{
  if (real_mobile(mob) >= 0) {
    mob_index[real_mobile(mob)].func = fname;
    if (get_spec_name(mob_procs, fname) == 0) {
      sprintf(buf, "SYSWAR: Assign spec not in table to mob #%d",
	    mob);
      log(buf);
    
    }
  }
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant mob #%d",
	    mob);
    log(buf);
  }
}

void ASSIGNOBJ(int obj, SPECIAL(fname))
{
  if (real_object(obj) >= 0) {
    obj_index[real_object(obj)].func = fname;
    if (get_spec_name(obj_procs, fname) == 0) {
      sprintf(buf, "SYSWAR: Assign spec not in table to obj #%d",
	    obj);
      log(buf);
    
    }
  }
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant obj #%d",
	    obj);
    log(buf);
  }
}

void ASSIGNROOM(int room, SPECIAL(fname))
{
  if (real_room(room) >= 0) {
    world[real_room(room)].func = fname;
    if (get_spec_name(room_procs, fname) == 0) {
      sprintf(buf, "SYSWAR: Assign spec not in table to room #%d",
	    room);
      log(buf);
    
    }
  }
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant rm. #%d",
	    room);
    log(buf);
  }
}

/*
void count_specprocs()
{
  for (NUM_MOB_PROCS = 0; mob_procs[NUM_MOB_PROCS].name[0] != '\n'; NUM_MOB_PROCS++) {};
  for (NUM_OBJ_PROCS = 0; obj_procs[NUM_OBJ_PROCS].name[0] != '\n'; NUM_OBJ_PROCS++) {};
  for (NUM_ROOM_PROCS = 0; room_procs[NUM_ROOM_PROCS].name[0] != '\n'; NUM_ROOM_PROCS++) {};
}
*/

/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  void assign_kings_castle(void);

  return;
  assign_kings_castle();

  ASSIGNMOB(1, puff);

  /* Immortal Zone */
  ASSIGNMOB(1200, receptionist);
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(1202, janitor);

  /* Midgaard */
  ASSIGNMOB(3005, receptionist);
  ASSIGNMOB(3010, postmaster);
  ASSIGNMOB(3098, meta_physician);
//  ASSIGNMOB(3020, guild);
//  ASSIGNMOB(3021, guild);
//  ASSIGNMOB(3022, guild);
//  ASSIGNMOB(3023, guild);
//  ASSIGNMOB(3125, guild);
//  ASSIGNMOB(3127, guild);
//  ASSIGNMOB(3129, guild);
//  ASSIGNMOB(3131, guild);
//  ASSIGNMOB(3024, guild_guard);
//  ASSIGNMOB(3025, guild_guard);
//  ASSIGNMOB(3026, guild_guard);
//  ASSIGNMOB(3027, guild_guard);
//  ASSIGNMOB(3124, guild_guard);
//  ASSIGNMOB(3126, guild_guard);
//  ASSIGNMOB(3128, guild_guard);
//  ASSIGNMOB(3130, guild_guard);
  ASSIGNMOB(3059, cityguard);
  ASSIGNMOB(3060, cityguard);
  ASSIGNMOB(3061, janitor);
  ASSIGNMOB(3062, fido);
  ASSIGNMOB(3066, fido);
  ASSIGNMOB(3067, cityguard);
  ASSIGNMOB(3068, janitor);
  ASSIGNMOB(3095, cryogenicist);
  ASSIGNMOB(3105, mayor);
  ASSIGNMOB(3069, wiseman);
  ASSIGNMOB(3097, newbie_guide);
    
  /* DEATH AND DYING */
  

  /* Three of Swords */
  ASSIGNMOB(3310, receptionist);

  /* MORIA */
  ASSIGNMOB(4000, snake);
  ASSIGNMOB(4001, snake);
  ASSIGNMOB(4053, snake);
  ASSIGNMOB(4100, magic_user);
  ASSIGNMOB(4102, snake);
  ASSIGNMOB(4103, thief);

  /* Redferne's */
  ASSIGNMOB(7900, cityguard);

  /* PYRAMID */
  ASSIGNMOB(5300, snake);
  ASSIGNMOB(5301, snake);
  ASSIGNMOB(5304, thief);
  ASSIGNMOB(5305, thief);
  ASSIGNMOB(5309, magic_user); /* should breath fire */
  ASSIGNMOB(5311, magic_user);
  ASSIGNMOB(5313, magic_user); /* should be a cleric */
  ASSIGNMOB(5314, magic_user); /* should be a cleric */
  ASSIGNMOB(5315, magic_user); /* should be a cleric */
  ASSIGNMOB(5316, magic_user); /* should be a cleric */
  ASSIGNMOB(5317, magic_user);

  /* High Tower Of Sorcery */
  ASSIGNMOB(2501, magic_user); /* should likely be cleric */
  ASSIGNMOB(2504, magic_user);
  ASSIGNMOB(2507, magic_user);
  ASSIGNMOB(2508, magic_user);
  ASSIGNMOB(2510, magic_user);
  ASSIGNMOB(2511, thief);
  ASSIGNMOB(2514, magic_user);
  ASSIGNMOB(2515, magic_user);
  ASSIGNMOB(2516, magic_user);
  ASSIGNMOB(2517, magic_user);
  ASSIGNMOB(2518, magic_user);
  ASSIGNMOB(2520, magic_user);
  ASSIGNMOB(2521, magic_user);
  ASSIGNMOB(2522, magic_user);
  ASSIGNMOB(2523, magic_user);
  ASSIGNMOB(2524, magic_user);
  ASSIGNMOB(2525, magic_user);
  ASSIGNMOB(2526, magic_user);
  ASSIGNMOB(2527, magic_user);
  ASSIGNMOB(2528, magic_user);
  ASSIGNMOB(2529, magic_user);
  ASSIGNMOB(2530, magic_user);
  ASSIGNMOB(2531, magic_user);
  ASSIGNMOB(2532, magic_user);
  ASSIGNMOB(2533, magic_user);
  ASSIGNMOB(2534, magic_user);
  ASSIGNMOB(2536, magic_user);
  ASSIGNMOB(2537, magic_user);
  ASSIGNMOB(2538, magic_user);
  ASSIGNMOB(2540, magic_user);
  ASSIGNMOB(2541, magic_user);
  ASSIGNMOB(2548, magic_user);
  ASSIGNMOB(2549, magic_user);
  ASSIGNMOB(2552, magic_user);
  ASSIGNMOB(2553, magic_user);
  ASSIGNMOB(2554, magic_user);
  ASSIGNMOB(2556, magic_user);
  ASSIGNMOB(2557, magic_user);
  ASSIGNMOB(2559, magic_user);
  ASSIGNMOB(2560, magic_user);
  ASSIGNMOB(2562, magic_user);
  ASSIGNMOB(2564, magic_user);

  /* SEWERS */
  ASSIGNMOB(7006, snake);
  ASSIGNMOB(7009, magic_user);
  ASSIGNMOB(7200, magic_user);
  ASSIGNMOB(7201, magic_user);
  ASSIGNMOB(7202, magic_user);

  /* FOREST */
  ASSIGNMOB(6112, magic_user);
  ASSIGNMOB(6113, snake);
  ASSIGNMOB(6114, magic_user);
  ASSIGNMOB(6115, magic_user);
  ASSIGNMOB(6116, magic_user); /* should be a cleric */
  ASSIGNMOB(6117, magic_user);

  /* ARACHNOS */
  ASSIGNMOB(6302, magic_user);
  ASSIGNMOB(6309, magic_user);
  ASSIGNMOB(6312, magic_user);
  ASSIGNMOB(6314, magic_user);
  ASSIGNMOB(6315, magic_user);

  /* Desert */
  ASSIGNMOB(5004, magic_user);
  ASSIGNMOB(5005, dragon_guard); /* brass dragon */
  ASSIGNMOB(5010, magic_user);
  ASSIGNMOB(5014, magic_user);

  /* Drow City */
  ASSIGNMOB(5103, magic_user);
  ASSIGNMOB(5104, magic_user);
  ASSIGNMOB(5107, magic_user);
  ASSIGNMOB(5108, magic_user);

  /* Old Thalos */
  ASSIGNMOB(5200, magic_user);
  ASSIGNMOB(5201, magic_user);
  ASSIGNMOB(5209, magic_user);

  /* New Thalos */
/* 5481 - Cleric (or Mage... but he IS a high priest... *shrug*) */
  ASSIGNMOB(5404, receptionist);
  ASSIGNMOB(5421, magic_user);
  ASSIGNMOB(5422, magic_user);
  ASSIGNMOB(5423, magic_user);
  ASSIGNMOB(5424, magic_user);
  ASSIGNMOB(5425, magic_user);
  ASSIGNMOB(5426, magic_user);
  ASSIGNMOB(5427, magic_user);
  ASSIGNMOB(5428, magic_user);
  ASSIGNMOB(5434, cityguard);
  ASSIGNMOB(5440, magic_user);
  ASSIGNMOB(5455, magic_user);
  ASSIGNMOB(5461, cityguard);
  ASSIGNMOB(5462, cityguard);
  ASSIGNMOB(5463, cityguard);
  ASSIGNMOB(5482, cityguard);
  ASSIGNMOB(6302, dragon_acid);		/* Yevaud */
/*
5400 - Guildmaster (Mage)
5401 - Guildmaster (Cleric)
5402 - Guildmaster (Warrior)
5403 - Guildmaster (Thief)
5456 - Guildguard (Mage)
5457 - Guildguard (Cleric)
5458 - Guildguard (Warrior)
5459 - Guildguard (Thief)
*/

  /* ROME */
  ASSIGNMOB(12009, magic_user);
  ASSIGNMOB(12018, cityguard);
  ASSIGNMOB(12020, magic_user);
  ASSIGNMOB(12021, cityguard);
  ASSIGNMOB(12025, magic_user);
  ASSIGNMOB(12030, magic_user);
  ASSIGNMOB(12031, magic_user);
  ASSIGNMOB(12032, magic_user);

  /* King Welmar's Castle (not covered in castle.c) */
  ASSIGNMOB(15015, thief);      /* Ergan... have a better idea? */
  ASSIGNMOB(15032, magic_user); /* Pit Fiend, have something better?  Use it */

  /* DWARVEN KINGDOM */
  ASSIGNMOB(6500, cityguard);
  ASSIGNMOB(6502, magic_user);
  ASSIGNMOB(6509, magic_user);
  ASSIGNMOB(6516, magic_user);
  ASSIGNMOB(6600, sund_earl);        /* Earl of Sundhaven */
  ASSIGNMOB(6601, cityguard);
  ASSIGNMOB(6602, hangman);
  ASSIGNMOB(6664, postmaster);
//  ASSIGNMOB(6656, guild_guard);
//  ASSIGNMOB(6655, guild_guard); 
//  ASSIGNMOB(6658, guild_guard);
//  ASSIGNMOB(6657, guild_guard);
  ASSIGNMOB(6666, stu);
  ASSIGNMOB(6606, fido);             /* Smoke rat */
//  ASSIGNMOB(6616, guild);
//  ASSIGNMOB(6619, guild);
//  ASSIGNMOB(6617, guild);  
//  ASSIGNMOB(6618, guild);
  ASSIGNMOB(6659, cityguard);
  ASSIGNMOB(6660, cityguard);    
  ASSIGNMOB(6607, thief);
  ASSIGNMOB(6648, butcher);
  ASSIGNMOB(6661, blinder);
  ASSIGNMOB(6637, silktrader);
  ASSIGNMOB(6615, idiot);
  ASSIGNMOB(6653, athos);
  ASSIGNMOB(6604, cleric);
  
  
  ASSIGNMOB(7040, dragon_fire);		/* red dragon */


 /* New Sparta */
 ASSIGNMOB(21003, cityguard); 
 ASSIGNMOB(21084, janitor);
 ASSIGNMOB(21085, fido);
 ASSIGNMOB(21057, thief);
 ASSIGNMOB(21078, receptionist);
 

 /* Abandoned Cathedral */
 ASSIGNMOB(22003, magic_user);
 ASSIGNMOB(22004, magic_user);


 /* Vice Island */
 ASSIGNMOB(22101, slave);
 ASSIGNMOB(22102, slave);
 ASSIGNMOB(22103, slave);
 ASSIGNMOB(22104, slave);
 ASSIGNMOB(22105, slave);
 ASSIGNMOB(22106, master);
 ASSIGNMOB(22110, Priest_of_Fear);
 ASSIGNMOB(22113, butcher);
 ASSIGNMOB(22119, Grand_Inquisitor);
 ASSIGNMOB(22116, High_Priest_of_Terror);
 ASSIGNMOB(22116, High_Priest_of_Terror);
 ASSIGNMOB(22111, sea_serpent);
 ASSIGNMOB(22108, tarbaby);
 ASSIGNMOB(22114, High_Priest_of_Terror);
 
 /* Grave */
 ASSIGNMOB(3202, grave_undertaker);
 ASSIGNMOB(3212, grave_demilich);
 ASSIGNMOB(3205, grave_ghoul);
 ASSIGNMOB(3203, grave_priest);
 
 
}



/* assign special procedures to objects */
void assign_objects(void)
{
  SPECIAL(bank);
  SPECIAL(gen_board);
  SPECIAL(tokens);
  SPECIAL(marbles);
  SPECIAL(portal);
  SPECIAL(recharger);
  SPECIAL(pop_dispenser);

  return;
  ASSIGNOBJ(9017, tokens);
  ASSIGNOBJ(3088, recharger);
  ASSIGNOBJ(3096, gen_board);	/* social board */
  ASSIGNOBJ(3097, gen_board);	/* freeze board */
  ASSIGNOBJ(3098, gen_board);	/* immortal board */
  ASSIGNOBJ(3099, gen_board);	/* mortal board */

  ASSIGNOBJ(3034, bank);	/* atm */
  ASSIGNOBJ(3036, bank);	/* cashcard */
  
  ASSIGNOBJ(6612, bank);
  ASSIGNOBJ(6647, marbles);
  ASSIGNOBJ(6709, marbles);
  
  ASSIGNOBJ(31, portal);
  ASSIGNOBJ(21031, pop_dispenser);
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  int i;

  
/* 
  ASSIGNROOM(3001, hometown_room);
  ASSIGNROOM(1204, hometown_room);
  ASSIGNROOM(5509, hometown_room);
  ASSIGNROOM(21100, hometown_room);
  ASSIGNROOM(6601, hometown_room);
  ASSIGNROOM(3030, dump);
  ASSIGNROOM(3031, pet_shops);
  ASSIGNROOM(3082, horse_shops);
*/
  if (dts_are_dumps)
    for (i = 0; i < top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	world[i].func = dump;
}

