# makefile voor JapieG

TARGET		= jg
# DEBUG should be either 1 or 0
DEBUG		= 1
DEFINES		= 
WARNINGS	= -Wall -Wno-multichar
#LIBS		= pcre
LIBPATHS	= 
INCPATHS	= 
SYSINCPATHS	= 
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas
CFLAGS		= -fsigned-char -g -finput-charset=UTF-8
#CFLAGS		+= -fmessage-length=200 -fdiagnostics-show-location=every-line
CFLAGS		+= $(shell pkg-config --cflags gtk+-2.0)
LDFLAGS		= -fdata-sections -g
LDFLAGS		+= $(shell pkg-config --libs gtk+-2.0)
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

Sources = $(wildcard Sources/*.cpp)

Sources := $(filter-out Sources/MStyles.cpp Sources/MPrefsDialog.cpp, $(Sources))

define SOURCES
	$(Sources)
endef

define SRC_TO_OBJ
	$(addprefix $(OBJDIR)/, $(addsuffix .o, $(foreach file, $(SOURCES), $(basename $(notdir $(file))))))
endef

OBJS = $(SRC_TO_OBJ)

# create a unique list of paths to our sources
SRC_PATHS += $(sort $(foreach file, $(SOURCES), $(dir $(file))))

VPATH :=
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

$(TARGET)::
	@ echo Compiler flags used: $(CFLAGS)
	@ echo Include paths: $(INCLUDES)

$(TARGET):: $(OBJDIR) $(OBJS)
	@ echo "Linking "$(@F)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@ chmod a+rx $(TARGET)
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
