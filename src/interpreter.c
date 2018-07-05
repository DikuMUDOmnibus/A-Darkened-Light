/* ************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "olc.h"
#include "clan.h"

extern sh_int r_mortal_start_room;
extern sh_int r_immort_start_room;
extern sh_int r_frozen_start_room;
extern sh_int r_death_start_room;
extern const char *class_menu;
extern char *motd;
extern char *imotd;
extern char *background;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int circle_restrict;
extern int no_specials;
extern int max_bad_pws;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern HOMETOWN hometowns[];
extern int top_of_zone_table;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern struct obj_data *object_list;
extern const char circlemud_version[];
extern char *spell_tables[];
extern sh_int mortal_start_room; /* config.c */
extern sh_int death_start_room; /* config.c */

/* external functions */
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch);
int parse_class(char arg);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void oedit_parse(struct descriptor_data *d, char *arg);
void redit_parse(struct descriptor_data *d, char *arg);
void zedit_parse(struct descriptor_data *d, char *arg);
void medit_parse(struct descriptor_data *d, char *arg);
void sedit_parse(struct descriptor_data *d, char *arg);
void gedit_parse(struct descriptor_data *d, char *arg);
void roll_real_abils(struct char_data *ch); 
void help_class(struct char_data *ch, int class);
void parse_clan_edit(struct descriptor_data *d, char *arg);
void update_player_clan(struct char_data *ch);
void print_world(char *buffer, int level);
//void do_decapitate(struct char_data * ch, struct char_data * killer);
/* local functions */
int perform_dupe_check(struct descriptor_data *d);
struct alias *find_alias(struct alias *alias_list, char *str);
void free_alias(struct alias *a);
void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias *a);
int perform_alias(struct descriptor_data *d, char *orig);
int reserved_word(char *argument);
int find_name(char *name);
int _parse_name(char *arg, char *name);


/* prototypes for all do_x functions. */
ACMD(do_test);
ACMD(do_action);
ACMD(do_advance);
ACMD(do_affects);
ACMD(do_afk);
ACMD(do_alias);
ACMD(do_areas);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_brew);
ACMD(do_buck);
ACMD(do_bury);
ACMD(do_cast);
ACMD(do_color);
ACMD(do_copyover);
ACMD(do_copyto);
ACMD(do_commands);
ACMD(do_compare);
ACMD(do_consider);
ACMD(do_chown);
ACMD(do_credits);
ACMD(do_create);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_delete);
ACMD(do_depiss);
ACMD(do_diagnose);
ACMD(do_addquest);
ACMD(do_dig);
ACMD(do_disarm);
ACMD(do_dismount);
ACMD(do_display);
ACMD(do_dns);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_file);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_forage);
ACMD(do_force);
ACMD(do_forge);
ACMD(do_gecho);
ACMD(do_gmote);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_links);
ACMD(do_load);
ACMD(do_look);
ACMD(do_meditate);
ACMD(do_mex);
ACMD(do_mount);
ACMD(do_multi);
/* ACMD(do_move); -- interpreter.h */
ACMD(do_mpasound);
ACMD(do_mpblock);
ACMD(do_mpjunk);
ACMD(do_mpecho);
ACMD(do_mpechoat);
ACMD(do_mpechoaround);
ACMD(do_mpkill);
ACMD(do_mpmload);
ACMD(do_mpoload);
ACMD(do_mppurge);
ACMD(do_mpgoto);
ACMD(do_mpat);
ACMD(do_mptransfer);
ACMD(do_mpforce);
ACMD(do_newbie);
ACMD(do_not_here);
ACMD(do_offer);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
ACMD(do_peace);
ACMD(do_players); 
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_prompt);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_reboot);
ACMD(do_relist);
ACMD(do_remove);
ACMD(do_rent);
ACMD(do_repiss);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_retreat);
ACMD(do_return);
ACMD(do_roomlink);
ACMD(do_save);
ACMD(do_say);
ACMD(do_scan);
ACMD(do_scare);
ACMD(do_score);
ACMD(do_scribe);
ACMD(do_send);
ACMD(do_set);
ACMD(do_settime);
ACMD(do_show);
ACMD(do_shoot);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_skillstat);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_speedwalk);
ACMD(do_spells);
ACMD(do_split);
ACMD(do_spring);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_tame);
ACMD(do_tedit);
ACMD(do_teleport);
ACMD(do_throw);
ACMD(do_pull);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_verify);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_vwear);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_xname);
ACMD(do_zreset);
ACMD(do_liblist);

/* Special commands for running programs from mud */
ACMD(do_ukill);
ACMD(do_prgexec);

/* CLASS POWERS */
/* Demon Powers */
ACMD(do_master);
ACMD(do_dropkick);
ACMD(do_expand);
ACMD(do_circle_strike);
ACMD(do_distort);
ACMD(do_twisterflip);
ACMD(do_deathsense);
ACMD(do_stop_time);
ACMD(do_earthshatter);
ACMD(do_cloak);
ACMD(do_gut);
ACMD(do_mastery);
ACMD(do_swallow);
ACMD(do_frighten);
ACMD(do_shadowplane);
ACMD(do_sense_spawn);
ACMD(do_push);
ACMD(do_darkness);
ACMD(do_fog);
ACMD(do_enchant);
ACMD(do_demoneq);
ACMD(do_claws);
ACMD(do_fangs);
ACMD(do_dragonform);
ACMD(do_clear_demon);
ACMD(do_piledriver);
ACMD(do_mist);
ACMD(do_spit_venom);
ACMD(do_scry);
/* Saiyan Powers */
ACMD(do_discipline);
ACMD(do_unveil);
ACMD(do_astralwalk);
ACMD(do_parry);
ACMD(do_deathblow);
ACMD(do_trip);
ACMD(do_punch);
ACMD(do_right_kick);
ACMD(do_left_kick);
ACMD(do_side_kick);
ACMD(do_elbow);
ACMD(do_invert_kick);
ACMD(do_kamehameha);
ACMD(do_power_up);
/* Vampire powers */
ACMD(do_divinity);
ACMD(do_freeze);
ACMD(do_burning);
ACMD(do_clear_vamp);
ACMD(do_talons);
ACMD(do_retract);
ACMD(do_talon_twist);
ACMD(do_vampeq);
ACMD(do_nightsight);
ACMD(do_vampin);
ACMD(do_vampout);
ACMD(do_shadowwalk);
ACMD(do_grant_vamp);
/* Wizard Powers */
ACMD(do_runes);
ACMD(do_book);
ACMD(do_evap_fog);
/* New commands added by Mayhem */
ACMD(do_relevel);
ACMD(do_decapitate);
ACMD(do_classme);
ACMD(do_rpoints);
ACMD(do_train_legend);
ACMD(do_haveleft);
ACMD(do_gain);
ACMD(do_train);
ACMD(do_bribe);
ACMD(do_recall);
ACMD(do_clear_research);
/* (FIDO) New commands here */
ACMD(do_world);
ACMD(do_clan);
ACMD(do_clan_say);

/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN },

  /* now, the main list */
  { "at"       , POS_DEAD    , do_at       , LVL_IMMORT, 0 },
  { "advance"  , POS_DEAD    , do_advance  , LVL_GRGOD, 0 },
  { "affects"  , POS_SITTING , do_affects  , 0, 0 },
  { "afk"      , POS_RESTING , do_afk      , 0, SCMD_AFK },
  { "afk*"     , POS_RESTING , do_afk      , 0, SCMD_LOCK_AFK },
  { "alias"    , POS_DEAD    , do_alias    , 0, 0 },
  { "accuse"   , POS_SITTING , do_action   , 0, 0 },
  { "applaud"  , POS_RESTING , do_action   , 0, 0 },
  { "areas"    , POS_DEAD    , do_areas    , 0, 0 },
  { "assist"   , POS_FIGHTING, do_assist   , 1, 0 },
  { "ask"      , POS_RESTING , do_spec_comm, 0, SCMD_ASK },
  { "auction"  , POS_SLEEPING, do_gen_comm , 0, SCMD_AUCTION },
  { "autoassist", POS_DEAD, do_gen_tog, 0, SCMD_AUTOASSIST},
  { "autoexit" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOEXIT },
  { "autogold" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOGOLD },
  { "autoloot" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOLOOT },
  { "automail" , POS_DEAD    , do_gen_tog  , 0, SCMD_AUTOMAIL }, 
  { "autosplit" , POS_DEAD    , do_gen_tog , 0, SCMD_AUTOSPLIT },

  { "bounce"   , POS_STANDING, do_action   , 0, 0 },
  { "backstab" , POS_STANDING, do_backstab , 1, 0 },
  { "ban"      , POS_DEAD    , do_ban      , LVL_GRGOD, 0 },
  { "balance"  , POS_STANDING, do_not_here , 1, 0 },
  { "bankxfer" , POS_STANDING, do_not_here , 1, 0 },
  { "bash"     , POS_FIGHTING, do_bash     , 1, 0 },
  { "beg"      , POS_RESTING , do_action   , 0, 0 },
  { "bleed"    , POS_RESTING , do_action   , 0, 0 },
  { "blush"    , POS_RESTING , do_action   , 0, 0 },
  { "bow"      , POS_STANDING, do_action   , 0, 0 },
  { "brb"      , POS_RESTING , do_action   , 0, 0 },
  { "brew"     , POS_STANDING, do_brew     , 0, 0 },
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "buck"     , POS_STANDING, do_buck	   , 0, 0 },
  { "buildbook", POS_DEAD    , do_gen_ps   , LVL_BUILDER, SCMD_BLDBOOK },
  { "burp"     , POS_RESTING , do_action   , 0, 0 },
  { "bury"     , POS_STANDING, do_bury	   , 0, 0 },
  { "buy"      , POS_STANDING, do_not_here , 0, 0 },
  { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG },
  { "Resurect" , POS_STANDING, do_bribe , 0, 0 },

  { "chant"     , POS_SITTING , do_cast     , 1, 0 },
  { "cackle"   , POS_RESTING , do_action   , 0, 0 },
  { "check"    , POS_STANDING, do_not_here , 1, 0 },
  { "chuckle"  , POS_RESTING , do_action   , 0, 0 },
  { "chown"    , POS_DEAD    , do_chown    , LVL_GOD, 0 },
  { "clan"     , POS_DEAD    , do_clan     , 0, 0 },
  { "clap"     , POS_RESTING , do_action   , 0, 0 },
  { "clear"    , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "consider" , POS_RESTING , do_consider , 0, 0 },
  { "color"    , POS_DEAD    , do_color    , 0, 0 },
  { "comfort"  , POS_RESTING , do_action   , 0, 0 },
  { "comb"     , POS_RESTING , do_action   , 0, 0 },
  { "compare"  , POS_SITTING , do_compare  , 0, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
  { "copyover" , POS_DEAD    , do_copyover , LVL_CODER, 0 },
  { "copyto"   , POS_DEAD    , do_copyto   , LVL_BUILDER, 0 },
  { "cough"    , POS_RESTING , do_action   , 0, 0 },
  { "create"   , POS_RESTING , do_create   , LVL_GRGOD, 0 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },
  { "cringe"   , POS_RESTING , do_action   , 0, 0 },
  { "cry"      , POS_RESTING , do_action   , 0, 0 },
  { "cuddle"   , POS_RESTING , do_action   , 0, 0 },
  { "curse"    , POS_RESTING , do_action   , 0, 0 },
  { "curtsey"  , POS_STANDING, do_action   , 0, 0 },
  { "clearresearch", POS_STANDING, do_clear_research, 0, 0 },
  { "classme"  , POS_STANDING, do_classme  , 0, 0 },
  { "decapitate", POS_STANDING, do_decapitate, 0, 0 },
  { "dance"    , POS_STANDING, do_action   , 0, 0 },
  { "date"     , POS_DEAD    , do_date     , LVL_GURU, SCMD_DATE },
  { "daydream" , POS_SLEEPING, do_action   , 0, 0 },
  { "dc"       , POS_DEAD    , do_dc       , LVL_GOD, 0 },
  { "delete"   , POS_DEAD    , do_delete   , LVL_BUILDER, SCMD_DELETE },
  { "depiss"   , POS_DEAD    , do_depiss   , LVL_MINOR_GOD, 0},
  { "deposit"  , POS_STANDING, do_not_here , 1, 0 },
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0 },
  { "addquest" , POS_DEAD    , do_addquest , LVL_IMMORT, 0 },
  { "disarm"   , POS_FIGHTING, do_disarm   , 1, 0 },
  { "dismount" , POS_STANDING, do_dismount , 0, 0 },
  { "display"  , POS_DEAD    , do_display  , 0, 0 },
  { "dig"      , POS_STANDING, do_dig	   , 0, 0 },
  { "dns"      , POS_DEAD    , do_dns      , LVL_COIMPL, 0 },
  { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE },
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP },
  { "drool"    , POS_RESTING , do_action   , 0, 0 },

  //{ "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_IMMORT, SCMD_ECHO },
  { "emote"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE },
  { ":"        , POS_RESTING, do_echo      , 1, SCMD_EMOTE },
  { "embrace"  , POS_STANDING, do_action   , 0, 0 },
  { "engrave"  , POS_STANDING, do_not_here , 0, 0 },
  { "enter"    , POS_STANDING, do_enter    , 0, 0 },
  { "equipment", POS_SLEEPING, do_equipment, 0, 0 },
  { "exits"    , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , POS_SITTING , do_examine  , 0, 0 },
  { "examunit" , POS_RESTING , do_gen_tog  , 1, SCMD_EXAMUNIT },

  { "force"    , POS_SLEEPING, do_force    , LVL_MINOR_GOD, 0 },
  { "fart"     , POS_RESTING , do_action   , 0, 0 },
  { "file"     , POS_DEAD    , do_file     , LVL_GRGOD, 0 },
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0 },
  { "flip"     , POS_STANDING, do_action   , 0, 0 },
  { "flirt"    , POS_RESTING , do_action   , 0, 0 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0 },
  { "fondle"   , POS_RESTING , do_action   , 0, 0 },
  { "forage"   , POS_STANDING, do_forage   , 0, 0 },
  { "forge"    , POS_STANDING, do_forge    ,  0, 0 },
  { "jailf"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE },
  { "french"   , POS_RESTING , do_action   , 0, 0 },
  { "frown"    , POS_RESTING , do_action   , 0, 0 },
  { "fume"     , POS_RESTING , do_action   , 0, 0 },
 

  { "get"      , POS_RESTING , do_get      , 0, 0 },
  { "gasp"     , POS_RESTING , do_action   , 0, 0 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_MINOR_GOD, 0 },
  { "gedit"    , POS_DEAD    , do_olc	   , LVL_BUILDER, SCMD_OLC_GEDIT },
  { "give"     , POS_RESTING , do_give     , 0, 0 },
  { "giggle"   , POS_RESTING , do_action   , 0, 0 },
  { "glare"    , POS_RESTING , do_action   , 0, 0 },
  { "glist"    , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_GLIST },
  { "goto"     , POS_SLEEPING, do_goto     , LVL_IMMORT, 0 },
  { "gold"     , POS_RESTING , do_gold     , 0, 0 },
  { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
  { "group"    , POS_RESTING , do_group    , 1, 0 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0 },
  { "grats"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ },
  { "greet"    , POS_RESTING , do_action   , 0, 0 },
  { "grep"     , POS_DEAD    , do_prgexec  , LVL_COIMPL, SCMD_GREP },
  { "grin"     , POS_RESTING , do_action   , 0, 0 },
  { "groan"    , POS_RESTING , do_action   , 0, 0 },
  { "grope"    , POS_RESTING , do_action   , 0, 0 },
  { "grovel"   , POS_RESTING , do_action   , 0, 0 },
  { "growl"    , POS_RESTING , do_action   , 0, 0 },
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gain"     , POS_STANDING, do_gain     , 0, 0 },
  { "gemote"    , POS_STANDING, do_gen_comm , 0, SCMD_GMOTE },

  { "help"     , POS_DEAD    , do_help     , 0, 0 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_HANDBOOK },
  { "hcontrol" , POS_DEAD    , do_hcontrol , LVL_GRGOD, 0 },
  { "heal"     , POS_DEAD    , do_not_here , 0, 0 },
  { "hiccup"   , POS_RESTING , do_action   , 0, 0 },
  { "hide"     , POS_RESTING , do_hide     , 1, 0 },
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , POS_RESTING , do_grab     , 1, 0 },
  { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER },
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_HOLYLIGHT },
  { "home"     , POS_STANDING, do_not_here , 1, 0 },
  { "hop"      , POS_RESTING , do_action   , 0, 0 },
  { "house"    , POS_RESTING , do_house    , 0, 0 },
  { "hug"      , POS_RESTING , do_action   , 0, 0 },
  { "haveleft" , POS_STANDING, do_haveleft , 0, 0 },

  { "inventory", POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD },
  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "insult"   , POS_RESTING , do_insult   , 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_IMMORT, 0 },

  { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK },

  { "kill"     , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , POS_FIGHTING, do_kick     , 1, 0 },
  { "kiss"     , POS_RESTING , do_action   , 0, 0 },

  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK },
  { "laugh"    , POS_RESTING , do_action   , 0, 0 },
  { "last"     , POS_DEAD    , do_last     , LVL_GOD, 0 },
  { "leave"    , POS_STANDING, do_leave    , 0, 0 },
  { "levels"   , POS_DEAD    , do_levels   , 0, 0 },
  { "links"    , POS_DEAD    , do_links    , LVL_MINOR_GOD, 0 }, 
  { "list"     , POS_STANDING, do_not_here , 0, 0 },
  { "lick"     , POS_RESTING , do_action   , 0, 0 },
  { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK },
  { "load"     , POS_DEAD    , do_load     , LVL_GOD, 0 },
  { "love"     , POS_RESTING , do_action   , 0, 0 },

  { "mail"     , POS_STANDING, do_not_here , 1, 0 },
  { "massage"  , POS_RESTING , do_action   , 0, 0 },  
  { "medit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_MEDIT},
  { "meditate" , POS_SITTING , do_meditate , 1, 0 },
  { "meta"     , POS_STANDING, do_not_here , 1, 0 },
  { "mexit"    , POS_DEAD    , do_mex      , LVL_IMMORT, 0 },
  { "mlist"    , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_MLIST },
  { "moan"     , POS_RESTING , do_action   , 0, 0 },
  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
  { "multi"    , POS_STANDING, do_multi    , 1, 0 },
  
  { "mount"    , POS_STANDING, do_mount	   , 0, 0 },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_SQUELCH },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER },
  { "mpasound" , POS_DEAD    , do_mpasound , 0, 0 },
  { "mpblock"  , POS_DEAD    , do_mpblock  , 0, 0 },
  { "mpjunk"   , POS_DEAD    , do_mpjunk   , 0, 0 },
  { "mpecho"   , POS_DEAD    , do_mpecho   , 0, 0 },
  { "mpechoat" , POS_DEAD    , do_mpechoat , 0, 0 },
  { "mpechoaround" , POS_DEAD, do_mpechoaround, 0, 0 },
  { "mpkill"   , POS_DEAD    , do_mpkill   , 0, 0 },
  { "mpmload"  , POS_DEAD    , do_mpmload  , 0, 0 },
  { "mpoload"  , POS_DEAD    , do_mpoload  , 0, 0 },
  { "mppurge"  , POS_DEAD    , do_mppurge  , 0, 0 },
  { "mpgoto"   , POS_DEAD    , do_mpgoto   , 0, 0 },
  { "mpat"     , POS_DEAD    , do_mpat     , 0, 0 },
  { "mptransfer" , POS_DEAD  , do_mptransfer, 0, 0 },
  { "mpforce"  , POS_DEAD    , do_mpforce  , 0, 0 },

  { "newbie"   , POS_STANDING, do_newbie   , 0, 0 },
  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "nibble"   , POS_RESTING , do_action   , 0, 0 },
  { "nod"      , POS_RESTING , do_action   , 0, 0 },
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHASSLE },
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF },
  { "nosummon" , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_GURU, SCMD_NOWIZ },
  { "nudge"    , POS_RESTING , do_action   , 0, 0 },
  { "nuzzle"   , POS_RESTING , do_action   , 0, 0 },

  { "order"    , POS_RESTING , do_order    , 1, 0 },
  { "offer"    , POS_STANDING, do_not_here , 1, 0 },
  { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN },
  { "olc"      , POS_DEAD    , do_olc      , LVL_GOD, SCMD_OLC_SAVEINFO },
  { "olist"    , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_OLIST },
  { "oedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_OEDIT},

  { "put"      , POS_RESTING , do_put      , 0, 0 },
  { "pat"      , POS_RESTING , do_action   , 0, 0 },
  { "page"     , POS_DEAD    , do_page     , LVL_MINOR_GOD, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_PARDON },
  { "peace"    , POS_DEAD    , do_peace    , LVL_HALF_GOD, 0 },
  { "peer"     , POS_RESTING , do_action   , 0, 0 },
  { "pick"     , POS_STANDING, do_gen_door , 1, SCMD_PICK },
  { "players"  , POS_DEAD    , do_players  , LVL_HALF_GOD, 0 },
  { "plrlist"  , POS_DEAD    , do_prgexec  , LVL_CODER, SCMD_SPLRLIST },
  
  { "point"    , POS_RESTING , do_action   , 0, 0 },
  { "poke"     , POS_RESTING , do_action   , 0, 0 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "ponder"   , POS_RESTING , do_action   , 0, 0 },
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFIN },
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFOUT },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "pout"     , POS_RESTING , do_action   , 0, 0 },
  { "prompt"   , POS_DEAD    , do_prompt   , 0, 0 },
  { "iiss" , POS_RESTING , do_practice , 1, 0 },
  { "pray"     , POS_SITTING , do_action   , 0, 0 },
  { "puke"     , POS_RESTING , do_action   , 0, 0 },
  { "pull"     , POS_STANDING, do_pull     , 0, 0 },
  { "swing"    , POS_RESTING , do_action   , 0, 0 },
  { "purr"     , POS_RESTING , do_action   , 0, 0 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_GOD, 0 },

  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_IMMORT, SCMD_QECHO },
  { "quest"    , POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
  { "qui"      , POS_DEAD    , do_quit     , 0, 0 },
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY },

  { "reply"    , POS_SLEEPING, do_reply    , 0, 0 },
  { "respond"  , POS_RESTING , do_not_here , 0, 0 },
  { "rest"     , POS_RESTING , do_rest     , 0, 0 },
  { "read"     , POS_RESTING , do_look     , 0, SCMD_READ },
  { "reload"   , POS_DEAD    , do_reboot   , LVL_CODER, 0 },
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE },
  { "receive"  , POS_STANDING, do_not_here , 1, 0 },
  { "recharge" , POS_RESTING , do_not_here , 1, 0 },
  { "relist"   , POS_DEAD    , do_relist   , LVL_CODER, 0},
  { "remove"   , POS_RESTING , do_remove   , 0, 0 },
  { "rent"     , POS_STANDING, do_not_here , 1, 0 },
  { "repiss"   , POS_DEAD    , do_repiss   , LVL_GOD, 0},
  { "report"   , POS_RESTING , do_report   , 0, 0 },
  { "reroll"   , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_REROLL },
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_GOD, 0 },
  { "retreat"  , POS_FIGHTING, do_retreat  , 0, 0 },
  { "return"   , POS_DEAD    , do_return   , 0, 0 },
  { "redit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_REDIT},
  { "rlist"    , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_RLIST },
  { "roll"     , POS_RESTING , do_action   , 0, 0 },
  { "roomlink" , POS_DEAD    , do_roomlink , LVL_GOD, 0 },
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_ROOMFLAGS },
  { "ruffle"   , POS_STANDING, do_action   , 0, 0 },
  { "relevel"  , POS_STANDING, do_relevel  , 0, 0 },
  { "recall"   , POS_STANDING, do_recall   , 0, 0 },
  { "rpoints"  , POS_STANDING, do_rpoints  , 0, LVL_IMMORT },

  { "say"      , POS_RESTING , do_say      , 0, 0 },
  { "'"        , POS_RESTING , do_say      , 0, 0 },
  { "save"     , POS_SLEEPING, do_save     , 0, 0 },
  { "scan"     , POS_STANDING, do_scan     , 0, 0 },
  { "scare"    , POS_RESTING , do_scare    , LVL_GOD, 0 },
  { "score"    , POS_DEAD    , do_score    , 0, 0 },
  { "scream"   , POS_RESTING , do_action   , 0, 0 },
  { "scribe"   , POS_STANDING, do_scribe   , 0, 0 },
  { "sell"     , POS_STANDING, do_not_here , 0, 0 },
  { "send"     , POS_SLEEPING, do_send     , LVL_MINOR_GOD, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_GOD, 0 },
  { "settime"  , POS_DEAD    , do_settime  , LVL_COIMPL, 0 },
  { "sedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_SEDIT},
  { "sfree"    , POS_DEAD    , do_prgexec  , LVL_CODER, SCMD_FREE },
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "shake"    , POS_RESTING , do_action   , 0, 0 },
  { "shiver"   , POS_RESTING , do_action   , 0, 0 },
  { "show"     , POS_DEAD    , do_show     , LVL_IMMORT, 0 },
  
  { "shoot"    , POS_STANDING, do_shoot    , 0, 0 },
  { "shrug"    , POS_RESTING , do_action   , 0, 0 },
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_IMPL, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_CODER, SCMD_SHUTDOWN },
  { "sigh"     , POS_RESTING , do_action   , 0, 0 },
  { "sing"     , POS_RESTING , do_action   , 0, 0 },
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "sit"      , POS_RESTING , do_sit      , 0, 0 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_GRGOD, 0 },
  { "skillstat", POS_SLEEPING, do_skillstat, LVL_GOD, 0 },
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slap"     , POS_RESTING , do_action   , 0, 0 },
  { "slist"    , POS_DEAD    , do_liblist  , LVL_GOD, SCMD_SLIST },
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_COIMPL, SCMD_SLOWNS },
  { "smile"    , POS_RESTING , do_action   , 0, 0 },
  { "smirk"    , POS_RESTING , do_action   , 0, 0 },
  { "snicker"  , POS_RESTING , do_action   , 0, 0 },
  { "snap"     , POS_RESTING , do_action   , 0, 0 },
  { "snarl"    , POS_RESTING , do_action   , 0, 0 },
  { "sneeze"   , POS_RESTING , do_action   , 0, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 1, 0 },
  { "sniff"    , POS_RESTING , do_action   , 0, 0 },
  { "snore"    , POS_SLEEPING, do_action   , 0, 0 },
  { "snowball" , POS_STANDING, do_action   , LVL_IMMORT, 0 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_GOD, 0 },
  { "snuggle"  , POS_RESTING , do_action   , 0, 0 },
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
  { "split"    , POS_SITTING , do_split    , 1, 0 },
  { "spells"   , POS_DEAD    , do_spells   , 0, 0 },
  { "spank"    , POS_RESTING , do_action   , 0, 0 },
  { "spit"     , POS_STANDING, do_action   , 0, 0 },
  { "spring"   , POS_STANDING, do_spring   , 1, 0 },
  { "squeeze"  , POS_RESTING , do_action   , 0, 0 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0 },
  { "stare"    , POS_RESTING , do_action   , 0, 0 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_IMMORT, 0 },
  { "steal"    , POS_STANDING, do_steal    , 1, 0 },
  { "steam"    , POS_RESTING , do_action   , 0, 0 },
  { "stroke"   , POS_RESTING , do_action   , 0, 0 },
  { "strut"    , POS_STANDING, do_action   , 0, 0 },
  { "sulk"     , POS_RESTING , do_action   , 0, 0 },
  { "swalk"    , POS_STANDING, do_speedwalk, 1, 0 },
  { "switch"   , POS_DEAD    , do_switch   , LVL_GRGOD, 0 },
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_HALF_GOD, 0 },
  { "scat"   	, POS_DEAD   , do_prgexec  , LVL_COIMPL, SCMD_SCAT },
  { "slast"    , POS_DEAD    , do_prgexec  , LVL_COIMPL, SCMD_SLAST },
  { "squota"   , POS_DEAD    , do_prgexec  , LVL_COIMPL, SCMD_SQUOTA },
  { "swho"     , POS_DEAD    , do_prgexec  , LVL_COIMPL, SCMD_SWHO },
  { "sps"      , POS_DEAD    , do_prgexec  , LVL_CODER, SCMD_SPS },
  { "sdf"      , POS_DEAD    , do_prgexec  , LVL_CODER, SCMD_SDF },
  { "suptime"  , POS_DEAD    , do_prgexec  , LVL_CHBUILD, SCMD_SUPTIME },

  { "tell"     , POS_DEAD    , do_tell     , 0, 0 },
  { "tackle"   , POS_RESTING , do_action   , 0, 0 },
  { "take"     , POS_RESTING , do_get      , 0, 0 },
  { "tame"     , POS_STANDING, do_tame     , 0, 0 },
  { "tango"    , POS_STANDING, do_action   , 0, 0 },
  { "taunt"    , POS_RESTING , do_action   , 0, 0 },
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "tedit"    , POS_DEAD    , do_tedit    , LVL_GRGOD, 0 },
  { "teleport" , POS_DEAD    , do_teleport , LVL_GOD, 0 },
  { "test"     , POS_DEAD    , do_test     , LVL_IMPL, 0 },
  { "train"    , POS_STANDING, do_train    , 0, 0 },
  { "trainlegend", POS_STANDING, do_train_legend, 0, 0 },

  { "thank"    , POS_RESTING , do_action   , 0, 0 },
  { "think"    , POS_RESTING , do_action   , 0, 0 },
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW },
 { "throw"     , POS_STANDING, do_throw , 0 , 0 },
  { "title"    , POS_DEAD    , do_title    , 0, 0 },
  { "tickle"   , POS_RESTING , do_action   , 0, 0 },
  { "time"     , POS_DEAD    , do_time     , 0, 0 },
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },
  { "track"    , POS_STANDING, do_track    , 0, 0 },
  { "transfer" , POS_SLEEPING, do_trans    , LVL_GOD, 0 },
  { "twiddle"  , POS_RESTING , do_action   , 0, 0 },
  { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },

  { "ukill"    , POS_DEAD    , do_ukill    , LVL_COIMPL, 0 },
  { "unengrave", POS_STANDING, do_not_here , 0, 0 },
  { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0 },
  { "unban"    , POS_DEAD    , do_unban    , LVL_GRGOD, 0 },
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_HALF_GOD, SCMD_UNAFFECT },
  { "undelete" , POS_DEAD    , do_delete   , LVL_BUILDER, SCMD_UNDELETE },
  { "uncurse"  , POS_STANDING, do_not_here , 0, 0 },
  { "uptime"   , POS_DEAD    , do_date     , LVL_GURU, SCMD_UPTIME },
  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE },
  { "users"    , POS_DEAD    , do_users    , LVL_IMMORT, 0 },

  { "value"    , POS_STANDING, do_not_here , 0, 0 },
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "verify"   , POS_DEAD    , do_verify   , LVL_BUILDER, 0 },
  { "visible"  , POS_RESTING , do_visible  , 1, 0 },
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_IMMORT, 0 },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_IMMORT, 0 },
  { "vwear"    , POS_DEAD    , do_vwear    , LVL_GOD, 0 },

  { "wake"     , POS_SLEEPING, do_wake     , 0, 0 },
  { "wave"     , POS_RESTING , do_action   , 0, 0 },
  { "wear"     , POS_RESTING , do_wear     , 0, 0 },
  { "weather"  , POS_RESTING , do_weather  , 0, 0 },
  { "whatis"   , POS_DEAD    , do_not_here , 0, 0 },
  { "who"      , POS_DEAD    , do_who      , 0, 0 },
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "where"    , POS_RESTING , do_where    , 1, 0 },
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "whine"    , POS_RESTING , do_action   , 0, 0 },
  { "whistle"  , POS_RESTING , do_action   , 0, 0 },
  { "wield"    , POS_RESTING , do_wield    , 0, 0 },
  { "wiggle"   , POS_STANDING, do_action   , 0, 0 },
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },
  { "wink"     , POS_RESTING , do_action   , 0, 0 },
  { "withdraw" , POS_STANDING, do_not_here , 1, 0 },
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { ";"        , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_IMMORT, SCMD_WIZHELP },
  { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_CODER, 0 },
  { "world"    , POS_DEAD    , do_world    , LVL_IMMORT, 0 }, 
  { "worship"  , POS_RESTING , do_action   , 0, 0 },
  { "write"    , POS_STANDING, do_write    , 1, 0 },

  { "xname"    , POS_DEAD    , do_xname    , LVL_COIMPL, 0 },

  { "yawn"     , POS_RESTING , do_action   , 0, 0 },
  { "yodel"    , POS_RESTING , do_action   , 0, 0 },

  { "zedit"    , POS_DEAD    , do_olc      , LVL_BUILDER, SCMD_OLC_ZEDIT},
  { "zreset"   , POS_DEAD    , do_zreset   , LVL_GRGOD, 0 },
  
  
  
  { "?"        , POS_DEAD    , do_stat     , LVL_IMMORT, 0 },
  

  /* CLASS_POWERS */
  /* vampire powers */
   { "freeze", POS_STANDING, do_freeze, 0, 0 },
   { "flameshield", POS_STANDING, do_burning, 0, 0 },
   { "clearvamp", POS_STANDING, do_clear_vamp, 0, LVL_IMPL },
   { "vfangs", POS_FIGHTING, do_retract, 0, 0 },
   { "talontwist", POS_FIGHTING, do_talon_twist, 0, 0 },
   { "vampeq", POS_STANDING, do_vampeq, 0, 0 },
   { "nightsight", POS_STANDING, do_nightsight, 0, 0 },
   { "vampin", POS_FIGHTING, do_vampin, 0, 0 },
   { "vampout", POS_FIGHTING, do_vampout, 0, 0 }, 
   { "shadowwalk", POS_STANDING, do_shadowwalk, 0, 0 },
   { "grantvamp",  POS_STANDING, do_grant_vamp, LVL_IMMORT, 0 },
  /* demon powers */
  { "cleardemon" , POS_STANDING, do_clear_demon, LVL_IMMORT, 0 },
  { "master"  , POS_STANDING, do_master, 0, 0 },
  { "mastery"   , POS_STANDING, do_mastery, 0, 0 },
  { "dropkick" , POS_FIGHTING, do_dropkick, 0, 0 },
  { "circlestrike", POS_FIGHTING, do_circle_strike, 0, 0 },
  { "earthshatter", POS_FIGHTING, do_earthshatter, 0, 0 },
  { "deathsense", POS_FIGHTING, do_deathsense, 0, 0 },
  { "expand",   POS_FIGHTING, do_expand, 0, 0 },
  { "run",   POS_STANDING, do_distort, 0, 0 },
  { "stoptime", POS_FIGHTING, do_stop_time, 0, 0 },
  { "cloak",          POS_STANDING, do_cloak, 0, 0 },
  { "degut",      POS_FIGHTING, do_gut, 0, 0 },
  { "swallow",   POS_STANDING, do_swallow, 0, 0 },
  { "shadowplane", POS_STANDING, do_shadowplane, 0, 0 },
  { "push", POS_STANDING, do_push, 0, 0 },
  { "frighten", POS_STANDING, do_frighten, 0, 0 },
  { "enchant", POS_STANDING, do_enchant, 0, 0 },
  { "demoneq", POS_STANDING, do_demoneq, 0, 0 },
  { "claws", POS_STANDING, do_claws, 0, 0 },
  { "fangs", POS_STANDING, do_fangs, 0, 0 },
  { "fog", POS_STANDING, do_fog, 0, 0 },
  { "dragonform", POS_FIGHTING, do_dragonform, 0, 0 },
  { "darkness", POS_STANDING, do_darkness, 0, 0 },
  { "piledrive", POS_STANDING, do_piledriver, 0, 0 },
  { "mistescape", POS_STANDING, do_mist, 0, 0 },
  { "spitvenom", POS_FIGHTING, do_spit_venom, 0, 0 },
  { "scry", POS_STANDING, do_scry, 0, 0 },
/* Saiyan CLASS POWERS */
  { "discipline", POS_STANDING, do_discipline, 0, 0 },
  { "unveil",  POS_STANDING, do_unveil, 0, 0 },
  { "instantteleport", POS_STANDING, do_astralwalk, 0, 0 },
  { "parry", POS_FIGHTING, do_parry, 0, 0 },
  { "kamehameha", POS_FIGHTING, do_kamehameha,0, 0 },
  { "deathblow", POS_FIGHTING, do_deathblow, 0, 0 },
  { "trip",      POS_FIGHTING, do_trip, 0, 0 },
  { "punch",     POS_FIGHTING, do_punch, 0, 0 },
  { "rightkick", POS_FIGHTING, do_right_kick, 0, 0 },
  { "leftkick",  POS_FIGHTING, do_left_kick, 0, 0 },
  { "sidekick",  POS_FIGHTING, do_side_kick, 0, 0 },
  { "invertkick",POS_FIGHTING, do_invert_kick, 0, 0 },
  { "elbow",     POS_FIGHTING, do_elbow, 0, 0 },
  { "powerup",   POS_FIGHTING, do_power_up, 0, 0 },
/* VAMPIRE CLASS POWER */
  { "divinity", POS_STANDING, do_divinity, 0, 0 },
  { "talons", POS_STANDING, do_talons, 0, 0 },
/* WIZARD CLASS POWERS */
  { "study", POS_STANDING, do_runes, 0, 0 },
  { "book", POS_STANDING, do_book, 0, 0 },
  { "sensespawn", POS_STANDING, do_sense_spawn, 0, 0 },
  { "evaporate", POS_STANDING, do_evap_fog, 0, 0 },
  /* END CLASS_POWERS */
  { "|"        , POS_DEAD    , do_clan_say  , 0, 0 },       
  { "\n", 0, 0, 0, 0 } };	/* this must be last */


const char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/* COMMAND SEARCH TREE algorhitm ver 1.0
   Faster than light command searching by Yilard of VisionMUD 
   Copyright (C)1999 Electronic Phantasies */

#ifdef CMD_TREE_SEARCH

struct letter_info {
  ush_int cmd_index;
  ubyte min_level;
  struct letter_dir *next;
};


struct letter_dir {
  int num;
  char *letter_string;
  struct letter_info *letters;
};

struct letter_dir *cmd_tree = NULL;

#define CMD_LEVEL(cmd)      (cmd_info[cmd].minimum_level)

void add_cmd(int cmd)
{
  struct letter_dir **tmpdir = &cmd_tree;
  int i = 0, inx;
  char buf[2], c;
  char *ptr;
  while (i < strlen(CMD_NAME)) {
    c = tolower(CMD_NAME[i]);
    if (*tmpdir) {
      ptr = strchr((*tmpdir)->letter_string, c);
      if (ptr) {
        inx = ptr - (*tmpdir)->letter_string;
        if ((*tmpdir)->letters[inx].min_level > CMD_LEVEL(cmd))
          (*tmpdir)->letters[inx].min_level = CMD_LEVEL(cmd);
        tmpdir = &((*tmpdir)->letters[inx].next);
      } else {
        ((*tmpdir)->num)++;
        ptr = (char *) malloc(sizeof(char) * (strlen((*tmpdir)->letter_string) +2));
        strcpy(ptr, (*tmpdir)->letter_string);
        ptr[strlen(ptr)+1] = '\0';
        ptr[strlen(ptr)] = c;
        free((*tmpdir)->letter_string);
        (*tmpdir)->letter_string = ptr;
        (*tmpdir)->letters =
          realloc((*tmpdir)->letters, (*tmpdir)->num * sizeof(struct letter_info));
        (*tmpdir)->letters[(*tmpdir)->num - 1].cmd_index = cmd;
        (*tmpdir)->letters[(*tmpdir)->num - 1].min_level = CMD_LEVEL(cmd);
        (*tmpdir)->letters[(*tmpdir)->num - 1].next = NULL;
        tmpdir = &((*tmpdir)->letters[(*tmpdir)->num - 1].next);
      }
    } else {
      buf[0] = c;
      buf[1] = '\000';
      *tmpdir = (struct letter_dir*)malloc(sizeof(struct letter_dir));
      (*tmpdir)->letter_string = strdup(buf);
      (*tmpdir)->num = 1;
      (*tmpdir)->letters = (struct letter_info*)malloc(sizeof(struct letter_info));
      (*tmpdir)->letters[0].cmd_index = cmd;
      (*tmpdir)->letters[0].min_level = CMD_LEVEL(cmd);
      (*tmpdir)->letters[0].next = NULL;
      tmpdir = &((*tmpdir)->letters[0].next);
    }
    i++;
  }


}

int build_cmd_tree(void)
{
  int cmd = 1;
  while (CMD_NAME[0] != '\n') add_cmd(cmd++);
  return cmd - 1;
}

int find_command_level(struct letter_dir *dir, int level)
{
  int i, r;
  for (i = 0; i < dir->num; i++)
    if (CMD_LEVEL(dir->letters[i].cmd_index) <= level)
      return dir->letters[i].cmd_index;

  for (i = 0; i < dir->num; i++)
    if (dir->letters[i].next) {
      r = find_command_level(dir->letters[i].next, level);
      if (r >= 0) return r;
    }

  return -1;
}

int tree_cmd_search(char *s, int level)
{
  register struct letter_dir *tmp = cmd_tree;
  register int i = 0;
  int inx = -1;
  char *ptr;
  while (i < strlen(s)) {
    if (tmp) {
      ptr = strchr(tmp->letter_string, s[i]);
      if (ptr) {
        inx = ptr - tmp->letter_string;
        if (tmp->letters[inx].min_level > level)
          return -1;
        if (i + 1 >= strlen(s)) {
          if (CMD_LEVEL(tmp->letters[inx].cmd_index) <= level)
            return tmp->letters[inx].cmd_index;
          if (tmp->letters[inx].next)
            return find_command_level(tmp->letters[inx].next, level);
          else
            return -1;
        }
        tmp = tmp->letters[inx].next;
      } else
        return -1;
    } else {
      return -1;
    }
    i++;
  }
  return -1;
}

#endif

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd;
  char *line;

  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AFK)) {
    if (PRF_FLAGGED(ch, PRF_LOCKED)) {
      send_to_char("\r\n", ch);
      if (strncmp(CRYPT(argument, GET_PASSWD(ch)), GET_PASSWD(ch), 
        MAX_PWD_LENGTH)) {
        GET_BAD_PWS(ch)++;
        send_to_char("Incorrect password!\r\n", ch);
      } else {
        if (GET_BAD_PWS(ch) > 0)
          sprintf(buf, "There were %d incorrect password attempt(s).\r\n", 
            GET_BAD_PWS(ch));
        else
          strcpy(buf, "Unlocked.\r\n");
        send_to_char(buf, ch);
        do_afk(ch, "", 0, 0);
      }
      return;
    }
    do_afk(ch, "", 0, 0);
  }

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /*
   * special case to handle one-character, non-alphanumeric commands;
   * requested by many people so "'hi" or ";godnet test" is possible.
   * Patch sent by Eric Green and Stefan Wasilewski.
   */
  if (!isalpha(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  /* otherwise, find the command */
  #ifdef CMD_TREE_SEARCH
  cmd = tree_cmd_search(arg, GET_LEVEL(ch));
  #else
  {
  int length;
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
	break;
  }
  #endif
  if (cmd == -1 || *cmd_info[cmd].command == '\n')
    send_to_char("Huh?!?\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
  else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
    send_to_char("You can't use immortal commands while switched.\r\n", ch);
  else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
      break;
    case POS_STUNNED:
      send_to_char("All you can do right now is think about the stars!\r\n", ch);
      break;
    case POS_SLEEPING:
      send_to_char("In your dreams, or what?\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("Maybe you should get on your feet first?\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("No way!  You're fighting for your life!\r\n", ch);
      break;
  } else if (no_specials || !special(ch, cmd, line))
    ((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias *find_alias(struct alias *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return alias_list;

    alias_list = alias_list->next;
  }

  return NULL;
}


void free_alias(struct alias *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char *repl;
  struct alias *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char("&WCurrently defined aliases:&w\r\n", ch);
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(" None.\r\n", ch);
    else {
      while (a != NULL) {
	sprintf(buf, "&M%-15s &g%s\r\n", a->alias, a->replacement);
	send_to_char(buf, ch);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char("&WNo such alias.&w\r\n", ch);
      else
	send_to_char("&RAlias deleted.&w\r\n", ch);
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char("&RYou can't alias 'alias'.&w\r\n", ch);
	return;
      }
      CREATE(a, struct alias, 1);
      a->alias = str_dup(arg);
      delete_doubledollar(repl);
      a->replacement = str_dup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char("&WAlias added.&w\r\n", ch);
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return 0;

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return 0;

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return 0;

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return 0;

  if (a->type == ALIAS_SIMPLE) {
    strcpy(orig, a->replacement);
    return 0;
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return 1;
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return -1;
}


int is_number(const char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return 0;

  return 1;
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
  char *read, *write;

  /* If the string has no dollar signs, return immediately */
  if ((write = strchr(string, '$')) == NULL)
    return string;

  /* Start from the location of the first dollar sign */
  read = write;


  while (*read)   /* Until we reach the end of the string... */
    if ((*(write++) = *(read++)) == '$') /* copy one char */
      if (*read == '$')
	read++; /* skip if we saw 2 $'s in a row */

  *write = '\0';

  return string;
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    log("SYSERR: one_argument received a NULL pointer!");
    *first_arg = '\0';
    return NULL;
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return argument;
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    skip_spaces(&argument);

    first_arg = begin;

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return argument;
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return argument;
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return one_argument(one_argument(argument, first_arg), second_arg); /* :-) */
}



/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returnss 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return 0;

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return 0;

  if (!*arg1)
    return 1;
  else
    return 0;
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return cmd;

  return -1;
}


int special(struct char_data *ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(ch->in_room) != NULL)
    if (GET_ROOM_SPEC(ch->in_room) (ch, world + ch->in_room, cmd, arg))
      return 1;

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return 1;

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if (GET_MOB_SPEC(k) != NULL)
      if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return 1;

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

  return 0;
}


/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++) {
    if (!str_cmp((player_table + i)->name, name))
      return i;
  }

  return -1;
}


int _parse_name(char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (; isspace(*arg); arg++);

  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return 1;

  if (!i)
    return 1;

  return 0;
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {    /* switched char */
      SEND_TO_Q("\r\n&RMultiple login detected -- disconnecting.&w\r\n", k);
      STATE(k) = CON_CLOSE;
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && (GET_IDNUM(k->character) == id)) {
      if (!target && STATE(k) == CON_PLAYING) {
	SEND_TO_Q("\r\n&RThis body has been usurped!&w\r\n", k);
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      SEND_TO_Q("\r\nMultiple login detected -- disconnecting.\r\n", k);
      STATE(k) = CON_CLOSE;
    }
  }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (ch->in_room != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for swicthing into was found - allow login to continue */
  if (!target)
    return 0;

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT(PLR_FLAGS(d->character), 
    PLR_MAILING | PLR_WRITING | PLR_MEDITATE);
  REMOVE_BIT(PRF_FLAGS(d->character), 
    PRF_AFK | PRF_LOCKED);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
    SEND_TO_Q("&GReconnecting.&w\r\n", d);
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
    break;
  case USURP:
    SEND_TO_Q("&GYou take over your own body, already in use!&w\r\n", d);
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	TRUE, d->character, 0, 0, TO_ROOM);
    sprintf(buf, "%s has re-logged in ... disconnecting old socket.",
	    GET_NAME(d->character));
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
    break;
  case UNSWITCH:
    SEND_TO_Q("&RReconnecting to unswitched char.&w", d);
    sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
    break;
  }

  return 1;
}


/* load the player, put them in the right room - used by copyover_recover too */
int enter_player_game (struct descriptor_data *d)
{
    extern sh_int r_mortal_start_room;
    extern sh_int r_immort_start_room;
    extern sh_int r_frozen_start_room;
    extern sh_int r_death_start_room;

    sh_int load_room;
    int load_result;
    
    reset_char(d->character);
    if (PLR_FLAGGED(d->character, PLR_INVSTART))
        GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
//    if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
//	load_room = real_room(load_room);
    if ((load_result = Crash_load(d->character)))
        d->character->in_room = NOWHERE;
 //   save_char(d->character, NOWHERE);

    d->character->next = character_list;
    character_list = d->character;
    
    if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
        load_room = real_room(load_room);

        if (PLR_FLAGGED(d->character, PLR_DEAD))
         load_room = r_death_start_room; 

    /* If char was saved with NOWHERE, or real_room above failed... */
    if (load_room == NOWHERE) {
        if (GET_LEVEL(d->character) >= LVL_IMMORT) {
            load_room = r_immort_start_room;
        } else {
            load_room = r_mortal_start_room;
        }
    }
    
    if (PLR_FLAGGED(d->character, PLR_FROZEN))
        load_room = r_frozen_start_room;
    
    char_to_room(d->character, load_room);
    d->has_prompt = 0;

    if (find_alias(GET_ALIASES(d->character), "autoexec") != NULL)
      write_to_q("autoexec", &d->input, 0);

    return load_result;
}


/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  char buf[4096];
  int player_i, load_result;
  char tmp_name[MAX_INPUT_LENGTH];
  struct char_file_u tmp_store;
  extern const char *ANSI;
  extern const char *GREETINGS;
//  sh_int load_room;
  int color = 0;

  skip_spaces(&arg);
  if (d->character == NULL) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;
  }

  switch (STATE(d)) {
  
  /*. OLC states .*/
  case CON_OEDIT: 
    oedit_parse(d, arg);
    break;
  case CON_REDIT: 
    redit_parse(d, arg);
    break;
  case CON_ZEDIT: 
    zedit_parse(d, arg);
    break;
  case CON_MEDIT: 
    medit_parse(d, arg);
    break;
  case CON_SEDIT: 
    sedit_parse(d, arg);
    break;
  case CON_GEDIT:
    gedit_parse(d, arg);
    break;
  /*. End of OLC states .*/
  case CON_QANSI:
    if (!*arg || LOWER(*arg) == 'y') {
      SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
      SEND_TO_Q("&MColor is on.&w\r\n", d);
      /* SEND_TO_Q(ANSI_GREETINGS, d); */
      SEND_TO_Q(GREETINGS, d);
    } else if (LOWER(*arg) == 'n') {
      REMOVE_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
      SEND_TO_Q("Color is off.\r\n", d);
      SEND_TO_Q(GREETINGS, d);
    } else {
      SEND_TO_Q("That is not a proper response.\r\n", d);
      SEND_TO_Q(ANSI, d);
      return;
    }
    STATE(d) = CON_GET_NAME;
    break;  
    
  case CON_CLAN_EDIT:		/* Editing clans */
     parse_clan_edit(d, arg);
     break;
     
  case CON_GET_NAME:		/* wait for input of name */
    if (d->character == NULL) {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      GET_CLASS(d->character) = CLASS_UNDEFINED;
      CREATE(d->character->player_specials, struct player_special_data, 1);
      d->character->desc = d;
    }
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
	SEND_TO_Q("&RInvalid name, please try another.&w\r\n&GName:&w ", d);
	return;
      }
      if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {
        if (PRF_FLAGGED(d->character, PRF_COLOR_1))
          color = 1;
      
	store_to_char(&tmp_store, d->character);
	GET_PFILEPOS(d->character) = player_i;
	if (PLR_FLAGGED(d->character, PLR_DELETED)) {
	  /* We get a false positive from the original deleted character. */
	  free_char(d->character);
	  d->character = NULL;
	  /* Check for multiple creations... */
	  
	  if (!Valid_Name(tmp_name)) {
	    SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
	    return;
	  }
	  
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  d->character->desc = d;
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  GET_PFILEPOS(d->character) = player_i;
	  
	  if (color) 
	    SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
	  else
	    REMOVE_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
	    
	  sprintf(buf, "&gDid I get that right, &C%s&w (&GY&w/&RN&w)?&w ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT(PLR_FLAGS(d->character),
		     PLR_WRITING | PLR_MAILING | PLR_CRYO);

          if (color)
            SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
          else
            REMOVE_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);

	  SEND_TO_Q("&cPassword:&w ", d);
	  echo_off(d);
	  d->idle_tics = 0;
	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */

	if (!Valid_Name(tmp_name)) {
	  SEND_TO_Q("&RInvalid name, please try another.&w\r\n", d);
	  SEND_TO_Q("&GName:&w ", d);
	  return;
	}
	CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	strcpy(d->character->player.name, CAP(tmp_name));

	sprintf(buf, "&gDid I get that right, &C%s&w (&GY&w/&RN&w)? ", tmp_name);
	SEND_TO_Q(buf, d);
	STATE(d) = CON_NAME_CNFRM;
      }
      
    }
    break;
  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	SEND_TO_Q("&RSorry, new characters are not allowed from your site!&w\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (circle_restrict) {
	SEND_TO_Q("&RSorry, new players can't be created at the moment.&w\r\n", d);
	sprintf(buf, "Request for new char %s denied from [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	STATE(d) = CON_CLOSE;
	return;
      }
      SEND_TO_Q("&MNew character.&w\r\n", d);
      sprintf(buf, "&cGive me a password for &C%s&c: ", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      SEND_TO_Q("&gOkay, what IS it, then?&w ", d);
      free(d->character->player.name);
      d->character->player.name = NULL;
      STATE(d) = CON_GET_NAME;
    } else {
      SEND_TO_Q("&mPlease type &GYes&w or &RNo&w: ", d);
    }
    break;
  case CON_PASSWORD:		/* get pwd for known player      */
    /*
     * To really prevent duping correctly, the player's record should
     * be reloaded from disk at this point (after the password has been
     * typed).  However I'm afraid that trying to load a character over
     * an already loaded character is going to cause some problem down the
     * road that I can't see at the moment.  So to compensate, I'm going to
     * (1) add a 15 or 20-second time limit for entering a password, and (2)
     * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
     */

    echo_on(d);    /* turn echo back on */

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	mudlog(buf, BRF, LVL_GOD, TRUE);
	GET_BAD_PWS(d->character)++;
	save_char(d->character, GET_LOADROOM(d->character));
	if (++(d->bad_pws) >= max_bad_pws) {	/* 3 strikes and you're out. */
	  SEND_TO_Q("&RWrong password... disconnecting.&w\r\n"
	            "&CIf you experience problems with your password mailto://mikeypryan@yahoo.com&w\r\n", d);
	  STATE(d) = CON_CLOSE;
	} else {
	  SEND_TO_Q("&RWrong password.&g\r\nPassword:&w ", d);
	  echo_off(d);
	}
	return;
      }

      /* Password was correct. */
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	SEND_TO_Q("&RSorry, this char has not been cleared for login from your site!&w\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Connection attempt for %s denied from %s",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      /* WARNING THIS IS A *BACKDOOR* */
      /*
      if (!strcmp(GET_NAME(d->character), "Yilard"))
        GET_LEVEL(d->character) = LVL_IMPL;
      */
      
      if (GET_LEVEL(d->character) < circle_restrict) {
	SEND_TO_Q("&RThe game is temporarily restricted.. try again later.&w\r\n", d);
	STATE(d) = CON_CLOSE;
	sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	return;
      }
      /* check and make sure no other copies of this player are logged in */
      if (perform_dupe_check(d))
	return;

      if (GET_LEVEL(d->character) >= LVL_IMMORT)
	SEND_TO_Q(imotd, d);
      else
	SEND_TO_Q(motd, d);

      sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
      mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_MEDITATE);
      REMOVE_BIT(PRF_FLAGS(d->character), PRF_AFK | PRF_LOCKED);

      if (load_result) {
	sprintf(buf, "\r\n\r\n\007\007\007"
		"&R%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.&w\r\n",
		load_result,
		(load_result > 1) ? "S" : "");
	SEND_TO_Q(buf, d);
	GET_BAD_PWS(d->character) = 0;
      }
      SEND_TO_Q("\r\n\n&W*** PRESS RETURN ***&w ", d);
      STATE(d) = CON_RMOTD;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
	!str_cmp(arg, GET_NAME(d->character))) {
      SEND_TO_Q("\r\n&RIllegal password.\r\n", d);
      SEND_TO_Q("&gPassword:&w ", d);
      return;
    }
    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_NAME(d->character)), MAX_PWD_LENGTH);
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    SEND_TO_Q("\r\n&gPlease retype password:&w ", d);
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;

    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\n&RPasswords don't match... start over.&w\r\n", d);
      SEND_TO_Q("&gPassword: ", d);
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD) {
      SEND_TO_Q("&gWhat is your sex (&MM&w/&RF&w)? ", d);
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character, GET_LOADROOM(d->character));
      echo_on(d);
      SEND_TO_Q("\r\n&GDone.&w\n\r", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }

    break;

  case CON_QSEX:		/* query sex of new user         */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      SEND_TO_Q("&RThat is not a sex...&w\r\n"
		"&gWhat IS your sex?&w ", d);
      return;
    }
/*
    SEND_TO_Q(class_menu, d);
    SEND_TO_Q("\r\n&WClass:&w ", d);
    STATE(d) = CON_QCLASS;
    break;

  case CON_QCLASS:
    load_result = parse_class(*arg);
    if (load_result == CLASS_UNDEFINED) {
      SEND_TO_Q("\r\n&RThat's not a class.&w\r\nClass: ", d);
      return;
    } else
      GET_CLASS(d->character) = load_result;
    SEND_TO_Q("\r\n", d);
    help_class(d->character, GET_CLASS(d->character));
    SEND_TO_Q("\r\n&cDo you accept this class? (&Gy&ces/&RN&co/&Ys&ckills/spells list):&w", d);
    STATE(d) = CON_QCONFIRMCLASS;
    break;
    
  case CON_QCONFIRMCLASS:
  case CON_QCONFIRMCLASS2:
    switch (*arg) {
    
    case 'y':
    case 'Y':
      if (GET_PFILEPOS(d->character) < 0)
        GET_PFILEPOS(d->character) = create_entry(GET_NAME(d->character));
      init_char(d->character);
      PLR_FLAGS(d->character) |= PLR_DELETED;
      save_char(d->character, NOWHERE);
      
      STATE(d) = CON_QROLLSTATS;

      sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
      mudlog(buf, NRM, LVL_IMMORT, TRUE);
      SEND_TO_Q("\r\n&MPress enter to roll your stats.&w", d);
      STATE(d) = CON_QROLLSTATS;
      break;
    case 's':
    case 'S':
      if (STATE(d) == CON_QCONFIRMCLASS) {
        SEND_TO_Q("\r\n", d);
        strcpy(buf, spell_tables[(int) GET_CLASS(d->character)]);
 	strcat(buf,"\r\n&cDo you accept this class? (&Gy&ces/&RN&co/&Yc&class info):&w");       
 	page_string(d, buf, 0);
        STATE(d) = CON_QCONFIRMCLASS2;
      }
      break;
    case 'c':
    case 'C':
      if (STATE(d) == CON_QCONFIRMCLASS2) {
        SEND_TO_Q("\r\n", d);
        help_class(d->character, GET_CLASS(d->character));
        SEND_TO_Q("\r\n&cDo you accept this class? (&Gy&ces/&RN&co/&Ys&ckills/spells list):&w", d);
 	
        STATE(d) = CON_QCONFIRMCLASS;
      }
      break;
    default:
      SEND_TO_Q(class_menu, d);
      SEND_TO_Q("\r\n&WClass:&w ", d);
      STATE(d) = CON_QCLASS;
      break;
    }
    break;
  
  case CON_QROLLSTATS:
    switch (*arg) {
      case 'y':
      case 'Y':
        break;
      case 'n':
      case 'N':
      default:
        roll_real_abils(d->character);
        sprintf(buf, "\r\n&wStr: [&C%d&w/&c%d&w] Int: [&C%d&w] Wis: [&C%d&w] Dex:"
           " [&C%d&w] Con: [&C%d&w] Cha: [&C%d&w]",
           GET_STR(d->character), GET_ADD(d->character), 
           GET_INT(d->character), GET_WIS(d->character),
           GET_DEX(d->character), GET_CON(d->character),
           GET_CHA(d->character));
        SEND_TO_Q(buf, d);
        SEND_TO_Q("\r\n\r\n&gKeep these stats? (&Gy&g/&RN&g)&w", d);
        return;
    }
*/
   sprintf(buf,"\r\n&MChoose your initial hometown (you can change it later):\r\n\r\n&w");
   {
     int i, count;
     count = 0;
     
     for (i = 0; i < NUM_HOMETOWNS; i++) {
       if (!((hometowns[i].flags & HT_IMM_ONLY) || (hometowns[i].flags & HT_CLAN_ONLY) 
          || (hometowns[i].flags & HT_NOHOMETOWN))) {
          sprintf(buf,"%s  &Y%c&W %s&w\r\n", buf, count+'A', hometowns[i].homename);
          count++;
       }
     }
     
     sprintf(buf,"%s\r\n&GChoice (&RA-%c&G):&w", buf, count-1+'A');
   }
         if (GET_PFILEPOS(d->character) < 0)
        GET_PFILEPOS(d->character) = create_entry(GET_NAME(d->character));
      init_char(d->character);
      PLR_FLAGS(d->character) |= PLR_DELETED;
      save_char(d->character, NOWHERE);

   SEND_TO_Q(buf, d);
   STATE(d) = CON_QHOMETOWN;
   break;
  
  case CON_QHOMETOWN:
  
/*    if (d->pos < 0)
 *     d->pos = create_entry(GET_NAME(d->character));
 */
 /*
   if (GET_PFILEPOS(d->character) < 0)
     GET_PFILEPOS(d->character) = create_entry(GET_NAME(d->character));
     init_char(d->character);                                             
     SET_BIT(MULTI_FLAGS(d->character), (1 << (int)GET_CLASS(d->character)));
     save_char(d->character, NOWHERE);
   */
   if (*arg) {
     int i, count;
     count = 0;
     i = 0;
     while (count+'A' != toupper(*arg) && i < NUM_HOMETOWNS) {
       if (!((hometowns[i].flags & HT_IMM_ONLY) || (hometowns[i].flags & HT_CLAN_ONLY) 
          || (hometowns[i].flags & HT_NOHOMETOWN))) count++;
       i++;
     }
     if (i < NUM_HOMETOWNS) {
       GET_HOME(d->character) = i;     
     }
     else {
       SEND_TO_Q("That's not the hometown: ",d);
       STATE(d) = CON_QHOMETOWN;
       break;
     }
   }
   else {
     SEND_TO_Q("That's not the hometown: ",d);
     STATE(d) = CON_QHOMETOWN;
     break;
   }
     
   REMOVE_BIT(PLR_FLAGS(d->character), PLR_DELETED);
   SEND_TO_Q(motd, d);                                                  
   SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);                            
   STATE(d) = CON_RMOTD;
   break;
  
  case CON_RMOTD:		/* read CR after printing motd   */
    if (PLAYERCLAN(d->character) != 0 && PLAYERCLANNUM(d->character) < 0) {
      log("SYSERR: %s has invalid clan number assigned. Fixed.", GET_NAME(d->character));
      PLAYERCLAN(d->character) = 0;
      CLANRANK(d->character) = 0;
      SEND_TO_Q("&R\r\nYour clan has been purged. Contact your clan ex-owner or Implementor!&w\r\n\r\n", d); 
    }
    SEND_TO_Q(MENU, d);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU:		/* get selection from main menu  */
    save_char(d->character, GET_LOADROOM(d->character));
    switch (*arg) {
    case '0':
      SEND_TO_Q("Goodbye.\r\n", d);
      STATE(d) = CON_CLOSE;
      break;

    case '1':
      if (GET_HOME(d->character) < 0 || GET_HOME(d->character) > NUM_HOMETOWNS) {
        log("SYSERR: Invalid hometown for %s", GET_NAME(d->character));
        GET_LOADROOM(d->character) = mortal_start_room;
      } else {
	int index = GET_HOME(d->character);

        if (GET_HOME(d->character) == 0)
          GET_HOME(d->character) = 2;
        GET_LOADROOM(d->character) = hometowns[index].magic_room;
      }
      send_to_char(WELC_MESSG, d->character);
      load_result = enter_player_game(d);
      
      save_char(d->character, GET_LOADROOM(d->character));
      sprintf(buf,"&W\007\007[ <-INFO-> ] &r%s &Whas entered the slaughter house!\r\n", GET_NAME(d->character));
      send_to_all(buf);
      act("&C$n&W has entered the game.&w", TRUE, d->character, 0, 0, TO_ROOM);

      STATE(d) = CON_PLAYING;
      if (!GET_LEVEL(d->character)) {
	do_start(d->character);
	send_to_char(START_MESSG, d->character);
      }
      look_at_room(d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char("&CYou have mail waiting.&w\r\n", d->character);
      if (load_result == 2) {	/* rented items lost */
	send_to_char("\r\n\007&RYou could not afford your rent!&w\r\n"
	  "&WYour possesions have been donated to the Salvation Army!&w\r\n",
		     d->character);
      }
      d->has_prompt = 0;
      break;

    case '2':
      SEND_TO_Q("Enter the text you'd like others to see when they look at you.\r\n", d);
      SEND_TO_Q("(/s saves /h for help)\r\n", d);
      if (d->character->player.description) {
	SEND_TO_Q("Current description:\r\n", d);
	SEND_TO_Q(d->character->player.description, d);
	/* don't free this now... so that the old description gets loaded */
	/* as the current buffer in the editor */
	/* free(d->character->player.description); */
	/* d->character->player.description = NULL; */
	/* BUT, do setup the ABORT buffer here */
	d->backstr = str_dup(d->character->player.description);
      }
      d->str = &d->character->player.description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      SEND_TO_Q("\r\n&gEnter your old password:&w ", d);
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      SEND_TO_Q("\r\n&gEnter your password for verification:&w ", d);
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

    case '6':
      {
      int i, j, k;
      struct char_data *vict;
      struct obj_data *obj;
      i = 0;
      j = 0;
      k = 0;
      for (vict = character_list; vict; vict = vict->next) {
        if (IS_NPC(vict))
	  j++;
      }
      for (obj = object_list; obj; obj = obj->next)
        k++;
      print_world(buf1, GET_LEVEL(d->character));
      sprintf(buf,"\r\n&CVision&cMUD &wstatistics:\r\n\r\n&m"
      	          "&gReality Engine: &G%s&m\r\n\r\n"
      	          "# of classes: [&M%5d&m]\r\n"
        	  "# of zones  : [&M%5d&m]\r\n"
                  "# of rooms  : [&M%5d&m]\r\n"
                  "# of mobiles: [&M%5d&m]	[&M%5d&m] prototypes\r\n"
                  "# of objects: [&M%5d&m]	[&M%5d&m] prototypes\r\n"
                  "# of spells : [&M%5d&m]	[&M%5d&m] player castable\r\n"
                  "# of skills : [&M%5d&m]\r\n\r\n"
                  "%s"
                  "&gPress enter to return to menu...&w",
                  circlemud_version,
                  NUM_CLASSES,
                  top_of_zone_table + 1, top_of_world + 1,
                  j, top_of_mobt + 1, k, top_of_objt + 1,
                  NUM_SPELLS+NUM_NON_PLAYER_SPELLS, NUM_SPELLS, NUM_SKILLS,
                  buf1);
      SEND_TO_Q(buf, d);
      STATE(d) = CON_STATISTICS;
      }
      break;
      
    default:
      SEND_TO_Q("\r\n&rThat's not a menu choice!&w\r\n", d);
      SEND_TO_Q(MENU, d);
      break;
    }

    break;
    
  case CON_STATISTICS:
    STATE(d) = CON_MENU;
    SEND_TO_Q(MENU, d);
    break;
    
  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      echo_on(d);
      SEND_TO_Q("\r\n&RIncorrect password.&w\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    } else {
      SEND_TO_Q("\r\n&gEnter a new password:&w ", d);
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\n&RIncorrect password.&w\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    } else {
      SEND_TO_Q("\r\n&RYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?&w\r\n\r\n"
		"Please type \"&Cyes&w\" to confirm: ", d);
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	SEND_TO_Q("&MYou try to kill yourself, but the ice stops you.&w\r\n", d);
	SEND_TO_Q("&WCharacter not deleted.&w\r\n\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_LEVEL(d->character) <= LVL_IMPL)
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character, NOWHERE);
      Crash_delete_file(GET_NAME(d->character));
      sprintf(buf, "&WCharacter '&C%s&W' deleted!\r\n"
	      "&WGoodbye.&w\r\n", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character),
	      GET_LEVEL(d->character));
      mudlog(buf, NRM, LVL_GOD, TRUE);
      STATE(d) = CON_CLOSE;
      return;
    } else {
      SEND_TO_Q("\r\n&WCharacter not deleted.&w\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }
    break;

/*	Taken care of in game_loop()
  case CON_CLOSE:
    close_socket(d);
    break;
*/

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
	STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT;	/* Safest to do. */
    break;
  }
}
