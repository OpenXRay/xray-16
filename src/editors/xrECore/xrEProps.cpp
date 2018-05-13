#include "pch.hpp"
#include "xrEProps.h"
#include "Controls/NumericVector.h"
#include "Controls/ShaderFunction.h"
#include "Controls/GameType.h"
#include "Controls/TextEdit.h"
#include "xrEngine/WaveForm.h"

namespace XRay::Editor::Controls
{
bool NumericVectorRun(pcstr title, Fvector* data, int decimal, Fvector* reset_value, Fvector* min, Fvector* max, int* X, int* Y)
{
    auto form = gcnew NumericVector();
    return form->Run(title, data, decimal, reset_value, min, max, X, Y);
}

bool ShaderFunctionRun(WaveForm* func)
{
    auto form = gcnew ShaderFunction();
    return form->Run(func);
}

bool GameTypeRun(pcstr title, GameTypeChooser* data)
{
    auto form = gcnew GameType();
    return form->Run(title, data);
}

bool TextEditRun(xr_string& text, pcstr caption /*= "Text"*/, bool read_only /*= false*/,
                 int lim /*= 0*/, pcstr apply_name /*= "Apply"*/,
                 TOnApplyClick on_apply /*= 0*/, TOnCloseClick on_close /*= 0*/,
                 TOnCodeInsight on_insight /*= 0*/)
{
    auto form = gcnew TextEdit();
    return form->Run(text, caption, read_only, lim, apply_name, on_apply, on_close, on_insight);
}
} // namespace XRay::Editor::Controls
