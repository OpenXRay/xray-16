/* 	Refactoring from https://github.com/ocornut/imgui/issues/306
    It's basically the same exact code with a few modifications (and tons of additions)
*/

#ifndef IMGUINODEGRAPHEDITOR_H_
#define IMGUINODEGRAPHEDITOR_H_

#ifndef IMGUI_API
#include <imgui.h>
#endif //IMGUI_API
#define NO_IMGUIFILESYSTEM

/*
 *   // Basic usage:
    // Here we create a window (please skip if already in a window)
    ImGui::SetNextWindowSize(ImVec2(700,600), ImGuiSetCond_FirstUseEver);
    if (ImGui::Begin("Example: Custom Node Graph", NULL))
    {
        ImGui::TestNodeGraphEditor();   // see its code for further info

    }
    ImGui::End();

// Please see: https://gist.github.com/Flix01/a7873b73f0ffb00c87260e5bf13a18d4
// for another test demo
*/

// TODO:
/*
-> DONE - Implement a copy/paste functionality for standard Nodes (i.e. nodes that use FieldInfo)
-> DONE - Load/Save NodeGraphEditor Style.
-> DONE - Serialization/Deserialization of the whole NodeGraphEditor + Nodes
-> DONE - Add node clipping: node links are not culled at all, but it's better than nothing.
-> DONE - Adjust zooming.
          NOW ZOOMING WORKS PROPERLY ONLY IF ImGui::GetIO().FontAllowUserScaling = false,
          (otherwise there's a BAD fallback).
-> DONE - Nodes links are culled too (not sure if t's faster... it should).

-> DONE - Added multiple node selection plus the concept of 'active node' (that is actually not used by the current code).
-> DONE - Added link hovering (holding SHIFT down) and direct link deletion (SHIFT down + LMB).
-> DONE - Added node name renaming with double LMB click.

-> Add/Adjust/Fix more FieldTypes. TODO! And test/fix FT_CUSTOM field type too.

-> Complete integration with TabWindow::TabLabel: set the modified flag correctly ( I guess this will need tons of testing :( )
*/

// BUGS:
/*
-> BUG: When MMB drags the background ImGui::GetIO().WantMouseCapture incorrectly returns false (because g.ActiveID==0; no idea on how to fix this)
        This happens only with the glfw3 binding (not sure if it depends on its version or not)
-> FIXED: Interference of rectangular selections in multiple Node Graph Editors
-> FIXED: When links are very horizontal, holding SHIFT down, they are not hoverable (because the AABB's height of the link is zero).
-> FIXED: https://github.com/Flix01/imgui/issues/16
*/

// Enforce cdecl calling convention for functions called by the standard library, in case compilation settings changed the default to e.g. __vectorcall
#ifdef _MSC_VER
#define IMGUINGE_CDECL __cdecl
#else
#define IMGUINGE_CDECL
#endif

namespace ImGui	{
#   ifndef IMGUIHELPER_H_
// To make it compatible without serialization, we must still
// clone the FieldType enum from imguihelper.h...
// (Mmmh, this might be a problem if another addons will do the same in the future...)

// IMPORTANT: FT_INT,FT_UNSIGNED,FT_FLOAT,FT_DOUBLE,FT_BOOL support from 1 to 4 components.
enum FieldType {
    FT_INT=0,
    FT_UNSIGNED,
    FT_FLOAT,
    FT_DOUBLE,
    //--------------- End types that support 1 to 4 array components ----------
    FT_STRING,      // an arbitrary-length string (or a char blob that can be used as custom type)
    FT_ENUM,        // serialized/deserialized as FT_INT
    FT_BOOL,
    FT_COLOR,       // serialized/deserialized as FT_FLOAT (with 3 or 4 components)
    FT_TEXTLINE,    // a (series of) text line(s) (separated by '\n') that are fed one at a time in the Deserializer callback
    FT_CUSTOM,      // a custom type that is served like FT_TEXTLINE (=one line at a time).
    FT_COUNT
};
#   endif //IMGUIHELPER_H_

    class FieldInfo {
    protected:
#       ifndef IMGUIFIELDINFO_MAX_LABEL_LENGTH
#       define IMGUIFIELDINFO_MAX_LABEL_LENGTH 32
#       endif //IMGUIFIELDINFO_MAX_LABEL_LENGTH
#       ifndef IMGUIFIELDINFO_MAX_TOOLTIP_LENGTH
#       define IMGUIFIELDINFO_MAX_TOOLTIP_LENGTH 64
#       endif //IMGUIFIELDINFO_MAX_TOOLTIP_LENGTH

    public:
        int type;                   // usually one of the ImGui::FieldTypes in imguihelper.h
        void* pdata;                // ptr to a variable of type "type" (or to an array of "types")
        char label[IMGUIFIELDINFO_MAX_LABEL_LENGTH];
        char tooltip[IMGUIFIELDINFO_MAX_TOOLTIP_LENGTH];
        // in case of FT_STRING max number of characters, in case of FT_FLOAT or FT_DOUBLE the number of decimals to be displayed (experiment for other types and see)
        int precision;
        // used only for FT_INT, FT_UNSIGNED, FT_FLOAT, FT_DOUBLE
        int numArrayElements;       // up to 4
        double minValue,maxValue;
        bool needsRadiansToDegs;    // optional for FT_FLOAT and FT_DOUBLE only
        // used only for FT_ENUM (internally it uses FT_INT, pdata must point to an int):
        int numEnumElements;
        typedef bool (*TextFromEnumDelegate)(void*, int, const char**); // userData is the first param
        TextFromEnumDelegate  textFromEnumFunctionPointer;  // used only when type==FT_ENUM, otherwise set it to NULL. The method is used to convert an int to a char*.
        void* userData;          // passed to textFromEnumFunctionPointer when type==FT_ENUM (useful if you want to share the same TextFromEnumDelegate for multiple enums). Otherwise set it to NULL or use it as you like.
        typedef int (*GetNumEnumElementsDelegate)(void*); // userData is the first param
        GetNumEnumElementsDelegate getNumEnumElementsFunctionPointer;  // used OPTIONALLY only when type==FT_ENUM, otherwise set it to NULL. The method overrides the value of 'numEnumElements'.
        typedef void (*EditedFieldDelegate)(FieldInfo& field,int widgetIndex);  // widgetIndex is always zero
        EditedFieldDelegate editedFieldDelegate;
        // used only for FT_CUSTOM
        typedef bool (*RenderFieldDelegate)(FieldInfo& field);
        RenderFieldDelegate renderFieldDelegate;
        typedef bool (*CopyFieldDelegate)(FieldInfo& fdst,const FieldInfo& fsrc);
        CopyFieldDelegate copyFieldDelegate;

//-------------------------------------------------------------------------------
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
        typedef bool (*SerializeFieldDelegate)(ImGuiHelper::Serializer& s,const FieldInfo& src);
        SerializeFieldDelegate serializeFieldDelegate;
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
        typedef bool (*DeserializeFieldDelegate)(FieldInfo& dst,int ft,int numArrayElements,const void* pValue,const char* name);
        DeserializeFieldDelegate deserializeFieldDelegate;
        // ------------------------------------------------------
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------
        IMGUI_API bool render(int nodeWidth);

    protected:
        FieldInfo() {}
        void init (int _type=FT_INT,void* _pdata=NULL,const char* _label=NULL,const char* _tooltip=NULL,
                   int _precision=0,int _numArrayElements=0,double _lowerLimit=0,double _upperLimit=1,bool _needsRadiansToDegs=false,
                   int _numEnumElements=0,TextFromEnumDelegate _textFromEnumFunctionPointer=NULL,void* _userData=NULL,GetNumEnumElementsDelegate _getNumEnumElementsFunctionPointer=NULL,
                   RenderFieldDelegate _renderFieldDelegate=NULL,EditedFieldDelegate _editedFieldDelegate=NULL)
        {
            label[0]='\0';if (_label) {strncpy(label,_label,IMGUIFIELDINFO_MAX_LABEL_LENGTH);label[IMGUIFIELDINFO_MAX_LABEL_LENGTH-1]='\0';}
            tooltip[0]='\0';if (_tooltip) {strncpy(tooltip,_tooltip,IMGUIFIELDINFO_MAX_TOOLTIP_LENGTH);tooltip[IMGUIFIELDINFO_MAX_TOOLTIP_LENGTH-1]='\0';}
            type = _type;
            pdata = _pdata;
            precision = _precision;
            numArrayElements = _numArrayElements;
            minValue = _lowerLimit;
            maxValue = _upperLimit;
            needsRadiansToDegs = _needsRadiansToDegs;
            numEnumElements = _numEnumElements;
            textFromEnumFunctionPointer = _textFromEnumFunctionPointer;
            userData = _userData;
            getNumEnumElementsFunctionPointer = _getNumEnumElementsFunctionPointer;
            renderFieldDelegate = _renderFieldDelegate;
            editedFieldDelegate = _editedFieldDelegate;
        }

        inline bool isCompatibleWith(const FieldInfo& f) const {
            return (type==f.type &&
                    numArrayElements == f.numArrayElements);   // Warning: we can't use numArrayElements for other purposes when it's not used....
        }
        //bool copyFrom(const FieldInfo& f);
        IMGUI_API bool copyPDataValueFrom(const FieldInfo& f);
//-------------------------------------------------------------------------------
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
        IMGUI_API bool serialize(ImGuiHelper::Serializer& s) const;
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
        IMGUI_API const char* deserialize(const ImGuiHelper::Deserializer& d,const char* start);
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------

        friend class FieldInfoVector;
        friend class Node;
    };
    class FieldInfoVector : public ImVector < FieldInfo >    {
    public:
    // Warning: returned reference might not stay valid for long in these methods
    IMGUI_API FieldInfo& addField(int* pdata,int numArrayElements=1,const char* label=NULL,const char* tooltip=NULL,int precision=0,int lowerLimit=0,int upperLimit=100,void* userData=NULL);
    IMGUI_API FieldInfo& addField(unsigned* pdata,int numArrayElements=1,const char* label=NULL,const char* tooltip=NULL,int precision=0,unsigned lowerLimit=0,unsigned upperLimit=100,void* userData=NULL);
    IMGUI_API FieldInfo& addField(float* pdata,int numArrayElements=1,const char* label=NULL,const char* tooltip=NULL,int precision=3,float lowerLimit=0,float upperLimit=1,void* userData=NULL,bool needsRadiansToDegs=false);
    IMGUI_API FieldInfo& addField(double* pdata,int numArrayElements=1,const char* label=NULL,const char* tooltip=NULL,int precision=3,double lowerLimit=0,double upperLimit=100,void* userData=NULL,bool needsRadiansToDegs=false);

    IMGUI_API FieldInfo& addFieldEnum(int* pdata,int numEnumElements,FieldInfo::TextFromEnumDelegate textFromEnumFunctionPtr,const char* label=NULL,const char* tooltip=NULL,void* userData=NULL);
    IMGUI_API FieldInfo& addFieldEnum(int* pdata,FieldInfo::GetNumEnumElementsDelegate getNumEnumElementsFunctionPtr,FieldInfo::TextFromEnumDelegate textFromEnumFunctionPtr,const char* label=NULL,const char* tooltip=NULL,void* userData=NULL);
    IMGUI_API FieldInfo& addFieldEnum(int *pdata, int numEnumElements, const char* const* items, const char *label=NULL, const char *tooltip=NULL);
    IMGUI_API FieldInfo& addFieldEnum(int *pdata,const char* items_separated_by_zeros, const char *label=NULL, const char *tooltip=NULL);     // separate items with \0, end item-list with \0\0
    IMGUI_API FieldInfo& addField(bool* pdata,const char* label=NULL,const char* tooltip=NULL,void* userData=NULL);
    IMGUI_API FieldInfo& addFieldColor(float* pdata,bool useAlpha=true,const char* label=NULL,const char* tooltip=NULL,int precision=3,void* userData=NULL);
    FieldInfo& addFieldTextEdit(char* pdata, int textLength=0, const char* label=NULL, const char* tooltip=NULL, int flags=ImGuiInputTextFlags_EnterReturnsTrue, void* userData=NULL) {
        return addField(pdata,textLength,label,tooltip,flags,false,-1.f,userData);
    }
    FieldInfo& addFieldTextEditMultiline(char* pdata, int textLength=0, const char* label=NULL, const char* tooltip=NULL, int flags=ImGuiInputTextFlags_EnterReturnsTrue, float optionalHeight=-1.f, void* userData=NULL) {
        return addField(pdata,textLength,label,tooltip,flags,true,optionalHeight,userData);
    }
    FieldInfo& addFieldTextWrapped(char* pdata, int textLength=0, const char* label=NULL, const char* tooltip=NULL,void* userData=NULL) {
        return addField(pdata,textLength,label,tooltip,-1,true,-1.f,userData);
    }
    FieldInfo& addFieldTextEditAndBrowseButton(char* pdata, int textLength=0, const char* label=NULL, const char* tooltip=NULL, int flags=ImGuiInputTextFlags_EnterReturnsTrue,void* userData=NULL) {
        return addField(pdata,textLength,label,tooltip,flags,false,-1.f,userData,true);
    }

    IMGUI_API FieldInfo& addFieldCustom(FieldInfo::RenderFieldDelegate renderFieldDelegate,FieldInfo::CopyFieldDelegate copyFieldDelegate,void* userData
//-------------------------------------------------------------------------------
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
        ,FieldInfo::SerializeFieldDelegate serializeFieldDelegate=NULL,
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
        FieldInfo::DeserializeFieldDelegate deserializeFieldDelegate=NULL
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------
    );

    void copyPDataValuesFrom(const FieldInfoVector& o)   {
        for (int i=0,isz=o.size()<size()?o.size():size();i<isz;i++) {
            const FieldInfo& of = o[i];
            FieldInfo& f = (*this)[i];
            f.copyPDataValueFrom(of);
        }
    }

//-------------------------------------------------------------------------------
#   if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#    ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
    bool serialize(ImGuiHelper::Serializer& s) const {
        bool rt = true;
        for (int i=0,isz=size();i<isz;i++) {
            const FieldInfo& f = (*this)[i];
            rt|=f.serialize(s);
            // should I stop if rt is false ?
        }
        return rt;
    }
#   endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#   ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
    const char* deserialize(const ImGuiHelper::Deserializer& d,const char* start)   {
        const char* pend = start;
        for (int i=0,isz=size();i<isz;i++) {
            FieldInfo& f = (*this)[i];
            pend = f.deserialize(d,pend);
        }
        return pend;
    }
#   endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#   endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------

protected:
    IMGUI_API FieldInfo& addField(char* pdata, int textLength=0, const char* label=NULL, const char* tooltip=NULL, int flags=ImGuiInputTextFlags_EnterReturnsTrue, bool multiline=false,float optionalHeight=-1.f, void* userData=NULL,bool isSingleEditWithBrowseButton=false);
    friend class Node;
};
//--------------------------------------------------------------------------------------------

class Node
{
    public:
    virtual ~Node() {}
    mutable void* user_ptr;
    mutable int userID;
    inline const char* getName() const {return Name;}
    inline int getType() const {return typeID;}
    inline int getNumInputSlots() const {return InputsCount;}
    inline int getNumOutputSlots() const {return OutputsCount;}
    inline void setOpen(bool flag) {isOpen=flag;}    
	inline const ImVec2 GetPos(float currentFontWindowScale = 1.f) const { return ImVec2(Pos.x*currentFontWindowScale, Pos.y*currentFontWindowScale); }

    protected:
    FieldInfoVector fields; // I guess you can just skip these at all and implement virtual methods... but it was supposed to be useful...
    // virtual methods
    virtual bool render(float nodeWidth) // should return "true" if the node has been edited and its values modified (to fire "edited callbacks")
    {
        bool nodeEdited = false;
        for (int i=0,isz=fields.size();i<isz;i++)   {
            FieldInfo& f = fields[i];
            nodeEdited|=f.render(nodeWidth);
        }
        return nodeEdited;
    }
    virtual const char* getTooltip() const {return NULL;}
    virtual const char* getInfo() const {return NULL;}
    virtual void onEdited() {}  // called (a few seconds) after the node has been edited
    virtual void onCopied() {}  // called after the node fileds has been copied from another node
    virtual void onLoaded() {}  // called after the node has been loaded (=deserialized from file)
    virtual bool canBeCopied() const {return true;}
    // called on a class basis to set the default colors different from the ones defined in NodeGraphEditor::GetStyle()
    // [but on an instance basis these colors can still be overridden using the protected fields defined below, or better NodeGraphEditor::overrideNodeTitleBarColors(...)]
    virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut,ImU32& defaultTitleBgColorOut,float& defaultTitleBgColorGradientOut) const {
            defaultTitleTextColorOut = defaultTitleBgColorOut = 0;  // 0 -> use values defined in NodeGraphEditor::GetStyle()
            defaultTitleBgColorGradientOut = -1;                    // -1 -> use value defined in NodeGraphEditor::GetStyle()
    }

    // some constants
#   ifndef IMGUINODE_MAX_NAME_LENGTH
#   define IMGUINODE_MAX_NAME_LENGTH 32
#   endif //IMGUINODE_MAX_NAME_LENGTH
#   ifndef IMGUINODE_MAX_INPUT_SLOTS
#   define IMGUINODE_MAX_INPUT_SLOTS 8
#   endif //IMGUINODE_MAX_INPUT_SLOTS
#   ifndef IMGUINODE_MAX_OUTPUT_SLOTS
#   define IMGUINODE_MAX_OUTPUT_SLOTS 8
#   endif //IMGUINODE_MAX_OUTPUT_SLOTS
#   ifndef IMGUINODE_MAX_SLOT_NAME_LENGTH
#   define IMGUINODE_MAX_SLOT_NAME_LENGTH 12
#   endif //IMGUINODE_MAX_SLOT_NAME_LENGTH
    // ---------------

    ImVec2  Pos, Size;
    char    Name[IMGUINODE_MAX_NAME_LENGTH];
    int     InputsCount, OutputsCount;
    char    InputNames[IMGUINODE_MAX_INPUT_SLOTS][IMGUINODE_MAX_SLOT_NAME_LENGTH];
    char    OutputNames[IMGUINODE_MAX_OUTPUT_SLOTS][IMGUINODE_MAX_SLOT_NAME_LENGTH];
    mutable float startEditingTime; // used for Node Editing Callbacks
    mutable bool isOpen;
    mutable bool isSelected;
    int typeID;
    float baseWidthOverride;
    bool mustOverrideName,mustOverrideInputSlots,mustOverrideOutputSlots;
    ImU32 overrideTitleTextColor,overrideTitleBgColor;  // 0 -> don't override
    float overrideTitleBgColorGradient;                 //-1 -> don't override
    bool isInEditingMode;

    Node() : Pos(0,0),Size(0,0),isSelected(false),baseWidthOverride(-1),mustOverrideName(false),mustOverrideInputSlots(false),mustOverrideOutputSlots(false),overrideTitleTextColor(0),overrideTitleBgColor(0),overrideTitleBgColorGradient(-1.f),isInEditingMode(false),parentNodeGraphEditor(NULL) {}
    IMGUI_API void init(const char* name, const ImVec2& pos,const char* inputSlotNamesSeparatedBySemicolons=NULL,const char* outputSlotNamesSeparatedBySemicolons=NULL,int _nodeTypeID=0/*,float currentWindowFontScale=-1.f*/);

    inline ImVec2 GetInputSlotPos(int slot_no,float currentFontWindowScale=1.f) const   { return ImVec2(Pos.x*currentFontWindowScale,           Pos.y*currentFontWindowScale + Size.y * ((float)slot_no+1) / ((float)InputsCount+1)); }
    inline ImVec2 GetOutputSlotPos(int slot_no,float currentFontWindowScale=1.f) const  { return ImVec2(Pos.x*currentFontWindowScale + Size.x,  Pos.y*currentFontWindowScale + Size.y * ((float)slot_no+1) / ((float)OutputsCount+1)); }

    friend struct NodeLink;
    friend class NodeGraphEditor;

    // Helper static methods to simplify code of the derived classes
    // casts:
    template <typename T> inline static T* Cast(Node* n,int TYPE) {return ((n && n->getType()==TYPE) ? static_cast<T*>(n) : NULL);}
    template <typename T> inline static const T* Cast(const Node* n,int TYPE) {return ((n && n->getType()==TYPE) ? static_cast<const T*>(n) : NULL);}

    private:
    class NodeGraphEditor* parentNodeGraphEditor;

    protected:
    NodeGraphEditor& getNodeGraphEditor() {IM_ASSERT(parentNodeGraphEditor);return *parentNodeGraphEditor;}
    const NodeGraphEditor& getNodeGraphEditor() const {IM_ASSERT(parentNodeGraphEditor);return *parentNodeGraphEditor;}


};

struct NodeLink
{
    Node*  InputNode;   int InputSlot;
    Node*  OutputNode;  int OutputSlot;

    NodeLink(Node* input_node, int input_slot, Node* output_node, int output_slot) {
        InputNode = input_node; InputSlot = input_slot;
        OutputNode = output_node; OutputSlot = output_slot;
    }

    friend struct NodeGraphEditor;
};

class NodeGraphEditor
#if (defined(IMGUITABWINDOW_H_) && !defined(IMGUINODEGRAPHEDITOR_NO_TABLABEL))
: public TabWindow::TabLabel
#endif //IMGUITABWINDOW_H_
{
    public:
    typedef Node* (*NodeFactoryDelegate)(int nodeType,const ImVec2& pos,const NodeGraphEditor& nge);
    enum NodeState {NS_ADDED,NS_DELETED,NS_EDITED};
    enum LinkState {LS_ADDED,LS_DELETED};

    protected:
    ImVector<Node*> nodes;          // used as a garbage collector too
    ImVector<NodeLink> links;
    ImVec2 scrolling;
    Node *activeNode;               // It's one of the selected nodes (ATM always the first, but the concept of 'active node' is never used by this code: i.e. we could have not included any 'active node' selection at all)
    Node *sourceCopyNode;           // this is owned by the NodeGraphEditor
    Node *menuNode;                 // It's one of the 2 hovered nodes (hovered _in_list or hovered_in_scene), so that the context-menu can retrieve it.
    bool inited;
    bool allowOnlyOneLinkPerInputSlot;  // multiple links can still be connected to single output slots
    bool avoidCircularLinkLoopsInOut;   // however multiple paths from a node to another are still allowed (only in-out circuits are prevented)
    //bool isAContextMenuOpen;            // to fix a bug
    float oldFontWindowScale;           // to fix zooming (CTRL+mouseWheel)    
    float maxConnectorNameWidth;        //used to enlarge node culling space to include connector names
    int nodeListFilterComboIndex;

    // Node types here are supposed to be zero-based and contiguous
    const char** pNodeTypeNames; // NOT OWNED! -> Must point to a static reference. Must contain ALL node names.
    int numNodeTypeNames;
    NodeFactoryDelegate nodeFactoryFunctionPtr;

    struct AvailableNodeInfo {
        int type,maxNumInstances,curNumInstances;
        const char* name;   // from static persitent user storage
        AvailableNodeInfo(int _type=0,int _maxNumInstances=-1,int _curNumInstances=0,const char* _name=NULL) : type(_type),maxNumInstances(_maxNumInstances),curNumInstances(_curNumInstances),name(_name) {}
    };
    ImVector<AvailableNodeInfo> availableNodesInfo;     // These will appear in the "add node menu"
    ImVector<int> availableNodesInfoInverseMap;         // map: absolute node type -> availableNodesInfo index. Must be size() = totalNumberOfNodeTypes.

    typedef void (*NodeCallback)(Node*& node,NodeState state,NodeGraphEditor& editor);
    typedef void (*LinkCallback)(const NodeLink& link,LinkState state,NodeGraphEditor& editor);
	typedef void(*LeftPaneCallback)(NodeGraphEditor& editor);
	typedef void(*SaveCallback)(NodeGraphEditor& editor);
    LinkCallback linkCallback;// called after a link is added and before it's deleted
    NodeCallback nodeCallback;// called after a node is added, after it's edited and before it's deleted
	LeftPaneCallback leftPaneCallback;// called in left pane render
	SaveCallback saveCallback;// called after button "Save" is pressed
    float nodeEditedTimeThreshold; // time in seconds that must elapse after the last "editing touch" before the NS_EDITED callback is called

    public:
    struct Style {
        ImVec4 color_background;
        ImU32 color_grid;
        float grid_line_width,grid_size;
        ImU32 color_node;
        ImU32 color_node_frame;
        ImU32 color_node_selected;
        ImU32 color_node_active;
        ImU32 color_node_frame_selected;
        ImU32 color_node_frame_active;
        ImU32 color_node_hovered;
        ImU32 color_node_frame_hovered;
        float node_rounding;
        ImVec2 node_window_padding;
        ImU32 color_node_input_slots;
        ImU32 color_node_input_slots_border;
        ImU32 color_node_output_slots;
        ImU32 color_node_output_slots_border;
        float node_slots_radius;
        int node_slots_num_segments;
        ImU32 color_link;
        float link_line_width;
        float link_control_point_distance;
        int link_num_segments;  // in AddBezierCurve(...)
        ImVec4 color_node_title;
        ImU32 color_node_title_background;
        float color_node_title_background_gradient;
        ImVec4 color_node_input_slots_names;
        ImVec4 color_node_output_slots_names;        
        ImU32 color_mouse_rectangular_selection;
        ImU32 color_mouse_rectangular_selection_frame;
        Style() {
            color_background =          ImColor(21, 21, 21, 200);
            color_grid =                ImColor(200,200,200,40);
            grid_line_width =           1.f;
            grid_size =                 64.f;

            color_node =                ImColor(60,60,60);
            color_node_frame =          ImColor(100,100,100);
            color_node_selected =       ImColor(75,75,85);
            color_node_active =         ImColor(85,85,65);
            color_node_frame_selected = ImColor(115,115,115);
            color_node_frame_active =   ImColor(125,125,105);
            color_node_hovered =        ImColor(85,85,85);
            color_node_frame_hovered =  ImColor(125,125,125);
            node_rounding =             4.f;
            node_window_padding =       ImVec2(8.f,8.f);

            color_node_input_slots =    ImColor(47, 241, 74, 150);
            color_node_output_slots =   ImColor(0, 231, 49, 150);
            node_slots_radius =         5.f;

            color_link =                ImColor(93, 193, 72);
            link_line_width =           3.f;
            link_control_point_distance = 50.f;
            link_num_segments =         0;

            color_node_title =          ImColor(255, 255, 255, 255);
            color_node_title_background = 0;//ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]);
            color_node_title_background_gradient = 0.f;   // in [0,0.5f] used only if available (performance is better when 0)
            color_node_input_slots_names = ImColor(49, 255, 43, 191);
			color_node_output_slots_names = ImColor(143, 255, 128, 191);

            color_mouse_rectangular_selection =         ImColor(255,0,0,45);
            color_mouse_rectangular_selection_frame =   ImColor(45,0,0,175);

            color_node_input_slots_border = color_node_output_slots_border = ImColor(60,60,60,0);
            node_slots_num_segments = 12;
        }

        IMGUI_API static bool Edit(Style& style);
        static void Reset(Style& style) {style = Style();}

#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
        IMGUI_API static bool Save(const Style& style,ImGuiHelper::Serializer& s);
        static inline bool Save(const Style &style, const char *filename)    {
            ImGuiHelper::Serializer s(filename);
            return Save(style,s);
        }
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
        IMGUI_API static bool Load(Style& style, ImGuiHelper::Deserializer& d, const char ** pOptionalBufferStart=NULL);
        static inline bool Load(Style& style,const char* filename) {
            ImGuiHelper::Deserializer d(filename);
            return Load(style,d);
        }
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION

    };
    bool show_grid;
    bool show_connection_names;
    bool show_left_pane;
    bool show_style_editor;         // in the left_pane
    bool show_load_save_buttons;    // in the left_pane
    bool show_top_pane;
    bool show_node_copy_paste_buttons;
    static bool UseSlidersInsteadOfDragControls;
    mutable void* user_ptr;
    static Style& GetStyle() {static Style style;return style;}
    /*mutable ImGuiColorEditMode colorEditMode;*/
    float nodesBaseWidth;

    NodeGraphEditor(bool show_grid_= true,bool show_connection_names_=true,bool _allowOnlyOneLinkPerInputSlot=true,bool _avoidCircularLinkLoopsInOut=true,bool init_in_ctr=false) {
        scrolling = ImVec2(0.0f, 0.0f);
        show_grid = show_grid_;
        show_connection_names = show_connection_names_;
        activeNode = dragNode.node = sourceCopyNode = NULL;
        allowOnlyOneLinkPerInputSlot = _allowOnlyOneLinkPerInputSlot;
        avoidCircularLinkLoopsInOut = _avoidCircularLinkLoopsInOut;
		nodeCallback = NULL;
		linkCallback = NULL;
		leftPaneCallback = nullptr;
		saveCallback = nullptr;
		nodeEditedTimeThreshold = 1.5f;
        user_ptr = NULL;
        show_left_pane = true;
        show_top_pane = true;
        show_style_editor = false;
        show_load_save_buttons = false;
        show_node_copy_paste_buttons = true;
        pNodeTypeNames = NULL;
        numNodeTypeNames = 0;
        nodeFactoryFunctionPtr = NULL;
        inited = init_in_ctr;
        //colorEditMode = ImGuiColorEditMode_RGB;
        //isAContextMenuOpen = false;
        oldFontWindowScale = 0.f;
        nodesBaseWidth = 120.f;
        maxConnectorNameWidth = 0;
        nodeListFilterComboIndex = 0;
        setModified(false);
    }
    virtual ~NodeGraphEditor() {
        clear();
    }
    void clear() {
        if (linkCallback)   {
            for (int i=links.size()-1;i>=0;i--)  {
                const NodeLink& link = links[i];
                linkCallback(link,LS_DELETED,*this);
            }
        }
        links.clear();
        for (int i=nodes.size()-1;i>=0;i--)  {
            Node*& node = nodes[i];
            if (node)   {
                if (nodeCallback) nodeCallback(node,NS_DELETED,*this);
                node->~Node();              // ImVector does not call it
                ImGui::MemFree(node);       // items MUST be allocated by the user using ImGui::MemAlloc(...)
                node = NULL;
            }
        }
        nodes.clear();
        scrolling = ImVec2(0,0);
        if (sourceCopyNode) {
                sourceCopyNode->~Node();              // ImVector does not call it
                ImGui::MemFree(sourceCopyNode);       // items MUST be allocated by the user using ImGui::MemAlloc(...)
                sourceCopyNode = NULL;
        }
        activeNode = dragNode.node = NULL;
        oldFontWindowScale = 0.f;
        for (int i=0,isz=availableNodesInfo.size();i<isz;i++) {availableNodesInfo[i].curNumInstances=0;}
        nodeListFilterComboIndex = 0;
    }

#if (!(defined(IMGUITABWINDOW_H_) && !defined(IMGUINODEGRAPHEDITOR_NO_TABLABEL)))
    private:
    mutable bool modified;
    protected:
    void setModified(bool flag) {modified=flag;}    // so that this method is always available
    public:
    bool getModified() const {return modified;}     // so that this method is always available
#   else
    virtual bool saveAs(const char* savePath=NULL) {
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION) && !defined(NO_IMGUIHELPER_SERIALIZATION_SAVE))
        return save(savePath);
#       else
        return false;
#       endif
    }
#   endif //IMGUITABWINDOW_H_

    bool isInited() const {return !inited;}

    bool isEmpty() const {return nodes.size()==0;}

    // nodeTypeNames must point to a block of static memory: it's not owned, nor copied. pOptionalNodeTypesToUse is copied.
    IMGUI_API void registerNodeTypes(const char* nodeTypeNames[], int numNodeTypeNames, NodeFactoryDelegate _nodeFactoryFunctionPtr, const int* pOptionalNodeTypesToUse=NULL, int numNodeTypesToUse=-1, const int* pOptionalMaxNumAllowedInstancesToUse=NULL, int numMaxNumAllowedInstancesToUse=0, bool sortEntriesAlphabetically=true);
    inline int getNumAvailableNodeTypes() const {return availableNodesInfo.size();}
    bool registerNodeTypeMaxAllowedInstances(int nodeType,int maxAllowedNodeTypeInstances=-1) {
        AvailableNodeInfo* ni = fetchAvailableNodeInfo(nodeType);
        if (ni) ni->maxNumInstances = maxAllowedNodeTypeInstances;
        return (ni);
    }

    Node* addNode(int nodeType,const ImVec2& Pos=ImVec2(0,0))  {return addNode(nodeType,Pos,NULL);}
    bool deleteNode(Node* node) {
        if (node == activeNode)  activeNode = NULL;
        if (node == dragNode.node) dragNode.node = NULL;
        if (node == menuNode)  menuNode = NULL;
        for (int i=0;i<nodes.size();i++)    {
            Node*& n = nodes[i];
            if (n==node)  {
                AvailableNodeInfo* ni = fetchAvailableNodeInfo(node->getType());
                if (ni) --(ni->curNumInstances);
                removeAnyLinkFromNode(n);
                if (nodeCallback) nodeCallback(n,NS_DELETED,*this);
                n->~Node();              // ImVector does not call it
                ImGui::MemFree(n);       // items MUST be allocated by the user using ImGui::MemAlloc(...)
                if (i+1 < nodes.size()) n = nodes[nodes.size()-1];    // swap with the last node
                nodes.resize(nodes.size()-1);
                if (!activeNode) findANewActiveNode();
                return true;
            }
        }
        if (!activeNode) findANewActiveNode();
        return false;
    }
    int getNumNodeInstances(int nodeType,int* pMaxNumAllowedInstancesForThisNodeType=NULL) const {
        const AvailableNodeInfo* ni = fetchAvailableNodeInfo(nodeType);
        if (pMaxNumAllowedInstancesForThisNodeType) *pMaxNumAllowedInstancesForThisNodeType = ni ? ni->maxNumInstances : -1;
        return ni->curNumInstances;
    }
    bool addLink(Node* inputNode, int input_slot, Node* outputNode, int output_slot,bool checkIfAlreadyPresent = false)	{
        if (!inputNode || !outputNode) return false;
        bool insert = true;
        if (checkIfAlreadyPresent) insert = !isLinkPresent(inputNode,input_slot,outputNode,output_slot);
        if (insert) {
            links.push_back(NodeLink(inputNode,input_slot,outputNode,output_slot));
            if (linkCallback) linkCallback(links[links.size()-1],LS_ADDED,*this);
        }
        return insert;
    }
    bool removeLink(Node* inputNode, int input_slot, Node* outputNode, int output_slot) {
        int link_idx = -1;
        bool ok = isLinkPresent(inputNode,input_slot,outputNode,output_slot,&link_idx);
        if (ok) ok = removeLinkAt(link_idx);
        return ok;
    }
    IMGUI_API void removeAnyLinkFromNode(Node* node,bool removeInputLinks=true,bool removeOutputLinks=true);
    IMGUI_API bool isLinkPresent(Node* inputNode, int input_slot, Node* outputNode, int output_slot,int* pOptionalIndexInLinkArrayOut=NULL) const;

    // To be called INSIDE a window
    IMGUI_API void render();

    // Optional helper methods:
    Node* getHoveredNode() {return menuNode;}  // This is actually not strictly the hovered node, but the node called 'menuNode'
    const Node* getHoveredNode() const {return menuNode;}
    IMGUI_API int getSelectedNodes(ImVector<Node*>& rv);  // returns rv.size(). The active node should be contained inside rv (the first AFAIK).
    IMGUI_API int getSelectedNodes(ImVector<const Node*>& rv) const;
    Node* getActiveNode() {return activeNode;}  // The 'active' node is the first of the selected nodes
    const Node* getActiveNode() const {return activeNode;}
    const char* getActiveNodeInfo() const {return activeNode->getInfo();}
    void setActiveNode(const Node* node) {if (node) {node->isSelected=true;activeNode=const_cast<Node*>(node);setModified(true);}}
    void selectNode(const Node* node,bool findANewActiveNodeWhenNeeded=true)   {selectNodePrivate(node,true,findANewActiveNodeWhenNeeded);}
    void unselectNode(const Node* node,bool findANewActiveNodeWhenNeeded=true)   {selectNodePrivate(node,false,findANewActiveNodeWhenNeeded);}
    void selectAllNodes(bool findANewActiveNodeWhenNeeded=true) {selectAllNodesPrivate(true,findANewActiveNodeWhenNeeded);}
    void unselectAllNodes() {selectAllNodesPrivate(false);}
    bool isNodeSelected(const Node* node) const {return (node && node->isSelected);}

    IMGUI_API void getOutputNodesForNodeAndSlot(const Node* node,int output_slot,ImVector<Node*>& returnValueOut,ImVector<int>* pOptionalReturnValueInputSlotOut=NULL) const;
    IMGUI_API void getInputNodesForNodeAndSlot(const Node* node,int input_slot,ImVector<Node*>& returnValueOut,ImVector<int>* pOptionalReturnValueOutputSlotOut=NULL) const;
    // if allowOnlyOneLinkPerInputSlot == true:
    IMGUI_API Node* getInputNodeForNodeAndSlot(const Node* node,int input_slot,int* pOptionalReturnValueOutputSlotOut=NULL) const;
    IMGUI_API bool isNodeReachableFrom(const Node *node1, int slot1, bool goBackward,const Node* nodeToFind,int* pOptionalNodeToFindSlotOut=NULL) const;
    IMGUI_API bool isNodeReachableFrom(const Node *node1, bool goBackward,const Node* nodeToFind,int* pOptionalNode1SlotOut=NULL,int* pOptionalNodeToFindSlotOut=NULL) const;
    IMGUI_API bool hasLinks(Node* node) const;
    IMGUI_API int getAllNodesOfType(int typeID,ImVector<Node*>* pNodesOut=NULL,bool clearNodesOutBeforeUsage=true);
    IMGUI_API int getAllNodesOfType(int typeID,ImVector<const Node*>* pNodesOut=NULL,bool clearNodesOutBeforeUsage=true) const;
    int getNumNodes() const {return nodes.size();}
    Node* getNode(int index) {return (index>=0 && index<nodes.size()) ? nodes[index] : NULL;}
    const Node* getNode(int index) const {return (index>=0 && index<nodes.size()) ? nodes[index] : NULL;}
    Node* getCopiedNode() {return sourceCopyNode;}
    const Node* getCopiedNode() const {return sourceCopyNode;}


    // It should be better not to add/delete node/links in the callbacks... (but all is untested here)
    void setNodeCallback(NodeCallback cb) {nodeCallback=cb;}
    void setLinkCallback(LinkCallback cb) {linkCallback=cb;}
    void setNodeEditedCallbackTimeThreshold(int seconds) {nodeEditedTimeThreshold=seconds;}
	void setLeftPaneCallback(LeftPaneCallback cb) { leftPaneCallback = cb; }
	void setSaveCallback(SaveCallback cb) { saveCallback = cb; }

//-------------------------------------------------------------------------------
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
        IMGUI_API bool save(ImGuiHelper::Serializer& s);
        inline bool save(const char *filename)    {
            ImGuiHelper::Serializer s(filename);
            return save(s);
        }
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
        IMGUI_API bool load(ImGuiHelper::Deserializer& d, const char ** pOptionalBufferStart=NULL);
        inline bool load(const char* filename) {
            ImGuiHelper::Deserializer d(filename);
            return load(d);
        }
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------

    // I suggest we don not use these 3; however we can:
    IMGUI_API bool overrideNodeName(Node* node,const char* newName);
    IMGUI_API void overrideNodeTitleBarColors(Node* node,const ImU32* pTextColor,const ImU32* pBgColor,const float* pBgColorGradient);    // default values can be reset using 0 for colors and -1 for gradient
    IMGUI_API bool overrideNodeInputSlots(Node* node,const char* slotNamesSeparatedBySemicolons);
    IMGUI_API bool overrideNodeOutputSlots(Node* node,const char* slotNamesSeparatedBySemicolons);

    // This are the chars used as buttons in the nodes' titlebars. Users might want to change them.
    static char CloseCopyPasteChars[3][5];  // By default = {"x","^","v"};

    protected:

    struct DragNode {
        ImVec2 pos;
        Node* node;int inputSlotIdx,outputSlotIdx;
        DragNode() : node(NULL),inputSlotIdx(-1),outputSlotIdx(-1) {}
        bool isValid() const {return node && (inputSlotIdx>=0 || outputSlotIdx>=0);}
        void reset() {*this=DragNode();}
    };
    DragNode dragNode;

    inline AvailableNodeInfo* fetchAvailableNodeInfo(int nodeType) {
        const int tmp = availableNodesInfoInverseMap[nodeType];return tmp>=0 ? &availableNodesInfo[tmp] : NULL;
    }
    inline const AvailableNodeInfo* fetchAvailableNodeInfo(int nodeType) const {
        const int tmp = availableNodesInfoInverseMap[nodeType];return tmp>=0 ? &availableNodesInfo[tmp] : NULL;
    }

    Node* addNode(int nodeType,const ImVec2& Pos,AvailableNodeInfo* pOptionalNi)  {
        if (!nodeFactoryFunctionPtr) return NULL;
        if (!pOptionalNi) pOptionalNi = fetchAvailableNodeInfo(nodeType);
        if (!pOptionalNi || (pOptionalNi->maxNumInstances>=0 && pOptionalNi->curNumInstances>=pOptionalNi->maxNumInstances)) return NULL;
        Node* rv = nodeFactoryFunctionPtr(pOptionalNi->type,Pos,*this);
        if (rv) ++(pOptionalNi->curNumInstances);
        return addNode(rv);
    }
    // BEST PRACTICE: always call this method like: Node* node = addNode(ExampleNode::Create(...));
    Node* addNode(Node* justCreatedNode)	{
        if (justCreatedNode) {
            justCreatedNode->parentNodeGraphEditor = this;
            nodes.push_back(justCreatedNode);
            if (nodeCallback) nodeCallback(nodes[nodes.size()-1],NS_ADDED,*this);
        }
        return justCreatedNode;
    }
    IMGUI_API void copyNode(Node* n);
    IMGUI_API bool removeLinkAt(int link_idx);
    // Warning: node index changes when a node becomes active!
    inline int getNodeIndex(const Node* node) {
        for (int i=0;i<nodes.size();i++)    {
            const Node* n = nodes[i];
            if (n==node) return i;
        }
        return -1;
    }
    inline int findANewActiveNode() {
        activeNode=NULL;
        for (int i=0,isz=nodes.size();i<isz;i++)    {
            Node* n = nodes[i];
            if (n->isSelected) {activeNode=n;return i;}
        }
        return -1;
    }
    static Style style;


    private:
    // Refactored for cleaner exposure (without the misleading 'flag' argument)
    void selectNodePrivate(const Node* node, bool flag=true,bool findANewActiveNodeWhenNeeded=true);
    void selectAllNodesPrivate(bool flag=true,bool findANewActiveNodeWhenNeeded=true);
    static int IMGUINGE_CDECL AvailableNodeInfoNameSorter(const void *s0, const void *s1) {
        const AvailableNodeInfo& ni0 = *((AvailableNodeInfo*) s0);
        const AvailableNodeInfo& ni1 = *((AvailableNodeInfo*) s1);
        return strcmp(ni0.name,ni1.name);
    }
    struct RectangularSelectionData {
        ImVec2 mouseRectangularSelectionForNodesMin,mouseRectangularSelectionForNodesMax;
        ImVec2  absSelectionMin,absSelectionMax;
        bool mouseRectangularSelectionForNodesStarted;
        RectangularSelectionData() :
            mouseRectangularSelectionForNodesMin(0,0),mouseRectangularSelectionForNodesMax(0,0),
            absSelectionMin(0,0),absSelectionMax(0,0),mouseRectangularSelectionForNodesStarted(false) {}
    };
    RectangularSelectionData rectangularSelectionData;

};


#ifndef IMGUINODEGRAPHEDITOR_NOTESTDEMO
IMGUI_API void TestNodeGraphEditor();
#endif //IMGUINODEGRAPHEDITOR_NOTESTDEMO


}	// namespace ImGui



#endif //IMGUINODEGRAPHEDITOR_H_

