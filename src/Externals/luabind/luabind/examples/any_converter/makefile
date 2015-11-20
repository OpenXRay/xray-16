include ../../config

all: any_converter

any_converter: any_converter.cpp
	$(CXX) any_converter.cpp -O3 -o any_converter -I$(LUA_PATH)/include -I- -I$(BOOST_ROOT) -I../../ -L../../lib -L$(LUA_PATH)/lib -lluabind -llua -llualib

clean:
	rm -f any_converter.o

