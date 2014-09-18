include ../../config

all: filesystem

filesystem: filesystem.cpp
	$(CXX) filesystem.cpp -O3 -o filesystem.exe -I$(LUA_PATH)/include -I./ -I- -I$(BOOST_ROOT) -I../../ -L. -L../../lib -L$(LUA_PATH)/lib -lluabind -llua -llualib -L./ -lboost_filesystem

clean:
	rm -f filesystem.o

