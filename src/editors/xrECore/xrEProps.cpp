#include "pch.hpp"
#include "xrEProps.h"
#include "Props/NumericVector.h"

using namespace XRay::ECore;

bool NumericVectorRun(pcstr title, Fvector* data, int decimal, Fvector* reset_value, Fvector* min, Fvector* max, int* X, int* Y)
{
    auto frm = gcnew NumericVector();
    return frm->Run(title, data, decimal, reset_value, min, max, X, Y);
}
