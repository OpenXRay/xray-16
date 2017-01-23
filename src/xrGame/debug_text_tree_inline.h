////////////////////////////////////////////////////////////////////////////
//	Module 		: debug_text_tree_inline.h
//	Created 	: 02.04.2008
//  Modified 	: 03.04.2008
//	Author		: Lain
//	Description : Text tree for onscreen debugging 
////////////////////////////////////////////////////////////////////////////


template <class Type1>
void   text_tree::add_text (const Type1& a)
{
	strings.push_back(make_xrstr(a));
}

template <class Type1, class Type2>
void   text_tree::add_text (const Type1& a1, const Type2& a2)
{
	add_text(a1);
	add_text(a2);
}

template <class Type1>
text_tree&   text_tree::add_line (const Type1& a1)
{
	text_tree& child = add_line();
	child.add_text(a1);
	return child;
}

template <class Type1, class Type2>
text_tree&   text_tree::add_line (const Type1& a1, const Type2& a2)
{
	text_tree& child = add_line(a1);
	child.add_text(a2);
	return child;
}

template <class Type1, class Type2, class Type3>
text_tree&   text_tree::add_line (const Type1& a1, const Type2& a2, const Type3& a3)
{
	text_tree& child = add_line(a1, a2);
	child.add_text(a3);
	return child;
}

template <class Type1, class Type2, class Type3, class Type4>
text_tree&   text_tree::add_line (const Type1& a1, 
								  const Type2& a2, 
								  const Type3& a3, 
								  const Type4& a4)
{
	text_tree& child = add_line(a1, a2, a3);
	child.add_text(a4);
	return child;
}

template <class Type1, class Type2, class Type3, class Type4, class Type5>
text_tree&   text_tree::add_line (const Type1& a1, 
								  const Type2& a2, 
								  const Type3& a3, 
								  const Type4& a4, 
								  const Type5& a5)
{
	text_tree& child = add_line(a1, a2, a3, a4);
	child.add_text(a5);
	return child;
}

template <class OutFunc>
void   text_tree::output (OutFunc func, int indent)
{
	Columns columns;
	prepare(0, indent, columns);
	output(0, indent, columns, func);
}

template <class OutFunc>
void   text_tree::output (int current_indent, int indent, Columns& columns, OutFunc func)
{
	xr_string buffer;
	buffer.reserve(1024);
	buffer = "";

	for ( int j=0; j<current_indent; ++j )
	{
		buffer += " ";
	}			

	Strings::iterator i = strings.begin();
	Columns::iterator c = columns.begin();
	for ( ; i!=strings.end(); ++i, ++c )
	{
		buffer += *i;
		size_t num_padding_spaces = *c - (*i).size();
		num_padding_spaces -= (i==strings.begin()) ? current_indent : 0;

		if ( strings.size() == 1 )
		{
			num_padding_spaces = 0;
		}

		for ( size_t j=0; j<num_padding_spaces; ++j )
		{
			buffer += " ";
		}

		Strings::iterator next = i;
		++next;
		if ( next != strings.end() )
		{
			// trailing separator
			buffer += " ";
			buffer += separator;
			buffer += " "; 
		}		
	}

	buffer += char(0);

	if ( strings.size() && buffer.size() )
	{
		func(buffer.data(), num_siblings);
	}

	for ( Children::iterator i=children.begin(); i!=children.end(); ++i )
	{
		if ( (*i)->shown )
		{
			(*i)->output(current_indent+indent, indent, columns, func);
		}			
	}
}
