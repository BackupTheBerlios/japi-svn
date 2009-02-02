# makefile voor Japi
#
# highly simplified to create just a starter japi
#
# This makefile needs GNU make.

PREFIX		= /usr/local

DEFINES		= BUILDING_TEMPORARY_JAPI DEBUG
LIBS		= z cryptopp boost_thread boost_filesystem gthread-2.0 expat
WARNINGS	= all no-multichar no-unknown-pragmas
SOURCES		= $(wildcard Sources/*.cpp NetSources/*.cpp zeep/*.cpp)
OBJDIR		= Obj.temp/
OBJECTS		= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
VPATH		:= :Sources:NetSources:zeep:
LDFLAGS		= -g $(shell pkg-config --libs libglade-2.0 gstreamer-0.10) $(LIBPATHS:%=-L%) $(LIBS:%=-l%)
CC			= c++
CFLAGS		= -fsigned-char -g -finput-charset=UTF-8 -pipe $(WARNINGS:%=-W%) $(DEFINES:%=-D%)
CFLAGS		+= $(shell pkg-config --cflags libglade-2.0 gstreamer-0.10)
CFLAGS		+= $(addprefix -iquote, Sources NetSources zeep)

all: japi

# build rules

japi-temp: $(OBJDIR) $(OBJECTS)
	@ echo "Linking "$(@F)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)
	@ chmod a+rx $@

japi: japi-temp
	@ echo "Rebuilding japi using japi"
	./japi-temp -m 'Japi-Release' japi.prj
	@ echo "Done"
	
clean: FORCE
	rm -rf $(OBJDIR)

install: japi
	install japi $(PREFIX)/bin/japi
	msgfmt -o $(PREFIX)/share/locale/nl/LC_MESSAGES/japi.mo Resources/Dutch/japi.po
	sed -e 's|__EXE__|$(PREFIX)/bin/japi|' Resources/japi.desktop > japi.desktop_tmp && \
		install japi.desktop_tmp $(PREFIX)/share/applications/japi.desktop &&
		rm japi.desktop_tmp

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJECTS:%.o=%.d)

${OBJECTS:.o=.d}:

FORCE:
