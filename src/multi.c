/*********************************************************************\
 * File  : multi.c                                Based on CircleMUD *
 * Usage : Controls player multi levels                              *
 * By    : The Finality Development Team                             *
 *   								     *
 * Used with permission from Finality.com                            *
\*********************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "interpreter.h"
#include "comm.h"

extern char *pc_class_types[];

/* CUSTOMIZABLE OPTIONS	*/

/* Multi system defines */
#define MULTI_MAGIC_USER	(1 << 0) /* Has multied to magic user */
#define MULTI_CLERIC		(1 << 1) /* Has multied to cleric     */
#define MULTI_THIEF		(1 << 2) /* Has multied to thief      */
#define MULTI_WARRIOR		(1 << 3) /* Has multied to warrior    */
#define MULTI_PALADIN           (1 << 4)
#define MULTI_RANGER            (1 << 5)
#define MULTI_WARLOCK           (1 << 6)
#define MULTI_CYBORG            (1 << 7)
#define MULTI_NECROMANCER       (1 << 8)
#define MULTI_DRUID             (1 << 9)
#define MULTI_ALCHEMIST         (1 << 10)
#define MULTI_BARBARIAN         (1 << 11)
/* Add new defines above, adding in sequential order. 
 * ie: Next would be (1 << 4) 
 */

/* Set to -1 if you don't want a char to be in a particular room, otherwise
 * set to whatever room you wish.
 */
#define MULTI_ROOM		3031

/* The level you wish to be the multi level */
#define MULTI_LEVEL		(LVL_IMMORT - 1)

/* Stats char should start at after multi */
#define MULTI_HP		100
#define MULTI_MANA		100
#define MULTI_MOVE		100

struct multi_classes {
  char *name;
  int flag;
} multi_class[] = {
  { "wizard"	, MULTI_MAGIC_USER	},
  { "saiyan"	, MULTI_CLERIC		},
  { "vampire"	, MULTI_THIEF		},
  { "demon"	, MULTI_WARRIOR		},
  { "paladin"   , MULTI_PALADIN         },
  { "ranger"    , MULTI_RANGER          },
  { "warlock"   , MULTI_WARLOCK         },
  { "cyborg"    , MULTI_CYBORG          },
  { "necromancer", MULTI_NECROMANCER    },
  { "druid"     , MULTI_DRUID           },
  { "alchemist" , MULTI_ALCHEMIST       },
  { "barbarian" , MULTI_BARBARIAN       },  
  /* Insert new classes here 		*/
  { "\n"	, 0			}
};

/* Multi message - modify this message to set multi message */
char *multi_message = 
"You have multied, and must begin anew. However, you will retain knowledge\r\n"
"of your previous skills and spells, you just will not be able to use them\r\n"
"until you reach the appropriate level.\r\n";

/* END CUSTOMIZABLE OPTIONS 
 *
 * Only modify code below this point if you know what you are doing. It is set
 * up however, so that if you are a newbie coder, but want a functional multi
 * system, you will not have to modify ANYTHING below here.
 *
 */

#define MULTI_NAME(i)		(multi_class[i].name)
#define MULTI_FLAG(i)		(multi_class[i].flag)

/* Will parse multi_class for proper passed argument */
int find_multi_flag(char *arg)
{
  int i;

  for (i = 0; *MULTI_NAME(i) != '\n'; i++)
    if (is_abbrev(arg, MULTI_NAME(i)))
      return i;

  return -1;
}

int okay_to_multi(struct char_data * ch, int flag)
{
  int room;

  /* Is wanted class same as current class? */
  if (flag == GET_CLASS(ch)) {
    sprintf(buf, "You are currently a %s!\r\n", pc_class_types[(int)GET_CLASS(ch)]);
    send_to_char(buf, ch);
    return FALSE;
  }

  /* Has char already completed this class? */
  if (MULTI_FLAGGED(ch, MULTI_FLAG(flag))) {
    send_to_char("You can not repeat a class already completed.\r\n", ch);
    return FALSE;
  }

  if (GET_LEVEL(ch) < MULTI_LEVEL) {
    sprintf(buf, "You are only level %d, you must be at least Level %d before you can multi.\r\n",
		GET_LEVEL(ch), MULTI_LEVEL);
    send_to_char(buf, ch);
    return FALSE;
  }

  /* Is the char in the appropriate room? */
  if (MULTI_ROOM >= 0) {
    room = real_room(MULTI_ROOM);
    if (room >= 0 && ch->in_room != room) {
      send_to_char("You are not in the correct room to multi!\r\n", ch);
      return FALSE;
    }
  }

  /* Everything else is okay, return TRUE! */
  return TRUE;
}

void reset_char_stats(struct char_data * ch, int flag)
{
  GET_MAX_HIT(ch) = MULTI_HP;
  GET_MAX_MANA(ch) = MULTI_MANA;
  GET_MAX_MOVE(ch) = MULTI_MOVE;
 
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);

  GET_LEVEL(ch) = 1;
  GET_CLASS(ch) = flag;
  GET_EXP(ch) = 1;
  SET_BIT(MULTI_FLAGS(ch), MULTI_FLAG(flag));
  GET_TOT_LEVEL(ch)++;

  send_to_char(multi_message, ch);
}

ACMD(do_multi)
{
  int flag;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Usage: multi <class name>\r\n\r\n", ch);
    send_to_char("Classes: Demon, Wizard, Saiyan, Vampire, Paladin, Ranger\r\n",ch);
    send_to_char("Warlock, Cyborg, Necromancer, Druid, Alchemist, Barbarian.",ch);
    return;
  }

  if ((flag = find_multi_flag(arg)) < 0) {
    send_to_char("Improper class name, please try again.\r\n", ch);
    return;
  }

  if (!okay_to_multi(ch, flag))
    return;

  reset_char_stats(ch, flag);
}
