#ifndef ANTICHEAT_DUMPABLE_OBJECT
#define ANTICHEAT_DUMPABLE_OBJECT

class IAnticheatDumpable
{
public:
	virtual void				DumpActiveParams		(shared_str const & section_name, CInifile & dst_ini) const = 0;
	virtual shared_str const 	GetAnticheatSectionName	() const { return ""; }
};

#endif //#ifndef ANTICHEAT_DUMPABLE_OBJECT