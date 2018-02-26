#pragma once

struct WaveForm;
struct GameTypeChooser;

namespace XRay
{
namespace ECore
{
namespace Props
{
bool NumericVectorRun(pcstr title, Fvector* data, int decimal, Fvector* reset_value, Fvector* min, Fvector* max, int* X, int* Y);
bool ShaderFunctionRun(WaveForm* func);
bool GameTypeRun(pcstr title, GameTypeChooser* data);
} // namespace Props
} // namespace ECore
} // namespace XRay
