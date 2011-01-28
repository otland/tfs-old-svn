# Makefile for Linux (tested under Debian and Ubuntu)
TFS = forgottenserver

INCLUDEDIRS = -I. -I/usr/include/libxml2 \
		-I /usr/include/lua5.1

LIBDIRS =

FLAGS = -D_THREAD_SAFE -D_REENTRANT -D__NO_HOMEDIR_CONF__ -D__ENABLE_SERVER_DIAGNOSTIC__ -D__USE_MYSQL__ -D__USE_SQLITE__

CXXFLAGS = $(INCLUDEDIRS) $(FLAGS) -Werror -Wall -O2
CXX = g++

LIBS = -lxml2 -lpthread -llua5.1 -lgmp -lmysqlclient -lsqlite3 -lboost_regex -llua5.1-sql-mysql -ldl -lboost_system -lboost_thread

#For windows:
#	mingw32-make
#For Linux:
#	make

#mingw32-make WIN32=1 LATEST_MINGW=1

#TODO: fix this
ifdef WIN32
	#@echo "Building with platform win32, with options: "
	LIBS += -llua5.1-sql-sqlite3
	ifdef LATEST_MINGW
		#@echo "Latest MinGW\n"
		LIBS -= -lboost_thread
		LIBS -= -lboost_regex
		LIBS -= -lboost_system
		LIBS -= -llua5.1-sql-mysql
		LIBS -= -ldl
		LIBS -= lpthread
		LIBS += -lboost_system-mgw45-1_45 -lboost_thread-mgw45-mt-1_45 -lboost_regex-mgw45-1_45
	else
		#@echo "None\n"
		LIBS -= lpthread
		LIBS -= -llua5.1-sql-mysql
		LIBS -= -ldl
	endif
endif

#mingw32-make DEBUG=1
#make DEBUG=1

ifdef DEBUG
	CXXFLAGS += -g
endif

LDFLAGS = $(LIBDIRS) $(LIBS)

CXXSOURCES = actions.cpp admin.cpp allocator.cpp ban.cpp baseevents.cpp beds.cpp creature.cpp creatureevent.cpp chat.cpp combat.cpp commands.cpp condition.cpp configmanager.cpp connection.cpp container.cpp cylinder.cpp database.cpp databasemysql.cpp databasesqlite.cpp depot.cpp exception.cpp fileloader.cpp game.cpp globalevent.cpp gui.cpp house.cpp housetile.cpp ioguild.cpp iologindata.cpp iomap.cpp iomapserialize.cpp inputbox.cpp item.cpp items.cpp logger.cpp luascript.cpp mailbox.cpp map.cpp md5.cpp monster.cpp monsters.cpp mounts.cpp movement.cpp networkmessage.cpp npc.cpp otserv.cpp outfit.cpp outputmessage.cpp party.cpp player.cpp playerbox.cpp position.cpp protocol.cpp protocolgame.cpp protocollogin.cpp protocolold.cpp quests.cpp raids.cpp rsa.cpp scheduler.cpp scriptmanager.cpp server.cpp sha1.cpp spawn.cpp spells.cpp status.cpp talkaction.cpp tasks.cpp teleport.cpp textlogger.cpp thing.cpp tile.cpp tools.cpp trashholder.cpp vocation.cpp waitlist.cpp weapons.cpp

CXXOBJECTS = $(CXXSOURCES:.cpp=.o)

all: $(TFS)

clean:
	$(RM) $(CXXOBJECTS)

$(TFS): $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CXXOBJECTS) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<