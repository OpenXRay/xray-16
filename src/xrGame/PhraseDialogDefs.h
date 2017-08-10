#pragma once

#include "xrCore/intrusive_ptr.h"

class CPhraseDialog;

using DIALOG_SHARED_PTR = intrusive_ptr<CPhraseDialog>;

#include "PhraseDialog.h"

using DIALOG_ID_VECTOR = xr_vector<shared_str>;
