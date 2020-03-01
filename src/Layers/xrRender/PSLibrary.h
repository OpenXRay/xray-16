//----------------------------------------------------
// file: PSLibrary.h
//----------------------------------------------------
#ifndef PSLibraryH
#define PSLibraryH

#include "Include/xrRender/particles_systems_library_interface.hpp"

namespace PS
{
class CPEDef;
using PEDVec = xr_vector<CPEDef*>;
using PEDIt = PEDVec::iterator;

class CPGDef;
using PGDVec = xr_vector<CPGDef*>;
using PGDIt = PGDVec::iterator;

} // namespace PS

class ECORE_API CPSLibrary : public particles_systems::library_interface
{
    PS::PEDVec m_PEDs;
    PS::PGDVec m_PGDs;

#ifdef _EDITOR
    AnsiString m_CurrentParticles;

public:
    void __stdcall FindByName(LPCSTR new_name, bool& res);
#endif

public:
    bool Load(LPCSTR nm);
    bool Save(LPCSTR nm);

    bool Load2();
    bool Save2();

public:
    CPSLibrary() { ; }
    ~CPSLibrary() { ; }
    void OnCreate();
    void OnDestroy();

    PS::CPEDef* FindPED(LPCSTR name);
    PS::PEDIt FindPEDIt(LPCSTR name);
    PS::CPGDef* FindPGD(LPCSTR name);
    PS::PGDIt FindPGDIt(LPCSTR name);

    // get object properties methods
    PS::PEDIt FirstPED() { return m_PEDs.begin(); }
    PS::PEDIt LastPED() { return m_PEDs.end(); }
    PS::PGDIt FirstPGD() { return m_PGDs.begin(); }
    PS::PGDIt LastPGD() { return m_PGDs.end(); }
    PS::CPEDef* AppendPED(PS::CPEDef* src = nullptr);
    PS::CPGDef* AppendPGD(PS::CPGDef* src = nullptr);
    void Remove(LPCSTR name);
    void RenamePED(PS::CPEDef* src, LPCSTR new_name);
    void RenamePGD(PS::CPGDef* src, LPCSTR new_name);

    void Reload();
    bool Save();

    virtual PS::CPGDef const* const* particles_group_begin() const;
    virtual PS::CPGDef const* const* particles_group_end() const;
    virtual void particles_group_next(PS::CPGDef const* const*& iterator) const;
    virtual shared_str const& particles_group_id(PS::CPGDef const& particles_group) const;
};

#define PS_LIB_SIGN "PS_LIB"

#define PS_VERSION 0x0001
//----------------------------------------------------
#define PS_CHUNK_VERSION 0x0001
#define PS_CHUNK_FIRSTGEN 0x0002
#define PS_CHUNK_SECONDGEN 0x0003
#define PS_CHUNK_THIRDGEN 0x0004

#endif /*_INCDEF_PSLibrary_H_*/
