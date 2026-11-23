#include "StdAfx.h"
#include "IActivationShape.h"
#include "PHActivationShape.h"
#include "IPhysicsShellHolder.h"

void ActivateShapeExplosive(IPhysicsShellHolder* holder, const Fvector& size,
    Fvector& out_size, Fvector& position)
{
    CPHActivationShape activation;
    activation.Create(position, size, holder);
    activation.CheckCharacters(false);
    activation.Activate(size, 1, 1.f, float(M_PI) / 8.f);
    position = activation.Position();
    activation.Size(out_size);
    activation.Destroy();
}
void ActivateShapePhysShellHolder(IPhysicsShellHolder* holder, const Fmatrix& transform,
    const Fvector& size, Fvector& position, Fvector& result)
{
    CPHActivationShape activation;
    activation.Create(position, size, holder);
    activation.set_rotation(transform);
    activation.Activate(size, 1, 1.f, float(M_PI) / 8.f);
    result = activation.Position();
    activation.Destroy();
}
bool ActivateShapeCharacterPhysicsSupport(Fvector& result, const Fvector& size,
    const Fvector& position, const Fmatrix& transform, bool ignore_characters,
    bool rotate, IPhysicsShellHolder* holder)
{
    CPHActivationShape activation;
    activation.Create(position, size, holder);
    activation.CheckCharacters(!ignore_characters);
    if (rotate) activation.set_rotation(transform);
    const bool success = activation.Activate(size, 1, 1.f, float(M_PI) / 8.f);
    result = activation.Position();
    activation.Destroy();
    return success;
}
