include ../../config

all: intrusive_ptr

intrusive_ptr: intrusive_ptr.cpp
	$(CXX) intrusive_ptr.cpp -o intrusive_ptr -I$(LUA_PATH)/include -I- -I$(BOOST_ROOT) -I../../ -L../../lib -L$(LUA_PATH)/lib -lluabind -llua -llualib

clean:
	rm -f intrusive_ptr

