noinst_PROGRAMS = theforgottenserver

CXXFLAGS = -g -O1
AM_CXXFLAGS = $(XML_CPPFLAGS) $(OTSERV_FLAGS) $(LUA_CFLAGS) $(DEBUG_FLAGS)\
$(MYSQL_FLAGS) $(SQLITE_FLAGS) $(ODBC_FLAGS) $(PGSQL_FLAGS)\
$(PROFILER_FLAGS) $(OPTIONAL_FLAGS) -D_THREAD_SAFE -D_REENTRANT -Wall

theforgottenserver_LDADD = $(LUA_LIBS)

if USE_MYSQL
MAYBE_MYSQL = databasemysql.cpp databasemysql.h
endif
if USE_SQLITE
MAYBE_SQLITE = databasesqlite.cpp databasesqlite.h
endif
if USE_PGSQL
MAYBE_PGSQL = databasepgsql.cpp databasepgsql.h
endif
if USE_ODBC
MAYBE_ODBC = databaseodbc.cpp databaseodbc.h
endif
if LOGIN_SERVER
MAYBE_LOGIN = gameservers.cpp gameservers.h
endif
if REMOTE_CONTROL
MAYBE_REMOTE = admin.cpp admin.h
endif

theforgottenserver_SOURCES = account.h actions.cpp actions.h $(MAYBE_REMOTE) \
	allocator.cpp allocator.h baseevents.cpp baseevents.h beds.cpp \
	beds.h chat.cpp chat.h combat.cpp combat.h condition.cpp condition.h \
	config.h configmanager.cpp configmanager.h connection.cpp \
	connection.h const.h container.cpp container.h creature.cpp \
	creature.h creatureevent.cpp creatureevent.h cylinder.cpp cylinder.h \
	database.cpp database.h databasemanager.cpp databasemanager.h \
	$(MAYBE_MYSQL) $(MAYBE_SQLITE) $(MAYBE_PGSQL) $(MAYBE_ODBC) \
	depot.cpp depot.h exception.cpp exception.h fileloader.cpp \
	fileloader.h game.cpp game.h $(MAYBE_LOGIN) globalevent.cpp \
	globalevent.h group.cpp group.h gui.cpp gui.h house.cpp house.h \
	housetile.cpp housetile.h inputbox.cpp inputbox.h ioban.cpp ioban.h \
	ioguild.cpp ioguild.h iologindata.cpp iologindata.h iomap.cpp \
	iomapserialize.cpp iomapserialize.h item.cpp item.h itemattributes.cpp \
	itemattributes.h items.cpp items.h luascript.cpp luascript.h \
	mailbox.cpp mailbox.h map.cpp map.h md5.cpp md5.h monster.cpp \
	monster.h monsters.cpp monsters.h movement.cpp movement.h \
	networkmessage.cpp networkmessage.h npc.cpp npc.h otpch.h \
	otserv.cpp otsystem.h outfit.cpp outfit.h outputmessage.cpp \
	outputmessage.h party.cpp party.h playerbox.cpp playerbox.h \
	player.cpp player.h position.cpp position.h protocol.cpp protocol.h \
	protocolgame.cpp protocolgame.h protocollogin.cpp protocollogin.h \
	protocolold.cpp protocolold.h quests.cpp quests.h raids.cpp raids.h \
	resources.h rsa.cpp rsa.h scheduler.cpp scheduler.h scriptmanager.cpp \
	scriptmanager.h server.cpp server.h sha1.cpp sha1.h spawn.cpp spawn.h \
	spells.cpp spells.h status.cpp status.h talkaction.cpp talkaction.h \
	tasks.cpp tasks.h teleport.cpp teleport.h templates.h textlogger.cpp \
	textlogger.h thing.cpp thing.h tile.cpp tile.h tools.cpp tools.h \
	town.h trashholder.cpp trashholder.h waitlist.cpp waitlist.h \
	waypoints.h weapons.cpp weapons.h vocation.cpp vocation.h
