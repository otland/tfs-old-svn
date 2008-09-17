LIBLINK = -L/usr/lib -lmysqlclient -lsqlite3

OBJ = updater.o

all: sqlupdater

new: clean sqlupdater

clean:
	rm -rf *.o

sqlupdater: $(OBJ)
	g++ -o ./SQLUpdater $(OBJ) $(LIBLINK)

    %.o:%.cpp
	g++ -c $+