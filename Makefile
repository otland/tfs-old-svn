# Makefile for Linux (Ubuntu, Debian), Windows (mingw, probably supports MSVC too)
TFS = forgottenserver

INCLUDEDIRS = -I"." -I"/usr/include/libxml2" -I"/usr/include/lua5.1" -I"/usr/include/mysql"

LIBDIRS =

FLAGS = -D_THREAD_SAFE -D_REENTRANT -D__NO_HOMEDIR_CONF__ -D__ENABLE_SERVER_DIAGNOSTIC__

CXXFLAGS = $(INCLUDEDIRS) $(FLAGS) -Werror -Wall -O2
CXX = g++

#Those are common libraries
LIBS = -lxml2 -lpthread -llua5.1 -lgmp

#For windows:
#	mingw32-make
#For Linux:
#	make

ifndef MYSQL
MYSQL = 1
endif
ifndef SQLITE
SQLITE = 1
endif

ifeq ($(MYSQL),1)
	ifdef WIN32
		LIBS += -lmysql
	else
		LIBS += -lmysqlclient
	endif
	FLAGS += -D__USE_MYSQL__
endif

ifeq ($(SQLITE),1)
	LIBS += -lsqlite3
	FLAGS += -D__USE_SQLITE__
endif

#mingw32-make WIN32=1 LATEST_MINGW=1

ifdef WIN32
	ifdef LATEST_MINGW
		#Causes notices when compiling with latest MinGW without these flags.
		FLAGS += -enable-stdcall-fixup -enable-auto-import -enable-runtime-pseudo-reloc
		LIBS += -lboost_system-mgw45-1_45 -lboost_thread-mgw45-mt-1_45 -lboost_regex-mgw45-1_45
	else
		LIBS += -lboost_system -lboost_thread -lboost_regex
	endif
	LIBS += -lws2_32 -lwsock32
	FLAGS += -mwindows
else
	LIBS += -lboost_thread -lboost_system -lboost_regex -lpthread -ldl
endif

#mingw32-make DEBUG=1
#make DEBUG=1

ifdef DEBUG
	CXXFLAGS += -g
	#TODO: add all flags
	FLAGS += -D__DEBUG__ -D__DEBUG_PLAYERS__ 
endif

LDFLAGS = $(LIBDIRS) $(LIBS)

ifdef WIN32
	DEL = del
endif

CXXSOURCES = actions.cpp admin.cpp allocator.cpp ban.cpp baseevents.cpp beds.cpp \
	creature.cpp creatureevent.cpp chat.cpp combat.cpp commands.cpp condition.cpp configmanager.cpp \
	connection.cpp container.cpp cylinder.cpp database.cpp databasemanager.cpp databasemysql.cpp \
	databasesqlite.cpp depot.cpp exception.cpp fileloader.cpp game.cpp globalevent.cpp gui.cpp house.cpp \
	housetile.cpp ioguild.cpp iologindata.cpp iomap.cpp iomapserialize.cpp iomarket.cpp inputbox.cpp \
	item.cpp items.cpp logger.cpp luascript.cpp mailbox.cpp map.cpp md5.cpp monster.cpp monsters.cpp \
	mounts.cpp movement.cpp networkmessage.cpp npc.cpp otserv.cpp outfit.cpp outputmessage.cpp party.cpp \
	player.cpp playerbox.cpp position.cpp protocol.cpp protocolgame.cpp protocollogin.cpp \
	protocolold.cpp quests.cpp raids.cpp rsa.cpp scheduler.cpp scriptmanager.cpp server.cpp \
	sha1.cpp spawn.cpp spells.cpp status.cpp talkaction.cpp tasks.cpp teleport.cpp textlogger.cpp \
	thing.cpp tile.cpp tools.cpp trashholder.cpp vocation.cpp waitlist.cpp weapons.cpp

OBJDIR = obj
CXXOBJECTS = $(CXXSOURCES:%.cpp=$(OBJDIR)/%.o)

all: $(TFS)

clean:
ifdef WIN32
	-$(DEL) $(CXXOBJECTS) $(TFS)
else
	$(RM) $(CXXOBJECTS) $(TFS)
endif

$(TFS): $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CXXOBJECTS) $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<
