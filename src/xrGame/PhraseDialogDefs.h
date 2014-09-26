#pragma once

#include "../xrCore/intrusive_ptr.h"

class CPhraseDialog;

typedef intrusive_ptr<CPhraseDialog>	DIALOG_SHARED_PTR;

#include "PhraseDialog.h"

DEFINE_VECTOR	( shared_str, DIALOG_ID_VECTOR, DIALOG_ID_IT );