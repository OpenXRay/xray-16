/*
www.sourceforge.net/projects/tinyxml
Original code (2.0 and earlier )copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/
#include "stdafx.h"
#include <ctype.h>
#include "tinyxml.h"

bool TiXmlBase::condenseWhiteSpace = true;

TiXmlNode::TiXmlNode(NodeType _type) : TiXmlBase()
{
    parent = 0;
    type = _type;
    firstChild = 0;
    lastChild = 0;
    prev = 0;
    next = 0;
}

TiXmlNode::~TiXmlNode()
{
    TiXmlNode* node = firstChild;
    TiXmlNode* temp = 0;

    while (node)
    {
        temp = node;
        node = node->next;
        delete temp;
    }
}

void TiXmlNode::Clear()
{
    TiXmlNode* node = firstChild;
    TiXmlNode* temp = 0;

    while (node)
    {
        temp = node;
        node = node->next;
        delete temp;
    }

    firstChild = 0;
    lastChild = 0;
}

TiXmlNode* TiXmlNode::LinkEndChild(TiXmlNode* node)
{
    assert(node->parent == 0 || node->parent == this);
    assert(node->GetDocument() == 0 || node->GetDocument() == this->GetDocument());

    if (node->Type() == TiXmlNode::DOCUMENT)
    {
        delete node;
        if (GetDocument())
            GetDocument()->SetError(TIXML_ERROR_DOCUMENT_TOP_ONLY, 0, 0, TIXML_ENCODING_UNKNOWN);
        return 0;
    }

    node->parent = this;

    node->prev = lastChild;
    node->next = 0;

    if (lastChild)
        lastChild->next = node;
    else
        firstChild = node; // it was an empty list.

    lastChild = node;
    return node;
}

const TiXmlNode* TiXmlNode::FirstChild(const char* _value) const
{
    const TiXmlNode* node;
    for (node = firstChild; node; node = node->next)
    {
        if (xr_strcmp(node->Value(), _value) == 0)
            return node;
    }
    return 0;
}

const TiXmlNode* TiXmlNode::LastChild(const char* _value) const
{
    const TiXmlNode* node;
    for (node = lastChild; node; node = node->prev)
    {
        if (xr_strcmp(node->Value(), _value) == 0)
            return node;
    }
    return 0;
}

const TiXmlNode* TiXmlNode::IterateChildren(const TiXmlNode* previous) const
{
    if (!previous)
    {
        return FirstChild();
    }
    else
    {
        assert(previous->parent == this);
        return previous->NextSibling();
    }
}

const TiXmlNode* TiXmlNode::IterateChildren(const char* val, const TiXmlNode* previous) const
{
    if (!previous)
    {
        return FirstChild(val);
    }
    else
    {
        assert(previous->parent == this);
        return previous->NextSibling(val);
    }
}

const TiXmlNode* TiXmlNode::NextSibling(const char* _value) const
{
    const TiXmlNode* node;
    for (node = next; node; node = node->next)
    {
        if (xr_strcmp(node->Value(), _value) == 0)
            return node;
    }
    return 0;
}

const TiXmlNode* TiXmlNode::PreviousSibling(const char* _value) const
{
    const TiXmlNode* node;
    for (node = prev; node; node = node->prev)
    {
        if (xr_strcmp(node->Value(), _value) == 0)
            return node;
    }
    return 0;
}

const TiXmlElement* TiXmlNode::FirstChildElement() const
{
    const TiXmlNode* node;

    for (node = FirstChild(); node; node = node->NextSibling())
    {
        if (node->ToElement())
            return node->ToElement();
    }
    return 0;
}

const TiXmlElement* TiXmlNode::FirstChildElement(const char* _value) const
{
    const TiXmlNode* node;

    for (node = FirstChild(_value); node; node = node->NextSibling(_value))
    {
        if (node->ToElement())
            return node->ToElement();
    }
    return 0;
}

const TiXmlElement* TiXmlNode::NextSiblingElement() const
{
    const TiXmlNode* node;

    for (node = NextSibling(); node; node = node->NextSibling())
    {
        if (node->ToElement())
            return node->ToElement();
    }
    return 0;
}

const TiXmlElement* TiXmlNode::NextSiblingElement(const char* _value) const
{
    const TiXmlNode* node;

    for (node = NextSibling(_value); node; node = node->NextSibling(_value))
    {
        if (node->ToElement())
            return node->ToElement();
    }
    return 0;
}

const TiXmlDocument* TiXmlNode::GetDocument() const
{
    const TiXmlNode* node;

    for (node = this; node; node = node->parent)
    {
        if (node->ToDocument())
            return node->ToDocument();
    }
    return 0;
}

TiXmlElement::TiXmlElement(const char* _value) : TiXmlNode(TiXmlNode::ELEMENT)
{
    firstChild = lastChild = 0;
    value = _value;
}

TiXmlElement::~TiXmlElement() { ClearThis(); }
void TiXmlElement::ClearThis()
{
    Clear();
    while (attributeSet.First())
    {
        TiXmlAttribute* node = attributeSet.First();
        attributeSet.Remove(node);
        delete node;
    }
}

const char* TiXmlElement::Attribute(const char* name) const
{
    const TiXmlAttribute* node = attributeSet.Find(name);
    if (node)
        return node->Value();
    return 0;
}

//#ifdef TIXML_USE_STL
// const xr_string* TiXmlElement::Attribute( const xr_string& name ) const
//{
//	const TiXmlAttribute* node = attributeSet.Find( name );
//	if ( node )
//		return &node->ValueStr();
//	return 0;
//}
//#endif

const char* TiXmlElement::Attribute(const char* name, int* i) const
{
    const char* s = Attribute(name);
    if (i)
    {
        if (s)
        {
            *i = atoi(s);
        }
        else
        {
            *i = 0;
        }
    }
    return s;
}

const char* TiXmlElement::Attribute(const char* name, double* d) const
{
    const char* s = Attribute(name);
    if (d)
    {
        if (s)
        {
            *d = atof(s);
        }
        else
        {
            *d = 0;
        }
    }
    return s;
}

int TiXmlElement::QueryIntAttribute(const char* name, int* ival) const
{
    const TiXmlAttribute* node = attributeSet.Find(name);
    if (!node)
        return TIXML_NO_ATTRIBUTE;
    return node->QueryIntValue(ival);
}

int TiXmlElement::QueryDoubleAttribute(const char* name, double* dval) const
{
    const TiXmlAttribute* node = attributeSet.Find(name);
    if (!node)
        return TIXML_NO_ATTRIBUTE;
    return node->QueryDoubleValue(dval);
}

bool TiXmlElement::Accept(TiXmlVisitor* visitor) const
{
    if (visitor->VisitEnter(*this, attributeSet.First()))
    {
        for (const TiXmlNode* node = FirstChild(); node; node = node->NextSibling())
        {
            if (!node->Accept(visitor))
                break;
        }
    }
    return visitor->VisitExit(*this);
}

const char* TiXmlElement::GetText() const
{
    const TiXmlNode* child = this->FirstChild();
    if (child)
    {
        const TiXmlText* childText = child->ToText();
        if (childText)
        {
            return childText->Value();
        }
    }
    return 0;
}

TiXmlDocument::TiXmlDocument() : TiXmlNode(TiXmlNode::DOCUMENT)
{
    tabsize = 4;
    useMicrosoftBOM = false;
    ClearError();
}

bool TiXmlDocument::Accept(TiXmlVisitor* visitor) const
{
    if (visitor->VisitEnter(*this))
    {
        for (const TiXmlNode* node = FirstChild(); node; node = node->NextSibling())
        {
            if (!node->Accept(visitor))
                break;
        }
    }
    return visitor->VisitExit(*this);
}

const TiXmlAttribute* TiXmlAttribute::Next() const
{
    // We are using knowledge of the sentinel. The sentinel
    // have a value or name.
    if (next->value.empty() && next->name.empty())
        return 0;
    return next;
}

// const TiXmlAttribute* TiXmlAttribute::Previous() const
//{
//	// We are using knowledge of the sentinel. The sentinel
//	// have a value or name.
//	if ( prev->value.empty() && prev->name.empty() )
//		return 0;
//	return prev;
//}

int TiXmlAttribute::QueryIntValue(int* ival) const
{
    if (TIXML_SSCANF(value.c_str(), "%d", ival) == 1)
        return TIXML_SUCCESS;
    return TIXML_WRONG_TYPE;
}

int TiXmlAttribute::QueryDoubleValue(double* dval) const
{
    if (TIXML_SSCANF(value.c_str(), "%lf", dval) == 1)
        return TIXML_SUCCESS;
    return TIXML_WRONG_TYPE;
}

void TiXmlAttribute::SetIntValue(int _value)
{
    char buf[64];
#if defined(TIXML_SNPRINTF)
    TIXML_SNPRINTF(buf, sizeof(buf), "%d", _value);
#else
    xr_sprintf(buf, "%d", _value);
#endif
    SetValue(buf);
}

void TiXmlAttribute::SetDoubleValue(double _value)
{
    char buf[256];
#if defined(TIXML_SNPRINTF)
    TIXML_SNPRINTF(buf, sizeof(buf), "%lf", _value);
#else
    xr_sprintf(buf, "%lf", _value);
#endif
    SetValue(buf);
}

int TiXmlAttribute::IntValue() const { return atoi(value.c_str()); }
double TiXmlAttribute::DoubleValue() const { return atof(value.c_str()); }
bool TiXmlComment::Accept(TiXmlVisitor* visitor) const { return visitor->Visit(*this); }
bool TiXmlText::Accept(TiXmlVisitor* visitor) const { return visitor->Visit(*this); }
TiXmlDeclaration::TiXmlDeclaration(const char* _version, const char* _encoding, const char* _standalone)
    : TiXmlNode(TiXmlNode::DECLARATION)
{
    version = _version;
    encoding = _encoding;
    standalone = _standalone;
}

#ifdef TIXML_USE_STL
TiXmlDeclaration::TiXmlDeclaration(const xr_string& _version, const xr_string& _encoding, const xr_string& _standalone)
    : TiXmlNode(TiXmlNode::DECLARATION)
{
    version = _version;
    encoding = _encoding;
    standalone = _standalone;
}
#endif

bool TiXmlDeclaration::Accept(TiXmlVisitor* visitor) const { return visitor->Visit(*this); }
bool TiXmlUnknown::Accept(TiXmlVisitor* visitor) const { return visitor->Visit(*this); }
TiXmlAttributeSet::TiXmlAttributeSet()
{
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

TiXmlAttributeSet::~TiXmlAttributeSet()
{
    assert(sentinel.next == &sentinel);
    assert(sentinel.prev == &sentinel);
}

void TiXmlAttributeSet::Add(TiXmlAttribute* addMe)
{
#ifdef TIXML_USE_STL
    assert(!Find(TIXML_STRING(addMe->Name()))); // Shouldn't be multiply adding to the set.
#else
    assert(!Find(addMe->Name())); // Shouldn't be multiply adding to the set.
#endif

    addMe->next = &sentinel;
    addMe->prev = sentinel.prev;

    sentinel.prev->next = addMe;
    sentinel.prev = addMe;
}

void TiXmlAttributeSet::Remove(TiXmlAttribute* removeMe)
{
    TiXmlAttribute* node;

    for (node = sentinel.next; node != &sentinel; node = node->next)
    {
        if (node == removeMe)
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->next = 0;
            node->prev = 0;
            return;
        }
    }
    assert(0); // we tried to remove a non-linked attribute.
}

#ifdef TIXML_USE_STL
const TiXmlAttribute* TiXmlAttributeSet::Find(const xr_string& name) const
{
    for (const TiXmlAttribute* node = sentinel.next; node != &sentinel; node = node->next)
    {
        if (node->name == name)
            return node;
    }
    return 0;
}

#endif

const TiXmlAttribute* TiXmlAttributeSet::Find(const char* name) const
{
    for (const TiXmlAttribute* node = sentinel.next; node != &sentinel; node = node->next)
    {
        if (xr_strcmp(node->name.c_str(), name) == 0)
            return node;
    }
    return 0;
}
