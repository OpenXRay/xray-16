//- Common Code For All Addons needed just to ease inclusion as separate files in user code ----------------------
#include "../../imgui.h"
#undef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../imgui_internal.h"
//-----------------------------------------------------------------------------------------------------------------

#include <stdlib.h> // qsort


#if !defined(alloca)
#   ifdef _WIN32
#       include <malloc.h>     // alloca
#       if !defined(alloca)
#           define alloca _alloca  // for clang with MS Codegen
#       endif //alloca
#   elif defined(__GLIBC__) || defined(__sun)
#       include <alloca.h>     // alloca
#   else
#       include <stdlib.h>     // alloca
#   endif
#endif //alloca

#include "imguinodegrapheditor.h"

// NB: You can use math functions/operators on ImVec2 if you #define IMGUI_DEFINE_MATH_OPERATORS and #include "imgui_internal.h"
// Here we only declare simple +/- operators so others don't leak into the demo code.
//static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x+rhs.x, lhs.y+rhs.y); }
//static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x-rhs.x, lhs.y-rhs.y); }


#if (defined(_MSC_VER) && !defined(snprintf))
#   define snprintf _snprintf
#endif //(defined(_MSC_VER) && !defined(snprintf))


namespace ImGui	{

namespace NGE_Draw {
// This methods are a subset of the ones already present in imguihelper.h, copied here to enable stand alone usage. Scoped for better isolation (in case of multi .cpp chaining and/or usage in amalgamation engines).
static void ImDrawListPathFillAndStroke(ImDrawList *dl, const ImU32 &fillColor, const ImU32 &strokeColor, bool strokeClosed, float strokeThickness)    {
    if (!dl) return;
    if ((fillColor & IM_COL32_A_MASK) != 0) dl->AddConvexPolyFilled(dl->_Path.Data, dl->_Path.Size, fillColor);
    if ((strokeColor& IM_COL32_A_MASK)!= 0 && strokeThickness>0) dl->AddPolyline(dl->_Path.Data, dl->_Path.Size, strokeColor, strokeClosed, strokeThickness);
    dl->PathClear();
}
static void ImDrawListPathArcTo(ImDrawList *dl, const ImVec2 &centre, const ImVec2 &radii, float amin, float amax, int num_segments)  {
    if (!dl) return;
    if (radii.x == 0.0f || radii.y==0) dl->_Path.push_back(centre);
    dl->_Path.reserve(dl->_Path.Size + (num_segments + 1));
    for (int i = 0; i <= num_segments; i++)
    {
        const float a = amin + ((float)i / (float)num_segments) * (amax - amin);
        dl->_Path.push_back(ImVec2(centre.x + cosf(a) * radii.x, centre.y + sinf(a) * radii.y));
    }
}
static void ImDrawListAddCircle(ImDrawList *dl, const ImVec2 &centre, float radius, const ImU32 &fillColor, const ImU32 &strokeColor, int num_segments, float strokeThickness)   {
    if (!dl) return;
    const ImVec2 radii(radius,radius);
    const float a_max = IM_PI*2.0f * ((float)num_segments - 1.0f) / (float)num_segments;
    ImDrawListPathArcTo(dl,centre, radii, 0.0f, a_max, num_segments);
    ImDrawListPathFillAndStroke(dl,fillColor,strokeColor,true,strokeThickness);
}

inline static void GetVerticalGradientTopAndBottomColors(ImU32 c,float fillColorGradientDeltaIn0_05,ImU32& tc,ImU32& bc)  {
    if (fillColorGradientDeltaIn0_05==0) {tc=bc=c;return;}

    static ImU32 cacheColorIn=0;static float cacheGradientIn=0.f;static ImU32 cacheTopColorOut=0;static ImU32 cacheBottomColorOut=0;
    if (cacheColorIn==c && cacheGradientIn==fillColorGradientDeltaIn0_05)   {tc=cacheTopColorOut;bc=cacheBottomColorOut;return;}
    cacheColorIn=c;cacheGradientIn=fillColorGradientDeltaIn0_05;

    const bool negative = (fillColorGradientDeltaIn0_05<0);
    if (negative) fillColorGradientDeltaIn0_05=-fillColorGradientDeltaIn0_05;
    if (fillColorGradientDeltaIn0_05>0.5f) fillColorGradientDeltaIn0_05=0.5f;


    // New code:
    //#define IM_COL32(R,G,B,A)    (((ImU32)(A)<<IM_COL32_A_SHIFT) | ((ImU32)(B)<<IM_COL32_B_SHIFT) | ((ImU32)(G)<<IM_COL32_G_SHIFT) | ((ImU32)(R)<<IM_COL32_R_SHIFT))
    const int fcgi = fillColorGradientDeltaIn0_05*255.0f;
    const int R = (unsigned char) (c>>IM_COL32_R_SHIFT);    // The cast should reset upper bits (as far as I hope)
    const int G = (unsigned char) (c>>IM_COL32_G_SHIFT);
    const int B = (unsigned char) (c>>IM_COL32_B_SHIFT);
    const int A = (unsigned char) (c>>IM_COL32_A_SHIFT);

    int r = R+fcgi, g = G+fcgi, b = B+fcgi;
    if (r>255) r=255;if (g>255) g=255;if (b>255) b=255;
    if (negative) bc = IM_COL32(r,g,b,A); else tc = IM_COL32(r,g,b,A);

    r = R-fcgi; g = G-fcgi; b = B-fcgi;
    if (r<0) r=0;if (g<0) g=0;if (b<0) b=0;
    if (negative) tc = IM_COL32(r,g,b,A); else bc = IM_COL32(r,g,b,A);

    // Old legacy code (to remove)... [However here we lerp alpha too...]
    /*// Can we do it without the double conversion ImU32 -> ImVec4 -> ImU32 ?
    const ImVec4 cf = ColorConvertU32ToFloat4(c);
    ImVec4 tmp(cf.x+fillColorGradientDeltaIn0_05,cf.y+fillColorGradientDeltaIn0_05,cf.z+fillColorGradientDeltaIn0_05,cf.w+fillColorGradientDeltaIn0_05);
    if (tmp.x>1.f) tmp.x=1.f;if (tmp.y>1.f) tmp.y=1.f;if (tmp.z>1.f) tmp.z=1.f;if (tmp.w>1.f) tmp.w=1.f;
    if (negative) bc = ColorConvertFloat4ToU32(tmp); else tc = ColorConvertFloat4ToU32(tmp);
    tmp=ImVec4(cf.x-fillColorGradientDeltaIn0_05,cf.y-fillColorGradientDeltaIn0_05,cf.z-fillColorGradientDeltaIn0_05,cf.w-fillColorGradientDeltaIn0_05);
    if (tmp.x<0.f) tmp.x=0.f;if (tmp.y<0.f) tmp.y=0.f;if (tmp.z<0.f) tmp.z=0.f;if (tmp.w<0.f) tmp.w=0.f;
    if (negative) tc = ColorConvertFloat4ToU32(tmp); else bc = ColorConvertFloat4ToU32(tmp);*/

    cacheTopColorOut=tc;cacheBottomColorOut=bc;
}
inline static ImU32 GetVerticalGradient(const ImVec4& ct,const ImVec4& cb,float DH,float H)    {
    IM_ASSERT(H!=0);
    const float fa = DH/H;
    const float fc = (1.f-fa);
    return ColorConvertFloat4ToU32(ImVec4(
        ct.x * fc + cb.x * fa,
        ct.y * fc + cb.y * fa,
        ct.z * fc + cb.z * fa,
        ct.w * fc + cb.w * fa)
    );
}
static void ImDrawListAddConvexPolyFilledWithVerticalGradient(ImDrawList *dl, const ImVec2 *points, const int points_count, ImU32 colTop, ImU32 colBot, float miny,float maxy)
{
    if (!dl) return;
    if (colTop==colBot)  {
        dl->AddConvexPolyFilled(points,points_count,colTop);
        return;
    }
    const ImVec2 uv = GImGui->DrawListSharedData.TexUvWhitePixel;
    const bool anti_aliased = GImGui->Style.AntiAliasedFill;
    //if (ImGui::GetIO().KeyCtrl) anti_aliased = false; // Debug

    int height=0;
    if (miny<=0 || maxy<=0) {
        const float max_float = 999999999999999999.f;
        miny=max_float;maxy=-max_float;
        for (int i = 0; i < points_count; i++) {
            const float h = points[i].y;
            if (h < miny) miny = h;
            else if (h > maxy) maxy = h;
        }
    }
    height = maxy-miny;
    const ImVec4 colTopf = ColorConvertU32ToFloat4(colTop);
    const ImVec4 colBotf = ColorConvertU32ToFloat4(colBot);


    if (anti_aliased)
    {
        // Anti-aliased Fill
        const float AA_SIZE = 1.0f;

        const ImVec4 colTransTopf(colTopf.x,colTopf.y,colTopf.z,0.f);
        const ImVec4 colTransBotf(colBotf.x,colBotf.y,colBotf.z,0.f);
        const int idx_count = (points_count-2)*3 + points_count*6;
        const int vtx_count = (points_count*2);
        dl->PrimReserve(idx_count, vtx_count);

        // Add indexes for fill
        unsigned int vtx_inner_idx = dl->_VtxCurrentIdx;
        unsigned int vtx_outer_idx = dl->_VtxCurrentIdx+1;
        for (int i = 2; i < points_count; i++)
        {
            dl->_IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx); dl->_IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+((i-1)<<1)); dl->_IdxWritePtr[2] = (ImDrawIdx)(vtx_inner_idx+(i<<1));
            dl->_IdxWritePtr += 3;
        }

        // Compute normals
        ImVec2* temp_normals = (ImVec2*)alloca(points_count * sizeof(ImVec2));
        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            const ImVec2& p0 = points[i0];
            const ImVec2& p1 = points[i1];
            ImVec2 diff = p1 - p0;
            diff *= ImInvLength(diff, 1.0f);
            temp_normals[i0].x = diff.y;
            temp_normals[i0].y = -diff.x;
        }

        for (int i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++)
        {
            // Average normals
            const ImVec2& n0 = temp_normals[i0];
            const ImVec2& n1 = temp_normals[i1];
            ImVec2 dm = (n0 + n1) * 0.5f;
            float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f)
            {
                float scale = 1.0f / dmr2;
                if (scale > 100.0f) scale = 100.0f;
                dm *= scale;
            }
            dm *= AA_SIZE * 0.5f;

            // Add vertices
            //_VtxWritePtr[0].pos = (points[i1] - dm); _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;        // Inner
            //_VtxWritePtr[1].pos = (points[i1] + dm); _VtxWritePtr[1].uv = uv; _VtxWritePtr[1].col = col_trans;  // Outer
            dl->_VtxWritePtr[0].pos = (points[i1] - dm); dl->_VtxWritePtr[0].uv = uv; dl->_VtxWritePtr[0].col = GetVerticalGradient(colTopf,colBotf,points[i1].y-miny,height);        // Inner
            dl->_VtxWritePtr[1].pos = (points[i1] + dm); dl->_VtxWritePtr[1].uv = uv; dl->_VtxWritePtr[1].col = GetVerticalGradient(colTransTopf,colTransBotf,points[i1].y-miny,height);  // Outer
            dl->_VtxWritePtr += 2;

            // Add indexes for fringes
            dl->_IdxWritePtr[0] = (ImDrawIdx)(vtx_inner_idx+(i1<<1)); dl->_IdxWritePtr[1] = (ImDrawIdx)(vtx_inner_idx+(i0<<1)); dl->_IdxWritePtr[2] = (ImDrawIdx)(vtx_outer_idx+(i0<<1));
            dl->_IdxWritePtr[3] = (ImDrawIdx)(vtx_outer_idx+(i0<<1)); dl->_IdxWritePtr[4] = (ImDrawIdx)(vtx_outer_idx+(i1<<1)); dl->_IdxWritePtr[5] = (ImDrawIdx)(vtx_inner_idx+(i1<<1));
            dl->_IdxWritePtr += 6;
        }
        dl->_VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
    else
    {
        // Non Anti-aliased Fill
        const int idx_count = (points_count-2)*3;
        const int vtx_count = points_count;
        dl->PrimReserve(idx_count, vtx_count);
        for (int i = 0; i < vtx_count; i++)
        {
            //_VtxWritePtr[0].pos = points[i]; _VtxWritePtr[0].uv = uv; _VtxWritePtr[0].col = col;
            dl->_VtxWritePtr[0].pos = points[i]; dl->_VtxWritePtr[0].uv = uv; dl->_VtxWritePtr[0].col = GetVerticalGradient(colTopf,colBotf,points[i].y-miny,height);
            dl->_VtxWritePtr++;
        }
        for (int i = 2; i < points_count; i++)
        {
            dl->_IdxWritePtr[0] = (ImDrawIdx)(dl->_VtxCurrentIdx); dl->_IdxWritePtr[1] = (ImDrawIdx)(dl->_VtxCurrentIdx+i-1); dl->_IdxWritePtr[2] = (ImDrawIdx)(dl->_VtxCurrentIdx+i);
            dl->_IdxWritePtr += 3;
        }
        dl->_VtxCurrentIdx += (ImDrawIdx)vtx_count;
    }
}
static void ImDrawListPathFillWithVerticalGradientAndStroke(ImDrawList *dl, const ImU32 &fillColorTop, const ImU32 &fillColorBottom, const ImU32 &strokeColor, bool strokeClosed = false, float strokeThickness = 1.0f, float miny=-1.f, float maxy=-1.f)    {
    if (!dl) return;
    if (fillColorTop==fillColorBottom) dl->AddConvexPolyFilled(dl->_Path.Data,dl->_Path.Size, fillColorTop);
    else if ((fillColorTop & IM_COL32_A_MASK) != 0 || (fillColorBottom & IM_COL32_A_MASK) != 0) ImDrawListAddConvexPolyFilledWithVerticalGradient(dl, dl->_Path.Data, dl->_Path.Size, fillColorTop, fillColorBottom, miny,maxy);
    if ((strokeColor& IM_COL32_A_MASK)!= 0 && strokeThickness>0) dl->AddPolyline(dl->_Path.Data, dl->_Path.Size, strokeColor, strokeClosed, strokeThickness);
    dl->PathClear();
}
static void ImDrawListAddRectWithVerticalGradient(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, const ImU32 &fillColorTop, const ImU32 &fillColorBottom, const ImU32 &strokeColor, float rounding = 0.0f, int rounding_corners = 0x0F,float strokeThickness = 1.0f) {
    if (!dl || (((fillColorTop & IM_COL32_A_MASK) == 0) && ((fillColorBottom & IM_COL32_A_MASK) == 0) && ((strokeColor & IM_COL32_A_MASK) == 0)))  return;
    if (rounding==0.f || rounding_corners==0) {
        dl->AddRectFilledMultiColor(a,b,fillColorTop,fillColorTop,fillColorBottom,fillColorBottom); // Huge speedup!
        if ((strokeColor& IM_COL32_A_MASK)!= 0 && strokeThickness>0.f) {
            dl->PathRect(a, b, rounding, rounding_corners);
            dl->AddPolyline(dl->_Path.Data, dl->_Path.Size, strokeColor, true, strokeThickness);
            dl->PathClear();
        }
    }
    else    {
        dl->PathRect(a, b, rounding, rounding_corners);
        ImDrawListPathFillWithVerticalGradientAndStroke(dl,fillColorTop,fillColorBottom,strokeColor,true,strokeThickness,a.y,b.y);
    }
}
static void ImDrawListAddRectWithVerticalGradient(ImDrawList *dl, const ImVec2 &a, const ImVec2 &b, const ImU32 &fillColor, float fillColorGradientDeltaIn0_05, const ImU32 &strokeColor, float rounding = 0.0f, int rounding_corners = 0x0F,float strokeThickness = 1.0f)   {
    ImU32 fillColorTop,fillColorBottom;GetVerticalGradientTopAndBottomColors(fillColor,fillColorGradientDeltaIn0_05,fillColorTop,fillColorBottom);
    ImDrawListAddRectWithVerticalGradient(dl,a,b,fillColorTop,fillColorBottom,strokeColor,rounding,rounding_corners,strokeThickness);
}
} // namespace

inline static bool EditColorImU32(const char* label,ImU32& color) {
    static ImVec4 tmp;
    tmp = ImColor(color);
    const bool changed = ImGui::ColorEdit4(label,&tmp.x);
    if (changed) color = ImColor(tmp);
    return changed;
}
bool NodeGraphEditor::Style::Edit(NodeGraphEditor::Style& s) {
    bool changed = false;ImVec4 tmp;
    const float dragSpeed = 0.5f;
    const char prec[] = "%1.1f";
    ImGui::PushID(&s);
    ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.5f); // default size is: ImGui::GetWindowWidth()*0.65f

    changed|=ImGui::ColorEdit4( "color_background",&s.color_background.x);
    changed|=EditColorImU32(    "color_grid",s.color_grid);
    changed|=ImGui::DragFloat(  "grid_line_width",&s.grid_line_width,dragSpeed,1.f,32.f,prec);
    changed|=ImGui::DragFloat(  "grid_size",&s.grid_size,dragSpeed,8.f,512.f,prec);
    ImGui::Spacing();
    changed|=EditColorImU32(    "color_node",s.color_node);
    changed|=EditColorImU32(    "color_node_selected",s.color_node_selected);
    changed|=EditColorImU32(    "color_node_active",s.color_node_active);
    changed|=EditColorImU32(    "color_node_hovered",s.color_node_hovered);
    ImGui::Spacing();
    changed|=EditColorImU32(    "color_node_frame",s.color_node_frame);
    changed|=EditColorImU32(    "color_node_frame_selected",s.color_node_frame_selected);
    changed|=EditColorImU32(    "color_node_frame_active",s.color_node_frame_active);
    changed|=EditColorImU32(    "color_node_frame_hovered",s.color_node_frame_hovered);
    ImGui::Spacing();
    changed|=ImGui::DragFloat(  "node_rounding",&s.node_rounding,dragSpeed,0.f,16.f,prec);
    ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.25f-2);
    changed|=ImGui::DragFloat(  "###node_window_padding_x",&s.node_window_padding.x,dragSpeed,0.f,32.f,prec);
    ImGui::SameLine(0,4);
    changed|=ImGui::DragFloat(  "node_window_padding##_y",&s.node_window_padding.y,dragSpeed,0.f,8.f,prec);
    ImGui::PopItemWidth();
    ImGui::Spacing();
    changed|=EditColorImU32(    "color_node_input_slots",s.color_node_input_slots);
    changed|=EditColorImU32(    "color_node_input_slots_border",s.color_node_input_slots_border);
    changed|=EditColorImU32(    "color_node_output_slots",s.color_node_output_slots);
    changed|=EditColorImU32(    "color_node_output_slots_border",s.color_node_output_slots_border);
    changed|=ImGui::DragFloat(  "node_slots_radius",&s.node_slots_radius,dragSpeed,1.f,10.f,prec);
    changed|=ImGui::DragInt("node_slots_num_segments",&s.node_slots_num_segments,1.0f,2,24);
    ImGui::Spacing();
    changed|=EditColorImU32(    "color_mouse_rectangular_selection",s.color_mouse_rectangular_selection);
    changed|=EditColorImU32(    "color_mouse_rectangular_selection_frame",s.color_mouse_rectangular_selection_frame);
    ImGui::Spacing();
    changed|=EditColorImU32(    "color_link",s.color_link);
    changed|=ImGui::DragFloat(  "link_line_width",&s.link_line_width,dragSpeed,1.f,6.f,prec);
    changed|=ImGui::DragFloat(  "link_control_point_distance",&s.link_control_point_distance,dragSpeed,10.f,200.f,prec);
    changed|=ImGui::DragInt(  "link_num_segments",&s.link_num_segments,dragSpeed,0,16.f);
    ImGui::Spacing();
    changed|=ImGui::ColorEdit4( "color_node_title",&s.color_node_title.x);
    changed|=ImGui::EditColorImU32( "color_node_title_background",s.color_node_title_background);
    changed|=ImGui::DragFloat("color_node_title_background_gradient",&s.color_node_title_background_gradient,0.01f,0.f,.5f,"%1.3f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s","Zero gradient renders much faster\nwhen \"node_rounding\" is positive.");
    ImGui::Spacing();
    changed|=ImGui::ColorEdit4( "color_node_input_slots_names",&s.color_node_input_slots_names.x);
    changed|=ImGui::ColorEdit4( "color_node_output_slots_names",&s.color_node_output_slots_names.x);

    ImGui::PopItemWidth();
    ImGui::PopID();
    return changed;
}

#if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
#include "../imguihelper/imguihelper.h"
bool NodeGraphEditor::Style::Save(const NodeGraphEditor::Style &style,ImGuiHelper::Serializer& s)    {
    if (!s.isValid()) return false;

    ImVec4 tmpColor = ImColor(style.color_background);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_background",4);
    tmpColor = ImColor(style.color_grid);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_grid",4);
    s.save(ImGui::FT_FLOAT,&style.grid_line_width,"grid_line_width");
    s.save(ImGui::FT_FLOAT,&style.grid_size,"grid_size");

    tmpColor = ImColor(style.color_node);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node",4);
    tmpColor = ImColor(style.color_node_frame);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_frame",4);
    tmpColor = ImColor(style.color_node_selected);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_selected",4);
    tmpColor = ImColor(style.color_node_active);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_active",4);
    tmpColor = ImColor(style.color_node_frame_selected);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_frame_selected",4);
    tmpColor = ImColor(style.color_node_frame_active);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_frame_active",4);
    tmpColor = ImColor(style.color_node_hovered);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_hovered",4);
    tmpColor = ImColor(style.color_node_frame_hovered);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_frame_hovered",4);
    s.save(ImGui::FT_FLOAT,&style.node_rounding,"node_rounding");
    s.save(ImGui::FT_FLOAT,&style.node_window_padding.x,"node_window_padding",2);

    tmpColor = ImColor(style.color_node_input_slots);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_input_slots",4);
    tmpColor = ImColor(style.color_node_input_slots_border);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_input_slots_border",4);
    tmpColor = ImColor(style.color_node_output_slots);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_output_slots",4);
    tmpColor = ImColor(style.color_node_output_slots_border);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_output_slots_border",4);
    s.save(ImGui::FT_FLOAT,&style.node_slots_radius,"node_slots_radius");
    s.save(&style.node_slots_num_segments,"node_slots_num_segments");

    tmpColor = ImColor(style.color_mouse_rectangular_selection);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_mouse_rectangular_selection",4);
    tmpColor = ImColor(style.color_mouse_rectangular_selection_frame);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_mouse_rectangular_selection_frame",4);

    tmpColor = ImColor(style.color_link);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_link",4);
    s.save(ImGui::FT_FLOAT,&style.link_line_width,"link_line_width");
    s.save(ImGui::FT_FLOAT,&style.link_control_point_distance,"link_control_point_distance");
    s.save(ImGui::FT_INT,&style.link_num_segments,"link_num_segments");

    s.save(ImGui::FT_COLOR,&style.color_node_title.x,"color_node_title",4);
    tmpColor = ImColor(style.color_node_title_background);s.save(ImGui::FT_COLOR,&tmpColor.x,"color_node_title_background",4);
    s.save(ImGui::FT_FLOAT,&style.color_node_title_background_gradient,"color_node_title_background_gradient");

    s.save(ImGui::FT_COLOR,&style.color_node_input_slots_names.x,"color_node_input_slots_names",4);
    s.save(ImGui::FT_COLOR,&style.color_node_output_slots_names.x,"color_node_output_slots_names",4);


    return true;
}
#endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
#include "../imguihelper/imguihelper.h"
static bool StyleParser(ImGuiHelper::FieldType ft,int /*numArrayElements*/,void* pValue,const char* name,void* userPtr)    {
    NodeGraphEditor::Style& s = *((NodeGraphEditor::Style*) userPtr);
    ImVec4& tmp = *((ImVec4*) pValue);  // we cast it soon to float for now...
    switch (ft) {
    case FT_FLOAT:
        if (strcmp(name,"grid_line_width")==0)                              s.grid_line_width = tmp.x;
        else if (strcmp(name,"grid_size")==0)                               s.grid_size = tmp.x;
        else if (strcmp(name,"node_rounding")==0)                           s.node_rounding = tmp.x;
        else if (strcmp(name,"node_window_padding")==0)                     s.node_window_padding = ImVec2(tmp.x,tmp.y);
        else if (strcmp(name,"node_slots_radius")==0)                       s.node_slots_radius = tmp.x;
        else if (strcmp(name,"link_line_width")==0)                         s.link_line_width = tmp.x;
        else if (strcmp(name,"link_control_point_distance")==0)             s.link_control_point_distance = tmp.x;
        else if (strcmp(name,"color_node_title_background_gradient")==0)    s.color_node_title_background_gradient = tmp.x;
    break;
    case FT_INT:
        if (strcmp(name,"link_num_segments")==0)                            s.link_num_segments = *((int*)pValue);
        else if (strcmp(name,"node_slots_num_segments")==0)                 s.node_slots_num_segments = *((int*)pValue);
    break;
    case FT_COLOR:
        if (strcmp(name,"color_background")==0)                             s.color_background = ImColor(tmp);
        else if (strcmp(name,"color_grid")==0)                              s.color_grid = ImColor(tmp);
        else if (strcmp(name,"color_node")==0)                              s.color_node = ImColor(tmp);
        else if (strcmp(name,"color_node_frame")==0)                        s.color_node_frame = ImColor(tmp);
        else if (strcmp(name,"color_node_selected")==0)                     s.color_node_selected = ImColor(tmp);
        else if (strcmp(name,"color_node_active")==0)                       s.color_node_active = ImColor(tmp);
        else if (strcmp(name,"color_node_frame_selected")==0)               s.color_node_frame_selected = ImColor(tmp);
        else if (strcmp(name,"color_node_frame_active")==0)                 s.color_node_frame_active = ImColor(tmp);
        else if (strcmp(name,"color_node_hovered")==0)                      s.color_node_hovered = ImColor(tmp);
        else if (strcmp(name,"color_node_frame_hovered")==0)                s.color_node_frame_hovered = ImColor(tmp);
        else if (strcmp(name,"color_node_input_slots")==0)                  s.color_node_input_slots = ImColor(tmp);
        else if (strcmp(name,"color_node_input_slots_border")==0)           s.color_node_input_slots_border = ImColor(tmp);
        else if (strcmp(name,"color_node_output_slots")==0)                 s.color_node_output_slots = ImColor(tmp);
        else if (strcmp(name,"color_node_output_slots_border")==0)          s.color_node_output_slots_border = ImColor(tmp);
        else if (strcmp(name,"color_mouse_rectangular_selection")==0)       s.color_mouse_rectangular_selection = ImColor(tmp);
        else if (strcmp(name,"color_mouse_rectangular_selection_frame")==0) s.color_mouse_rectangular_selection_frame = ImColor(tmp);
        else if (strcmp(name,"color_link")==0)                              s.color_link = ImColor(tmp);
        else if (strcmp(name,"color_node_title")==0)                        s.color_node_title = ImColor(tmp);
	else if (strcmp(name,"color_node_title_background")==0)		    s.color_node_title_background = ImColor(tmp);
	else if (strcmp(name,"color_node_input_slots_names")==0)            s.color_node_input_slots_names = ImColor(tmp);
        else if (strcmp(name,"color_node_output_slots_names")==0)           s.color_node_output_slots_names = ImColor(tmp);
    break;
    default:
    // TODO: check
    break;
    }
    return false;
}
bool NodeGraphEditor::Style::Load(NodeGraphEditor::Style &style,  ImGuiHelper::Deserializer& d, const char ** pOptionalBufferStart)  {
    if (!d.isValid()) return false;
    const char* offset = d.parse(StyleParser,(void*)&style,pOptionalBufferStart?(*pOptionalBufferStart):NULL);
    if (pOptionalBufferStart) *pOptionalBufferStart=offset;
    return true;
}
#endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#endif //NO_IMGUIHELPER_SERIALIZATION

inline static float ImVec2Dot(const ImVec2& S1,const ImVec2& S2) {return (S1.x*S2.x+S1.y*S2.y);}

// Based on: http://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
inline static float GetSquaredDistancePointSegment(const ImVec2& P,const ImVec2& S1,const ImVec2& S2) {
  const float l2 = (S1.x-S2.x)*(S1.x-S2.x)+(S1.y-S2.y)*(S1.y-S2.y);
  if (l2 < 0.0000001f) return (P.x-S2.x)*(P.x-S2.x)+(P.y-S2.y)*(P.y-S2.y);   // S1 == S2 case
  ImVec2 T(S2 - S1);
  const float tf = ImVec2Dot(P - S1, T)/l2;
  const float minTf = 1.f < tf ? 1.f : tf;
  const float t = 0.f > minTf ? 0.f : minTf;
  T = S1 + T*t;  // T = Projection on the segment
  return (P.x-T.x)*(P.x-T.x)+(P.y-T.y)*(P.y-T.y);
}

inline static float GetSquaredDistanceToBezierCurve(const ImVec2& point,const ImVec2& p1,const ImVec2& p2,const ImVec2& p3,const ImVec2& p4)   {
    static const int num_segments = 2;   // Num Sampling Points In between p1 and p4
    static bool firstRun = true;
    static ImVec4 weights[num_segments];

    if (firstRun)    {
        // static init here
        IM_ASSERT(num_segments>0);    // This are needed for computing distances: cannot be zero
        firstRun = false;
        for (int i = 1; i <= num_segments; i++) {
            float t = (float)i/(float)(num_segments+1);
            float u = 1.0f - t;
            weights[i-1].x=u*u*u;
            weights[i-1].y=3*u*u*t;
            weights[i-1].z=3*u*t*t;
            weights[i-1].w=t*t*t;
        }
    }

    float minSquaredDistance=FLT_MAX,tmp;   // FLT_MAX is probably in <limits.h>
    ImVec2 L = p1,tp;
    for (int i = 0; i < num_segments; i++)  {
        const ImVec4& W=weights[i];
        tp.x = W.x*p1.x + W.y*p2.x + W.z*p3.x + W.w*p4.x;
        tp.y = W.x*p1.y + W.y*p2.y + W.z*p3.y + W.w*p4.y;

	tmp = GetSquaredDistancePointSegment(point,L,tp);
	if (minSquaredDistance>tmp) minSquaredDistance=tmp;
	L=tp;
    }
    tp = p4;
    tmp = GetSquaredDistancePointSegment(point,L,tp);
    if (minSquaredDistance>tmp) minSquaredDistance=tmp;

    return minSquaredDistance;
}

static ImVec2 gNodeGraphEditorWindowPadding(8.f,8.f);
inline static void NodeGraphEditorSetTooltip(const char* tooltip) {
    ImGuiStyle& igStyle = ImGui::GetStyle();
    igStyle.WindowPadding.x = gNodeGraphEditorWindowPadding.x;
    igStyle.WindowPadding.y = gNodeGraphEditorWindowPadding.y;
    ImGui::SetTooltip("%s",tooltip);
    igStyle.WindowPadding.x = igStyle.WindowPadding.y = 0.f;
}

void NodeGraphEditor::render()
{
    if (!inited) inited=true;
    static const ImVec4 transparent = ImVec4(1,1,1,0);

    const ImGuiIO io = ImGui::GetIO();

    // Draw a list of nodes on the left side
    bool open_context_menu = false,open_delete_only_context_menu = false;
    Node* node_hovered_in_list = NULL;
    Node* node_hovered_in_scene = NULL;
    Node* node_to_center_view_around = NULL;
    Node *node_to_fire_edit_callback = NULL,* node_to_paste_from_copy_source = NULL;

    if (show_left_pane) {
        // Helper stuff for setting up the left splitter
        static ImVec2 lastWindowSize=ImGui::GetWindowSize();      // initial window size
        ImVec2 windowSize = ImGui::GetWindowSize();
        const bool windowSizeChanged = lastWindowSize.x!=windowSize.x || lastWindowSize.y!=windowSize.y;
        if (windowSizeChanged) lastWindowSize = windowSize;
        static float w = lastWindowSize.x*0.2f;                    // initial width of the left window

        //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

        ImGui::BeginChild("node_list", ImVec2(w,0));

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Node List##node_list_1",NULL,false))   {
            ImGui::Separator();
            typedef struct _MyDummyStuff {
                const ImVector<AvailableNodeInfo>& availableNodesInfo;
                _MyDummyStuff(const ImVector<AvailableNodeInfo>& _availableNodesInfo) : availableNodesInfo(_availableNodesInfo){}
                static bool item_getter(void* pmds,int idx,const char** pOut) {
                    const _MyDummyStuff& mds = * ((const _MyDummyStuff*) pmds);
                    if (idx<0 || idx>mds.availableNodesInfo.size()) return false;
                    static const char ZeroName[] = "ALL";
                    *pOut = idx==0 ? ZeroName : mds.availableNodesInfo[idx-1].name;
                    return true;
                }
            } MyDummyStuff;
            MyDummyStuff mds(availableNodesInfo);
            ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.5f);
            const int numEntriesInNodeListFilterCombo = availableNodesInfo.size()+1;
            ImGui::Combo("Type Filter",&nodeListFilterComboIndex,&MyDummyStuff::item_getter,&mds,numEntriesInNodeListFilterCombo,numEntriesInNodeListFilterCombo<25 ? numEntriesInNodeListFilterCombo : 25);
            ImGui::PopItemWidth();
            for (int node_idx = 0; node_idx < nodes.Size; node_idx++)   {
                Node* node = nodes[node_idx];
                if (nodeListFilterComboIndex>0 && availableNodesInfoInverseMap[node->getType()]!=nodeListFilterComboIndex-1) continue;
                ImGui::PushID((const void*) node);
                if (ImGui::Selectable(node->Name, node->isSelected)) {
                    if (!node->isSelected || !io.KeyCtrl)  {
                        if (!io.KeyCtrl) unselectAllNodes();
                        selectNodePrivate(node,true,false);
                        //activeNode = node;    // Better not. Changing the active node [currently] makes it go at the bottom of the list: this is bad while we are clicking on it
                    }
                    else unselectNode(node,false);
                }
                //if (ImGui::Selectable(node->Name, node == activeNode)) activeNode = node;
                if (ImGui::IsItemHovered()) {
                    node_hovered_in_list = node;//menuNode=node;
                    if (ImGui::IsMouseClicked(1))   {
                        menuNode=node;
                        open_context_menu=true;
                    }
                    else if (io.MouseReleased[2] || ImGui::IsKeyPressed(io.KeyMap[ImGuiKey_Home]) || io.MouseDoubleClicked[0]) node_to_center_view_around = node;
                }
                ImGui::PopID();
            }
        }
        ImGui::Separator();
        if (activeNode)   {
            const char* nodeInfo = activeNode->getInfo();
            if (nodeInfo && nodeInfo[0]!='\0')  {
                ImGui::Spacing();
                ImGui::Separator();
                if (ImGui::CollapsingHeader("Active Node##activeNode",NULL,false))   {
                    ImGui::Separator();
                    ImGui::TextWrapped("%s",nodeInfo);
                }
                ImGui::Separator();
            }
        }
        if (show_style_editor)   {
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Style Editor##styleEditor",NULL,false))   {
                ImGui::Separator();
                //ImGui::ColorEditMode(colorEditMode);
                Style& thisStyle = GetStyle();
                Style::Edit(thisStyle);
                ImGui::Separator();
#if             (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
                const char* saveName = "nodeGraphEditor.nge.style";
                const char* saveNamePersistent = "/persistent_folder/nodeGraphEditor.nge.style";
                const char* pSaveName = saveName;
#               ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
                if (ImGui::SmallButton("Save##saveGNEStyle")) {
#                   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    pSaveName = saveNamePersistent;
#                   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    if (Style::Save(thisStyle,pSaveName)) {
#                   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                        ImGui::EmscriptenFileSystemHelper::Sync();
#                   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    }
                }
                ImGui::SameLine();
#               endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#               ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
                if (ImGui::SmallButton("Load##loadGNEStyle")) {
#                   ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    if (ImGuiHelper::FileExists(saveNamePersistent)) pSaveName = saveNamePersistent;
#                   endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    Style::Load(thisStyle,pSaveName);
                }
                ImGui::SameLine();
#               endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#               endif //NO_IMGUIHELPER_SERIALIZATION

                if (ImGui::SmallButton("Reset##resetGNEStyle")) {
                    Style::Reset(thisStyle);
                }
            }
            ImGui::Separator();
        }
#if	(defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
        if (show_load_save_buttons) {
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Serialization##serialization",NULL,false))   {
                ImGui::Separator();
                const char* saveName = "nodeGraphEditor.nge";
                const char* saveNamePersistent = "/persistent_folder/nodeGraphEditor.nge";
                const char* pSaveName = saveName;
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
                if (ImGui::SmallButton("Save##saveGNE")) {
#           ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    pSaveName = saveNamePersistent;
#           endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    if (save(pSaveName))    {
#           ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                        ImGui::EmscriptenFileSystemHelper::Sync();
#           endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    }
                }
                ImGui::SameLine();
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
                if (ImGui::SmallButton("Load##loadGNE")) {
#           ifdef YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    if (ImGuiHelper::FileExists(saveNamePersistent)) pSaveName = saveNamePersistent;
#           endif //YES_IMGUIEMSCRIPTENPERSISTENTFOLDER
                    load(pSaveName);
                }
                ImGui::SameLine();
#		endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
                if (ImGui::SmallButton("Clear##clearGNE")) {
                    clear();
                }
            }
            ImGui::Separator();
        }
#       endif //NO_IMGUIHELPER_SERIALIZATION

		if (leftPaneCallback)
			leftPaneCallback(*this);

        ImGui::EndChild();

        // horizontal splitter
        ImGui::SameLine(0);
        static const float splitterWidth = 6.f;

        ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(1,1,1,0.2f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(1,1,1,0.35f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(1,1,1,0.5f));
        /*bool splitterHovered = false,splitterActive=false;
        ImGui::ButtonExEx("##hsplitter1",ImVec2(splitterWidth,-1),&splitterHovered,&splitterActive);
        if (splitterHovered || splitterActive) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);*/
        ImGui::Button("##hsplitter1", ImVec2(splitterWidth,-1));
        const bool splitterActive = ImGui::IsItemActive();
        if (ImGui::IsItemHovered() || splitterActive) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        ImGui::PopStyleColor(3);
        if (splitterActive)  w += ImGui::GetIO().MouseDelta.x;
        if (splitterActive || windowSizeChanged)  {
            const float minw = ImGui::GetStyle().WindowPadding.x + ImGui::GetStyle().FramePadding.x;
            const float maxw = minw + windowSize.x - splitterWidth - ImGui::GetStyle().WindowMinSize.x;
            if (w>maxw)         w = maxw;
            else if (w<minw)    w = minw;
        }
        ImGui::SameLine(0);

        //ImGui::PopStyleVar();

    }

    const bool isMouseDraggingForScrolling = ImGui::IsMouseDragging(2, 0.0f);

    if (ImGui::BeginChild("GraphNodeChildWindow", ImVec2(0,0), true))   {

        // Create our child canvas
        if (show_top_pane)   {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));

            ImGui::Checkbox("Show connection names.", &show_connection_names);
            //if (io.FontAllowUserScaling)
            {ImGui::SameLine(0,15);ImGui::Text("Use CTRL+MW to zoom. Scroll with MMB.");}
            ImGui::SameLine(ImGui::GetWindowWidth()-120);
            ImGui::Checkbox("Show grid", &show_grid);
            ImGui::Text("%s","Double-click LMB on slots to remove their links (or SHIFT+LMB on links).");
            /*ImGui::SameLine(ImGui::GetWindowWidth()-120);
            // Color Mode
            static const char* btnlbls[2]={"HSV##myColorBtnType","RGB##myColorBtnType"};
            if (colorEditMode!=ImGuiColorEditMode_RGB)  {
                if (ImGui::SmallButton(btnlbls[0])) {
                    colorEditMode = ImGuiColorEditMode_RGB;
                    ImGui::ColorEditMode(colorEditMode);
                }
            }
            else if (colorEditMode!=ImGuiColorEditMode_HSV)  {
                if (ImGui::SmallButton(btnlbls[1])) {
                    colorEditMode = ImGuiColorEditMode_HSV;
                    ImGui::ColorEditMode(colorEditMode);
                }
            }
            ImGui::SameLine(0);ImGui::Text("Color Mode");*/
            // ------------------
            ImGui::PopStyleVar(2);
        }

	gNodeGraphEditorWindowPadding = ImGui::GetStyle().WindowPadding;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1,1));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    const Style& style = GetStyle();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, style.color_background);
        if (ImGui::BeginChild("scrolling_region", ImVec2(0,0), true, ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollWithMouse))  {

            ImGuiContext& g = *GImGui;
            ImGuiWindow* window = ImGui::GetCurrentWindow();

            // New: to ensure font scaling in subchilds of the nodes too, we MUST track g.Font->Scale,
            // instead of ImGui::GetCurrentWindow()->FontWindowScale.
	    // Note that this change could break io.FontAllowUserScaling==true (To test, but it didn't work as expacted anyway)
            float oldFontScaleToReset = g.Font->Scale;      // We'll clean up at the bottom
            float fontScaleStored = oldFontWindowScale ? oldFontWindowScale : oldFontScaleToReset;
            float& fontScaleToTrack = g.Font->Scale;

            if (!io.FontAllowUserScaling)   {
                // Set the correct font scale (3 lines)
                fontScaleToTrack = fontScaleStored;
                g.FontBaseSize = io.FontGlobalScale * g.Font->Scale * g.Font->FontSize;
                g.FontSize = window->CalcFontSize();

                if (io.KeyCtrl && ImGui::GetCurrentWindow()==GImGui->HoveredWindow && (io.MouseWheel || io.MouseClicked[2]))   {
                    // Zoom / Scale window
                    float new_font_scale = ImClamp(fontScaleToTrack + g.IO.MouseWheel * 0.075f, 0.50f, 2.50f);
                    if (io.MouseClicked[2]) new_font_scale = 1.f;   // MMB = RESET ZOOM
                    float scale = new_font_scale/fontScaleToTrack;
                    if (scale!=1)	{
                        scrolling=scrolling*scale;
                        // Set the correct font scale (3 lines), and store it
                        fontScaleStored = fontScaleToTrack = new_font_scale;
                        g.FontBaseSize = io.FontGlobalScale * g.Font->Scale * g.Font->FontSize;
                        g.FontSize = window->CalcFontSize();
                    }
                }
            }

            // fixes zooming just a bit
            bool nodesHaveZeroSize = false;
            const float currentFontWindowScale = !io.FontAllowUserScaling ? fontScaleStored : ImGui::GetCurrentWindow()->FontWindowScale;
            if (oldFontWindowScale==0.f) {
                oldFontWindowScale = currentFontWindowScale;
                nodesHaveZeroSize = true;   // at start or after clear()
                scrolling = ImGui::GetWindowSize()*.5f;
            }
            else if (oldFontWindowScale!=currentFontWindowScale) {
                nodesHaveZeroSize = true;
                for (int i=0,isz=nodes.size();i<isz;i++)    {
                    Node* node = nodes[i];
                    node->Size = ImVec2(0,0);   // we must reset the size
                }
                // These two lines makes the scaling work around the mouse position AFAICS
                if (io.FontAllowUserScaling)	{
                    const ImVec2 delta = (io.MousePos-ImGui::GetCursorScreenPos());//-ImGui::GetWindowSize()*.5f));
                    scrolling+=(delta*currentFontWindowScale-delta*oldFontWindowScale)/currentFontWindowScale;
                    /*ImGuiWindow* window = ImGui::GetCurrentWindow();
        float scale = currentFontWindowScale / oldFontWindowScale;
        ImVec2 oldWindowSize = window->Size/scale;
        const ImVec2 offset = window->Size * (1.0f - scale) * (io.MousePos - window->Pos) / window->Size;
        */
                    //------------------------------------------------------------------------
                }
                oldFontWindowScale = currentFontWindowScale;
                maxConnectorNameWidth = 0.f;
            }

            const float NODE_SLOT_RADIUS = style.node_slots_radius*currentFontWindowScale;
            const float NODE_SLOT_RADIUS_SQUARED = (NODE_SLOT_RADIUS*NODE_SLOT_RADIUS);
            const ImVec2& NODE_WINDOW_PADDING = style.node_window_padding;
            const float MOUSE_DELTA_SQUARED = io.MouseDelta.x*io.MouseDelta.x+io.MouseDelta.y*io.MouseDelta.y;
            const float MOUSE_DELTA_SQUARED_THRESHOLD = NODE_SLOT_RADIUS_SQUARED * 0.05f;    // We don't detect "mouse release" events while dragging links onto slots. Instead we check that our mouse delta is small enough. Otherwise we couldn't hover other slots while dragging links.

            const float baseNodeWidth = nodesBaseWidth*currentFontWindowScale;
            float currentNodeWidth = baseNodeWidth;
            ImGui::PushItemWidth(currentNodeWidth);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->ChannelsSplit(5);

            ImVec2 canvasSize = ImGui::GetWindowSize();
            ImVec2 win_pos = ImGui::GetCursorScreenPos();
            if (node_to_center_view_around && !nodesHaveZeroSize) {scrolling = node_to_center_view_around->GetPos(currentFontWindowScale)+node_to_center_view_around->Size*0.5f;node_to_center_view_around = NULL;}
            ImVec2 effectiveScrolling = scrolling - canvasSize*.5f;
            ImVec2 offset = ImGui::GetCursorScreenPos() - effectiveScrolling;
            ImVec2 offset2 = ImGui::GetCursorPos() - effectiveScrolling;//scrolling;



            // Display grid
            if (show_grid)
            {
                const ImU32& GRID_COLOR = style.color_grid;
                const float& GRID_SZ = currentFontWindowScale * style.grid_size;
                const float grid_Line_width = currentFontWindowScale * style.grid_line_width;
                for (float x = fmodf(offset2.x,GRID_SZ); x < canvasSize.x; x += GRID_SZ)
                    draw_list->AddLine(ImVec2(x,0.0f)+win_pos, ImVec2(x,canvasSize.y)+win_pos, GRID_COLOR,grid_Line_width);
                for (float y = fmodf(offset2.y,GRID_SZ); y < canvasSize.y; y += GRID_SZ)
                    draw_list->AddLine(ImVec2(0.0f,y)+win_pos, ImVec2(canvasSize.x,y)+win_pos, GRID_COLOR,grid_Line_width);
            }

            const ImVec2 link_cp(style.link_control_point_distance * currentFontWindowScale,0); // Bezier control point of the links
            const float link_line_width = style.link_line_width * currentFontWindowScale;

            ImVec2& mouseRectangularSelectionForNodesMin = rectangularSelectionData.mouseRectangularSelectionForNodesMin;
            ImVec2& mouseRectangularSelectionForNodesMax = rectangularSelectionData.mouseRectangularSelectionForNodesMax;
            bool& mouseRectangularSelectionForNodesStarted = rectangularSelectionData.mouseRectangularSelectionForNodesStarted;


            // Clipping Data (used to cull nodes and/or links)
            const bool enableNodeCulling = true;const bool enableLinkCulling = true;    // Tweakables
            int numberOfCulledNodes = 0, numberOfCulledLinks = 0;
            ImRect windowClipRect,linkClipRect;
            if (enableNodeCulling || enableLinkCulling) {
                ImVec2 windowClipRect0(win_pos.x-offset.x,win_pos.y-offset.y);
                ImVec2 windowClipRect1 = windowClipRect0 + canvasSize;
                if (enableLinkCulling) linkClipRect = ImRect(windowClipRect0+offset-link_cp-ImVec2(0,link_line_width),windowClipRect1+offset+link_cp+ImVec2(0,link_line_width));   // used to clip links
                if (enableNodeCulling)  {
                    const float windowClipHalfExtraWidth = NODE_SLOT_RADIUS + (show_connection_names ? maxConnectorNameWidth : 0.f);  // Otherwise node names are culled too early
                    windowClipRect0.x-=     windowClipHalfExtraWidth;
                    windowClipRect1.x+=     windowClipHalfExtraWidth;
                    windowClipRect = ImRect(windowClipRect0,windowClipRect1);   // used to clip nodes (= windows)
                }
            }
            // End Clipping Data

            const bool isMouseHoveringWindow = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

            const bool isLMBClicked = ImGui::IsMouseClicked(0);
            const bool isLMBDoubleClicked = ImGui::IsMouseDoubleClicked(0);
            const bool isMouseDraggingForMovingNodes = isMouseHoveringWindow && ImGui::IsMouseDragging(0, 8.0f);	// This is not enough for a node to be actually moved

            // Display links variable (their display code have been moved down)
            const bool cantDragAnything = isMouseDraggingForScrolling || mouseRectangularSelectionForNodesStarted;
            bool isLMBDraggingForMakingLinks = isMouseHoveringWindow && !cantDragAnything && ImGui::IsMouseDragging(0, 0.0f);
            bool isDragNodeValid = dragNode.isValid();
            const bool mustCheckForNearestLink = isMouseHoveringWindow && !isLMBDraggingForMakingLinks && io.KeyShift;


            // Display nodes
            //ImGui::PushStyleColor(ImGuiCol_Header,transparent);ImGui::PushStyleColor(ImGuiCol_HeaderActive,transparent);ImGui::PushStyleColor(ImGuiCol_HeaderHovered,transparent);    // moved inside the loop to wrap the ImGui::TreeNode()
            bool isSomeNodeMoving = false;bool mustDeleteANodeSoon = false;
            //ImGui::ColorEditMode(colorEditMode);

            const float nodeTitleBarBgHeight = ImGui::GetTextLineHeightWithSpacing() + NODE_WINDOW_PADDING.y;
            Node* nodeThatIsBeingEditing = NULL;
            const float textSizeButtonPaste = ImGui::CalcTextSize(NodeGraphEditor::CloseCopyPasteChars[0]).x;
            const float textSizeButtonCopy = ImGui::CalcTextSize(NodeGraphEditor::CloseCopyPasteChars[1]).x;
            const float textSizeButtonX = ImGui::CalcTextSize(NodeGraphEditor::CloseCopyPasteChars[2]).x;
            int activeNodeIndex = -1;
            const ImGuiStyle& mainStyle = ImGui::GetStyle();
            const ImVec4& defaultTextColor = mainStyle.Colors[ImGuiCol_Text];
            for (int node_idx = 0; node_idx < nodes.Size; node_idx++)
            {
                Node* node = nodes[node_idx];
                const ImVec2 nodePos = node->GetPos(currentFontWindowScale);
                if (activeNode==node) activeNodeIndex = node_idx;

                // culling attempt
                if (enableNodeCulling && !nodesHaveZeroSize) {
                    const ImRect cullRect(nodePos,nodePos+node->Size);
                    if (!windowClipRect.Overlaps(cullRect)) {
                        ++numberOfCulledNodes;
                        continue;
                    }
                }

                if (node->baseWidthOverride>0) {
                    currentNodeWidth = node->baseWidthOverride*currentFontWindowScale;
                    ImGui::PushItemWidth(currentNodeWidth);
                }
                else currentNodeWidth = baseNodeWidth;

                ImGui::PushID((const void*) node);
                ImVec2 node_rect_min = offset + nodePos;

                // Display node contents first
                draw_list->ChannelsSetCurrent(activeNodeIndex==node_idx ? 4 : 2); // Foreground
                bool old_any_active = ImGui::IsAnyItemActive();
                ImGui::SetCursorScreenPos(node_rect_min + NODE_WINDOW_PADDING);

                bool nodeInEditMode = false;
                ImGui::BeginGroup(); // Lock horizontal position
                ImGui::SetNextTreeNodeOpen(node->isOpen,ImGuiCond_Always);

                ImU32 titleTextColorU32 = 0, titleBgColorU32 = 0;float titleBgGradient = -1.f;
                node->getDefaultTitleBarColors(titleTextColorU32,titleBgColorU32,titleBgGradient);

                ImVec4 titleTextColor = node->overrideTitleTextColor ? ImGui::ColorConvertU32ToFloat4(node->overrideTitleTextColor) : titleTextColorU32 ? ImGui::ColorConvertU32ToFloat4(titleTextColorU32) : style.color_node_title;
                if (titleTextColor.w==0.f) titleTextColor = defaultTextColor;
                ImGui::PushStyleColor(ImGuiCol_Text,titleTextColor);    // titleTextColor (these 3 lines can be moved down to leave the TreeNode triangle always 'defaultTextColor')

                ImGui::PushStyleColor(ImGuiCol_Header,transparent);ImGui::PushStyleColor(ImGuiCol_HeaderActive,transparent);ImGui::PushStyleColor(ImGuiCol_HeaderHovered,transparent);    // Moved from outside loop
                if (ImGui::TreeNode(node,"%s","")) {ImGui::TreePop();node->isOpen = true;}
                else node->isOpen = false;
                ImGui::PopStyleColor(3);   // Moved from outside loop
                ImGui::SameLine(0,2);

                // titleTextColor: the 3 lines above can be moved here  to leave the TreeNode triangle always 'defaultTextColor'
                static char NewNodeName[IMGUINODE_MAX_NAME_LENGTH]="";
                static bool mustStartEditingNodeName = false;
                if (!node->isInEditingMode) {
                    ImGui::Text("%s",node->Name);
                    if (ImGui::IsItemHovered()) {
                        const char* tooltip = node->getTooltip();
			if (tooltip && tooltip[0]!='\0') {
			    ImGuiStyle& igStyle = ImGui::GetStyle();
			    igStyle.WindowPadding.x = igStyle.WindowPadding.y =4.f;
			    ImGui::SetTooltip("%s",tooltip);
			    igStyle.WindowPadding.x = igStyle.WindowPadding.y =0.f;
			}
                        if (isLMBDoubleClicked) {
                            nodeThatIsBeingEditing = node;
                            node->isInEditingMode = mustStartEditingNodeName = true;
                            strcpy(&NewNodeName[0],node->Name);
                        }
                    }
                }
                else {
                    if (mustStartEditingNodeName) {ImGui::SetKeyboardFocusHere();}
                    if (ImGui::InputText("###imguiNodeGraphEditorNodeRename",NewNodeName,IMGUINODE_MAX_NAME_LENGTH,ImGuiInputTextFlags_EnterReturnsTrue))   {
                        if (NewNodeName[0]!='\0' && strcmp(node->Name,NewNodeName)!=0) overrideNodeName(node,NewNodeName);
                        node->isInEditingMode = mustStartEditingNodeName = false;
                        nodeThatIsBeingEditing = NULL;
                    }
                    else if (!mustStartEditingNodeName && !ImGui::IsItemActive()) {
                        node->isInEditingMode = mustStartEditingNodeName = false;
                    }
                    mustStartEditingNodeName = false;
                }
                ImGui::PopStyleColor();                                 // titleTextColor

                // Note: if node->isOpen, we'll draw the buttons later, because we need node->Size that is not known
                // BUTTONS ========================================================
                const bool canPaste = sourceCopyNode && sourceCopyNode->typeID==node->typeID;
                const bool canCopy = node->canBeCopied();
                static const ImVec4 transparentColor(1,1,1,0);
                const ImVec2 nodeTitleBarButtonsStartCursor = node->isOpen ? ImGui::GetCursorPos() : ImVec2(0,0);
                if (!node->isOpen && !node->isInEditingMode)    {
                    ImGui::SameLine();
                    //== Actual code to draw buttons (same code is copied below) =====================
                    ImGui::PushStyleColor(ImGuiCol_Button,transparentColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.75,0.75,0.75,0.5));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.75,0.75,0.75,0.77));
                    ImGui::PushStyleColor(ImGuiCol_Text,titleTextColor);
                    ImGui::PushID("NodeButtons");
                    if (show_node_copy_paste_buttons)   {
                        static const ImVec2 vec2zero(0,0);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,vec2zero);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing,vec2zero);
                        if (canPaste) {
                            if (ImGui::SmallButton(NodeGraphEditor::CloseCopyPasteChars[2])) {
                                node_to_paste_from_copy_source = node_hovered_in_scene = node;
                            }
			    if (ImGui::IsItemHovered()) NodeGraphEditorSetTooltip("Paste");
                            ImGui::SameLine(0);
                        }
                        if (canCopy)	{
                            if (ImGui::SmallButton(NodeGraphEditor::CloseCopyPasteChars[1])) {
                                node_hovered_in_scene = node;
                                copyNode(node);
                            }
			    if (ImGui::IsItemHovered()) NodeGraphEditorSetTooltip("Copy");
                            ImGui::SameLine(0);
                        }
                    }
                    if (ImGui::SmallButton(NodeGraphEditor::CloseCopyPasteChars[0])) {
                        node_hovered_in_scene = node;
                        if (!hasLinks(node))  {menuNode = node;mustDeleteANodeSoon=true;}
                        else {
                            menuNode = node;
                            open_delete_only_context_menu = true;  // will ask to delete node later
                        }
                    }
		    if (ImGui::IsItemHovered()) NodeGraphEditorSetTooltip("Delete");
		    if (show_node_copy_paste_buttons) ImGui::PopStyleVar(2);
                    ImGui::PopID();
                    ImGui::PopStyleColor(4);
                    //== End actual code to draw buttons (same code is copied below) ====================
                }
                //=================================================================

                ImGui::Spacing();

                if (node->isOpen)
                {
                    if (!node->isInEditingMode) ImGui::Spacing();
                    // ------------------------------------------------------------------
                    //ImGui::BeginGroup();
                    // this code goes into a virtual method==============================
                    nodeInEditMode|=node->render(currentNodeWidth);
                    //===================================================================
                    //ImGui::EndGroup();
                    //-------------------------------------------------------------------
                    isLMBDraggingForMakingLinks&=!nodeInEditMode;   // Don't create links while dragging the mouse to edit node values
                }
                ImGui::EndGroup();
                if (nodeInEditMode) node->startEditingTime = -1.f;
                else if (node->startEditingTime!=0.f) {
                    //if (nodeCallback)   {
                    if (node->startEditingTime<0) node->startEditingTime = ImGui::GetTime();
                    else if (ImGui::GetTime()-node->startEditingTime>nodeEditedTimeThreshold) {
                        node->startEditingTime = 0.f;
                        node_to_fire_edit_callback = node;
                    }
                    //}
                    //else node->startEditingTime = 0.f;
                }

                // Save the size of what we have emitted and whether any of the widgets are being used
                bool node_widgets_active = (!old_any_active && ImGui::IsAnyItemActive());
                node->Size = ImGui::GetItemRectSize() + NODE_WINDOW_PADDING + NODE_WINDOW_PADDING;
                ImVec2 node_rect_max = node_rect_min + node->Size;

                // Go backwards and display title bar buttons if node->isOpen
                // BUTTONS ========================================================
                if (node->Size.x!=0 && !node->isInEditingMode)    {
                    const ImVec2 cursorPosToRestore = ImGui::GetCursorPos();
                    ImGui::SetCursorPos(nodeTitleBarButtonsStartCursor);
                    //--------------------------------------------
                    ImGui::SameLine(-effectiveScrolling.x+nodePos.x+node->Size.x-textSizeButtonX-10
                                    -(show_node_copy_paste_buttons ?
                                          (
                                              (canCopy?(textSizeButtonCopy+2):0) +
                                              (canPaste?(textSizeButtonPaste+2):0)
                                              )
                                        : 0)
                                    ,0);
                    //== Actual code to draw buttons (same code is copied below) =====================
                    ImGui::PushStyleColor(ImGuiCol_Button,transparentColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.75,0.75,0.75,0.5));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.75,0.75,0.75,0.77));
                    ImGui::PushStyleColor(ImGuiCol_Text,titleTextColor);
                    ImGui::PushID("NodeButtons");
                    if (show_node_copy_paste_buttons)   {
                        static const ImVec2 vec2zero(0,0);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,vec2zero);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing,vec2zero);
                        if (canPaste) {
                            if (ImGui::SmallButton(NodeGraphEditor::CloseCopyPasteChars[2])) {
                                node_to_paste_from_copy_source = node_hovered_in_scene = node;
                            }
			    if (ImGui::IsItemHovered()) NodeGraphEditorSetTooltip("Paste");
                            ImGui::SameLine(0);
                        }
                        if (canCopy)	{
                            if (ImGui::SmallButton(NodeGraphEditor::CloseCopyPasteChars[1])) {
                                node_hovered_in_scene = node;
                                copyNode(node);
                            }
			    if (ImGui::IsItemHovered()) NodeGraphEditorSetTooltip("Copy");
                            ImGui::SameLine(0);
                        }
                    }
                    if (ImGui::SmallButton(NodeGraphEditor::CloseCopyPasteChars[0])) {
                        node_hovered_in_scene = node;
                        if (!hasLinks(node))  {menuNode = node;mustDeleteANodeSoon=true;}
                        else {
                            menuNode = node;
                            open_delete_only_context_menu = true;  // will ask to delete node later
                        }
                    }
		    if (ImGui::IsItemHovered()) NodeGraphEditorSetTooltip("Delete");
                    if (show_node_copy_paste_buttons) ImGui::PopStyleVar(2);
                    ImGui::PopID();
                    ImGui::PopStyleColor(4);
                    //== End actual code to draw buttons (same code is copied below) ====================
                    //-----------------------------------------------------------
                    ImGui::SetCursorPos(cursorPosToRestore);
                }
                //=================================================================


                // Display node box
                draw_list->ChannelsSetCurrent(activeNodeIndex == node_idx ? 3 : 1); // Background
                ImGui::SetCursorScreenPos(node_rect_min);
                ImGui::InvisibleButton("node##nodeinvbtn", node->Size);
                if (ImGui::IsItemHovered()) {
                    if (!node_hovered_in_scene) {
                        node_hovered_in_scene = node;
                        menuNode = node;
                    }
                    if (ImGui::IsMouseClicked(1))   {
                        //menuNode = node;
                        open_context_menu=true;
                    }
                }
                bool node_moving_active = !isMouseDraggingForScrolling && !nodeInEditMode && ImGui::IsItemActive() && !mouseRectangularSelectionForNodesStarted;

                // Handle selected and active states
                if (isLMBClicked && (node_widgets_active || node_moving_active))  {
                    if (!node->isSelected) {
                        if (!io.KeyCtrl) unselectAllNodes();
                        node->isSelected = true;activeNode = node;activeNodeIndex=node_idx;
                    }
                    else if (io.KeyCtrl) {
                        //unselectNode(node); // We must unwrap it, so we can store th new 'activeNodeIndex' for later use
                        node->isSelected=false;
                        if (node==activeNode) {activeNode = NULL;activeNodeIndex=findANewActiveNode();}
                    }
                    else if (io.KeyShift || (isLMBDoubleClicked && !node->isInEditingMode)/*|| io.MouseDelta.x*io.MouseDelta.x+io.MouseDelta.y*io.MouseDelta.y < 0.000000001f*/)    // Good idea to enable the correct behavior without pressing SHIFT, but the latter condition is almost always fullfilled...
                    {
                        unselectAllNodes();
                        node->isSelected = true;activeNode = node;activeNodeIndex=node_idx;
                    }
                    else {
                        activeNode = node;activeNodeIndex=node_idx;
                    }
                }

                // Perform selected nodes move
                if (node_moving_active && node->isSelected && !isDragNodeValid && isMouseDraggingForMovingNodes) {
                    const ImVec2 mouseDeltaPos = io.MouseDelta/currentFontWindowScale;
                    for (int j=0,jsz=nodes.size();j<jsz;j++) {
                        Node* jn = nodes[j];
                        if (jn->isSelected) jn->Pos = jn->Pos + mouseDeltaPos;
                    }
                    isSomeNodeMoving=true;
                }

                const ImU32& node_bg_color = (node_hovered_in_list == node || node_hovered_in_scene == node) ? style.color_node_hovered :
                                                                                                               (activeNode == node ? style.color_node_active :
                                                                                                                                     (node->isSelected ? style.color_node_selected :
                                                                                                                                                         style.color_node));

                draw_list->AddRectFilled(node_rect_min, node_rect_max, node_bg_color, style.node_rounding); // Bg

                // Node Title Bg Color
                const ImU32 nodeTitleBgColor = node->overrideTitleBgColor ? node->overrideTitleBgColor : titleBgColorU32 ? titleBgColorU32 : style.color_node_title_background;
                if ((nodeTitleBgColor&IM_COL32_A_MASK)!=0) {
                    //#           define SKIP_VERTICAL_GRADIENT
#                   ifndef SKIP_VERTICAL_GRADIENT
                    float fillGradientFactor = node->overrideTitleBgColorGradient>=0.f ? node->overrideTitleBgColorGradient : titleBgGradient>=0.f ? titleBgGradient : style.color_node_title_background_gradient;//0.15f;
                    if (node->isSelected) fillGradientFactor = -fillGradientFactor; // or if (node==activeNode)
                    if (fillGradientFactor!=0.f)    {
                        if (node->isOpen) ImGui::NGE_Draw::ImDrawListAddRectWithVerticalGradient(draw_list,node_rect_min, ImVec2(node_rect_max.x,node_rect_min.y+nodeTitleBarBgHeight), nodeTitleBgColor,fillGradientFactor,IM_COL32_BLACK_TRANS,style.node_rounding,1|2);
                        else ImGui::NGE_Draw::ImDrawListAddRectWithVerticalGradient(draw_list,node_rect_min, node_rect_max, nodeTitleBgColor,fillGradientFactor,IM_COL32_BLACK_TRANS, style.node_rounding);
                    }
                    else {
                        if (node->isOpen) draw_list->AddRectFilled(node_rect_min, ImVec2(node_rect_max.x,node_rect_min.y+nodeTitleBarBgHeight), nodeTitleBgColor, style.node_rounding,1|2);
                        else draw_list->AddRectFilled(node_rect_min, node_rect_max, nodeTitleBgColor, style.node_rounding);
                    }
#                   else // SKIP_VERTICAL_GRADIENT
                    if (node->isOpen) draw_list->AddRectFilled(node_rect_min, ImVec2(node_rect_max.x,node_rect_min.y+nodeTitleBarBgHeight), nodeTitleBgColor, style.node_rounding,1|2);
                    else draw_list->AddRectFilled(node_rect_min, node_rect_max, nodeTitleBgColor, style.node_rounding);
#                   undef SKIP_VERTICAL_GRADIENT
#                   endif // SKIP_VERTICAL_GRADIENT
                }


                // Display Frame (this can be moved to the and of the foreground draw channel if we like)---------------------------
                const ImU32& node_frame_color = (activeNode == node) ? style.color_node_frame_active :
                                                                       (node->isSelected ? style.color_node_frame_selected :
                                                                                           ((node_hovered_in_list == node || node_hovered_in_scene == node) ? style.color_node_frame_hovered :
                                                                                                                                                              style.color_node_frame));
                const float lineThickness = ((activeNode == node) ? 3.0f : (node->isSelected ? 2.0f : 1.f))*currentFontWindowScale;
                draw_list->AddRect(node_rect_min, node_rect_max, node_frame_color, style.node_rounding,0x0F,lineThickness);    // Frame

                // Line below node name
                if (node->isOpen) {
                    ImVec2 tmp1(node_rect_min.x,node_rect_min.y+nodeTitleBarBgHeight+1);
                    ImVec2 tmp2(node_rect_max.x,tmp1.y);
                    draw_list->AddLine(tmp1,tmp2,node_frame_color,lineThickness);
                }
                //-----------------------------------------------------------------------------------------------------------------


                // Display connectors
                const ImVec2 oldCursorScreenPos = ImGui::GetCursorScreenPos();
                const ImVec2 mouseScreenPos = io.MousePos;;
                ImVec2 connectorScreenPos,deltaPos;const bool canDeleteLinks = true;
                const bool mustDeleteLinkIfSlotIsHovered = canDeleteLinks && io.MouseDoubleClicked[0];
                const bool mustDetectIfSlotIsHoveredForDragNDrop = !cantDragAnything && !isSomeNodeMoving && (!isDragNodeValid || isLMBDraggingForMakingLinks);
                ImGui::PushStyleColor(ImGuiCol_Text,style.color_node_input_slots_names);
                const float connectorBorderThickness = NODE_SLOT_RADIUS*0.25f; // lineThickness = ((activeNode == node) ? 3.0f : (node->isSelected ? 2.0f : 1.f))*currentFontWindowScale;
                ImVec2 connectorNameSize(0,0);
                for (int slot_idx = 0; slot_idx < node->InputsCount; slot_idx++)    {
                    connectorScreenPos = offset + node->GetInputSlotPos(slot_idx,currentFontWindowScale);
                    //draw_list->AddCircleFilled(connectorScreenPos, NODE_SLOT_RADIUS, style.color_node_input_slots,connectorNumSegments);
                    ImGui::NGE_Draw::ImDrawListAddCircle(draw_list,connectorScreenPos,NODE_SLOT_RADIUS,style.color_node_input_slots,style.color_node_input_slots_border,style.node_slots_num_segments,connectorBorderThickness);
                    /*if ((style.color_node_input_slots & IM_COL32_A_MASK) != 0)  {
                const float a_max = IM_PI * 0.5f * 11.f/12.f;
                draw_list->PathArcTo(connectorScreenPos, NODE_SLOT_RADIUS, IM_PI-a_max, IM_PI+a_max, 12);
                draw_list->PathFill(style.color_node_input_slots);
            }*/
                    if (show_connection_names && node->InputNames[slot_idx][0]!='\0')   {
                        const char* name = node->InputNames[slot_idx];
                        if (name)   {
                            connectorNameSize = ImGui::CalcTextSize(name);
                            if (maxConnectorNameWidth<connectorNameSize.x) maxConnectorNameWidth = connectorNameSize.x;
                            ImGui::SetCursorScreenPos(offset + node->GetInputSlotPos(slot_idx,currentFontWindowScale)-ImVec2(NODE_SLOT_RADIUS,0)-connectorNameSize);
                            ImGui::Text("%s",name);
                        }
                    }
                    if (mustDetectIfSlotIsHoveredForDragNDrop || mustDeleteLinkIfSlotIsHovered)    {
                        deltaPos.x = mouseScreenPos.x-connectorScreenPos.x;
                        deltaPos.y = mouseScreenPos.y-connectorScreenPos.y;
                        if ((deltaPos.x*deltaPos.x)+(deltaPos.y*deltaPos.y)<NODE_SLOT_RADIUS_SQUARED)   {
                            if (mustDeleteLinkIfSlotIsHovered)  {
                                // remove the link
                                //printf("To be removed: input slot %d.\n",slot_idx);fflush(stdout);
                                for (int link_idx=0;link_idx<links.size();link_idx++)   {
                                    NodeLink& link = links[link_idx];
                                    if (link.OutputNode == node && slot_idx == link.OutputSlot)   {
                                        if (linkCallback) linkCallback(link,LS_DELETED,*this);
                                        // remove link
                                        if (link_idx+1 < links.size()) link = links[links.size()-1];    // swap with the last link
                                        links.resize(links.size()-1);
                                        --link_idx;
                                    }
                                }
                            }
                            else if (isLMBDraggingForMakingLinks && !isDragNodeValid) {
                                dragNode.node = node;
                                dragNode.outputSlotIdx = slot_idx;
                                dragNode.inputSlotIdx = -1;
                                dragNode.pos = mouseScreenPos;
                                //printf("Start dragging.\n");fflush(stdout);
                            }
                            else if (isDragNodeValid && dragNode.node!=node
                                     && MOUSE_DELTA_SQUARED<MOUSE_DELTA_SQUARED_THRESHOLD   // optional... what I wanted is not to end a connection just when I hover another node...
                                     ) {
                                // verify compatibility
                                if (dragNode.inputSlotIdx!=-1)  {
                                    // drag goes from the output (dragNode.inputSlotIdx) slot of dragNode.node to the input slot of 'node':
                                    if (!avoidCircularLinkLoopsInOut || !isNodeReachableFrom(dragNode.node,true,node))  {
                                        if (allowOnlyOneLinkPerInputSlot)   {
                                            // Remove all existing node links to node-slot_idx before adding new link:
                                            for (int link_idx=0;link_idx<links.size();link_idx++)   {
                                                NodeLink& link = links[link_idx];
                                                if (link.OutputNode == node && slot_idx == link.OutputSlot)   {
                                                    if (linkCallback) linkCallback(link,LS_DELETED,*this);
                                                    // remove link
                                                    if (link_idx+1 < links.size()) link = links[links.size()-1];    // swap with the last link
                                                    links.resize(links.size()-1);
                                                    --link_idx;
                                                }
                                            }
                                        }
                                        // create link
                                        addLink(dragNode.node,dragNode.inputSlotIdx,node,slot_idx,true);
                                    }
                                    // clear dragNode
                                    dragNode.node = NULL;
                                    dragNode.outputSlotIdx = dragNode.inputSlotIdx = -1;
                                    //printf("End dragging.\n");fflush(stdout);
                                }
                            }
                        }
                    }
                }
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text,style.color_node_output_slots_names);
                for (int slot_idx = 0; slot_idx < node->OutputsCount; slot_idx++)   {
                    connectorScreenPos = offset + node->GetOutputSlotPos(slot_idx,currentFontWindowScale);
                    //draw_list->AddCircleFilled(connectorScreenPos, NODE_SLOT_RADIUS, style.color_node_output_slots,connectorNumSegments);
                    ImGui::NGE_Draw::ImDrawListAddCircle(draw_list,connectorScreenPos,NODE_SLOT_RADIUS,style.color_node_output_slots,style.color_node_output_slots_border,style.node_slots_num_segments,connectorBorderThickness);
                    /*if ((style.color_node_output_slots & IM_COL32_A_MASK) != 0)  {
                const float a_max = IM_PI * 0.5f * 11.f/12.f;
                draw_list->PathArcTo(connectorScreenPos, NODE_SLOT_RADIUS, -a_max, a_max, 12);
                draw_list->PathFill(style.color_node_output_slots);
            }*/
                    if (show_connection_names && node->OutputNames[slot_idx][0]!='\0')   {
                        const char* name = node->OutputNames[slot_idx];
                        if (name)   {
                            connectorNameSize = ImGui::CalcTextSize(name);
                            if (maxConnectorNameWidth<connectorNameSize.x) maxConnectorNameWidth = connectorNameSize.x;
                            ImGui::SetCursorScreenPos(offset + node->GetOutputSlotPos(slot_idx,currentFontWindowScale)+ImVec2(NODE_SLOT_RADIUS,0)-ImVec2(0,connectorNameSize.y));
                            ImGui::Text("%s",name);
                        }
                    }
                    if (mustDetectIfSlotIsHoveredForDragNDrop || mustDeleteLinkIfSlotIsHovered)    {
                        deltaPos.x = mouseScreenPos.x-connectorScreenPos.x;
                        deltaPos.y = mouseScreenPos.y-connectorScreenPos.y;
                        if ((deltaPos.x*deltaPos.x)+(deltaPos.y*deltaPos.y)<NODE_SLOT_RADIUS_SQUARED)   {
                            if (mustDeleteLinkIfSlotIsHovered)  {
                                // remove the link
                                //printf("To be removed: output slot %d.\n",slot_idx);fflush(stdout);
                                for (int link_idx=0;link_idx<links.size();link_idx++)   {
                                    NodeLink& link = links[link_idx];
                                    if (link.InputNode == node && slot_idx == link.InputSlot)   {
                                        if (linkCallback) linkCallback(link,LS_DELETED,*this);
                                        // remove link
                                        if (link_idx+1 < links.size()) link = links[links.size()-1];    // swap with the last link
                                        links.resize(links.size()-1);
                                        --link_idx;
                                    }
                                }
                            }
                            else if (isLMBDraggingForMakingLinks && !isDragNodeValid) {
                                dragNode.node = node;
                                dragNode.inputSlotIdx = slot_idx;
                                dragNode.outputSlotIdx = -1;
                                dragNode.pos = mouseScreenPos;
                                //printf("Start dragging.\n");fflush(stdout);
                            }
                            else if (isDragNodeValid && dragNode.node!=node
                                     && MOUSE_DELTA_SQUARED<MOUSE_DELTA_SQUARED_THRESHOLD    // optional... what I wanted is not to end a connection just when I hover another node...
                                     ) {
                                // verify compatibility
                                if (dragNode.outputSlotIdx!=-1)  {
                                    // drag goes from the output slot_idx of node to the input slot (dragNode.outputSlotIdx) of dragNode.node:
                                    if (!avoidCircularLinkLoopsInOut || !isNodeReachableFrom(node,true,dragNode.node))    {
                                        if (allowOnlyOneLinkPerInputSlot)   {
                                            // Remove all existing node links to dragNode before adding new link:
                                            for (int link_idx=0;link_idx<links.size();link_idx++)   {
                                                NodeLink& link = links[link_idx];
                                                if (link.OutputNode == dragNode.node && dragNode.outputSlotIdx == link.OutputSlot)   {
                                                    if (linkCallback) linkCallback(link,LS_DELETED,*this);
                                                    // remove link
                                                    if (link_idx+1 < links.size()) link = links[links.size()-1];    // swap with the last link
                                                    links.resize(links.size()-1);
                                                    --link_idx;
                                                }
                                            }
                                        }
                                        // create link
                                        addLink(node,slot_idx,dragNode.node,dragNode.outputSlotIdx,true);
                                    }
                                    // clear dragNode
                                    dragNode.node = NULL;
                                    dragNode.outputSlotIdx = dragNode.inputSlotIdx = -1;
                                    //printf("End dragging.\n");fflush(stdout);
                                }
                            }
                        }
                    }
                }
                ImGui::PopStyleColor();
                if (!isLMBDraggingForMakingLinks) {
                    dragNode.node = NULL; // clear dragNode
                    //printf("Quit dragging.\n");fflush(stdout);
                }
                ImGui::SetCursorScreenPos(oldCursorScreenPos);



                ImGui::PopID();
                if (node->baseWidthOverride>0) ImGui::PopItemWidth();
            }
            //ImGui::PopStyleColor(3);      // moved inside the loop to wrap the ImGui::TreeNode()

            // Display Links
            draw_list->ChannelsSetCurrent(0); // Deeper than Background
            {
                ImRect cullLink;ImVec2 p1,p2,cp1,cp2;
                int nearestLinkId=-1;
                const float hoveredLinkDistSqrThres = 100.0f * currentFontWindowScale;
                for (int link_idx = 0; link_idx < links.Size; link_idx++)
                {
                    NodeLink& link = links[link_idx];
                    Node* node_inp = link.InputNode;
                    Node* node_out = link.OutputNode;
                    p1 = offset + node_inp->GetOutputSlotPos(link.InputSlot,currentFontWindowScale);
                    p2 = offset + node_out->GetInputSlotPos(link.OutputSlot,currentFontWindowScale);
                    if (enableLinkCulling) {
                        cullLink.Min=cullLink.Max=p1;cullLink.Add(p2);cullLink.Expand(link_line_width*2.f);
                        if (!linkClipRect.Overlaps(cullLink)) {
                            ++numberOfCulledLinks;
                            continue;
                        }
                    }
                    cp1 = p1+link_cp;
                    cp2 = p2-link_cp;

                    // highlight nearest link
                    if (mustCheckForNearestLink && nearestLinkId==-1 && (enableLinkCulling ? cullLink.Contains(io.MousePos) : true)) {
                        const float distanceSquared = GetSquaredDistanceToBezierCurve(io.MousePos,p1,cp1, cp2,p2);
                        if (distanceSquared<hoveredLinkDistSqrThres) nearestLinkId=link_idx;
                        // dbg line:
                        //if (io.MouseDelta.x!=0.f || io.MouseDelta.y!=0.f)   fprintf(stderr,"%d) MP{%1.0f,%1.0f} p1{%1.0f,%1.0f} p2{%1.0f,%1.0f} distanceSquared=%1.4f hoveredLinkDistSqrThres=%1.4f\n",link_idx,io.MousePos.x,io.MousePos.y,p1.x,p1.y,p2.x,p2.y,distanceSquared,hoveredLinkDistSqrThres);
                    }

                    draw_list->AddBezierCurve(p1,cp1,cp2,p2,style.color_link,(nearestLinkId!=link_idx) ? link_line_width : (link_line_width*2.f), style.link_num_segments);
                }
                if (nearestLinkId!=-1 && io.MouseReleased[0]) {
                    //fprintf(stderr,"Removing link at: %d\n",nearestLinkId);
                    removeLinkAt(nearestLinkId);
                    nearestLinkId=-1;
                }
            }
            // Display dragging link
            if (isLMBDraggingForMakingLinks && isDragNodeValid)   {
                if (dragNode.inputSlotIdx!=-1)  {   // Dragging from the output slot of dragNode
                    ImVec2 p1 = offset + dragNode.node->GetOutputSlotPos(dragNode.inputSlotIdx,currentFontWindowScale);
                    const ImVec2& p2 = io.MousePos;//offset + node_out->GetInputSlotPos(link.OutputSlot);
                    draw_list->AddBezierCurve(p1, p1+link_cp, p2-link_cp, p2, style.color_link, link_line_width, style.link_num_segments);
                }
                else if (dragNode.outputSlotIdx!=-1)  {  // Dragging from the input slot of dragNode
                    const ImVec2& p1 = io.MousePos;//
                    ImVec2 p2 = offset + dragNode.node->GetInputSlotPos(dragNode.outputSlotIdx,currentFontWindowScale);
                    draw_list->AddBezierCurve(p1, p1+link_cp, p2-link_cp, p2, style.color_link, link_line_width, style.link_num_segments);
                }
            }


            draw_list->ChannelsMerge();

            //#   define DEBUG_NODE_CULLING     // Debug works for a single Node Graph Editor present only
#   ifdef DEBUG_NODE_CULLING
            static int lastNumberOfCulledNodes=0;
            if (enableNodeCulling && numberOfCulledNodes!=lastNumberOfCulledNodes) {
                lastNumberOfCulledNodes = numberOfCulledNodes;
                fprintf(stderr,"numberOfCulledNodes: %d/%d  numberOfVisibleNodes: %d/%d\n",numberOfCulledNodes,nodes.Size,nodes.Size-numberOfCulledNodes,nodes.Size);
            }
#   undef DEBUG_NODE_CULLING
#   endif //DEBUG_NODE_CULLING

            //#   define DEBUG_LINK_CULLING     // Debug works for a single Node Graph Editor present only
#   ifdef DEBUG_LINK_CULLING
            static int lastNumberOfCulledLinks=0;
            if (enableLinkCulling && numberOfCulledLinks!=lastNumberOfCulledLinks) {
                lastNumberOfCulledLinks = numberOfCulledLinks;
                fprintf(stderr,"numberOfCulledLinks: %d/%d  numberOfVisibleLinks: %d/%d\n",numberOfCulledLinks,links.Size,links.Size-numberOfCulledLinks,links.Size);
            }
#   undef DEBUG_LINK_CULLING
#   endif //DEBUG_LINK_CULLING

            // Move activeNode at the start of the nodes list [This should improve active node overlapping other nodes]
            if (activeNode && nodes.size()>0 && activeNodeIndex!=0) {
                IM_ASSERT(activeNodeIndex>=0 && activeNodeIndex<nodes.size());
                IM_ASSERT(nodes[activeNodeIndex]==activeNode);
                Node* n=nodes[0];
                nodes[0]=activeNode;
                nodes[activeNodeIndex]=n;
                activeNodeIndex=0;
            }


            // Open context menu
            if (!open_context_menu && (node_hovered_in_scene || node_hovered_in_list) && ((ImGui::IsKeyReleased(io.KeyMap[ImGuiKey_Delete]) && !ImGui::GetIO().WantTextInput) || mustDeleteANodeSoon)) {
                // Delete selected node directly:
                menuNode = node_hovered_in_scene ? node_hovered_in_scene : node_hovered_in_list ? node_hovered_in_list : NULL;
                if (menuNode==node_to_fire_edit_callback) node_to_fire_edit_callback = NULL;
                if (menuNode==node_to_paste_from_copy_source) node_to_paste_from_copy_source = NULL;
                if (menuNode==nodeThatIsBeingEditing) nodeThatIsBeingEditing = NULL;
                deleteNode(menuNode);menuNode = NULL;
                menuNode = node_hovered_in_list = node_hovered_in_scene = NULL;
                open_delete_only_context_menu = false;	// just in case...
            }
            else if (/*!isAContextMenuOpen &&*/ !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) && getNumAvailableNodeTypes()>0 && nodeFactoryFunctionPtr)   {
                if (ImGui::IsMouseClicked(1))   {   // Open context menu for adding nodes
                    menuNode = node_hovered_in_scene ? node_hovered_in_scene : node_hovered_in_list ? node_hovered_in_list : NULL;
                    //fprintf(stderr,"menuNode.name=%s\n",menuNode?menuNode->getName():"NULL");
                    node_hovered_in_list = node_hovered_in_scene = NULL;
                    open_context_menu = true;
                }
            }

            // Open context menu
            if (open_context_menu || open_delete_only_context_menu)  {
                /*if (!menuNode)  {
            if (node_hovered_in_list) menuNode = node_hovered_in_list;
            if (node_hovered_in_scene) menuNode = node_hovered_in_scene;
        }*/
                ImGuiContext& g = *GImGui; while (g.OpenPopupStack.size() > 0) g.OpenPopupStack.pop_back();   // Close all existing context-menus
                ImGui::PushID(menuNode);
                if (open_delete_only_context_menu) ImGui::OpenPopup("delete_only_context_menu");
                else if (open_context_menu) ImGui::OpenPopup("context_menu");
                ImGui::PopID();
            }
            else if (mouseRectangularSelectionForNodesStarted || (!node_hovered_in_scene && !node_hovered_in_list && !isMouseDraggingForScrolling && !dragNode.isValid() && !isSomeNodeMoving && !io.KeyShift && !ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered())) {
                // Test mouse rect selection for nodes
                ImRect absSelection(rectangularSelectionData.absSelectionMin,rectangularSelectionData.absSelectionMax);
                if (isMouseDraggingForMovingNodes) {
                    if (!mouseRectangularSelectionForNodesStarted) {
                        mouseRectangularSelectionForNodesStarted=true;
                        mouseRectangularSelectionForNodesMin=mouseRectangularSelectionForNodesMax=io.MousePos;
                    }
                    mouseRectangularSelectionForNodesMax=io.MousePos;

                    absSelection = ImRect(mouseRectangularSelectionForNodesMin,mouseRectangularSelectionForNodesMax);
                    if (absSelection.Min.x>absSelection.Max.x)  {float tmp = absSelection.Min.x;absSelection.Min.x=absSelection.Max.x;absSelection.Max.x=tmp;}
                    if (absSelection.Min.y>absSelection.Max.y)  {float tmp = absSelection.Min.y;absSelection.Min.y=absSelection.Max.y;absSelection.Max.y=tmp;}

                    draw_list->AddRectFilled(absSelection.Min, absSelection.Max, style.color_mouse_rectangular_selection, 0.f);
                    draw_list->AddRect(absSelection.Min, absSelection.Max, style.color_mouse_rectangular_selection_frame, 0.f,0x0F,4.0f*currentFontWindowScale);    // Frame
                }
                else if (mouseRectangularSelectionForNodesStarted) {
                    // Note: merging this if branch into the main node loop would save a second loop for selecting nodes [Possible opt]
                    mouseRectangularSelectionForNodesStarted=false;
                    if (!io.KeyCtrl) unselectAllNodes();
                    //select all nodes inside selection-------------------
                    absSelection.Min-=offset;absSelection.Max-=offset;
                    ImVec2 nodePos;ImRect cullRect;
                    for (int i=0,isz=nodes.size();i<isz;i++)    {
                        Node* node = nodes[i];
                        nodePos = node->GetPos(currentFontWindowScale);
                        cullRect=ImRect(nodePos,nodePos+node->Size);
                        if (absSelection.Overlaps(cullRect)) {
                            node->isSelected=true;
                            if (!activeNode) activeNode = node;  //Optional faster alternative to: findANewActiveNode(); commented out below
                        }
                    }
                    //--------------------------------------------------------
                    //if (!activeNode) findANewActiveNode();
                }
                rectangularSelectionData.absSelectionMin = absSelection.Min;
                rectangularSelectionData.absSelectionMax = absSelection.Max;
            }

            // Draw context menu
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,8));
            ImGui::PushID(menuNode);
            if (ImGui::BeginPopup("delete_only_context_menu"))  {
                Node* node = menuNode;
                if (node)   {
                    ImGui::Text("Node '%s'", node->Name);
                    ImGui::Separator();
                    if (ImGui::MenuItem("Delete", NULL, false, true)) {
                        if (node==node_to_fire_edit_callback) node_to_fire_edit_callback = NULL;
                        if (node==node_to_paste_from_copy_source) node_to_paste_from_copy_source = NULL;
                        if (node==nodeThatIsBeingEditing) nodeThatIsBeingEditing = NULL;
                        //printf("Current nodes.size()=%d; Deleting node %s.\n",nodes.size(),node->Name);fflush(stdout);
                        deleteNode(node);menuNode = NULL;
                    }
                }
                ImGui::EndPopup();
                //isAContextMenuOpen = true;
            }
            else if (ImGui::BeginPopup("context_menu"))  {
                Node* node = menuNode;
                ImVec2 scene_pos = (ImGui::GetMousePosOnOpeningCurrentPopup() - offset)/currentFontWindowScale;
                if (node)   {
                    ImGui::Text("Node '%s'", node->Name);
                    ImGui::Separator();
                    //if (ImGui::MenuItem("Rename..", NULL, false, false)) {}
                    if (node->canBeCopied() && ImGui::MenuItem("Copy", NULL, false, true)) copyNode(node);
                    if (sourceCopyNode && sourceCopyNode->typeID==node->typeID) {
                        if (ImGui::MenuItem("Paste", NULL, false, true)) {
                            node_to_paste_from_copy_source = node;
                        }
                    }
                    if (ImGui::MenuItem("Delete", NULL, false, true)) {
                        if (node==node_to_fire_edit_callback) node_to_fire_edit_callback = NULL;
                        if (node==node_to_paste_from_copy_source) node_to_paste_from_copy_source = NULL;
                        if (node==nodeThatIsBeingEditing) nodeThatIsBeingEditing = NULL;
                        //printf("Current nodes.size()=%d; Deleting node %s.\n",nodes.size(),node->Name);fflush(stdout);
                        deleteNode(node);menuNode=NULL;
                    }
                    //if (ImGui::MenuItem("Copy", NULL, false, false)) {}
                }
                else    {
                    /*if (ImGui::MenuItem("Add ExampleNode")) {
                addNode(ExampleNode::Create(scene_pos,0.5f, ImColor(100,100,200)));
            }*/
                    ImGui::Text("%s","Add Node Menu");
                    ImGui::Separator();
                    if (nodeFactoryFunctionPtr) {
                        if (sourceCopyNode && sourceCopyNode->canBeCopied()) {
                            AvailableNodeInfo* ni = fetchAvailableNodeInfo(sourceCopyNode->getType());
                            if ((!ni || ni->maxNumInstances<0 || ni->curNumInstances<ni->maxNumInstances) && ImGui::MenuItem("Paste##cloneCopySource")) {
                                Node* clonedNode = addNode(nodeFactoryFunctionPtr(sourceCopyNode->typeID,scene_pos,*this));
                                clonedNode->fields.copyPDataValuesFrom(sourceCopyNode->fields);
                                clonedNode->onCopied();
                            }
                            ImGui::Separator();
                        }
                        for (int nt=0,ntSize=getNumAvailableNodeTypes();nt<ntSize;nt++) {
                            ImGui::PushID(nt);
                            AvailableNodeInfo& ni = availableNodesInfo[nt];
                            if ((ni.maxNumInstances<0 || ni.curNumInstances<ni.maxNumInstances) && ImGui::MenuItem(pNodeTypeNames[ni.type])) {
                                addNode(ni.type,scene_pos,&ni);
                            }
                            ImGui::PopID();
                        }
                    }
                    //if (ImGui::MenuItem("Paste", NULL, false, false)) {}
                }
                ImGui::EndPopup();
                //isAContextMenuOpen = true;
            }
            //else isAContextMenuOpen = false;
            ImGui::PopID();
            ImGui::PopStyleVar();



            ImGui::PopItemWidth();

            // Scrolling
            //if (!isSomeNodeMoving && !isaNodeInActiveState && !dragNode.node && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&  ImGui::IsMouseDragging(0, 6.0f)) scrolling = scrolling - io.MouseDelta;
            // TODO: We can probably adjust the or-group better, using the new flags/methods, in next line
            if (isMouseDraggingForScrolling && (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) || ImGui::IsWindowFocused() || IsWindowFocused(ImGuiFocusedFlags_RootWindow))) {
                scrolling = scrolling - io.MouseDelta;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                // This must be fixed somehow: ImGui::GetIO().WantCaptureMouse == false, because g.ActiveID == 0
                //        fprintf(stderr,"g.ActiveId=%d\n",g.ActiveId);     // This is the inner cause
                //        ImGui::GetIO().WantCaptureMouse = true; // does nothing
                //        g.ActiveId = window->MoveId;            // makes WantCaptureMouse and WantCaptureKeyboard toggle like crazy every frame
            }
            else if (isSomeNodeMoving) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

            if (!io.FontAllowUserScaling)   {
                // Reset the font scale (3 lines)
                fontScaleToTrack = oldFontScaleToReset;
                g.FontBaseSize = io.FontGlobalScale * g.Font->Scale * g.Font->FontSize;
                g.FontSize = window->CalcFontSize();
            }
        }
        ImGui::EndChild();  // scrolling_region
        ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
    }
    ImGui::EndChild();  // GraphNodeChildWindow

    if (node_to_paste_from_copy_source && sourceCopyNode && node_to_paste_from_copy_source->typeID==sourceCopyNode->typeID) {
        node_to_paste_from_copy_source->fields.copyPDataValuesFrom(sourceCopyNode->fields);
        node_to_paste_from_copy_source->onCopied();
    }

    if (node_to_fire_edit_callback) {
        node_to_fire_edit_callback->onEdited();
        if (nodeCallback) nodeCallback(node_to_fire_edit_callback,NS_EDITED,*this);
    }


}



void NodeGraphEditor::registerNodeTypes(const char *nodeTypeNames[], int numNodeTypeNames, NodeFactoryDelegate _nodeFactoryFunctionPtr, const int *pOptionalNodeTypesToUse, int numNodeTypesToUse, const int* pOptionalMaxNumAllowedInstancesToUse, int numMaxNumAllowedInstancesToUse,bool sortEntriesAlphabetically)
{
    this->numNodeTypeNames = numNodeTypeNames;
    this->pNodeTypeNames = numNodeTypeNames>0 ? &nodeTypeNames[0] : NULL;
    this->nodeFactoryFunctionPtr = _nodeFactoryFunctionPtr;
    this->availableNodesInfo.clear();this->availableNodesInfoInverseMap.clear();
    if (numNodeTypesToUse>numNodeTypeNames) numNodeTypesToUse = numNodeTypeNames;
    availableNodesInfoInverseMap.resize(numNodeTypeNames);
    for (int i=0;i<numNodeTypeNames;i++) availableNodesInfoInverseMap[i]=-1;
    AvailableNodeInfo tmp;
    if (pOptionalNodeTypesToUse && numNodeTypesToUse>0) {
        this->availableNodesInfo.reserve(numNodeTypesToUse);
	for (int i=0;i<numNodeTypesToUse;i++) {
	    IM_ASSERT(numNodeTypeNames>pOptionalNodeTypesToUse[i]); // Missing some names inside 'nodeTypeNames'
	    tmp = AvailableNodeInfo(pOptionalNodeTypesToUse[i],(numMaxNumAllowedInstancesToUse>i && pOptionalMaxNumAllowedInstancesToUse) ? pOptionalMaxNumAllowedInstancesToUse[i] : -1,0,pNodeTypeNames[pOptionalNodeTypesToUse[i]]);
            this->availableNodesInfo.push_back(tmp);
	    if (!sortEntriesAlphabetically) this->availableNodesInfoInverseMap[tmp.type] = i;
        }
    }
    else if (numNodeTypeNames>0)    {
        this->availableNodesInfo.reserve(numNodeTypeNames);
	IM_ASSERT(numNodeTypeNames>=numNodeTypeNames);
        for (int i=0;i<numNodeTypeNames;i++) {
	    tmp = AvailableNodeInfo(i,(numMaxNumAllowedInstancesToUse>i && pOptionalMaxNumAllowedInstancesToUse) ? pOptionalMaxNumAllowedInstancesToUse[i] : -1,0,pNodeTypeNames[i]);
            this->availableNodesInfo.push_back(tmp);
	    if (!sortEntriesAlphabetically) this->availableNodesInfoInverseMap[tmp.type] = i;
        }
    }

    // Is it possible to sort "this->availableNodeTypes" based on "this->pNodeTypeNames",
    // so that it display its elements in alphabetical order ? Attempt:
    if (sortEntriesAlphabetically && availableNodesInfo.size()>0)	{
	// qsort is in <stdlib.h>
	qsort(&availableNodesInfo[0], availableNodesInfo.size(), sizeof(AvailableNodeInfo), &AvailableNodeInfoNameSorter);
	for (int i=0;i<availableNodesInfo.size();i++) {this->availableNodesInfoInverseMap[availableNodesInfo[i].type] = i;}
    }

}

bool NodeGraphEditor::removeLinkAt(int link_idx) {
    if (link_idx<0 || link_idx>=links.size()) return false;
    // remove link
    NodeLink& link = links[link_idx];
    if (linkCallback) linkCallback(link,LS_DELETED,*this);
    if (link_idx+1 < links.size()) link = links[links.size()-1];    // swap with the last link
    links.resize(links.size()-1);
    return true;
}
void NodeGraphEditor::removeAnyLinkFromNode(Node* node, bool removeInputLinks, bool removeOutputLinks)  {
    for (int link_idx=0;link_idx<links.size();link_idx++)    {
        NodeLink& link = links[link_idx];
        if ((removeOutputLinks && link.InputNode==node) || (removeInputLinks && link.OutputNode==node))    {
            // remove link
            if (linkCallback) linkCallback(link,LS_DELETED,*this);
            if (link_idx+1 < links.size()) link = links[links.size()-1];    // swap with the last link
            links.resize(links.size()-1);
            --link_idx;
        }
    }
}
bool NodeGraphEditor::isLinkPresent(Node *inputNode, int input_slot, Node *outputNode, int output_slot,int* pOptionalIndexInLinkArrayOut) const  {
    if (pOptionalIndexInLinkArrayOut) *pOptionalIndexInLinkArrayOut=-1;
    for (int link_idx=0;link_idx<links.size();link_idx++)    {
        const NodeLink& l = links[link_idx];
        if (l.InputNode==inputNode && l.InputSlot==input_slot &&
            l.OutputNode==outputNode && l.OutputSlot==output_slot) {
            if (pOptionalIndexInLinkArrayOut) *pOptionalIndexInLinkArrayOut=link_idx;
            return true;
        }
    }
    return false;
}
bool NodeGraphEditor::hasLinks(Node *node) const    {
    for (int i=0,isz=links.size();i<isz;i++)    {
        const NodeLink& l = links[i];
        if (l.InputNode==node || l.OutputNode==node) return true;
    }
    return false;
}
int NodeGraphEditor::getAllNodesOfType(int typeID, ImVector<Node *> *pNodesOut, bool clearNodesOutBeforeUsage)  {
    if (pNodesOut && clearNodesOutBeforeUsage) pNodesOut->clear();
    int cnt = 0;
    for (int i=0,isz=nodes.size();i<isz;i++)    {
        Node* n = nodes[i];
        if (n->getType()==typeID) {
            ++cnt;
            if (pNodesOut) pNodesOut->push_back(n);
        }
    }
    return cnt;
}
int NodeGraphEditor::getAllNodesOfType(int typeID, ImVector<const Node *> *pNodesOut, bool clearNodesOutBeforeUsage) const  {
    if (pNodesOut && clearNodesOutBeforeUsage) pNodesOut->clear();
    int cnt = 0;
    for (int i=0,isz=nodes.size();i<isz;i++)    {
        const Node* n = nodes[i];
        if (n->getType()==typeID) {
            ++cnt;
            if (pNodesOut) pNodesOut->push_back(n);
        }
    }
    return cnt;
}

void NodeGraphEditor::copyNode(Node *n)	{
    const bool mustDeleteSourceCopyNode = sourceCopyNode && (!n || n->typeID!=sourceCopyNode->typeID);
    if (mustDeleteSourceCopyNode)   {
	sourceCopyNode->~Node();              // ImVector does not call it
	ImGui::MemFree(sourceCopyNode);       // items MUST be allocated by the user using ImGui::MemAlloc(...)
	sourceCopyNode = NULL;
    }
    if (!n) return;
    if (!sourceCopyNode)    {
	if (!nodeFactoryFunctionPtr) return;
    sourceCopyNode = nodeFactoryFunctionPtr(n->typeID,ImVec2(0,0),*this);
    }
    sourceCopyNode->fields.copyPDataValuesFrom(n->fields);
    //sourceCopyNode->onCopied();   // Nope: sourceCopyNode is just owned for storage
}

void NodeGraphEditor::getInputNodesForNodeAndSlot(const Node* node,int input_slot,ImVector<Node *> &returnValueOut, ImVector<int> *pOptionalReturnValueOutputSlotOut) const  {
    returnValueOut.clear();if (pOptionalReturnValueOutputSlotOut) pOptionalReturnValueOutputSlotOut->clear();
    for (int link_idx=0,link_idx_size=links.size();link_idx<link_idx_size;link_idx++)   {
        const NodeLink& link = links[link_idx];
        if (link.OutputNode == node && link.OutputSlot == input_slot)  {
            returnValueOut.push_back(link.InputNode);
            if (pOptionalReturnValueOutputSlotOut) pOptionalReturnValueOutputSlotOut->push_back(link.InputSlot);
        }
    }
}
Node* NodeGraphEditor::getInputNodeForNodeAndSlot(const Node* node,int input_slot,int* pOptionalReturnValueOutputSlotOut) const    {
    if (pOptionalReturnValueOutputSlotOut) *pOptionalReturnValueOutputSlotOut=-1;
    for (int link_idx=0,link_idx_size=links.size();link_idx<link_idx_size;link_idx++)   {
        const NodeLink& link = links[link_idx];
        if (link.OutputNode == node && link.OutputSlot == input_slot)  {
            if (pOptionalReturnValueOutputSlotOut) *pOptionalReturnValueOutputSlotOut = link.InputSlot;
            return link.InputNode;
        }
    }
    return NULL;
}
void NodeGraphEditor::getOutputNodesForNodeAndSlot(const Node* node,int output_slot,ImVector<Node *> &returnValueOut, ImVector<int> *pOptionalReturnValueInputSlotOut) const {
    returnValueOut.clear();if (pOptionalReturnValueInputSlotOut) pOptionalReturnValueInputSlotOut->clear();
    for (int link_idx=0,link_idx_size=links.size();link_idx<link_idx_size;link_idx++)   {
        const NodeLink& link = links[link_idx];
        if (link.InputNode == node && link.InputSlot == output_slot)  {
            returnValueOut.push_back(link.OutputNode);
            if (pOptionalReturnValueInputSlotOut) pOptionalReturnValueInputSlotOut->push_back(link.OutputSlot);
        }
    }
}
bool NodeGraphEditor::isNodeReachableFrom(const Node *node1, int slot1, bool goBackward,const Node* nodeToFind,int* pOptionalNodeToFindSlotOut) const    {

    for (int i=0,isz=links.size();i<isz;i++)    {
        const NodeLink& l = links[i];
        if (goBackward)  {
            if (l.OutputNode == node1 && l.OutputSlot == slot1) {
                if (l.InputNode == nodeToFind) {
                    if (pOptionalNodeToFindSlotOut) *pOptionalNodeToFindSlotOut = l.InputSlot;
                    return true;
                }
                if (isNodeReachableFrom(l.InputNode,goBackward,nodeToFind,pOptionalNodeToFindSlotOut)) return true;
            }
        }
        else {
            if (l.InputNode == node1 && l.InputSlot == slot1) {
                if (l.OutputNode == nodeToFind) {
                    if (pOptionalNodeToFindSlotOut) *pOptionalNodeToFindSlotOut = l.OutputSlot;
                    return true;
                }
                if (isNodeReachableFrom(l.OutputNode,goBackward,nodeToFind,pOptionalNodeToFindSlotOut)) return true;
            }
        }
    }
    return false;
}
bool NodeGraphEditor::isNodeReachableFrom(const Node *node1, bool goBackward,const Node* nodeToFind,int* pOptionalNode1SlotOut,int* pOptionalNodeToFindSlotOut) const  {
    if (pOptionalNode1SlotOut) *pOptionalNode1SlotOut=-1;
    for (int i=0,isz=(goBackward ? node1->InputsCount : node1->OutputsCount);i<isz;i++)  {
        if (isNodeReachableFrom(node1,i,goBackward,nodeToFind,pOptionalNodeToFindSlotOut))  {
            if (pOptionalNode1SlotOut) *pOptionalNode1SlotOut=i;
            return true;
        }
    }
    return false;
}

int NodeGraphEditor::getSelectedNodes(ImVector<Node *> &rv) {
    rv.resize(0);
    for (int i=0,isz=nodes.size();i<isz;i++)	{
	Node* n = nodes[i];
	if (n->isSelected) rv.push_back(n);
    }
    return rv.size();
}

int NodeGraphEditor::getSelectedNodes(ImVector<const Node *> &rv) const {
    rv.resize(0);
    for (int i=0,isz=nodes.size();i<isz;i++)	{
        const Node* n = nodes[i];
        if (n->isSelected) rv.push_back(n);
    }
    return rv.size();
}

void NodeGraphEditor::selectNodePrivate(const Node *node, const bool flag, bool findANewActiveNodeWhenNeeded) {
    if (!node) return;
    node->isSelected=flag;
    if (flag) {if (!activeNode && findANewActiveNodeWhenNeeded) activeNode=const_cast<Node*>(node);return;}
    if (node==activeNode) {activeNode = NULL;if (findANewActiveNodeWhenNeeded) findANewActiveNode();}
}

void NodeGraphEditor::selectAllNodesPrivate(bool flag, bool findANewActiveNodeWhenNeeded) {
    const int isz = nodes.size();
    for (int i=0;i<isz;i++) nodes[i]->isSelected=flag;
    if (flag) {if (!activeNode && isz>0 && findANewActiveNodeWhenNeeded)   activeNode=nodes[isz-1];}
    else activeNode = NULL;
}

bool NodeGraphEditor::overrideNodeName(Node* n,const char *newName)    {
    if (!n || !newName) return false;
    //if (strncmp(n->Name,newName,IMGUINODE_MAX_NAME_LENGTH)==0) return false;
    n->mustOverrideName = true;
    strncpy(n->Name,newName,IMGUINODE_MAX_NAME_LENGTH);n->Name[IMGUINODE_MAX_NAME_LENGTH-1]='\0';
    return true;
}

void NodeGraphEditor::overrideNodeTitleBarColors(Node *node, const ImU32 *pTextColor, const ImU32 *pBgColor, const float *pBgColorGradient) {
    if (!node) return;
    if (pTextColor)         node->overrideTitleTextColor=*pTextColor;
    if (pBgColor)           node->overrideTitleBgColor=*pBgColor;
    if (pBgColorGradient)   node->overrideTitleBgColorGradient=*pBgColorGradient;
}
template < int IMGUINODE_MAX_SLOTS > inline static int ProcessSlotNamesSeparatedBySemicolons(const char* slotNamesSeparatedBySemicolons,char Names[IMGUINODE_MAX_SLOTS][IMGUINODE_MAX_SLOT_NAME_LENGTH]) {
    int Count = 0;
    const char *tmp = slotNamesSeparatedBySemicolons,*tmp2 = NULL;int length;
    if (tmp && strlen(tmp)>0)    {
        while ((tmp2=strchr(tmp,(int)';')) && Count<IMGUINODE_MAX_SLOTS)    {
            length = (int) (tmp2-tmp);if (length>=IMGUINODE_MAX_SLOT_NAME_LENGTH) length=IMGUINODE_MAX_SLOT_NAME_LENGTH-1;
            strncpy(Names[Count],tmp, length);
            Names[Count][length] = '\0';
            ++Count;tmp = ++tmp2;
        }
        if (tmp && Count<IMGUINODE_MAX_SLOTS)    {
            length = (int) strlen(tmp);if (length>=IMGUINODE_MAX_SLOT_NAME_LENGTH) length=IMGUINODE_MAX_SLOT_NAME_LENGTH-1;
            strncpy(Names[Count],tmp, length);
            Names[Count][length] = '\0';
            ++Count;
        }
    }
    return Count;
}
bool NodeGraphEditor::overrideNodeInputSlots(Node* n,const char *slotNamesSeparatedBySemicolons)   {
    if (!n || !slotNamesSeparatedBySemicolons) return false;
    n->mustOverrideInputSlots = true;
    const int OldInputSlots = n->InputsCount;
    n->InputsCount = ProcessSlotNamesSeparatedBySemicolons<IMGUINODE_MAX_INPUT_SLOTS>(slotNamesSeparatedBySemicolons,n->InputNames);
    for (int i=n->InputsCount;i<OldInputSlots;i++) {
        for (int j=0,jsz=links.size();j<jsz;j++)  {
            NodeLink& l = links[j];
            if (l.OutputNode==n && l.OutputSlot==i)  {
                if (removeLinkAt(j)) --j;
            }
        }
    }
    return true;
}
bool NodeGraphEditor::overrideNodeOutputSlots(Node* n,const char *slotNamesSeparatedBySemicolons)  {
    if (!n || !slotNamesSeparatedBySemicolons) return false;
    n->mustOverrideOutputSlots = true;
    const int OldOutputSlots = n->OutputsCount;
    n->OutputsCount = ProcessSlotNamesSeparatedBySemicolons<IMGUINODE_MAX_OUTPUT_SLOTS>(slotNamesSeparatedBySemicolons,n->OutputNames);
    for (int i=n->OutputsCount;i<OldOutputSlots;i++) {
        for (int j=0,jsz=links.size();j<jsz;j++)  {
            NodeLink& l = links[j];
            if (l.InputNode==n && l.InputSlot==i)  {
                if (removeLinkAt(j)) --j;
            }
        }
    }
    return true;
}

char NodeGraphEditor::CloseCopyPasteChars[3][5] = {"x","^","v"};


void Node::init(const char *name, const ImVec2 &pos, const char *inputSlotNamesSeparatedBySemicolons, const char *outputSlotNamesSeparatedBySemicolons, int _nodeTypeID/*,float currentWindowFontScale*/) {
    /*if (currentWindowFontScale<0)   {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        currentWindowFontScale = window ? window->FontWindowScale  : 0.f;
    }*/
    strncpy(Name, name, IMGUINODE_MAX_NAME_LENGTH); Name[IMGUINODE_MAX_NAME_LENGTH-1] = '\0'; Pos = /*currentWindowFontScale==0.f?*/pos/*:pos/currentWindowFontScale*/;
    InputsCount = ProcessSlotNamesSeparatedBySemicolons<IMGUINODE_MAX_INPUT_SLOTS>(inputSlotNamesSeparatedBySemicolons,InputNames);
    OutputsCount = ProcessSlotNamesSeparatedBySemicolons<IMGUINODE_MAX_OUTPUT_SLOTS>(outputSlotNamesSeparatedBySemicolons,OutputNames);
    typeID = _nodeTypeID;
    user_ptr = NULL;userID=-1;
    startEditingTime = 0;
    isOpen = true;
}

/*
bool FieldInfo::copyFrom(const FieldInfo &f) {
    if (!isCompatibleWith(f)) return false;
    void* myOldPdata = pdata;
    *this = f;
    pdata = myOldPdata;
    return copyPDataValueFrom(f);
}
*/
bool FieldInfo::copyPDataValueFrom(const FieldInfo &f) {
    if (!isCompatibleWith(f) || (!pdata || !f.pdata)) return false;
    if (type==FT_CUSTOM)    {
        if (!copyFieldDelegate) return false;
        else return copyFieldDelegate(*this,f);
    }
    switch (type) {
    case FT_INT:
    case FT_ENUM:
    case FT_UNSIGNED:
    case FT_FLOAT:
    case FT_DOUBLE:
    case FT_COLOR:
    {
        const int numElements = numArrayElements<=0 ? 1 : numArrayElements;
        const size_t elemSize = ((type==FT_INT||type==FT_ENUM)?sizeof(int):
                                                               type==FT_UNSIGNED?sizeof(unsigned):
                                                                                 (type==FT_FLOAT || type==FT_COLOR)?sizeof(float):
                                                                                                                    type==FT_DOUBLE?sizeof(double):
                                                                                                                                    0);
        memcpy(pdata,f.pdata,numElements*elemSize);
    }
        break;
    case FT_BOOL: *((bool*)pdata) = *((bool*)f.pdata);
        break;
    case FT_TEXTLINE: memcpy(pdata,f.pdata,precision < f.precision ? precision : f.precision);
	break;
    case FT_STRING: memcpy(pdata,f.pdata,precision < f.precision ? precision : f.precision);
        break;
    default:
        //IM_ASSERT(true); // copyPDataValueFrom(...) not defined for this type [we can probably just skip this]
        return false;
    }
    return true;
}
#if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
bool FieldInfo::serialize(ImGuiHelper::Serializer& s) const   {
    const char* fieldName = label;    
    const int ft = this->type;
    switch (type) {
    case FT_INT:
    case FT_ENUM:
	return s.save((ImGui::FieldType) ft,(const int*)pdata,fieldName,numArrayElements,precision);
    case FT_BOOL: {
	return s.save((const bool*)pdata,fieldName,numArrayElements);
    }
    case FT_UNSIGNED:
	return s.save((const unsigned*)pdata,fieldName,numArrayElements,precision);
    case FT_DOUBLE:
    return s.save((const double*)pdata,fieldName,numArrayElements,(precision>=0 && needsRadiansToDegs) ? (precision+3) : precision);
    case FT_FLOAT:
    case FT_COLOR:
    return s.save((ImGui::FieldType) ft,(const float*)pdata,fieldName,numArrayElements,(precision>=0 && needsRadiansToDegs) ? (precision+3) : precision);
    case FT_STRING:
    return s.save((const char*)pdata,fieldName,precision);
    case FT_TEXTLINE:
    return s.saveTextLines((const char*)pdata,fieldName);
    case FT_CUSTOM:
        if (!serializeFieldDelegate) return false;
	return serializeFieldDelegate(s,*this);
    default:
        //IM_ASSERT(true); // copyPDataValueFrom(...) not defined for this type [we can probably just skip this]
        return false;
    }
    return false;
}
#endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
static bool fieldInfoParseCallback(ImGuiHelper::FieldType ft,int numArrayElements,void* pValue,const char* name,void* userPtr)    {
    FieldInfo& fi = *((FieldInfo*)(userPtr));
    if (strcmp(fi.label,name)!=0) {
        fprintf(stderr,"fieldInfoParseCallback Error: \"%s\"!=\"%s\"\n",fi.label,name);
        return true;    // true = stop parsing
    }
    const FieldType type = ft;
    switch (type) {
    case FT_INT:
    case FT_ENUM:
    case FT_UNSIGNED:
    case FT_FLOAT:
    case FT_DOUBLE:
    case FT_COLOR:
    case FT_BOOL:
    {
        const int numElements = numArrayElements<=0 ? 1 : numArrayElements;
        const size_t elemSize = ((type==FT_INT||type==FT_ENUM)?sizeof(int):
                                                               type==FT_UNSIGNED?sizeof(unsigned):
                                                                                 (type==FT_FLOAT || type==FT_COLOR)?sizeof(float):
                                                                                                                    type==FT_DOUBLE?sizeof(double):
																    type==FT_BOOL?sizeof(bool):
																		  0);
        memcpy(fi.pdata,pValue,numElements*elemSize);
        break;
    }
        break;
    case FT_STRING: {
        const char* txt = (const char*)pValue;
        int len = (int) strlen(txt);
        if (fi.precision>0 && fi.precision<len+1) len = fi.precision-1;
        char* dst = (char*) fi.pdata;
        strncpy(dst,(const char*)pValue,len+1);
        dst[len]='\0';
    }
        break;
    case FT_TEXTLINE: { // This is called more than once for multiple lines, but we just use a single line here
        const char* txt = (const char*)pValue;
        int len = (int) strlen(txt);
        if (fi.precision>0 && fi.precision<len+1) len = fi.precision-1;
        char* dst = (char*) fi.pdata;
        strncpy(dst,(const char*)pValue,len+1);
        dst[len]='\0';
    }
	break;
    case FT_CUSTOM:    {
	if (fi.deserializeFieldDelegate) fi.deserializeFieldDelegate(fi,type,numArrayElements,pValue,name);        
    }
	break;
    default:
        //IM_ASSERT(true); // copyPDataValueFrom(...) not defined for this type [we can probably just skip this]
        //return false;
        break;
    }
    return true;    // true = done parsing
}

const char* FieldInfo::deserialize(const ImGuiHelper::Deserializer& d,const char* start)    {
    if (type<ImGui::FT_COUNT) return d.parse(fieldInfoParseCallback,(void*)this,start);
    IM_ASSERT(true);	// Parse Custom type
    return start;
}
#endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#endif //NO_IMGUIHELPER_SERIALIZATION
FieldInfo &FieldInfoVector::addField(int *pdata, int numArrayElements, const char *label, const char *tooltip, int precision, int lowerLimit, int upperLimit, void *userData)   {
    IM_ASSERT(pdata && numArrayElements<=4);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_INT,(void*) pdata,label,tooltip,precision,numArrayElements,(double)lowerLimit,(double)upperLimit);
    f.userData = userData;
    return f;
}
FieldInfo &FieldInfoVector::addField(unsigned *pdata, int numArrayElements, const char *label, const char *tooltip, int precision, unsigned lowerLimit, unsigned upperLimit, void *userData)   {
    IM_ASSERT(pdata && numArrayElements<=4);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_UNSIGNED,(void*) pdata,label,tooltip,precision,numArrayElements,(double)lowerLimit,(double)upperLimit);
    f.userData = userData;
    return f;
}
FieldInfo &FieldInfoVector::addField(float *pdata, int numArrayElements, const char *label, const char *tooltip, int precision, float lowerLimit, float upperLimit, void *userData, bool needsRadiansToDegs)   {
    IM_ASSERT(pdata && numArrayElements<=4);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_FLOAT,(void*) pdata,label,tooltip,precision,numArrayElements,(double)lowerLimit,(double)upperLimit,needsRadiansToDegs);
    f.userData = userData;
    return f;
}
FieldInfo &FieldInfoVector::addField(double *pdata, int numArrayElements, const char *label, const char *tooltip, int precision, double lowerLimit, double upperLimit, void *userData, bool needsRadiansToDegs)   {
    IM_ASSERT(pdata && numArrayElements<=4);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_DOUBLE,(void*) pdata,label,tooltip,precision,numArrayElements,(double)lowerLimit,(double)upperLimit,needsRadiansToDegs);
    f.userData = userData;
    return f;
}
FieldInfo &FieldInfoVector::addField(char *pdata, int textLength, const char *label, const char *tooltip, int flags, bool multiline,float optionalHeight, void *userData,bool isSingleEditWithBrowseButton)   {
    IM_ASSERT(pdata);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(/*multiline ?*/ FT_STRING/* : FT_TEXTLINE*/,(void*) pdata,label,tooltip,textLength,flags,isSingleEditWithBrowseButton ? -500 : 0,(double)optionalHeight,multiline);
    f.userData = userData;
    return f;
}
FieldInfo &FieldInfoVector::addFieldEnum(int *pdata, int numEnumElements, FieldInfo::TextFromEnumDelegate textFromEnumFunctionPtr, const char *label, const char *tooltip, void *userData)   {
    IM_ASSERT(pdata && numEnumElements>0 && textFromEnumFunctionPtr);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_ENUM,(void*) pdata,label,tooltip,0,0,0,1,false,numEnumElements,textFromEnumFunctionPtr,userData);
    return f;
}
FieldInfo &FieldInfoVector::addFieldEnum(int *pdata, FieldInfo::GetNumEnumElementsDelegate getNumEnumElementsFunctionPtr, FieldInfo::TextFromEnumDelegate textFromEnumFunctionPtr, const char *label, const char *tooltip, void *userData)   {
    IM_ASSERT(pdata && getNumEnumElementsFunctionPtr && textFromEnumFunctionPtr);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_ENUM,(void*) pdata,label,tooltip,0,0,0,1,false,-1,textFromEnumFunctionPtr,userData,getNumEnumElementsFunctionPtr);
    return f;
}
// 2 overloads of addFieldEnum(...) follow here. Both are based on the code in imgui.cpp
static bool NGE_Enum_Items_ArrayGetter(void* data, int idx, const char** out_text)   {
    const char* const* items = (const char* const*)data;
    if (out_text) *out_text = items[idx];
    return true;
}
FieldInfo &FieldInfoVector::addFieldEnum(int *pdata, int numEnumElements, const char* const* items, const char *label, const char *tooltip)   {
    IM_ASSERT(pdata && numEnumElements>0);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_ENUM,(void*) pdata,label,tooltip,0,0,0,1,false,numEnumElements,NGE_Enum_Items_ArrayGetter,(void*)items);
    return f;
}
static bool NGE_Enum_Items_SingleStringGetter(void* data, int idx, const char** out_text)   {
    // FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)  {
        if (idx == items_count) break;
        p += strlen(p) + 1;
        items_count++;
    }
    if (!*p)        return false;
    if (out_text)   *out_text = p;
    return true;
}
FieldInfo &FieldInfoVector::addFieldEnum(int *pdata,const char* items_separated_by_zeros, const char *label, const char *tooltip)   {
    IM_ASSERT(pdata);
    int items_count = 0;
    const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
    while (*p)  {p += strlen(p) + 1;items_count++;}
    IM_ASSERT(items_count>0);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_ENUM,(void*) pdata,label,tooltip,0,0,0,1,false,items_count,NGE_Enum_Items_SingleStringGetter,(void*) items_separated_by_zeros);
    return f;
}
FieldInfo &FieldInfoVector::addField(bool *pdata, const char *label, const char *tooltip, void *userData)   {
    IM_ASSERT(pdata);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_BOOL,(void*) pdata,label,tooltip);
    f.userData = userData;
    return f;
}
FieldInfo &FieldInfoVector::addFieldColor(float *pdata, bool useAlpha, const char *label, const char *tooltip, int precision, void *userData)   {
    IM_ASSERT(pdata);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_COLOR,(void*) pdata,label,tooltip,precision,useAlpha?4:3,0,1);
    f.userData = userData;
    return f;
}
FieldInfo &FieldInfoVector::addFieldCustom(FieldInfo::RenderFieldDelegate renderFieldDelegate,FieldInfo::CopyFieldDelegate copyFieldDelegate, void *userData
//-------------------------------------------------------------------------------
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
	,FieldInfo::SerializeFieldDelegate serializeFieldDelegate,
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
	FieldInfo::DeserializeFieldDelegate deserializeFieldDelegate
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------
)   {
    IM_ASSERT(renderFieldDelegate);
    push_back(FieldInfo());
    FieldInfo& f = (*this)[size()-1];
    f.init(FT_CUSTOM);
    f.renderFieldDelegate=renderFieldDelegate;
    f.copyFieldDelegate=copyFieldDelegate;
    f.userData = userData;
//-------------------------------------------------------------------------------
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
    f.serializeFieldDelegate=serializeFieldDelegate;
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
    f.deserializeFieldDelegate=deserializeFieldDelegate;
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------

    return f;
}
bool NodeGraphEditor::UseSlidersInsteadOfDragControls = false;
template<typename T> inline static T GetRadiansToDegs() {
    static T factor = T(180)/(3.1415926535897932384626433832795029);
    return factor;
}
template<typename T> inline static T GetDegsToRadians() {
    static T factor = T(3.1415926535897932384626433832795029)/T(180);
    return factor;
}
bool FieldInfo::render(int nodeWidth)   {
    FieldInfo& f = *this;
    ImGui::PushID((const void*) &f);
    static const int precisionStrSize = 16;
    static char precisionStr[precisionStrSize];
    int precisionLastCharIndex;
    const char* label = (/*f.label &&*/ f.label[0]!='\0') ? &f.label[0] : "##DummyLabel";
    if (f.type!=FT_UNSIGNED && f.type!=FT_INT)  {
        if (f.precision>0) {
            strcpy(precisionStr,"%.");
            snprintf(&precisionStr[2], precisionStrSize-2,"%ds",f.precision);
            precisionLastCharIndex = strlen(precisionStr)-1;
        }
        else {
            strcpy(precisionStr,"%s");
            precisionLastCharIndex = 1;
        }
    }

    float dragSpeed = (float)(f.maxValue-f.minValue)/200.f;if (dragSpeed<=0) dragSpeed=1.f;

    bool changed = false;int widgetIndex = 0;bool skipTooltip = false;
    switch (f.type) {
    case FT_DOUBLE: {
        precisionStr[precisionLastCharIndex]='f';
        const float minValue = (float) f.minValue;
        const float maxValue = (float) f.maxValue;
        const double rtd = f.needsRadiansToDegs ? GetRadiansToDegs<double>() : 1.f;
        const double dtr = f.needsRadiansToDegs ? GetDegsToRadians<double>() : 1.f;
        double* pField = (double*)f.pdata;
        float value[4] = {0,0,0,0};
        for (int vl=0;vl<f.numArrayElements;vl++) {
            value[vl] = (float) ((*(pField+vl))*rtd);
        }
        if (NodeGraphEditor::UseSlidersInsteadOfDragControls)   {
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::SliderFloat2(label,value,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::SliderFloat3(label,value,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::SliderFloat4(label,value,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::SliderFloat(label,value,minValue,maxValue,precisionStr);break;
            }
        }
        else {
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::DragFloat2(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::DragFloat3(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::DragFloat4(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::DragFloat(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            }
        }
        if (changed)    {
            for (int vl=0;vl<f.numArrayElements;vl++) {
                *(pField+vl) = (double) value[vl] * dtr;
            }
        }

    }
        break;
    case FT_FLOAT: {
        precisionStr[precisionLastCharIndex]='f';
        const float minValue = (float) f.minValue;
        const float maxValue = (float) f.maxValue;
        const float rtd = f.needsRadiansToDegs ? GetRadiansToDegs<float>() : 1.f;
        const float dtr = f.needsRadiansToDegs ? GetDegsToRadians<float>() : 1.f;
        float* pField = (float*)f.pdata;
        float value[4] = {0,0,0,0};
        for (int vl=0;vl<f.numArrayElements;vl++) {
            value[vl] = (float) ((*(pField+vl))*rtd);
        }
        if (NodeGraphEditor::UseSlidersInsteadOfDragControls)   {
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::SliderFloat2(label,value,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::SliderFloat3(label,value,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::SliderFloat4(label,value,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::SliderFloat(label,value,minValue,maxValue,precisionStr);break;
            }
        }
        else {
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::DragFloat2(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::DragFloat3(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::DragFloat4(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::DragFloat(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            }
        }
        if (changed)    {
            for (int vl=0;vl<f.numArrayElements;vl++) {
                *(pField+vl) = (float) value[vl]*dtr;
            }
        }
    }
        break;
    case FT_UNSIGNED: {
        strcpy(precisionStr,"%1.0f");
        const int minValue = (int) f.minValue;
        const int maxValue = (int) f.maxValue;
        unsigned* pField = (unsigned*) f.pdata;
        int value[4] = {0,0,0,0};
        for (int vl=0;vl<f.numArrayElements;vl++) {
            value[vl] = (int) *(pField+vl);
        }
        if (NodeGraphEditor::UseSlidersInsteadOfDragControls)   {
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::SliderInt2(label,value,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::SliderInt3(label,value,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::SliderInt4(label,value,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::SliderInt(label,value,minValue,maxValue,precisionStr);break;
            }
        }
        else {
            if (dragSpeed<1.f) dragSpeed = 1.f;
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::DragInt2(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::DragInt3(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::DragInt4(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::DragInt(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            }
        }
        if (changed)    {
            for (int vl=0;vl<f.numArrayElements;vl++) {
                *(pField+vl) = (unsigned) value[vl];
            }
        }
    }
        break;
    case FT_INT: {
        strcpy(precisionStr,"%1.0f");
        const int minValue = (int) f.minValue;
        const int maxValue = (int) f.maxValue;
        int* pField = (int*) f.pdata;
        int value[4] = {0,0,0,0};
        for (int vl=0;vl<f.numArrayElements;vl++) {
            value[vl] = (int) *(pField+vl);
        }
        if (NodeGraphEditor::UseSlidersInsteadOfDragControls)   {
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::SliderInt2(label,value,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::SliderInt3(label,value,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::SliderInt4(label,value,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::SliderInt(label,value,minValue,maxValue,precisionStr);break;
            }
        }
        else {
            if (dragSpeed<1.f) dragSpeed = 1.f;
            switch (f.numArrayElements)    {
            case 2: changed = ImGui::DragInt2(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 3: changed = ImGui::DragInt3(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            case 4: changed = ImGui::DragInt4(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            default: changed = ImGui::DragInt(label,value,dragSpeed,minValue,maxValue,precisionStr);break;
            }
        }
        if (changed)    {
            for (int vl=0;vl<f.numArrayElements;vl++) {
                *(pField+vl) = (int) value[vl];
            }
        }
    }
        break;
    case FT_BOOL:   {
        bool * boolPtr = (bool*) f.pdata;
        changed|=ImGui::Checkbox(label,boolPtr);
    }
        break;
    case FT_ENUM: {
	if (f.getNumEnumElementsFunctionPointer) f.numEnumElements = f.getNumEnumElementsFunctionPointer(f.userData);
	changed|=ImGui::Combo(label,(int*) f.pdata,f.textFromEnumFunctionPointer,f.userData,f.numEnumElements,f.numEnumElements<12 ? f.numEnumElements : -1);
    }
        break;
    case FT_STRING: {
        char* txtField = (char*)  f.pdata;
        const int flags = f.numArrayElements;
        const float maxHeight =(float) (f.maxValue<0 ? 0 : f.maxValue);
        const bool multiline = f.needsRadiansToDegs;
        ImGui::Text("%s",label);
        float width = nodeWidth;
        if (flags<0) {
            //ImVec2 pos = ImGui::GetCursorScreenPos();
            const float startPos = ImGui::GetCursorPos().x + width;
            if (startPos>0)    {
                ImGui::PushTextWrapPos(startPos);
                ImGui::Text(txtField, width);
                //ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255,255,0,255));
                ImGui::PopTextWrapPos();
            }
        }
        else if (!multiline) {
            const bool addBrowseButton = (f.minValue==-500);
            float browseBtnWidth = 0;
            if (addBrowseButton) {
                browseBtnWidth = ImGui::CalcTextSize("...").x + 10;
                if (width-browseBtnWidth>0) width-=browseBtnWidth;
            }
            ImGui::PushItemWidth(width);
            changed|=ImGui::InputText("##DummyLabelInputText",txtField,f.precision,flags);
            ImGui::PopItemWidth();
            if (addBrowseButton)	{
		if (/*f.tooltip &&*/ f.tooltip[0]!='\0' && ImGui::IsItemHovered()) NodeGraphEditorSetTooltip(f.tooltip);
                skipTooltip = true;
                ImGui::SameLine(0,4);
                ImGui::PushItemWidth(browseBtnWidth);
                if (ImGui::Button("...##DummyLabelInputTextBrowseButton")) {
                    changed = true;
                    widgetIndex = 1;
                }
                ImGui::PopItemWidth();
		//if (ImGui::IsItemHovered()) NodeGraphEditorSetTooltip("Browse");
            }
        }
        else changed|=ImGui::InputTextMultiline("##DummyLabelInputText",txtField,f.precision,ImVec2(width,maxHeight),flags);
    }
        break;
    case FT_COLOR:  {
        float* pColor = (float*) f.pdata;
        if (f.numArrayElements==3) changed|=ImGui::ColorEdit3(label,pColor);//,ImGuiColorEditFlags_NoAlpha);
        else changed|=ImGui::ColorEdit4(label,pColor);//,ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
    }
    case FT_CUSTOM: {
        if (f.renderFieldDelegate) changed = f.renderFieldDelegate(f);
    }
        break;
    default:
        IM_ASSERT(true);    // should never happen
        break;
    }
    if (!skipTooltip && f.type!=FT_CUSTOM)  {
	if (/*f.tooltip &&*/ f.tooltip[0]!='\0' && ImGui::IsItemHovered()) NodeGraphEditorSetTooltip(f.tooltip);
    }
    if (changed && f.editedFieldDelegate) f.editedFieldDelegate(f,widgetIndex);
    ImGui::PopID();
    return changed;
}

//-------------------------------------------------------------------------------
#       if (defined(IMGUIHELPER_H_) && !defined(NO_IMGUIHELPER_SERIALIZATION))
#       ifndef NO_IMGUIHELPER_SERIALIZATION_SAVE
bool NodeGraphEditor::save(ImGuiHelper::Serializer& s)    {
    if (!s.isValid()) return false;
    const int numNodes = nodes.size();
    const int numLinks = links.size();
    int itmp;
    char ovrBuffer[((IMGUINODE_MAX_INPUT_SLOTS>IMGUINODE_MAX_OUTPUT_SLOTS?IMGUINODE_MAX_INPUT_SLOTS:IMGUINODE_MAX_OUTPUT_SLOTS)+1)*IMGUINODE_MAX_SLOT_NAME_LENGTH*200];
    //--------------------------------------------
    s.save(&scrolling.x,"scrolling",2);
    s.save(&numNodes,"num_nodes");
    itmp = activeNode ? getNodeIndex(activeNode) : -1;s.save(&itmp,"selected_node_index");
    s.save(&numLinks,"num_links");
    for (int i=0;i<numNodes;i++)    {
	const Node& n = (*nodes[i]);
	s.save(&i,"node_index");
	s.save(&n.typeID,"typeID");
	s.save(&n.userID,"userID");
	s.save(&n.Pos.x,"Pos",2);
	s.save(&n.isOpen,"isOpen");
    s.save(&n.isSelected,"isSelected");
    if (n.mustOverrideName) s.save(n.Name,"OvrName");
    if (n.overrideTitleTextColor!=0) s.save(&n.overrideTitleTextColor,"OvrTitleTextColor");
    if (n.overrideTitleBgColor!=0) s.save(&n.overrideTitleBgColor,"OvrTitleBgColor");
    if (n.overrideTitleBgColorGradient>=0.f) s.save(&n.overrideTitleBgColorGradient,"OvrTitleBgColorGradient");
    if (n.mustOverrideInputSlots)   {
        ovrBuffer[0]='\0';
        for (int st=0;st<n.InputsCount;st++) {
            if (st>0) strcat(ovrBuffer,";");
            strcat(ovrBuffer,&n.InputNames[st][0]);
        }
        s.save(ovrBuffer,"OvrInputSlots");
    }
    if (n.mustOverrideOutputSlots)   {
        ovrBuffer[0]='\0';
        for (int st=0;st<n.OutputsCount;st++) {
            if (st>0) strcat(ovrBuffer,";");
            strcat(ovrBuffer,&n.OutputNames[st][0]);
        }
        s.save(ovrBuffer,"OvrOutputSlots");
    }
	itmp = n.fields.size();s.save(&itmp,"numFields");
	n.fields.serialize(s);
    }
    for (int i=0;i<numLinks;i++)    {
	const NodeLink& l = links[i];
	s.save(&i,"link_index");
	itmp = getNodeIndex(l.InputNode);
	s.save(&itmp,"InputNode");
	s.save(&l.InputSlot,"InputSlot");
	itmp = getNodeIndex(l.OutputNode);
	s.save(&itmp,"OutputNode");
	s.save(&l.OutputSlot,"OutputSlot");
    }    
    //--------------------------------------------
    setModified(false);
    return true;
}
#       endif //NO_IMGUIHELPER_SERIALIZATION_SAVE
#       ifndef NO_IMGUIHELPER_SERIALIZATION_LOAD
struct NodeGraphEditorParseCallback1Struct {
    ImVec2 scrolling;
    int numNodes,selectedNodeIndex,numLinks;
    NodeGraphEditorParseCallback1Struct() : scrolling(0,0),numNodes(0),selectedNodeIndex(-1),numLinks(0) {}
};
static bool NodeGraphEditorParseCallback1(ImGuiHelper::FieldType /*ft*/,int /*numArrayElements*/,void* pValue,const char* name,void* userPtr)    {
    NodeGraphEditorParseCallback1Struct* cbs = (NodeGraphEditorParseCallback1Struct*) userPtr;
    if (strcmp(name,"scrolling")==0) cbs->scrolling = *((ImVec2*)pValue);
    else if (strcmp(name,"num_nodes")==0) cbs->numNodes = *((int*)pValue);
    else if (strcmp(name,"selected_node_index")==0) cbs->selectedNodeIndex = *((int*)pValue);
    else if (strcmp(name,"num_links")==0) {cbs->numLinks = *((int*)pValue);return true;}
    return false;
}
struct NodeGraphEditorParseCallback2Struct {
    int curNodeIndex,typeID,numFields,userID;bool isOpen;bool isSelected;
    ImVec2 Pos;
    bool mustOvrName,mustOvrInput,mustOvrOutput;
    char ovrName[IMGUINODE_MAX_NAME_LENGTH];
    ImU32 overrideTitleTextColor,overrideTitleBgColor;
    float overrideTitleBgColorGradient;
    char ovrInput[(IMGUINODE_MAX_INPUT_SLOTS+1)*IMGUINODE_MAX_SLOT_NAME_LENGTH*200];
    char ovrOutput[(IMGUINODE_MAX_OUTPUT_SLOTS+1)*IMGUINODE_MAX_SLOT_NAME_LENGTH*200];
    NodeGraphEditorParseCallback2Struct() : curNodeIndex(-1),typeID(-1),numFields(0),userID(-1),isOpen(false),isSelected(false),Pos(0,0),
    mustOvrName(false),mustOvrInput(false),mustOvrOutput(false),
    overrideTitleTextColor(0),overrideTitleBgColor(0),overrideTitleBgColorGradient(-1.f)
    {ovrName[0]='\0';ovrInput[0]='\0';ovrOutput[0]='\0';}
};
static bool NodeGraphEditorParseCallback2(ImGuiHelper::FieldType /*ft*/,int numArrayElements,void* pValue,const char* name,void* userPtr)    {
    NodeGraphEditorParseCallback2Struct* cbs = (NodeGraphEditorParseCallback2Struct*) userPtr;
    if (strcmp(name,"node_index")==0)   cbs->curNodeIndex = *((int*)pValue);
    else if (strcmp(name,"typeID")==0)  cbs->typeID = *((int*)pValue);
    else if (strcmp(name,"userID")==0)  cbs->userID = *((int*)pValue);
    else if (strcmp(name,"Pos")==0)     cbs->Pos = *((ImVec2*)pValue);
    else if (strcmp(name,"isOpen")==0)  cbs->isOpen = *((bool*)pValue);
    else if (strcmp(name,"isSelected")==0)  cbs->isSelected = *((bool*)pValue);
    else if (strcmp(name,"OvrName")==0) {
        cbs->mustOvrName = true;
        int maxLen = numArrayElements > IMGUINODE_MAX_NAME_LENGTH ? IMGUINODE_MAX_NAME_LENGTH : numArrayElements;
        strncpy(cbs->ovrName,(const char*)pValue,maxLen);
        cbs->ovrName[IMGUINODE_MAX_NAME_LENGTH-1]='\0';
    }
    else if (strcmp(name,"OvrTitleTextColor")==0)       cbs->overrideTitleTextColor         = *((const ImU32*)pValue);
    else if (strcmp(name,"OvrTitleBgColor")==0)         cbs->overrideTitleBgColor           = *((const ImU32*)pValue);
    else if (strcmp(name,"OvrTitleBgColorGradient")==0) cbs->overrideTitleBgColorGradient   = *((const float*)pValue);
    else if (strcmp(name,"OvrInputSlots")==0) {
        cbs->mustOvrInput = true;
        int maxLen = numArrayElements > (int) sizeof(cbs->ovrInput) ? (int) sizeof(cbs->ovrInput) : numArrayElements;
        strncpy(cbs->ovrInput,(const char*)pValue,maxLen);
        cbs->ovrInput[sizeof(cbs->ovrInput)-1]='\0';
    }
    else if (strcmp(name,"OvrOutputSlots")==0) {
        cbs->mustOvrOutput = true;
        int maxLen = numArrayElements > (int) sizeof(cbs->ovrOutput) ? (int) sizeof(cbs->ovrOutput) : numArrayElements;
        strncpy(cbs->ovrOutput,(const char*)pValue,maxLen);
        cbs->ovrOutput[sizeof(cbs->ovrOutput)-1]='\0';
    }
    else if (strcmp(name,"numFields")==0) {
        cbs->numFields = *((int*)pValue);return true;
    }
    return false;
}
struct NodeGraphEditorParseCallback3Struct {
    int link_index,node1_index,input_slot,node2_index,output_slot;
    NodeGraphEditorParseCallback3Struct() : link_index(-1),node1_index(-1),input_slot(-1),node2_index(-1),output_slot(-1) {}
};
static bool NodeGraphEditorParseCallback3(ImGuiHelper::FieldType /*ft*/,int /*numArrayElements*/,void* pValue,const char* name,void* userPtr)    {
    NodeGraphEditorParseCallback3Struct* cbl = (NodeGraphEditorParseCallback3Struct*) userPtr;
    if (strcmp(name,"link_index")==0)   cbl->link_index = *((int*)pValue);
    else if (strcmp(name,"InputNode")==0)  cbl->node1_index = *((int*)pValue);
    else if (strcmp(name,"InputSlot")==0)  cbl->input_slot = *((int*)pValue);
    else if (strcmp(name,"OutputNode")==0)  cbl->node2_index = *((int*)pValue);
    else if (strcmp(name,"OutputSlot")==0)  {
        cbl->output_slot = *((int*)pValue);return true;
    }
    return false;
}
bool NodeGraphEditor::load(ImGuiHelper::Deserializer& d, const char ** pOptionalBufferStart)    {
    if (!d.isValid() || !nodeFactoryFunctionPtr) return false;
    clear();
    setModified(false);
    //--------------------------------------------
    const char* amount = pOptionalBufferStart ? (*pOptionalBufferStart) : 0;
    NodeGraphEditorParseCallback1Struct cbs;
    amount = d.parse(NodeGraphEditorParseCallback1,(void*)&cbs,amount);
    scrolling = cbs.scrolling;
    for (int i=0;i<cbs.numNodes;i++)    {
        NodeGraphEditorParseCallback2Struct cbn;
        amount = d.parse(NodeGraphEditorParseCallback2,(void*)&cbn,amount);
        // TODO: do some checks on cbn
        Node* n = nodeFactoryFunctionPtr(cbn.typeID,cbn.Pos,*this);
        n->userID = cbn.userID;
        n->isOpen = cbn.isOpen;
        n->isSelected = cbn.isSelected;
        if (cbn.mustOvrName)    overrideNodeName(n,cbn.ovrName);
        overrideNodeTitleBarColors(n,cbn.overrideTitleTextColor>0 ? &cbn.overrideTitleTextColor : NULL,
                                   cbn.overrideTitleBgColor>0 ? &cbn.overrideTitleBgColor : NULL,
                                   cbn.overrideTitleBgColorGradient>=0 ? &cbn.overrideTitleBgColorGradient : NULL);
        if (cbn.mustOvrInput)   overrideNodeInputSlots(n,cbn.ovrInput);
        if (cbn.mustOvrOutput)  overrideNodeInputSlots(n,cbn.ovrOutput);
        IM_ASSERT(n->fields.size()==cbn.numFields); // optional check (to remove)
        amount = n->fields.deserialize(d,amount);        
        addNode(n);
        AvailableNodeInfo* pOptionalNi = fetchAvailableNodeInfo(n->getType());
        if (pOptionalNi) ++(pOptionalNi->curNumInstances);
    }
    if (cbs.selectedNodeIndex>=0 && cbs.selectedNodeIndex<nodes.size()) activeNode = nodes[cbs.selectedNodeIndex];
    for (int i=0;i<cbs.numLinks;i++)    {
        NodeGraphEditorParseCallback3Struct cbl;
        amount = d.parse(NodeGraphEditorParseCallback3,(void*)&cbl,amount);
        if (cbl.node1_index>=0 && cbl.node1_index<nodes.size() && cbl.input_slot>=0 &&
                cbl.node2_index>=0 && cbl.node2_index<nodes.size() && cbl.output_slot>=0 &&
                cbl.node1_index!=cbl.node2_index
        ) addLink(nodes[cbl.node1_index],cbl.input_slot,nodes[cbl.node2_index],cbl.output_slot,true); // last arg check if link is already present before adding it
    }
    // Fire node->onLoad() events-----------------
    for (int i=0,isz=nodes.size();i<isz;i++) {
	nodes[i]->onLoaded();
    }
    maxConnectorNameWidth = 0;
    oldFontWindowScale = 0;
    //--------------------------------------------
    if (pOptionalBufferStart) *pOptionalBufferStart = amount;
    setModified(false);
    return true;
    //--------------------------------------------
    //return true;
}
#       endif //NO_IMGUIHELPER_SERIALIZATION_LOAD
#       endif //NO_IMGUIHELPER_SERIALIZATION
//--------------------------------------------------------------------------------

}   // namespace ImGui





#ifndef IMGUINODEGRAPHEDITOR_NOTESTDEMO

#ifndef NO_IMGUIFILESYSTEM
#include "../imguifilesystem/imguifilesystem.h"
#endif //NO_IMGUIFILESYSTEM

namespace ImGui	{

enum MyNodeTypes {
    MNT_COLOR_NODE = 0,
    MNT_COMBINE_NODE,
    MNT_COMMENT_NODE,
    MNT_COMPLEX_NODE,
#   ifdef IMGUI_USE_AUTO_BINDING
    MNT_TEXTURE_NODE,
#   endif
    MNT_OUTPUT_NODE,    // One problem here when adding new values is backward compatibility with old saved files: they rely on the previously used int values (it should be OK only if we append new values at the end).
    MNT_COUNT
};
// used in the "add Node" menu (and optionally as node title names)
static const char* MyNodeTypeNames[MNT_COUNT] = {"Color","Combine","Comment","Complex"
#						ifdef IMGUI_USE_AUTO_BINDING
						 ,"Texture"
#						endif
                        ,"Output"
						};
class ColorNode : public Node {
    protected:
    typedef Node Base;  //Base Class
    typedef ColorNode ThisClass;
    ColorNode() : Base() {}
    static const int TYPE = MNT_COLOR_NODE;

    ImVec4 Color;       // field

    // Support static method for enumIndex (the signature is the same used by ImGui::Combo(...))
    static bool GetTextFromEnumIndex(void* ,int value,const char** pTxt) {
        if (!pTxt) return false;
        static const char* values[] = {"APPLE","LEMON","ORANGE"};
        static int numValues = (int)(sizeof(values)/sizeof(values[0]));
        if (value>=0 && value<numValues) *pTxt = values[value];
        else *pTxt = "UNKNOWN";
        return true;
    }

    virtual const char* getTooltip() const {return "ColorNode tooltip.";}
    virtual const char* getInfo() const {return "ColorNode info.\n\nThis is supposed to display some info about this node.";}
    /*virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut,ImU32& defaultTitleBgColorOut,float& defaultTitleBgColorGradientOut) const {
        // [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
        defaultTitleTextColorOut = IM_COL32(220,220,220,255);defaultTitleBgColorOut = IM_COL32(0,75,0,255);defaultTitleBgColorGradientOut = -1.f;
    }*/

    public:

    // create:
    static ThisClass* Create(const ImVec2& pos) {
        // 1) allocation
        // MANDATORY (NodeGraphEditor::~NodeGraphEditor() will delete these with ImGui::MemFree(...))
	// MANDATORY even with blank ctrs. Reason: ImVector does not call ctrs/dctrs on items.
	ThisClass* node = (ThisClass*) ImGui::MemAlloc(sizeof(ThisClass));IM_PLACEMENT_NEW (node) ThisClass();

        // 2) main init
        node->init("ColorNode",pos,"","r;g;b;a",TYPE);

        // 3) init fields ( this uses the node->fields variable; otherwise we should have overridden other virtual methods (to render and serialize) )
        node->fields.addFieldColor(&node->Color.x,true,"Color","color with alpha");

        // 4) set (or load) field values
        node->Color = ImColor(255,255,0,255);

        return node;
    }


    // casts:
    inline static ThisClass* Cast(Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
    inline static const ThisClass* Cast(const Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
};
class CombineNode : public Node {
    protected:
    typedef Node Base;  //Base Class
    typedef CombineNode ThisClass;
    CombineNode() : Base() {}
    static const int TYPE = MNT_COMBINE_NODE;

    float fraction;

    virtual const char* getTooltip() const {return "CombineNode tooltip.";}
    virtual const char* getInfo() const {return "CombineNode info.\n\nThis is supposed to display some info about this node.";}
    /*virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut,ImU32& defaultTitleBgColorOut,float& defaultTitleBgColorGradientOut) const {
        // [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
        defaultTitleTextColorOut = IM_COL32(220,220,220,255);defaultTitleBgColorOut = IM_COL32(0,75,0,255);defaultTitleBgColorGradientOut = -1.f;
    }*/

    public:

    // create:
    static ThisClass* Create(const ImVec2& pos) {
        // 1) allocation
        // MANDATORY (NodeGraphEditor::~NodeGraphEditor() will delete these with ImGui::MemFree(...))
	// MANDATORY even with blank ctrs. Reason: ImVector does not call ctrs/dctrs on items.
	ThisClass* node = (ThisClass*) ImGui::MemAlloc(sizeof(ThisClass));IM_PLACEMENT_NEW(node) ThisClass();

        // 2) main init
        node->init("CombineNode",pos,"in1;in2","out",TYPE);

        // 3) init fields ( this uses the node->fields variable; otherwise we should have overridden other virtual methods (to render and serialize) )
	node->fields.addField(&node->fraction,1,"Fraction","Fraction of in1 that is mixed with in2",2,0,1);

        // 4) set (or load) field values
        node->fraction = 0.5f;

        return node;
    }

    // casts:
    inline static ThisClass* Cast(Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
    inline static const ThisClass* Cast(const Node* n) {return Node::Cast<ThisClass>(n,TYPE);}


};
class CommentNode : public Node {
    protected:
    typedef Node Base;  //Base Class
    typedef CommentNode ThisClass;
    CommentNode() : Base() {}
    static const int TYPE = MNT_COMMENT_NODE;
    static const int TextBufferSize = 128;

    char comment[TextBufferSize];			    // field 1
    char comment2[TextBufferSize];			    // field 2
    char comment3[TextBufferSize];			    // field 3
    char comment4[TextBufferSize];			    // field 4
    bool flag;                                  // field 5

    virtual const char* getTooltip() const {return "CommentNode tooltip.";}
    virtual const char* getInfo() const {return "CommentNode info.\n\nThis is supposed to display some info about this node.";}
    /*virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut,ImU32& defaultTitleBgColorOut,float& defaultTitleBgColorGradientOut) const {
        // [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
        defaultTitleTextColorOut = IM_COL32(220,220,220,255);defaultTitleBgColorOut = IM_COL32(0,75,0,255);defaultTitleBgColorGradientOut = -1.f;
    }*/

    public:

    // create:
    static ThisClass* Create(const ImVec2& pos) {
	// 1) allocation
	// MANDATORY (NodeGraphEditor::~NodeGraphEditor() will delete these with ImGui::MemFree(...))
	// MANDATORY even with blank ctrs. Reason: ImVector does not call ctrs/dctrs on items.
	ThisClass* node = (ThisClass*) ImGui::MemAlloc(sizeof(ThisClass));IM_PLACEMENT_NEW(node) ThisClass();

	// 2) main init
	node->init("CommentNode",pos,"","",TYPE);
	node->baseWidthOverride = 200.f;    // (optional) default base node width is 120.f;


	// 3) init fields ( this uses the node->fields variable; otherwise we should have overridden other virtual methods (to render and serialize) )
	node->fields.addFieldTextEdit(		&node->comment[0],TextBufferSize,"Single Line","A single line editable field",ImGuiInputTextFlags_EnterReturnsTrue);
	node->fields.addFieldTextEditMultiline(&node->comment2[0],TextBufferSize,"Multi Line","A multi line editable field",ImGuiInputTextFlags_AllowTabInput,50);
	node->fields.addFieldTextEditMultiline(&node->comment3[0],TextBufferSize,"Multi Line 2","A multi line read-only field",ImGuiInputTextFlags_ReadOnly,50);
	node->fields.addFieldTextWrapped(      &node->comment4[0],TextBufferSize,"Text Wrapped ReadOnly","A text wrapped field");
	node->fields.addField(&node->flag,"Flag","A boolean field");

	// 4) set (or load) field values
	strcpy(node->comment,"Initial Text Line.");
	strcpy(node->comment2,"Initial Text Multiline.");
	static const char* tiger = "Tiger, tiger, burning bright\nIn the forests of the night,\nWhat immortal hand or eye\nCould frame thy fearful symmetry?";
	strncpy(node->comment3,tiger,TextBufferSize);
	static const char* txtWrapped = "I hope this text gets wrapped gracefully. But I'm not sure about it.";
	strncpy(node->comment4,txtWrapped,TextBufferSize);
	node->flag = true;

	return node;
    }

    // helper casts:
    inline static ThisClass* Cast(Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
    inline static const ThisClass* Cast(const Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
};
class ComplexNode : public Node {
    protected:
    typedef Node Base;  //Base Class
    typedef ComplexNode ThisClass;
    ComplexNode() : Base() {}
    static const int TYPE = MNT_COMPLEX_NODE;

    float Value[3];     // field 1
    ImVec4 Color;       // field 2
    int enumIndex;      // field 3

    // Support static method for enumIndex (the signature is the same used by ImGui::Combo(...))
    static bool GetTextFromEnumIndex(void* ,int value,const char** pTxt) {
        if (!pTxt) return false;
        static const char* values[] = {"APPLE","LEMON","ORANGE"};
        static int numValues = (int)(sizeof(values)/sizeof(values[0]));
        if (value>=0 && value<numValues) *pTxt = values[value];
        else *pTxt = "UNKNOWN";
        return true;
    }

    virtual const char* getTooltip() const {return "ComplexNode tooltip.";}
    virtual const char* getInfo() const {return "ComplexNode info.\n\nThis is supposed to display some info about this node.";}
    virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut,ImU32& defaultTitleBgColorOut,float& defaultTitleBgColorGradientOut) const {
        // [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
        defaultTitleTextColorOut = IM_COL32(220,220,220,255);defaultTitleBgColorOut = IM_COL32(125,35,0,255);defaultTitleBgColorGradientOut = -1.f;
    }

    public:

    // create:
    static ThisClass* Create(const ImVec2& pos) {
        // 1) allocation
        // MANDATORY (NodeGraphEditor::~NodeGraphEditor() will delete these with ImGui::MemFree(...))
        // MANDATORY even with blank ctrs.  Reason: ImVector does not call ctrs/dctrs on items.
        ThisClass* node = (ThisClass*) ImGui::MemAlloc(sizeof(ThisClass));IM_PLACEMENT_NEW(node) ThisClass();

        // 2) main init
        node->init("ComplexNode",pos,"in1;in2;in3","out1;out2",TYPE);

        // 3) init fields ( this uses the node->fields variable; otherwise we should have overridden other virtual methods (to render and serialize) )
        node->fields.addField(&node->Value[0],3,"Angles","Three floats that are stored in radiant units internally",2,0,360,NULL,true);
        node->fields.addFieldColor(&node->Color.x,true,"Color","color with alpha");
        // addFieldEnum(...) now has all the 3 overloads ImGui::Combo(...) has.
        // addFieldEnum(...) [1] (item_count + external callback)
        node->fields.addFieldEnum(&node->enumIndex,3,&GetTextFromEnumIndex,"Fruit","Choose your favourite");
        // addFieldEnum(...) [2] (items_count + item_names)
        //static const char* FruitNames[3] = {"APPLE","LEMON","ORANGE"};
        //node->fields.addFieldEnum(&node->enumIndex,3,FruitNames,"Fruit","Choose your favourite");
        // addFieldEnum(...) [3] (zero_separated_item_names)
        //node->fields.addFieldEnum(&node->enumIndex,"APPLE\0LEMON\0ORANGE\0\0","Fruit","Choose your favourite");


        // 4) set (or load) field values
        node->Value[0] = 0;node->Value[1] = 3.14f; node->Value[2] = 4.68f;
        node->Color = ImColor(126,200,124,230);
        node->enumIndex = 1;

        return node;
    }

    // helper casts:
    inline static ThisClass* Cast(Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
    inline static const ThisClass* Cast(const Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
};
#ifdef IMGUI_USE_AUTO_BINDING
class TextureNode : public Node {
    protected:
    typedef Node Base;  //Base Class
    typedef TextureNode ThisClass;
    TextureNode() : Base() {}
    virtual ~TextureNode() {if (textureID) {ImImpl_FreeTexture(textureID);}}
    static const int TYPE = MNT_TEXTURE_NODE;
    static const int TextBufferSize =
#   ifndef NO_IMGUIFILESYSTEM
    ImGuiFs::MAX_PATH_BYTES;
#   else
    2049;
#   endif

    ImTextureID textureID;
    char imagePath[TextBufferSize];				// field 1 (= the only one that is copied/serialized/handled by the Node)
    char lastValidImagePath[TextBufferSize];    // The path for which "textureID" was created
    bool startBrowseDialogNextFrame;
#   ifndef NO_IMGUIFILESYSTEM
    ImGuiFs::Dialog dlg;
#   endif //NO_IMGUIFILESYSTEM

    virtual const char* getTooltip() const {return "TextureNode tooltip.";}
    virtual const char* getInfo() const {return "TextureNode info.\n\nThis is supposed to display some info about this node.";}
    virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut,ImU32& defaultTitleBgColorOut,float& defaultTitleBgColorGradientOut) const {
        // [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
        defaultTitleTextColorOut = IM_COL32(220,220,220,255);defaultTitleBgColorOut = IM_COL32(105,105,0,255);defaultTitleBgColorGradientOut = -1.f;
    }

    public:

    // create (main method of this class):
    static ThisClass* Create(const ImVec2& pos) {
	// 1) allocation
	// MANDATORY (NodeGraphEditor::~NodeGraphEditor() will delete these with ImGui::MemFree(...))
	// MANDATORY even with blank ctrs.  Reason: ImVector does not call ctrs/dctrs on items.
	ThisClass* node = (ThisClass*) ImGui::MemAlloc(sizeof(ThisClass));
	IM_PLACEMENT_NEW(node) ThisClass();

	// 2) main init
	node->init("TextureNode",pos,"","r;g;b;a",TYPE);
	node->baseWidthOverride = 150.f;    // (optional) default base node width is 120.f;

	// 3) init fields ( this uses the node->fields variable; otherwise we should have overridden other virtual methods (to render and serialize) )
	FieldInfo* f=NULL;
#	ifndef NO_IMGUIFILESYSTEM
	f=&node->fields.addFieldTextEditAndBrowseButton(&node->imagePath[0],TextBufferSize,"Image Path:","A valid image path: press RETURN to validate or browse manually.",ImGuiInputTextFlags_EnterReturnsTrue,(void*) node);
#	else	//NO_IMGUIFILESYSTEM
	f=&node->fields.addFieldTextEdit(&node->imagePath[0],TextBufferSize,"Image Path:","A valid image path: press RETURN to validate or browse manually.",ImGuiInputTextFlags_EnterReturnsTrue,(void*) node);
#	endif //NO_IMGUIFILESYSTEM
    f->editedFieldDelegate = &ThisClass::StaticEditFieldCallback;   // we set an "edited callback" to this node field. It is fired soon (as opposed to other "outer" edited callbacks), but we have chosen: ImGuiInputTextFlags_EnterReturnsTrue.
	node->startBrowseDialogNextFrame = false;

	// 4) set (or load) field values
	node->textureID = 0;
	node->imagePath[0] = node->lastValidImagePath[0] = '\0';

	return node;
    }

    // helper casts:
    inline static ThisClass* Cast(Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
    inline static const ThisClass* Cast(const Node* n) {return Node::Cast<ThisClass>(n,TYPE);}

    protected:
    bool render(float nodeWidth)   {
	const bool changed = Base::render(nodeWidth);
#	ifndef NO_IMGUIFILESYSTEM
	const char* filePath = dlg.chooseFileDialog(startBrowseDialogNextFrame,dlg.getLastDirectory(),".jpg;.jpeg;.png;.gif;.tga;.bmp");
	if (strlen(filePath)>0) {
	    //fprintf(stderr,"Browsed..: %s\n",filePath);
	    strcpy(imagePath,filePath);
	    processPath(imagePath);
	}
#	endif //NO_IMGUIFILESYSTEM
	startBrowseDialogNextFrame = false;
	//----------------------------------------------------
	// draw textureID:
	ImGui::Image(reinterpret_cast<void*>(textureID),ImVec2(nodeWidth,nodeWidth));
	//----------------------------------------------------
	return changed;
    }

    void processPath(const char* filePath)  {
        if (!filePath || strcmp(filePath,lastValidImagePath)==0) return;
        if (!ValidateImagePath(filePath)) return;
        if (textureID) {ImImpl_FreeTexture(textureID);}
        textureID = ImImpl_LoadTexture(filePath);
        if (textureID) strcpy(lastValidImagePath,filePath);
    }

    // When the node is loaded from file or copied from another node, only the text field (="imagePath") is copied, so we must recreate "textureID":
    void onCopied() {
        processPath(imagePath);
    }
    void onLoaded() {
        processPath(imagePath);
    }
    //bool canBeCopied() const {return false;}	// Just for testing... TO REMOVE!

    void onEditField(FieldInfo& /*f*/,int widgetIndex) {
        //fprintf(stderr,"TextureNode::onEditField(\"%s\",%i);\n",f.label,widgetIndex);
        if (widgetIndex==1)         startBrowseDialogNextFrame = true;  // browsing button pressed
        else if (widgetIndex==0)    processPath(imagePath);             // text edited (= "return" pressed in out case)
    }
    static void StaticEditFieldCallback(FieldInfo& f,int widgetIndex) {
        reinterpret_cast<ThisClass*>(f.userData)->onEditField(f,widgetIndex);
    }

    static bool ValidateImagePath(const char* path) {
        if (!path || strlen(path)==0) return false;

        // TODO: check extension

        FILE* f = ImFileOpen(path,"rb");
        if (f) {fclose(f);f=NULL;return true;}

        return false;
    }

};
#endif //IMGUI_USE_AUTO_BINDING
class OutputNode : public Node {
    protected:
    typedef Node Base;  //Base Class
    typedef OutputNode ThisClass;
    OutputNode() : Base() {}
    static const int TYPE = MNT_OUTPUT_NODE;

    // No field values in this class

    virtual const char* getTooltip() const {return "OutputNode tooltip.";}
    virtual const char* getInfo() const {return "OutputNode info.\n\nThis is supposed to display some info about this node.";}
    virtual void getDefaultTitleBarColors(ImU32& defaultTitleTextColorOut,ImU32& defaultTitleBgColorOut,float& defaultTitleBgColorGradientOut) const {
        // [Optional Override] customize Node Title Colors [default values: 0,0,-1.f => do not override == use default values from the Style()]
        defaultTitleTextColorOut = IM_COL32(230,180,180,255);defaultTitleBgColorOut = IM_COL32(40,55,55,200);defaultTitleBgColorGradientOut = 0.025f;
    }
    virtual bool canBeCopied() const {return false;}

    public:

    // create:
    static ThisClass* Create(const ImVec2& pos) {
        // 1) allocation
        // MANDATORY (NodeGraphEditor::~NodeGraphEditor() will delete these with ImGui::MemFree(...))
    // MANDATORY even with blank ctrs. Reason: ImVector does not call ctrs/dctrs on items.
    ThisClass* node = (ThisClass*) ImGui::MemAlloc(sizeof(ThisClass));IM_PLACEMENT_NEW (node) ThisClass();

        // 2) main init
        node->init("OutputNode",pos,"ch1;ch2;ch3;ch4","",TYPE);

        // 3) init fields ( this uses the node->fields variable; otherwise we should have overridden other virtual methods (to render and serialize) )

        // 4) set (or load) field values

        return node;
    }

    // casts:
    inline static ThisClass* Cast(Node* n) {return Node::Cast<ThisClass>(n,TYPE);}
    inline static const ThisClass* Cast(const Node* n) {return Node::Cast<ThisClass>(n,TYPE);}

    protected:
    bool render(float /*nodeWidth*/)   {
        ImGui::Text("There can be a single\ninstance of this class.\nTry and see if it's true!");
        return false;
    }
};

static Node* MyNodeFactory(int nt,const ImVec2& pos,const NodeGraphEditor& /*nge*/) {
    switch (nt) {
    case MNT_COLOR_NODE: return ColorNode::Create(pos);
    case MNT_COMBINE_NODE: return CombineNode::Create(pos);
    case MNT_COMMENT_NODE: return CommentNode::Create(pos);
    case MNT_COMPLEX_NODE: return ComplexNode::Create(pos);
    case MNT_OUTPUT_NODE: return OutputNode::Create(pos);
#   ifdef IMGUI_USE_AUTO_BINDING
    case MNT_TEXTURE_NODE: return TextureNode::Create(pos);
#   endif //IMGUI_USE_AUTO_BINDING
    default:
    IM_ASSERT(true);    // Missing node type creation
    return NULL;
    }
    return NULL;
}
void TestNodeGraphEditor()  {
    static ImGui::NodeGraphEditor nge;
    if (nge.isInited())	{
        // This adds entries to the "add node" context menu
        nge.registerNodeTypes(MyNodeTypeNames,MNT_COUNT,MyNodeFactory,NULL,-1); // last 2 args can be used to add only a subset of nodes (or to sort their order inside the context menu)
        // The line above can be replaced by the following two lines, if we want to use only an active subset of the available node types:
        //const int optionalNodeTypesToUse[] = {MNT_COMPLEX_NODE,MNT_COMMENT_NODE,MNT_OUTPUT_NODE};
        //nge.registerNodeTypes(MyNodeTypeNames,MNT_COUNT,MyNodeFactory,optionalNodeTypesToUse,sizeof(optionalNodeTypesToUse)/sizeof(optionalNodeTypesToUse[0]));
        nge.registerNodeTypeMaxAllowedInstances(MNT_OUTPUT_NODE,1); // Here we set the max number of allowed instances of the output node (1)

        // Optional: starting nodes and links (TODO: load from file instead):-----------
        ImGui::Node* colorNode = nge.addNode(MNT_COLOR_NODE,ImVec2(40,50));
        ImGui::Node* complexNode =  nge.addNode(MNT_COMPLEX_NODE,ImVec2(40,150));
        ImGui::Node* combineNode =  nge.addNode(MNT_COMBINE_NODE,ImVec2(275,80)); // optionally use e.g.: ImGui::CombineNode::Cast(combineNode)->fraction = 0.8f;
        ImGui::Node* outputNode =  nge.addNode(MNT_OUTPUT_NODE,ImVec2(520,140));
        // Return values can be NULL (if node types are not registered or their instance limit has been already reached).
        //nge.overrideNodeName(combineNode,"CombineNodeCustomName");  // Test only (to remove)
        //nge.overrideNodeInputSlots(combineNode,"in1;in2;in3;in4");  // Test only (to remove)
        //ImU32 bg = IM_COL32(0,128,0,255);nge.overrideNodeTitleBarColors(combineNode,NULL,&bg,NULL);  // Test only (to remove)
        //nge.overrideNodeTitleBarColors(complexNode,NULL,&bg,NULL);  // Test only (to remove)
        // addLink(...) should be robust enough to handle NULL nodes, so we don't check it.
        nge.addLink(colorNode, 0, combineNode, 0);
        nge.addLink(complexNode, 1, combineNode, 1);
        nge.addLink(complexNode, 0, outputNode, 1);
        nge.addLink(combineNode, 0, outputNode, 0);
        //-------------------------------------------------------------------------------
        //nge.load("nodeGraphEditor.nge");  // Please note than if the saved graph has nodes out of our active subset, they will be displayed as usual (it's not clear what should be done in this case: hope that's good enough, it's a user's mistake).
        //-------------------------------------------------------------------------------
        nge.show_style_editor = true;
        nge.show_load_save_buttons = true;
        // optional load the style (for all the editors: better call it in InitGL()):
        //NodeGraphEditor::Style::Load(NodeGraphEditor::GetStyle(),"nodeGraphEditor.style");
        //--------------------------------------------------------------------------------
    }
    nge.render();
}

}	//nmespace ImGui

#endif //IMGUINODEGRAPHEDITOR_NOTESTDEMO



