////////////////////////////////////////////////////////////////////////////
//	Module 		: debug_text_tree.h
//	Created 	: 02.04.2008
//  Modified 	: 03.04.2008
//	Author		: Lain
//	Description : Text tree for onscreen debugging
////////////////////////////////////////////////////////////////////////////

#ifndef AI_DEBUG_TEXT_TREE_H_INCLUDED
#define AI_DEBUG_TEXT_TREE_H_INCLUDED

IC xr_string __cdecl make_xrstr(pcstr format, ...)
{
    va_list args;
    va_start(args, format);

    char temp[4096];
    vsprintf(temp, format, args);

    va_end(args);
    return xr_string(temp);
}

IC xr_string __cdecl make_xrstr(bool b) { return b ? "+" : "-"; }
IC xr_string __cdecl make_xrstr(float f) { return make_xrstr("%f", f); }
IC xr_string __cdecl make_xrstr(s32 d) { return make_xrstr("%i", d); }
IC xr_string __cdecl make_xrstr(u32 d) { return make_xrstr("%u", d); }
IC xr_string __cdecl make_xrstr(s64 d) { return make_xrstr("%i", d); }
IC xr_string __cdecl make_xrstr(u64 d) { return make_xrstr("%u", d); }
IC xr_string __cdecl make_xrstr(Fvector3 v) { return make_xrstr("[%f][%f][%f]", v.x, v.y, v.z); }
IC xr_string __cdecl make_xrstr(const xr_string& s) { return s; }

namespace debug
{
class text_tree
{
public: // START INTERFACE
    text_tree(char separator = ':', int group_id_ = 0) : group_id(group_id_), shown(true),
                                                         separator(separator), num_siblings(0) {}
    void toggle_show(int group_id);

    // finds node by first string
    text_tree* find_node(const xr_string& s1);
    // adds if cant find
    text_tree& find_or_add(const xr_string& s1);

    // add_text appends text to this node
    // add_line makes child nodes
    template <class Type1>
    void add_text(const Type1& t);
    template <class Type1, class Type2>
    void add_text(const Type1& t1, const Type2& t2);
    template <class Type1>
    text_tree& add_line(const Type1& a1);
    template <class Type1, class Type2>
    text_tree& add_line(const Type1& a1, const Type2& a2);
    template <class Type1, class Type2, class Type3>
    text_tree& add_line(const Type1& a1, const Type2& a2, const Type3& a3);
    template <class Type1, class Type2, class Type3, class Type4>
    text_tree& add_line(const Type1& a1, const Type2& a2, const Type3& a3, const Type4& a4);
    template <class Type1, class Type2, class Type3, class Type4, class Type5>
    text_tree& add_line(const Type1& a1, const Type2& a2, const Type3& a3, const Type4& a4, const Type5& a5);

    void clear();

    template <class OutFunc>
    void output(OutFunc func, int indent = 4);

    virtual ~text_tree() { clear(); }
private: // END INTERFACE
    typedef xr_list<text_tree*> Children;
    typedef xr_vector<int> Columns;
    typedef xr_vector<xr_string> Strings;

    template <class OutFunc>
    void output(int current_indent, int indent, Columns& columns, OutFunc func);
    void prepare(int current_indent, int indent, Columns& columns);

    static void deleter(text_tree* p) { xr_delete(p); }
    int group_id;
    bool shown;
    Strings strings;
    Children children;
    char separator;
    int num_siblings;

    text_tree& add_line();
    text_tree(const text_tree& t); // no copying allowed
};

void draw_text_tree(text_tree& tree,
    int indent, // in spaces
    int ori_x, int ori_y,
    int offs, // skip offs lines
    int column_size, // in pixels
    int max_rows, u32 color1, u32 color2);

void log_text_tree(text_tree& tree);

#include "debug_text_tree_inline.h"

} // namespace debug

#endif // defined(AI_DEBUG_TEXT_TREE_H_INCLUDED)
