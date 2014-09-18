include ../config

SOURCES = \
	main.cpp \
	test_attributes.cpp \
	test_const.cpp \
	test_construction.cpp \
	test_exceptions.cpp \
	test_free_functions.cpp \
	test_held_type.cpp \
	test_implicit_cast.cpp \
	test_iterator.cpp \
	test_lua_classes.cpp \
	test_null_pointer.cpp \
	test_object.cpp \
	test_operators.cpp \
	test_policies.cpp \
	test_scope.cpp \
	test_separate_registration.cpp \
	test_separation.cpp \
	test_typetraits.cpp \
	test_yield.cpp

# not converted yet

#	test_null_pointer.cpp \
	test_policies.cpp \
	test_free_functions.cpp \
	test_iterator.cpp \


TARGET = test_luabind




OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -g $(OBJECTS) -L$(LUA_PATH)/lib -L../lib -lluabind -llua -llualib -o $@

%.o:%.cpp
	$(CXX) -Wall -ftemplate-depth-100 -g $(CONFIG) -I$(LUA_PATH)/include -I. -I- -I../ -I$(BOOST_ROOT) -c $? -o $@

relink:
	rm -f $(TARGET)
	make

clean:
	rm -f $(OBJECTS) $(TARGET) core

