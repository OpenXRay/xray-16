include ../../config

all: regexp

regexp: regex_wrap.cpp
	$(CXX) regex_wrap.cpp -o regex_wrap -I$(LUA_PATH)/include -I- -I$(BOOST_ROOT) -I../../ -L. -L../../lib -L$(LUA_PATH)/lib -lluabind -lboost_regex -llua -llualib

clean:
	rm -f regex_wrap.o

