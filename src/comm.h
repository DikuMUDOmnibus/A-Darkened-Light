/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __COMM_H__
#define __COMM_H__

#define NUM_RESERVED_DESCS	8

#define COPYOVER_FILE "copyover.dat"

/* comm.c */
void	send_to_all(const char *messg);
void	send_to_char(const char *messg, struct char_data *ch);
void	send_to_room(const char *messg, int room);
void	send_to_outdoor(const char *messg);
void 	send_to_zone(const char *str, int zone_rnum);
void	perform_to_all(const char *messg, struct char_data *ch);
void	close_socket(struct descriptor_data *d);

void	perform_act(const char *orig, struct char_data *ch,
		struct obj_data *obj, const void *vict_obj, struct char_data *to);

void	act(const char *str, int hide_invisible, struct char_data *ch,
		struct obj_data *obj, const void *vict_obj, int type);

/* lowlevel procedures (mostly for AFK-LOCK) */
void echo_off(struct descriptor_data *d);
void echo_on(struct descriptor_data *d);

#ifndef __COMM_C__
extern int pulse;	/* pulled out for teleport */
extern bool show_color_codes;
#endif

#define YES	1
#define NO	0

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define TO_GMOTE        5
#define TO_SLEEP	128	/* to char, even if sleeping */


int	write_to_descriptor(socket_t desc,const  char *txt);
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
void	write_to_output(char *txt, struct descriptor_data *d);
void	page_string(struct descriptor_data *d, char *str, int keep_internal);
char	*interpret_colors(const char *, bool);
/* #define SEND_TO_Q(messg, desc) write_to_output((messg),desc) */

#define SEND_TO_Q(txt, d)     \
    (write_to_output(interpret_colors(txt, (d->character && \
          !IS_NPC(d->character) && (PRF_FLAGGED(d->character, PRF_COLOR_1) ||           \
           PRF_FLAGGED(d->character, PRF_COLOR_2)) ? TRUE : FALSE)), d))

#define USING_SMALL(d)	((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  ((d)->output == (d)->large_outbuf)

typedef RETSIGTYPE sigfunc(int);

#endif
