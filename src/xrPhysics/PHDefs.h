#pragma once

#include <utility>
#include "xrCommon/xr_vector.h"

class CPHElement;
class CPHJoint;
class CPhysicsShell;

// class CPHFracture;
class CShellSplitInfo;

typedef std::pair<CPhysicsShell*, u16> shell_root;

using ELEMENT_STORAGE = xr_vector<CPHElement*>;
typedef xr_vector<CPHElement*>::const_iterator ELEMENT_CI;
using JOINT_STORAGE = xr_vector<CPHJoint*>;
using PHSHELL_PAIR_VECTOR = xr_vector<shell_root>;
typedef xr_vector<shell_root>::reverse_iterator SHELL_PAIR_RI;

typedef xr_vector<CPHElement*>::reverse_iterator ELEMENT_RI;
