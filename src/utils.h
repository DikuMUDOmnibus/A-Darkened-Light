/* 
************************************************************************
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __UTILS_H__
#define __UTILS_H__

/* external declarations and prototypes **********************************/

#define log 	basic_mud_log

/* public functions in utils.c */
extern FILE *logfile;
char	*str_dup(const char *source);
int	str_cmp(const char *arg1, const char *arg2);
int	strn_cmp(const char *arg1, const char *arg2, int n);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
int	touch(const char *path);
void	mudlog(const char *str, int type, int level, int file);
void	log_death_trap(struct char_data *ch);
int	number(int from, int to);
int	dice(int number, int size);
void	sprintbit(long vektor, const char *names[], char *result);
void	sprintbit_multi(long vektor[], const char *names[], char *result);
void	sprinttype(int type, const char *names[], char *result);
int	get_line(FILE *fl, char *buf);
int	get_filename(char *orig_name, char *filename, int mode);
struct  time_info_data *age(struct char_data *ch);
int	num_pc_in_room(struct room_data *room);
int     replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size);
void    format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen);
char*   stripcr(char *dest, const char *src);
void    menubit(long_bitv bitvector, const char *names[], char *result, long_bitv mask);
void    updatebit(long_bitv bitvector, int num_flags, int arg, long_bitv mask);
void 	show_spec_proc_table(struct specproc_info table[], int level, char *result);
int 	get_spec_proc_index(struct specproc_info table[], int level, int arg);
int 	get_spec_proc(struct specproc_info table[], const char* arg);
int get_spec_name(const struct specproc_info table[], SPECIAL(fname));
void    set_afk(struct char_data *ch, char *afk_message, int result);
void	core_dump_real(const char *, ush_int);
void 	writebit_multi(long_bitv bitvector, int sizebitv, char *buf);
int 	is_setbit_multi(long_bitv bitvector, int sizebitv);
int 	get_bitv_from_string(char *str, const char *list[], long_bitv bitvector, int max);
int 	search_block_flag(char *str, const char *list[]);

#define core_dump()		core_dump_real(__FILE__, __LINE__)

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

#define GET_FIGHTING(ch)  ((ch)->char_specials.event_fight)
#define GET_POINTS_EVENT(ch, i)		((ch)->points_event[i])
#define GET_DAMAGE_EVENTS(ch)		((ch)->damage_events)


/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);

/* in magic.c */
bool	circle_follow(struct char_data *ch, struct char_data * victim);

/* in act.informative.c */
void	look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int	do_simple_move(struct char_data *ch, int dir, int following);
int	perform_move(struct char_data *ch, int dir, int following);

/* in limits.c */
int	mana_limit(struct char_data *ch);
int	hit_limit(struct char_data *ch);
int	move_limit(struct char_data *ch);
int	mana_gain(struct char_data *ch);
int	hit_gain(struct char_data *ch);
int	move_gain(struct char_data *ch);
void	advance_level(struct char_data *ch);
void	set_title(struct char_data *ch, char *title);
void	gain_exp(struct char_data *ch, int gain);
void	gain_exp_regardless(struct char_data *ch, int gain);
void	gain_condition(struct char_data *ch, int condition, int value);
void	check_idling(struct char_data *ch);
void	point_update(void);
void	update_pos(struct char_data *victim);
void    blood_update(void);
void    darkfog_update(void);
void    gain_demonxp(struct char_data *ch, int gain);
/* various constants *****************************************************/


/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define ALIAS_FILE	2
#define OBJPOS_FILE	3
#define SAVE_FILE	4

/* breadth-first searching */
#define BFS_ERROR		-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		-3

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */
#define SECS_PER_MUD_HOUR	75
#define SECS_PER_MUD_DAY	(24*SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH	(35*SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR	(17*SECS_PER_MUD_MONTH)

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a) ((a) ? "&GYES&w" : "&RNO &w")
#define ONOFF(a) ((a) ? "&GON &w" : "&ROFF&w")

/* IS_UPPER and IS_LOWER added by dkoepke */
#define IS_UPPER(c) ((c) >= 'A' && (c) <= 'Z')
#define IS_LOWER(c) ((c) >= 'a' && (c) <= 'z')

#define LOWER(c)   (IS_UPPER(c) ? ((c)+('a'-'A')) : (c))
#define UPPER(c)   (IS_LOWER(c) ? ((c)+('A'-'a')) : (c))

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
#define IF_STR(st) ((st) ? (st) : "\0")
#define CAP(st)  (*(st) = UPPER(*(st)), st)

#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")


/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
		{ perror("malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

#define ZON_FLAGS_BITV(zone)	(&((zone)->flags))
#define ZON_FLAGS(zone) ((zone)->zon_flags)
#define ZON_OWNER(zone) ((zone)->owner)
#define ZON_MASTER(zone) ((zone)->master_mob)
#define ZON_FLAGGED(zone, flag) (IS_SET(ZON_FLAGS(zone), (flag)))

#define MOB_FLAGS_BITV(ch)	((ch)->char_specials.saved.act)
#define MOB_FLAGS(ch) 	((ch)->char_specials.saved.act[0])
#define MOB2_FLAGS(ch) 	((ch)->char_specials.saved.act[1])
#define MOB3_FLAGS(ch) 	((ch)->char_specials.saved.act[2])
#define MOB4_FLAGS(ch) 	((ch)->char_specials.saved.act[3])

#define PLR_FLAGS_BITV(ch) ((ch)->char_specials.saved.act)
#define PLR_FLAGS(ch) ((ch)->char_specials.saved.act[0])
#define PLR2_FLAGS(ch) ((ch)->char_specials.saved.act[1])
#define PLR3_FLAGS(ch) ((ch)->char_specials.saved.act[2])
#define PLR4_FLAGS(ch) ((ch)->char_specials.saved.act[3])

#define GET_PLAYER_KILLS(ch) ((ch)->player_specials->saved.player_kills)


/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define PRF_FLAGS_BITV(ch) CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.pref))
#define PRF_FLAGS(ch) (PRF_FLAGS_BITV(ch)[0])
#define PRF2_FLAGS(ch) (PRF_FLAGS_BITV(ch)[1])
#define PRF3_FLAGS(ch) (PRF_FLAGS_BITV(ch)[2])
#define PRF4_FLAGS(ch) (PRF_FLAGS_BITV(ch)[3])

#define AFF_FLAGS_BITV(ch) ((ch)->char_specials.saved.affected_by)
#define AFF_FLAGS(ch) ((ch)->char_specials.saved.affected_by[0])
#define AFF2_FLAGS(ch) ((ch)->char_specials.saved.affected_by[1])
#define AFF3_FLAGS(ch) ((ch)->char_specials.saved.affected_by[2])
#define AFF4_FLAGS(ch) ((ch)->char_specials.saved.affected_by[3])

#define OBJ_AFF_FLAGS_BITV(obj) ((obj)->obj_flags.bitvector)
#define OBJ_AFF_FLAGS(obj) ((obj)->obj_flags.bitvector[0])
#define OBJ_AFF2_FLAGS(obj) ((obj)->obj_flags.bitvector[1])
#define OBJ_AFF3_FLAGS(obj) ((obj)->obj_flags.bitvector[2])
#define OBJ_AFF4_FLAGS(obj) ((obj)->obj_flags.bitvector[3])

#define ROOM_FLAGS_BITV(loc) (world[(loc)].room_flags)
#define ROOM_FLAGS(loc) (world[(loc)].room_flags[0])
#define ROOM2_FLAGS(loc) (world[(loc)].room_flags[1])
#define ROOM3_FLAGS(loc) (world[(loc)].room_flags[2])
#define ROOM4_FLAGS(loc) (world[(loc)].room_flags[3])

#define ROOM_AFFECTIONS(loc)    (world[(loc)].room_affections)
#define ROOM_AFFECTED(loc, aff) (IS_SET(ROOM_AFFECTIONS(loc), (aff)))
#define ROOM_ORIG_AFFECTIONS(loc)    (world[(loc)].orig_affections)
#define ROOM_ORIG_AFFECTED(loc, aff) (IS_SET(ROOM_ORIG_AFFECTIONS(loc), (aff)))

#define MULTI_FLAGS(ch) ((ch)->player_specials->saved.multi_flags)

#define IS_NPC(ch)  (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)  (IS_NPC(ch) && ((ch)->nr >-1))

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define MOB2_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB2_FLAGS(ch), (flag)))
#define MOB3_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB3_FLAGS(ch), (flag)))
#define MOB4_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB4_FLAGS(ch), (flag)))

#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define PLR2_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR2_FLAGS(ch), (flag)))
#define PLR3_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR3_FLAGS(ch), (flag)))
#define PLR4_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR4_FLAGS(ch), (flag)))

#define AFF_FLAGGED(ch, flag) (IS_SET(AFF_FLAGS(ch), (flag)))
#define AFF2_FLAGGED(ch, flag) (IS_SET(AFF2_FLAGS(ch), (flag)))
#define AFF3_FLAGGED(ch, flag) (IS_SET(AFF3_FLAGS(ch), (flag)))
#define AFF4_FLAGGED(ch, flag) (IS_SET(AFF4_FLAGS(ch), (flag)))

#define PRF_FLAGGED(ch, flag) (IS_SET(PRF_FLAGS(ch), (flag)))
#define PRF2_FLAGGED(ch, flag) (IS_SET(PRF2_FLAGS(ch), (flag)))
#define PRF3_FLAGGED(ch, flag) (IS_SET(PRF3_FLAGS(ch), (flag)))
#define PRF4_FLAGGED(ch, flag) (IS_SET(PRF4_FLAGS(ch), (flag)))


#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS(loc), (flag)))
#define ROOM2_FLAGGED(loc, flag) (IS_SET(ROOM2_FLAGS(loc), (flag)))
#define ROOM3_FLAGGED(loc, flag) (IS_SET(ROOM3_FLAGS(loc), (flag)))
#define ROOM4_FLAGGED(loc, flag) (IS_SET(ROOM4_FLAGS(loc), (flag)))

#define EXIT_FLAGGED(exit, flag) (IS_SET((exit)->exit_info, (flag)))
#define OBJVAL_FLAGGED(obj, flag) (IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag) (IS_SET((obj)->obj_flags.wear_flags, (flag)))
#define OBJ_FLAGGED(obj, flag) (IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define OBJ2_FLAGGED(obj, flag) (IS_SET(GET_OBJ_EXTRA2(obj), (flag)))
#define OBJ3_FLAGGED(obj, flag) (IS_SET(GET_OBJ_EXTRA3(obj), (flag)))
#define OBJ4_FLAGGED(obj, flag) (IS_SET(GET_OBJ_EXTRA4(obj), (flag)))

#define GET_OBJ_BOTTOM_LEV(obj)	((obj)->obj_flags.bottom_level)
#define GET_OBJ_TOP_LEV(obj)	((obj)->obj_flags.top_level)

#define MULTI_FLAGGED(ch, flag) (IS_SET(MULTI_FLAGS(ch), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

#define INTERNAL(ch)	((ch)->internal_flags)

/* room utils ************************************************************/

#define GET_ROOM_ZONE(room)	(world[(room)].zone)
#define ROOM_OWNER(room)	(zone_table[GET_ROOM_ZONE(room)].owner)

// #define ZONE_OWNER(zone_vnum)	(

#define SECT(room)	((int) world[(room)].sector_type)

#define IS_DARK(room)  ( !world[room].light && \
                         (ROOM_FLAGGED(room, ROOM_DARK) || \
                          ( ( SECT(room) != SECT_INSIDE && \
                              SECT(room) != SECT_CITY ) && \
                            (sunlight == SUN_SET || \
			     sunlight == SUN_DARK)) ) )


#define IS_FOG(room)  ( !world[room].light && \
                         (ROOM_FLAGGED(room, ROOM_DARK) || \
                          ( ( SECT(room) != SECT_INSIDE && \
                              SECT(room) != SECT_CITY ) && \
                            (sunlight == SUN_SET || \
                             sunlight == SUN_DARK)) ) )


// #define ENEMY_ROOM(ch, room)	(
// #define MAY_ENTER(ch, room)	(PLAYERCLAN(ch) != 0 && ENEMY_ROOM(ch, room))

#define IS_LIGHT(room)  (!IS_DARK(room))

#define GET_ROOM_VNUM(rnum)	((rnum) >= 0 && (rnum) <= top_of_world ? world[(rnum)].number : NOWHERE)
#define GET_ROOM_SPEC(room) ((room) >= 0 ? world[(room)].func : NULL)

/* char utils ************************************************************/

#define IN_ROOM(ch)	((ch)->in_room)
#define GET_WAS_IN(ch)	((ch)->was_in_room)

/* CLASS EXP TYPE */
//#define GET_DEMONXP(ch) ((ch)->player_specials->saved.demonxp)

// other materials


/* CLASS POWERS FOR WARRIOR *
#define GET_POWER1(ch) ((ch)->player_specials->saved.power_parras1)
#define GET_POWER2(ch) ((ch)->player_specials->saved.power_parras2)
#define GET_POWER3(ch) ((ch)->player_specials->saved.power_parras3)
#define GET_POWER4(ch) ((ch)->player_specials->saved.power_parras4)
#define GET_POWER5(ch) ((ch)->player_specials->saved.power_parras5)
#define GET_POWER6(ch) ((ch)->player_specials->saved.power_parras6)
#define GET_POWER7(ch) ((ch)->player_specials->saved.power_parras7)
#define GET_POWER8(ch) ((ch)->player_specials->saved.power_parras8)
#define GET_POWER9(ch) ((ch)->player_specials->saved.power_parras9)
#define GET_POWER10(ch
 CLASS POWERS FOR NINJA 
#define GET_POWER13(ch) ((ch)->player_specials->saved.power_parras13)
#define GET_POWER14(ch) ((ch)->player_specials->saved.power_parras14)
#define GET_POWER15(ch) ((ch)->player_specials->saved.power_parras15)
#define GET_POWER16(ch) ((ch)->player_specials->saved.power_parras16)
#define GET_POWER17(ch) ((ch)->player_specials->saved.power_parras17)
#define GET_POWER18(ch) ((ch)->player_specials->saved.power_parras18)
*/
/* CLASS_POWERS FOR VAMPIRE */
//#define GET_CELERITY(ch) ((ch)->player_specials->saved.celerity)
#define VAMP_POWER1(ch) ((ch)->player_specials->saved.vamp_power1)


/* END CLASS SHIT */
#define GET_TEMP_GOLD(ch) ((ch)->player_specials->saved.temp_gold)
#define GET_POWERL(ch)   ((ch)->player_specials->saved.powerl)
#define QPOINTS(ch)	((ch)->player_specials->saved.qpoints)
#define GET_AGE(ch)     (age(ch)->year)
#define GET_NAME(ch)    (IS_NPC(ch) ? \
			 (ch)->player.short_descr : (ch)->player.name)
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_PASSWD(ch)	((ch)->player.passwd)
#define GET_PROMPT(ch)	((ch)->player.prompt)
#define GET_PFILEPOS(ch)((ch)->pfilepos)
#define GET_NO_LARM(ch) ((ch)->player_specials->saved.no_larm)
#define GET_NO_RARM(ch) ((ch)->player_specials->saved.no_rarm)
#define GET_NO_LLEG(ch) ((ch)->player_specials->saved.no_lleg)
#define GET_NO_RLEG(ch) ((ch)->player_specials->saved.no_rleg)
#define GET_NO_WINDPIPE(ch) ((ch)->player_specials->saved.no_wpipe)
#define GET_NO_FACE(ch) ((ch)->player_specials->saved.no_face)
#define GET_LEGEND_LEVELS(ch) ((ch)->player_specials->saved.llevels)
#define GET_TOT_LEVEL(ch) ((ch)->player_specials->saved.total_level)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_CLASS(ch)   ((ch)->player.chclass)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_SEX(ch)	((ch)->player.sex)
#define GET_HOMETOWN(ch)	((ch)->player.hometown)

//#define GET_STR(ch)  ((ch)->player_specials->saved.str)
#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_ADD(ch)     ((ch)->aff_abils.str_add)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_CHA(ch)     ((ch)->aff_abils.cha)

//#define GET_PKILLP        ((ch)->player_specials->saved.pkillp)
//#define GET_DEMONXP(ch)   ((ch)->player_specials->saved.demonxp)
#define GET_EXP(ch)	  ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)	  ((ch)->points.hit)
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
#define GET_MOVE(ch)	  ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)	  ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_GOLD(ch)	  ((ch)->points.gold)
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)
/*
#define GET_CELERITY(ch)  ((ch)->player_specials->saved.celerity)
#define GET_BRENNUM(ch)   ((ch)->player_specials->saved.brennum)
#define GET_AFILASE(ch)   ((ch)->player_specials->saved.afilase)
#define GET_TUTULA(ch)    ((ch)->player_specials->saved.tutula)
#define GET_NINJITSU(ch)  ((ch)->player_specials->saved.ninjitsu)
#define GET_SAMURAI(ch)   ((ch)->player_specials->saved.samurai)
#define GET_BUDOKAI(ch)   ((ch)->player_specials->saved.budokai)
#define GET_KI(ch)	  ((ch)->player_specials->saved.ki)
#define GET_MITUS(ch)     ((ch)->player_specials->saved.mitus)
#define GET_XIAN(ch)      ((ch)->player_specials->saved.xian)
#define GET_MULIAN(ch)    ((ch)->player_specials->saved.mulian)
#define GET_HYBRID(ch)    ((ch)->player_specials->saved.hybrid)
*/
#define GET_CELERITY(ch)  ((ch)->points.celerity)
#define GET_BRENNUM(ch)   ((ch)->points.brennum)
#define GET_AFILASE(ch)   ((ch)->points.afilase)
#define GET_TUTULA(ch)    ((ch)->points.tutula)
#define GET_NINJITSU(ch)  ((ch)->points.ninjitsu)
#define GET_SAMURAI(ch)   ((ch)->points.samurai)
#define GET_BUDOKAI(ch)   ((ch)->points.budokai)
#define GET_KI(ch)	  ((ch)->points.ki)
#define GET_MITUS(ch)     ((ch)->points.mitus)
#define GET_XIAN(ch)      ((ch)->points.xian)
#define GET_MULIAN(ch)    ((ch)->points.mulian)
#define GET_HYBRID(ch)    ((ch)->points.hybrid)
#define GET_PKILLP            ((ch)->points.pkillp)
#define GET_DEMONXP(ch)       ((ch)->points.demonxp)


#define GET_POS(ch)	  ((ch)->char_specials.position)
#define GET_LEAVE_DIR(ch)	((ch)->char_specials.leavedir)
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
#define HUNTING(ch)	  ((ch)->char_specials.hunting)
#define RIDING(ch)	  ((ch)->char_specials.riding)		/* (DAK) */
#define RIDDEN_BY(ch)	  ((ch)->char_specials.ridden_by)	/* (DAK) */
#define GET_SAVE(ch, i)	  ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

#define GET_COND(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.conditions[(i)]))
#define GET_LOADROOM(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.load_room))
#define GET_PRACTICES(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.spells_to_learn))
#define GET_INVIS_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.invis_level))
#define GET_WIMP_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.wimp_level))
#define GET_FREEZE_LEV(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.freeze_level))
#define GET_BAD_PWS(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.bad_pws))
#define GET_TALK(ch, i)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.talks[i]))
#define POOFIN(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_LAST_OLC_TARG(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_targ))
#define GET_LAST_OLC_MODE(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_olc_mode))
#define GET_ALIASES(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
  
#define GET_SKILL(ch, i)	CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.skills[i]))
#define SET_SKILL(ch, i, pct)	do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->saved.skills[i]) = pct; } while(0)

#define GET_RIP_CNT(ch)         CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.rip_cnt))
#define GET_KILL_CNT(ch)        CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.kill_cnt))
#define GET_DT_CNT(ch)          CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->saved.dt_cnt))
#define GET_AFK(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->afk_message))

#define GET_EQ(ch, i)		((ch)->equipment[i])

#define GET_MOB_SPEC(ch) 	(IS_MOB(ch) ? (mob_index[(ch->nr)].func) : NULL)
#define GET_MOB_SPEC_INDEX(ch) 	(IS_MOB(ch) ? (get_spec_name(mob_procs, mob_index[(ch->nr)].func)) : 0)
#define SET_MOB_SPEC(ch, i) 	{if (IS_MOB(ch)) mob_index[(ch->nr)].func = i;}
#define SET_MOB_SPEC_INDEX(ch, i) {if (IS_MOB(ch)) mob_index[(ch->nr)].func_index = i;}
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].vnum : -1)

#define GET_MOB_WAIT(ch)	((ch)->mob_specials.wait_state)
/* Use only on mob protos!!! */
#define GET_MOB_PRICE(ch)	(GET_MOB_WAIT(ch))
#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
#define MEMORY(ch)		((ch)->mob_specials.memory)

#define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch)==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )

#define CAN_CARRY_W(ch) (str_app[STRENGTH_APPLY_INDEX(ch)].carry_w)
#define CAN_CARRY_N(ch) (5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)) \
     || GET_CLASS(ch) == CLASS_CYBORG)

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_IMMORT(ch)	(GET_LEVEL(ch) >= LVL_IMMORT)

#define lvD2(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 2)))
#define lvD3(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 3)))
#define lvD4(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 4)))
#define lvD5(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 5)))
#define lvD6(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 6)))
#define lvD8(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 8)))
#define lvD10(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 10)))
#define lvD12(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 12)))
#define lvD15(ch) ( MAX(1, (int) (GET_LEVEL(ch) / 15)))

/*
#define IS_WIZARD(ch)	(GET_CLASS(ch) == CLASS_MAGIC_USER  || 
			 GET_CLASS(ch) == CLASS_CLERIC      ||
			 GET_CLASS(ch) == CLASS_NECROMANCER ||
			 GET_CLASS(ch) == CLASS_ALCHEMIST   ||
			 GET_CLASS(ch) == CLASS_DRUID       ||
			 GET_CLASS(ch) == CLASS_WARLOCK)
			 
#define IS_FIGHTER(ch)	(GET_CLASS(ch) == CLASS_MAGIC_USER  || 
			 GET_CLASS(ch) == CLASS_CLERIC      ||
			 GET_CLASS(ch) == CLASS_NECROMANCER ||
			 GET_CLASS(ch) == CLASS_ALCHEMIST   ||
			 GET_CLASS(ch) == CLASS_DRUID       ||
			 GET_CLASS(ch) == CLASS_WARLOCK)
*/
/* descriptor-based utils ************************************************/


#define WAIT_STATE(ch, cycle) { \
	if ((ch)->desc) (ch)->desc->wait = (cycle); \
	else if (IS_NPC(ch)) GET_MOB_WAIT(ch) = (cycle); }

#define CHECK_WAIT(ch)	(((ch)->desc) ? ((ch)->desc->wait > 1) : 0)
#define STATE(d)	((d)->connected)


/* object utils **********************************************************/


#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)

#define GET_OBJ_EXTRA_BITV(obj)	((obj)->obj_flags.extra_flags)
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags[0])
#define GET_OBJ_EXTRA2(obj)	((obj)->obj_flags.extra_flags[1])
#define GET_OBJ_EXTRA3(obj)	((obj)->obj_flags.extra_flags[2])
#define GET_OBJ_EXTRA4(obj)	((obj)->obj_flags.extra_flags[3])

#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
// #define GET_OBJ_VAL_BITV(obj, val)	(&((obj)->obj_flags.value[(val)]))
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(GET_OBJ_RNUM(obj) >= 0 ? \
				 obj_index[GET_OBJ_RNUM(obj)].vnum : -1)
				 
#define GET_OBJ_OWNER_ID(obj)	((obj)->engraved_id)
				 
#define IS_OBJ_STAT(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags[0],stat))
#define IS_OBJ_STAT2(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags[1],stat))
#define IS_OBJ_STAT3(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags[2],stat))
#define IS_OBJ_STAT4(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags[3],stat))

#define IS_CORPSE(obj)		(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && \
					GET_OBJ_VAL((obj), 3) == 1)

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
	(obj_index[(obj)->item_number].func) : NULL)
	
#define GET_OBJ_SPEC_INDEX(obj) ((obj)->item_number >= 0 ? \
	(get_spec_name(obj_procs, obj_index[(obj)->item_number].func)) : 0)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))

#define IS_PK_ZONE(zone)	(!ZON_FLAGGED(&zone_table[zone], ZON_NO_PK))


#define CHAR_IN_PK_ZONE(ch)	(((ch)->in_room != NOWHERE) &&  \
				IS_PK_ZONE(GET_ROOM_ZONE(ch->in_room))) 		

#define LEVEL_OK(ch, vict)	(!(GET_LEVEL(ch) - GET_LEVEL(vict) > MAX_LEVEL_DIFF \
				|| (GET_LEVEL(vict) - GET_LEVEL(ch)) < MAX_LEVEL_DIFF))

#define ENEMY_CLAN(ch, vict)	((PLAYERCLAN(ch) != 0) && (PLAYERCLAN(vict) != 0) && \
				(CLANRANK(ch) > CLAN_APPLY) && (CLANRANK(vict) > CLAN_APPLY) && \
				(PLAYERCLAN(ch) != PLAYERCLAN(vict)))

#define CAN_MURDER(ch,vict)	(IS_NPC(ch) || IS_NPC(vict) || \
				(pk_allowed && \
				(GET_LEVEL(ch) < LVL_IMMORT) && \
				LEVEL_OK(ch, vict) && \
				ENEMY_CLAN(ch, vict) && \
				CHAR_IN_PK_ZONE(vict)))


/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!IS_AFFECTED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || IS_AFFECTED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 ((!IS_AFFECTED((obj),AFF_INVISIBLE) || IS_AFFECTED(sub,AFF_DETECT_INVIS)) && \
 (!IS_AFFECTED((obj), AFF_HIDE) || IS_AFFECTED(sub, AFF_SENSE_LIFE)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj) && \
	!ROOM_AFFECTED((obj)->in_room, RAFF_FOG))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
      IMM_CAN_SEE(sub, obj)))
   
#define MORT_CAN_SEE_ONETIME(sub, obj) (INVIS_OK(sub, obj))
   
#define IMM_CAN_SEE_ONETIME(sub, obj) \
   (MORT_CAN_SEE_ONETIME(sub, obj) || PRF_FLAGGED(sub, PRF_HOLYLIGHT))
   
#define CAN_SEE_ONETIME(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= GET_INVIS_LEV(obj)) && IMM_CAN_SEE_ONETIME(sub, obj)))

/* End of CAN_SEE */

#define IS_BURIED(obj) (IS_OBJ_STAT(obj, ITEM_BURIED))

#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj) \
   && !IS_BURIED(obj))

#define CAN_SEE_OBJ(sub, obj) \
	(MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))


#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? GET_NAME(ch) : "someone")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])

#define EXIT2(roomnum, door) (world[(roomnum)].dir_option[door])
#define CAN_GO2(roomnum, door) (EXIT2(roomnum, door) && \
                       (EXIT2(roomnum, door)->to_room != NOWHERE) && \
                       !IS_SET(EXIT2(roomnum,door)->exit_info, EX_CLOSED))

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))


#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])

#define IS_MAGIC_USER(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MAGIC_USER))
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_CLERIC))
#define IS_THIEF(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_THIEF))
#define IS_WARRIOR(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARRIOR))
#define IS_WARLOCK(ch)  	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARLOCK))
#define IS_PALADIN(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_PALADIN))
#define IS_RANGER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_RANGER))
#define IS_CYBORG(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_CYBORG))
#define IS_NECROMANCER(ch)  	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_NECROMANCER))
#define IS_DRUID(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_DRUID))
#define IS_ALCHEMIST(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_ALCHEMIST))
#define IS_BARBARIAN(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_BARBARIAN))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))

#define IS_WIZARD_TYPE(ch)	(IS_MAGIC_USER(ch) || IS_NECROMANCER(ch) || \
				 IS_WARLOCK(ch) || IS_ALCHEMIST(ch))

#define IS_CLERIC_TYPE(ch)	(IS_CLERIC(ch) || IS_DRUID(ch) || \
				 IS_PALADIN(ch))
				 
#define IS_WARRIOR_TYPE(ch)	(IS_WARRIOR(ch) || IS_BARBARIAN(ch) || \
				 IS_CYBORG(ch))

#define IS_THIEF_TYPE(ch)	(IS_THIEF(ch) || IS_RANGER(ch))

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif


/* Pretty general macros that could be used elsewhere */
#define IS_GOD(ch)		(!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD))
#define GET_OBJ_NUM(obj)	(obj->item_number)
#define END_OF(buffer)		((buffer) + strlen((buffer)))

/* others (added by Yilard) */

#define IS_SKILL(skill)		(skill > 130 && skill <= 200)


#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch, i) (IS_SKILL(i) ? \
                          prac_params[MIN_PER_PRAC_SKILL][(int)GET_CLASS(ch)] : \
                          prac_params[MIN_PER_PRAC_SPELL][(int)GET_CLASS(ch)])
#define MAXGAIN(ch, i) (IS_SKILL(i) ? \
                           prac_params[MAX_PER_PRAC_SKILL][(int)GET_CLASS(ch)] : \
                           prac_params[MAX_PER_PRAC_SPELL][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

#define HARMED(ch) \
	((AFF2_FLAGGED(ch, AFF2_BURNING) && !AFF2_FLAGGED(ch, AFF2_PROT_FIRE)) || \
      	(AFF2_FLAGGED(ch, AFF2_FREEZING) && !AFF2_FLAGGED(ch, AFF2_PROT_COLD)) || \
      	(AFF2_FLAGGED(ch, AFF2_ACIDED) && !AFF_FLAGGED(ch, AFF_STONESKIN)))
      	
#define IS_UNDEAD(ch) \
	(IS_NPC(ch) ? \
	  MOB_FLAGGED(ch, MOB_UNDEAD) : GET_CLASS(ch) == CLASS_NECROMANCER)
	  
#define NUM_MOB_PROCS	(num_mob_procs)
#define NUM_OBJ_PROCS	(num_obj_procs)
#define NUM_ROOM_PROCS	(num_room_procs)

#define AFK_DEFAULT	"I am afk, not looking at keyboard."

#define COPY_LONG_BITV(src, dest)	{	\
	int j; \
	for (j = 0; j < LONG_BITV_WIDTH; j++) dest[j] = src[j]; }

#endif /* __UTILS_H__ */
