# makefile voor JapieG

TARGET		= japi
# DEBUG should be either 1 or 0
DEBUG		= 1
DEFINES		= 
WARNINGS	= -Wall -Wno-multichar
LIBS		= pcre z cryptopp boost_regex boost_thread \
			  boost_filesystem boost_date_time attr \
			  gthread-2.0 magic
LIBPATHS	= 
INCPATHS	= 
SYSINCPATHS	= 
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas
CFLAGS		= -fsigned-char -g -finput-charset=UTF-8
CFLAGS		+= $(shell pkg-config --cflags libglade-2.0)
LDFLAGS		= -fdata-sections -g
LDFLAGS		+= $(shell pkg-config --libs libglade-2.0)
CC			= c++
OPT			+= -O3

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

Sources := $(filter-out Sources/MRSRCFileCreator.cpp Sources/MNewGroupDialog.cpp Sources/MProjectInfoDialog.cpp Sources/MProjectPathsDialog.cpp NetSources/MSshTerminalChannel.cpp, $(Sources))

define SOURCES
	$(Sources)
endef

define SRC_TO_OBJ
	$(addprefix $(OBJDIR)/, $(addsuffix .o, $(foreach file, $(SOURCES), $(basename $(notdir $(file))))))
endef

OBJS = $(SRC_TO_OBJ)

RSRC_DATA = \
	auth-dialog.xml \
	clean-target-alert.xml \
	diff-window.xml \
	discard-changes-alert.xml \
	edit-window-menus.xml \
	find-and-open-dialog.xml \
	find-dialog.xml \
	go-to-line-dialog.xml \
	host-key-changed-alert.xml \
	make-clean-alert.xml \
	mark-matching-dialog.xml \
	message-list-window.xml \
	prefs-dialog.xml \
	project-window-menus.xml \
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

all: $(TARGET)

# build rules

RSRCFileCreatorObjects = \
	$(OBJDIR)/MResources.o \
	$(OBJDIR)/MError.o \
	$(OBJDIR)/MObjectFile.o \
	$(OBJDIR)/MObjectFileImp_elf.o

RSRCFileCreator: Sources/MRSRCFileCreator.cpp
	@ echo "Linking "$(@F)
	$(CC) -o $@ Sources/MRSRCFileCreator.cpp $(CFLAGS) $(RSRCFileCreatorObjects) $(LDFLAGS)
	@ chmod a+rx $@

$(TARGET)::
	@ echo Compiler flags used: $(CFLAGS)
	@ echo Include paths: $(INCLUDES)

$(TARGET):: $(OBJDIR) $(OBJS) $(OBJDIR)/__rsrc__.o
	@ echo "Linking "$(@F)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) $(OBJDIR)/__rsrc__.o
	@ chmod a+rx $(TARGET)
	@ echo "Done"

clean: FORCE
	rm -rf $(OBJDIR)

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)

$(OBJDIR)/__rsrc__.o: RSRCFileCreator $(RSRC_DATA)
	./RSRCFileCreator -o $@ -r Resources $(addprefix -d, $(RSRC_DATA))

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

$(OBJDIR)/%.o: %.c
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJS:%.o=%.d)

${OBJS:.o=.d}:

FORCE:
