include ../config

SOURCES = \
	wrapper_base.cpp \
	pcall.cpp \
	error.cpp \
	ref.cpp \
	scope.cpp \
	class.cpp \
	class_rep.cpp \
	function.cpp \
	find_best_match.cpp \
	implicit_cast.cpp \
	open.cpp \
	create_class.cpp \
	stack_content_by_name.cpp \
	object.cpp \
	object_rep.cpp \
	class_info.cpp \
	class_registry.cpp \
	link_compatibility.cpp \
	weak_ref.cpp \
	link_compatibility.cpp

OBJECTS = $(SOURCES:.cpp=.o)

TARGET = ../lib/libluabind.a





all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	$(AR) rcu $@ $(OBJECTS)
	ranlib $@

%.o:%.cpp
	$(CXX) -g -ftemplate-depth-100 -Wall -DLUABIND_BUILDING -c $? -o $@ -I$(LUA_PATH)/include -I- -I.. -I$(BOOST_ROOT) $(CONFIG)

clean:
	rm -f $(OBJECTS) $(TARGET)

