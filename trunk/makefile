# makefile voor JapieG

# DEBUG should be either 1 or 0
DEBUG		= 1
DEFINES		= BUILDING_TEMPORARY_JAPI
WARNINGS	= -Wall -Wno-multichar
LIBS		= z cryptopp boost_regex boost_thread \
			  boost_filesystem boost_date_time attr \
			  gthread-2.0 magic
LIBPATHS	= 
INCPATHS	= 
SYSINCPATHS	= 
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas -Winvalid-pch
CFLAGS		= -fsigned-char -g -finput-charset=UTF-8
CFLAGS		+= $(shell pkg-config --cflags libglade-2.0 gstreamer-0.10)
LDFLAGS		= -fdata-sections -g
LDFLAGS		+= $(shell pkg-config --libs libglade-2.0 gstreamer-0.10)
CC			= g++-4.2
# OPT			+= -O3

ifeq ($(DEBUG),0)
	CFLAGS	+= $(OPT)
	OBJDIR	= Obj.NoDebug
	DEFINES	+= NDEBUG
else
	OBJDIR	= Obj.Debug
	DEFINES	+= DEBUG
endif

include make.config

LDFLAGS += $(addprefix -L, $(LIBPATHS)) $(LD_LIBS)

Sources = $(wildcard Sources/*.cpp NetSources/*.cpp)

Sources := $(filter-out Sources/MRSRCFileCreator.cpp Sources/MProjectPathsDialog.cpp NetSources/MSshTerminalChannel.cpp, $(Sources))

define SOURCES
	$(Sources)
endef

define SRC_TO_OBJ
	$(addprefix $(OBJDIR)/, $(addsuffix .o, $(foreach file, $(SOURCES), $(basename $(notdir $(file))))))
endef

OBJS = $(SRC_TO_OBJ)

RSRC_DATA = \
	auth-dialog.glade \
	clean-target-alert.xml \
	diff-window.glade \
	discard-changes-alert.xml \
	edit-window-menu.xml \
	find-and-open-dialog.glade \
	find-dialog.glade \
	go-to-line-dialog.glade \
	host-key-changed-alert.xml \
	make-clean-alert.xml \
	mark-matching-dialog.glade \
	message-list-window.glade \
	prefs-dialog.glade \
	project-window.glade \
	project-window-menu.xml \
	replace-all-alert.xml \
	save-changes-alert.xml \
	strings.xml \
	unknown-host-alert.xml \
	Templates/empty.c \
	Templates/Strict.xhtml \
	Templates/Template.html \
	Templates/Transitional.xhtml

# create a unique list of paths to our sources
SRC_PATHS += $(sort $(foreach file, $(SOURCES), $(dir $(file))))

VPATH := Resources
# add the list of source paths to VPATH if not already present
VPATH += $(addprefix :, $(subst  ,:, $(filter-out $($(subst, :, ,$(VPATH))), $(SRC_PATHS))))

# add the list of source paths to INLCUDES if not already present
INCLUDES = $(foreach path, $(INCPATHS) $(SRC_PATHS), $(addprefix -I, $(path)))

ifneq ($(IQUOTE),0)
INCLUDES += -iquote
else
INCLUDES += -I-
endif
INCLUDES += $(foreach path, $(SYSINCPATHS) $(BOOST), $(addprefix -I, $(path)))

# add the -L prefix to all library paths to search
LINK_PATHS = $(foreach path, $(LIBPATHS), $(addprefix -L, $(path)))

# add the -l prefix to all libs to be linked against
LINK_LIBS = $(foreach lib, $(LIBS), $(addprefix -l, $(lib)))

# add to the linker flags 
LDFLAGS += $(LINK_PATHS) $(LINK_LIBS)

CFLAGS	+= $(DBG) $(WARNINGS)

DEFS	= $(foreach define, $(DEFINES), $(addprefix -D, $(define)))
CFLAGS	+= $(DEFS)

all: japi

# build rules

japi-temp: $(OBJDIR) $(OBJS)
	@ echo "Linking "$(@F)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	@ chmod a+rx $@

japi: japi-temp
	@ echo "Rebuilding using japi"
	./japi-temp -m 'Japi-Debug' japi.prj
	@ echo "Done"
	
clean: FORCE
	rm -rf $(OBJDIR)

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

$(OBJDIR)/%.o: %.c
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJS:%.o=%.d)

${OBJS:.o=.d}:

FORCE:
