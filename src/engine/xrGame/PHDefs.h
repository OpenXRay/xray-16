#ifndef PHDEFS_H
#define PHDEFS_H
class CPHElement;
class CPHJoint;
class CPhysicsShell;

//class CPHFracture;
class CShellSplitInfo;

typedef std::pair<CPhysicsShell*,u16>	shell_root;

DEFINE_VECTOR(CPHElement*,ELEMENT_STORAGE,ELEMENT_I)
typedef		xr_vector<CPHElement*>::const_iterator	ELEMENT_CI;
DEFINE_VECTOR(CPHJoint*,JOINT_STORAGE,JOINT_I)
DEFINE_VECTOR(shell_root,PHSHELL_PAIR_VECTOR,SHELL_PAIR_I)
typedef xr_vector<shell_root>::reverse_iterator SHELL_PAIR_RI;

typedef		xr_vector<CPHElement*>::reverse_iterator	ELEMENT_RI;

#endif