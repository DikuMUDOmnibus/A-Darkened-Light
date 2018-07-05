# C compiler to use
CC = gcc

# Path to cxref utility
CXREF = cxref
NAME = ../bin/circle


# Any special flags you want to pass to the compiler
MYFLAGS = -Wall -Werror

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

# I have removed -g flag from CFLAGS (YIL)
CFLAGS = $(MYFLAGS) $(PROFILE)

OBJDIR = objs/

OBJFILES = comm.o act.comm.o act.create.o act.informative.o act.movement.o \
	act.item.o act.offensive.o act.other.o act.social.o act.wizard.o \
	ban.o boards.o castle.o class.o clan.o config.o constants.o db.o \
	fight.o graph.o guild.o handler.o house.o interpreter.o limits.o \
	magic.o mail.o mobact.o mobcmd.o mobprog.o modify.o objsave.o olc.o \
	prgrun.o random.o shop.o spec_assign.o spec_grave.o spec_procs.o \
	spell_parser.o spells.o teleport.o trains.o utils.o weather.o wizlist.o \
	redit.o oedit.o zedit.o medit.o sedit.o gedit.o demon.o \
        mayhem_wiz.o multi.o dempower.o saiyan.o donathin.o \
        vampire.o werewolf.o extras.o

OBJFILES := $(sort $(addprefix $(OBJDIR), $(OBJFILES)))


all : $(NAME)

$(NAME) : $(OBJFILES)
	@echo \*\*\* Making Final Link: $(NAME)
	@$(CC) -o $(NAME) $(OBJFILES)
	@echo \*\*\* Make Process Complete
	@echo "`date`"
	@echo Press CTRL-C to exit

default: .accepted
	$(MAKE) $(NAME)

clean:
	rm -f objs/*.o

.accepted:
	@./licheck more

$(OBJDIR)%.o : %.c
	@echo Making \"$<\"
	@$(CC) $(CFLAGS) -c $< -o $@

%.o : %.c
	@echo Making \"$<\"
	@$(CC) $(CFLAGS) -c $< -o $(OBJDIR)$@

.SUFFIXES:     .c
