#pragma once
#include "Include/xrRender/FactoryPtr.h"
#include "xrCore/_vector3d.h"

class IGameObject;

// Igor DEFINE_VECTOR(ref_shader, SHADER_VECTOR, SHADER_VECTOR_IT);
class CWalmarkManager
{
private:
    FactoryPtr<IWallMarkArray> m_wallmarks;
    Fvector m_pos;

public:
    IGameObject* m_owner;
    CWalmarkManager();
    ~CWalmarkManager();
    void Load(LPCSTR section);
    void Clear();
    void AddWallmark(const Fvector& dir, const Fvector& start_pos, float range, float wallmark_size,
        IWallMarkArray& wallmarks_vector, int t);
    void PlaceWallmarks(const Fvector& start_pos);

    void StartWorkflow();
};
