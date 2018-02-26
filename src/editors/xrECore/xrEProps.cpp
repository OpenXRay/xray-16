#include "pch.hpp"
#include "xrEProps.h"
#include "Props/NumericVector.h"
#include "Props/ShaderFunction.h"
#include "Props/GameType.h"
#include "xrEngine/WaveForm.h"

namespace XRay
{
namespace ECore
{
namespace Props
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
} // namespace Props
} // namespace ECore
} // namespace XRay
