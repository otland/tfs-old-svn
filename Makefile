TFS = forgottenserver

INCLUDEDIRS = -I. -I/usr/include/libxml2 \
		-I /usr/include/lua5.1

LIBDIRS =

FLAGS = -D_THREAD_SAFE -D_REENTRANT -D__NO_HOMEDIR_CONF__  -D__ENABLE_SERVER_DIAGNOSTIC__ -D__USE_SQLITE__ -D__USE_MYSQL__

CXXFLAGS = $(INCLUDEDIRS) $(FLAGS) -Werror -Wall -O0 -ggdb
CXX = g++

LIBS = -lxml2 -lpthread -llua5.1 -lgmp -lmysqlclient -lsqlite3 -lboost_regex -llua5.1-sql-mysql -llua5.1-sql-sqlite -ldl -lboost_system -lboost_thread

LDFLAGS = $(LIBDIRS) $(LIBS)

CXXSOURCES = account.cpp actions.cpp admin.cpp allocator.cpp ban.cpp baseevents.cpp beds.cpp creature.cpp creatureevent.cpp chat.cpp combat.cpp commands.cpp condition.cpp configmanager.cpp connection.cpp container.cpp cylinder.cpp database.cpp databasemysql.cpp databasesqlite.cpp depot.cpp exception.cpp fileloader.cpp game.cpp gui.cpp house.cpp housetile.cpp ioguild.cpp iologindata.cpp iomap.cpp iomapserialize.cpp inputbox.cpp item.cpp items.cpp logger.cpp luascript.cpp mailbox.cpp map.cpp md5.cpp monster.cpp monsters.cpp movement.cpp networkmessage.cpp npc.cpp otserv.cpp outfit.cpp outputmessage.cpp party.cpp player.cpp playerbox.cpp position.cpp protocol.cpp protocolgame.cpp protocollogin.cpp protocolold.cpp quests.cpp raids.cpp rsa.cpp scheduler.cpp scriptmanager.cpp server.cpp sha1.cpp spawn.cpp spells.cpp status.cpp talkaction.cpp tasks.cpp teleport.cpp textlogger.cpp thing.cpp tile.cpp tools.cpp trashholder.cpp vocation.cpp waitlist.cpp weapons.cpp

CXXOBJECTS = $(CXXSOURCES:.cpp=.o)

all: $(TFS)

clean:
	$(RM) $(CXXOBJECTS)

$(TFS): $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(CXXOBJECTS) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<