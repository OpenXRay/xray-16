#pragma once
#ifdef DEBUG
#include <string>

// fwd. decl.
template <class T> struct _vector3; typedef _vector3<float> Fvector;
template <class T> struct _matrix; typedef _matrix<float> Fmatrix;
template <class T> class _box3; typedef _box3<float> Fbox;

XRCORE_API std::string get_string(bool v);
XRCORE_API std::string get_string(const Fvector& v);
XRCORE_API std::string get_string(const Fmatrix& dop);
XRCORE_API std::string get_string(const Fbox& box);

XRCORE_API std::string dump_string(const char* name, const Fvector& v);
XRCORE_API std::string dump_string(const char* name, const Fmatrix& form);
XRCORE_API void dump(const char* name, const Fmatrix& form);
XRCORE_API void dump(const char* name, const Fvector& v);

#endif
