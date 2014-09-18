extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

#include <luabind/luabind.hpp>
#include <luabind/class.hpp>
#include <luabind/function.hpp>
#include <luabind/functor.hpp>


#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

struct glut_constants {};
struct gl_constants {};

namespace glut_bindings
{
	luabind::functor<void> displayfunc;

	void displayfunc_callback()
	{
		displayfunc();
	}

	void set_displayfunc(const luabind::functor<void>& fun)
	{
		glutDisplayFunc(&displayfunc_callback);
		displayfunc = fun;
	}

	luabind::functor<void> idlefunc;

	void idlefunc_callback()
	{
		idlefunc();
	}

	void set_idlefunc(const luabind::functor<void>& fun)
	{
		glutIdleFunc(&idlefunc_callback);
		idlefunc = fun;
	}


	luabind::functor<void> reshapefunc;

	void reshapefunc_callback(int w, int h)
	{
		reshapefunc(w,h);
	}

	void set_reshapefunc(const luabind::functor<void>& fun)
	{
		reshapefunc = fun;
	}

	luabind::functor<void> keyboardfunc;

	void keyboardfunc_callback(unsigned char key, int x, int y)
	{
		keyboardfunc(key,x,y);
	}

	void set_keyboardfunc(const luabind::functor<void>& fun)
	{
		glutKeyboardFunc(&keyboardfunc_callback);
		keyboardfunc = fun;
	}

	luabind::functor<void> mousefunc;

	void mousefunc_callback(int button, int state, int x, int y)
	{
		mousefunc(button, state, x, y);
	}

	void set_mousefunc(const luabind::functor<void>& fun)
	{
		mousefunc = fun;
	}
}

void bind_glut(lua_State* L)
{
	using namespace luabind;
	using namespace glut_bindings;

	open(L);

	module(L)
	[	
		def("glutInitWindowSize", &glutInitWindowSize),
		def("glutInitWindowPosition", &glutInitWindowPosition),
		def("glutInitDisplayMode", &glutInitDisplayMode),

		class_<glut_constants>("glut")
			.enum_("constants")
			[
				value("RGB", GLUT_RGB),
				value("RGBA", GLUT_RGBA),
				value("INDEX", GLUT_INDEX),
				value("SINGLE", GLUT_SINGLE),
				value("DOUBLE", GLUT_DOUBLE),
				value("DEPTH", GLUT_DEPTH),
				value("STENCIL", GLUT_STENCIL),
				value("LEFT_BUTTON", GLUT_LEFT_BUTTON),
				value("MIDDLE_BUTTON", GLUT_MIDDLE_BUTTON),
				value("RIGHT_BUTTON", GLUT_RIGHT_BUTTON),
				value("UP", GLUT_UP),
				value("DOWN", GLUT_DOWN),
				value("ELAPSED_TIME", GLUT_ELAPSED_TIME)
			],

		def("glutCreateWindow", &glutCreateWindow),
		def("glutDestroyWindow", &glutDestroyWindow),
		def("glutFullScreen", &glutFullScreen),
		def("glutDisplayFunc", &set_displayfunc),
		def("glutKeyboardFunc", &set_keyboardfunc),
		def("glutReshapeFunc", &set_reshapefunc),
		def("glutIdleFunc", &set_idlefunc),
		def("glutMainLoop", &glutMainLoop),
		def("glutSwapBuffers", &glutSwapBuffers),
		def("glutGet", &glutGet),
		def("glutSolidSphere", &glutSolidSphere),
		def("glutWireSphere", &glutWireSphere),
		def("glutWireTeapot", &glutWireTeapot),
		def("glutSolidTeapot", &glutSolidTeapot),

		// -- opengl

		class_<gl_constants>("gl")
			.enum_("constants")
			[
				value("COLOR_BUFFER_BIT", GL_COLOR_BUFFER_BIT),
					value("DEPTH_BUFFER_BIT", GL_DEPTH_BUFFER_BIT),
				value("TRIANGLES", GL_TRIANGLES),
				value("MODELVIEW", GL_MODELVIEW),
				value("PROJECTION", GL_PROJECTION)
			],

		def("glBegin", &glBegin),
		def("glVertex3", &glVertex3f),
		def("glEnd", &glEnd),
		def("glClear", &glClear),
		def("glPushMatrix", &glPushMatrix),
		def("glPopMatrix", &glPopMatrix),
		def("glRotate", &glRotatef),
		def("glColor3", &glColor3f),
		def("glColor4", &glColor4f),
		def("glMatrixMode", &glMatrixMode),
		def("glLoadIdentity", &glLoadIdentity),
		def("glViewport", &glViewport),
		def("glTranslate", &glTranslatef),

		// -- glu

		def("gluPerspective", &gluPerspective)
	];
}

int main()
{
	lua_State* L = lua_open();
	lua_baselibopen(L);
	lua_mathlibopen(L);
	bind_glut(L);

	int argc = 1;
	char* argv[1];
	argv[0] = "blabla";

	glutInit (&argc, argv);

	lua_dofile(L, "glut_bindings.lua");

	lua_close(L);
	return 0;
}

