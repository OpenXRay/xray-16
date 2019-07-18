//----------------------------------------------------
// file: rpoint.h
//----------------------------------------------------
#ifndef ESceneAIMapToolsH
#define ESceneAIMapToolsH

#include "ESceneCustomMTools.H"
#include "Common/LevelStructure.hpp"
#include "ESceneAIMapTools_Export.H"

// refs
class ESceneAIMapTool;
struct SAINode;

const DWORD InvalidNode = (1 << 24) - 1;

#pragma pack(push, 1)
struct SAINode // definition of "patch" or "node"
{
    union
    {
        struct
        {
            SAINode* n1; // Left
            SAINode* n2; // Forward
            SAINode* n3; // Right
            SAINode* n4; // Backward
        };

        SAINode* n[4];
    };

    Fplane Plane; // plane of patch
    Fvector Pos; // position of patch center

    enum
    {
        flSelected = (1 << 0),
        flHLSelected = (1 << 1),
        //    	flHide		= (1<<2), 	// obsolette

        flN1 = (1 << 4),
        flN2 = (1 << 5),
        flN3 = (1 << 6),
        flN4 = (1 << 7),
    };

    Flags8 flags;
    u32 idx;

    SAINode()
    {
        n1 = n2 = n3 = n4 = 0;
        idx = 0;
        flags.zero();
    }

    SAINode* nLeft() { return n1; }
    SAINode* nForward() { return n2; }
    SAINode* nRight() { return n3; }
    SAINode* nBack() { return n4; }
    int Links() const
    {
        int cnt = 0;
        for (int k = 0; k < 4; k++)
            if (n[k])
                cnt++;
        return cnt;
    }

    void PointLF(Fvector& D, float patch_size);
    void PointFR(Fvector& D, float patch_size);
    void PointRB(Fvector& D, float patch_size);
    void PointBL(Fvector& D, float patch_size);

    void LoadStream(IReader&, ESceneAIMapTool*);
    void SaveStream(IWriter&, ESceneAIMapTool*);
    void LoadLTX(CInifile& ini, LPCSTR sect_name, ESceneAIMapTool*);
    void SaveLTX(CInifile& ini, LPCSTR sect_name, ESceneAIMapTool*);

    void* operator new(std::size_t size);
    void* operator new(std::size_t size, SAINode*);
    void operator delete(void*);
};
#pragma pack(pop)

DEFINE_VECTOR(SAINode*, AINodeVec, AINodeIt);

const int HDIM_X = 128;
const int HDIM_Z = 128;

class ESceneAIMapTool : public ESceneToolBase
{
    friend class SAINode;
    typedef ESceneToolBase inherited;
    ObjectList m_SnapObjects;
    // hash
    AINodeVec m_HASH[HDIM_X + 1][HDIM_Z + 1];
    AINodeVec m_Nodes;

    SAIParams m_Params;
    Fbox m_AIBBox;

    ref_geom m_RGeom;
    ref_shader m_Shader;
    CDB::MODEL* m_CFModel;

protected:
    void hash_FillFromNodes();
    void hash_Initialize();
    void hash_Clear();
    void HashRect(const Fvector& v, float radius, Irect& result);
    AINodeVec* HashMap(int ix, int iz);
    AINodeVec* HashMap(Fvector& V);
    SAINode* FindNode(Fvector& vAt, float eps = 0.05f);
    SAINode* FindNeighbor(SAINode* N, int side, bool bIgnoreConstraints);
    void MotionSimulate(Fvector& result, Fvector& start, Fvector& end, float _radius, float _height);

    SAINode* BuildNode(Fvector& vFrom, Fvector& vAt, bool bIgnoreConstraints, bool bSuperIgnoreConstraints = false);
    int BuildNodes(const Fvector& pos, int sz, bool bIgnoreConstraints);
    void BuildNodes(bool bFromSelectedOnly);
    BOOL CreateNode(Fvector& vAt, SAINode& N, bool bIgnoreConstraints);
    BOOL CanTravel(Fvector _from, Fvector _at);

    SAINode* GetNode(Fvector vAt, bool bIgnoreConstraints);
    void UpdateLinks(SAINode* N, bool bIgnoreConstraints);

    void UnpackPosition(Fvector& Pdest, const NodePosition& Psrc, Fbox& bb, SAIParams& params);
    u32 UnpackLink(u32& L);
    void PackPosition(NodePosition& Dest, Fvector& Src, Fbox& bb, SAIParams& params);

    void EnumerateNodes();
    void DenumerateNodes();

    bool RealUpdateSnapList();
    int RemoveOutOfBoundsNodes();

    void CalculateNodesBBox(Fbox& bb);

    // controls
    virtual void CreateControls();
    virtual void RemoveControls();

public:
    enum EMode
    {
        mdAppend,
        mdRemove,
        mdInvert,
    };

    enum
    {
        flUpdateSnapList = (1 << 0),
        flHideNodes = (1 << 1),
        flSlowCalculate = (1 << 2),
        flUpdateHL = (1 << 15),
    };

    Flags32 m_Flags;

    float m_VisRadius;
    float m_SmoothHeight;
    u32 m_BrushSize;
    xr_vector<u16> m_ignored_materials;

    bool PickObjects(Fvector& dest, const Fvector& start, const Fvector& dir, float dist);

public:
    ESceneAIMapTool();
    virtual ~ESceneAIMapTool();

    virtual bool AllowEnabling() { return true; }
    virtual BOOL AllowMouseStart() { return true; }
    virtual void OnObjectRemove(CCustomObject* O, bool bDeleting);

    virtual void UpdateSnapList() { m_Flags.set(flUpdateSnapList, TRUE); }
    virtual ObjectList* GetSnapList() { return &m_SnapObjects; }
    // selection manipulate
    SAINode* PickNode(const Fvector& start, const Fvector& dir, float& dist);
    virtual int RaySelect(
        int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly);
    virtual int FrustumSelect(int flag, const CFrustum& frustum);
    virtual void SelectObjects(bool flag);
    virtual void InvertSelection();
    virtual void RemoveSelection();
    virtual int SelectionCount(bool testflag);

    virtual void ShowObjects(bool flag, bool bAllowSelectionFlag = false, bool bSelFlag = true) {}
    virtual void Clear(bool bOnlyNodes = false);

    // definition
    IC LPCSTR ClassName() { return "ai_map"; }
    IC LPCSTR ClassDesc() { return "AI Map"; }
    IC

        int
        RenderPriority()
    {
        return 10;
    }

    // validation
    virtual bool Valid();

    virtual bool Validate(bool) { return true; }
    virtual bool IsNeedSave();

    // events
    virtual void OnFrame();
    virtual void OnRender(int priority, bool strictB2F);
    virtual void OnActivate();

    // IO
    virtual bool LoadStream(IReader&);
    virtual bool LoadLTX(CInifile&);
    virtual void SaveStream(IWriter&);
    virtual void SaveLTX(CInifile&, int id);

    virtual bool can_use_inifile() { return false; }
    virtual bool LoadSelection(IReader&);
    virtual void SaveSelection(IWriter&);
    virtual bool Export(LPCSTR path);

    // device dependent funcs
    virtual void OnDeviceCreate();
    virtual void OnDeviceDestroy();

    virtual void OnSynchronize();

    // utils
    bool GenerateMap(bool bFromSelectedOnly);

    virtual bool GetSummaryInfo(SSceneSummary* inf) { return false; }
    virtual void GetBBox(Fbox& bb, bool bSelOnly);

    // properties
    virtual void FillProp(LPCSTR pref, PropItemVec& items);

    // other
    int AddNode(const Fvector& pos, bool bIgnoreConstraints, bool bAutoLink, int cnt);

    AINodeVec& Nodes() { return m_Nodes; }
    void MakeLinks(u8 side_flag, EMode mode, bool bIgnoreConstraints);
    void RemoveLinks();
    void InvertLinks();

    void UpdateHLSelected() { m_Flags.set(flUpdateHL, TRUE); }
    void SmoothNodes();
    void ResetNodes();
    void SelectNodesByLink(int link);
};
#endif // ESceneAIMapToolsH
