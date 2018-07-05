/* ******************************************************************/
//  File: Mayhem_wiz.c                           part of DLMUD      //
//     Darkened Lights Copyright 2003, Spunge entertainment.        //
//    this file was created for Darkened lights MUD.                //
//     Most of the material will not work under other patch levels  //
//     To be dated as of Dec, 20th 2003                             //
//     Darkened Lights and Material Created by Mike Ryan AKA Mayhem //
/* ******************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "olc.h"

/*   external vars  */
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct int_app_type int_app[];
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int restrict;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;

/* for objects */
extern char *item_types[];
extern char *wear_bits[];
extern char *extra_bits[];
extern char *container_bits[];
extern char *drinks[];

/* for rooms */
extern char *dirs[];
extern char *room_bits[];
extern char *exit_bits[];
extern char *sector_types[];

/* for chars */
extern char *spells[];
extern char *equipment_types[];
extern char *affected_bits[];
extern char *apply_types[];
extern char *pc_class_types[];
extern char *npc_class_types[];
extern char *action_bits[];
extern char *player_bits[];
extern char *preference_bits[];
extern char *position_types[];
extern char *connected_types[];

/* extern functions */
int level_exp(int class, int level);


/* Do relevel Added Dec 20th 2003 */
// Just incase you wanna play as a mortal every once and a while
// utilize this command to relevel back to your last level position.
void do_relevel( struct char_data *ch, char *argument )
{

    if (IS_NPC(ch)) return;

    if  (!str_cmp(GET_NAME(ch),"Mayhem") || !str_cmp(GET_NAME(ch), "Xanths"))
    {
	GET_LEVEL(ch)	        = 111;
        ch->points.max_hit	= 5000;
	ch->points.hit		= ch->points.max_hit;
	ch->points.max_mana	= 5000;
	ch->points.mana	        = ch->points.max_mana;
	ch->points.max_move	= 5000;
	ch->points.move	        = ch->points.max_move;
	send_to_char("Done.\n\r",ch);
    }
    else
	send_to_char("Huh?\n\r",ch);
    return;
}

ACMD(do_rpoints)
{

    send_to_char("Granting 5000 research points",ch); 
     GET_DEMONXP(ch) += 5000;
 
}
ACMD(do_classme)
{

  argument = one_argument( argument, arg );

 if (GET_CLASS(ch) != CLASS_PALADIN) {
    send_to_char("You have already classed yourself.\r\n",ch);
    return;
    }

    if (arg[0] == '\0') {
            send_to_char("Please specify a class:\r\n"
                         "Demon, Saiyan, Werewolf, Vampire.\r\n"
                         "Class coming soon: Android.\r\n" ,ch);
    return;
}   
    if (GET_PLAYER_KILLS(ch) < 2 ) {
         send_to_char("You must be an avatar to become classed.\r\n",ch);
         return;
}
        if (!str_cmp(arg,"demon")) {
         send_to_char("You have conformed to the wicked life of a demon,"
                      "Your skin turns, pale green, your hair falls off,"
                      "the color in your eyes fade.",ch);
       GET_CLASS(ch) = CLASS_WARRIOR;       
    return;
  }
}
