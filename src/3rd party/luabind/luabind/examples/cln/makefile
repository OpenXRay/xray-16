include ../../config

all: cln_test

cln_test: cln_test.cpp
	$(CXX) cln_test.cpp -o cln_test -I$(LUA_PATH)/include -I- -I$(BOOST_ROOT) -I../../ -L$(LUA_PATH)/lib -L../../lib -lluabind -llua -llualib -lcln

clean:
	rm -f cln_test.o

