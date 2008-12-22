# makefile voor Japi
#
# highly simplified to create just a starter japi
#
# This makefile needs GNU make.

DEFINES		= BUILDING_TEMPORARY_JAPI DEBUG
LIBS		= z cryptopp boost_thread boost_filesystem attr gthread-2.0 expat
WARNINGS	= all no-multichar no-unknown-pragmas
SOURCES		= $(wildcard Sources/*.cpp NetSources/*.cpp)
OBJDIR		= Obj.temp/
OBJECTS		= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
VPATH		:= :Sources:NetSources:
LDFLAGS		= -g $(shell pkg-config --libs libglade-2.0 gstreamer-0.10) $(LIBPATHS:%=-L%) $(LIBS:%=-l%)
CFLAGS		= -fsigned-char -g -finput-charset=UTF-8 -pipe $(WARNINGS:%=-W%) $(DEFINES:%=-D%)
CFLAGS		+= $(shell pkg-config --cflags libglade-2.0 gstreamer-0.10)
CFLAGS		+= $(addprefix -iquote, Sources NetSources)

all: japi

# build rules

japi-temp: $(OBJDIR) $(OBJECTS)
	@ echo "Linking "$(@F)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)
	@ chmod a+rx $@

japi: japi-temp
	@ echo "Rebuilding japi using japi"
	./japi-temp -m 'Japi-Release' japi.prj
	rm -rf japi-temp Obj.temp
	@ echo "Done"
	
clean: FORCE
	rm -rf $(OBJDIR)

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJECTS:%.o=%.d)

${OBJECTS:.o=.d}:

FORCE:
