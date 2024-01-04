#include "stdafx.h"

#include "SH_Constant.h"

/*
#include "xrCore/xr_resource.h"

// res
class	test_resource	: public xr_resource	{
public:
    void				_release_	(test_resource * ptr)	{	xr_delete	(ptr);	}
};

// ptr
typedef	xr_resorce_ptr<test_resource>	test_ptr;

// the test itself
BOOL	AAA_test		()
{
    test_ptr	A		(new test_resource());
    test_ptr	B		= new test_resource	();
    A					= B;
    return		A!=B;
}
*/

void CConstant::Calculate()
{
    if (dwFrame == Device.dwFrame)
        return;
    dwFrame = Device.dwFrame;
    if (modeProgrammable == dwMode)
        return;

    float t = Device.fTimeGlobal;
    set_float(R.Calculate(t), G.Calculate(t), B.Calculate(t), A.Calculate(t));
}

void CConstant::Load(IReader* fs)
{
    dwMode = modeWaveForm;
    fs->r(&R, sizeof(WaveForm));
    fs->r(&G, sizeof(WaveForm));
    fs->r(&B, sizeof(WaveForm));
    fs->r(&A, sizeof(WaveForm));
}

void CConstant::Save(IWriter* fs)
{
    fs->w(&R, sizeof(WaveForm));
    fs->w(&G, sizeof(WaveForm));
    fs->w(&B, sizeof(WaveForm));
    fs->w(&A, sizeof(WaveForm));
}
