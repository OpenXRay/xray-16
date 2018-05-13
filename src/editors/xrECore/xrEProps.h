#pragma once

template <class T>
struct _vector3;
using Fvector = _vector3<float>;
struct WaveForm;
struct GameTypeChooser;

using TOnApplyClick = fastdelegate::FastDelegate1<pcstr, bool>;
using TOnCloseClick = fastdelegate::FastDelegate0<bool>;
using TOnCodeInsight = fastdelegate::FastDelegate3<const xr_string&, xr_string&, bool&>;

namespace XRay::Editor::Controls
{
XRECORE_API bool NumericVectorRun(pcstr title, Fvector* data, int decimal, Fvector* reset_value, Fvector* min, Fvector* max, int* X, int* Y);
XRECORE_API bool ShaderFunctionRun(WaveForm* func);
XRECORE_API bool GameTypeRun(pcstr title, GameTypeChooser* data);
XRECORE_API bool TextEditRun(xr_string& text, pcstr caption = "Text", bool read_only = false, int lim = 0,
                 pcstr apply_name = "Apply", TOnApplyClick on_apply = 0, TOnCloseClick on_close = 0,
                 TOnCodeInsight on_insight = 0);
} // namespace XRay::Editor::Controls
