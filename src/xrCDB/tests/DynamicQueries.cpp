#include "TestObject.h"

#include "xrCDB/xr_area.h"

class SphereForm : public ICollisionForm
{
    Fvector center;

public:
    SphereForm(TestObject& object, const Fvector& center) : ICollisionForm(&object, cftObject), center(center)
    {
        object.SetCForm(this);
    }

    bool _RayQuery(const collide::ray_defs& ray, collide::rq_results& results) override
    {
        Fvector offset;
        offset.sub(ray.start, center);
        const float a = ray.dir.square_magnitude(), b = offset.dotproduct(ray.dir), c = offset.square_magnitude() - .01f;
        const float discriminant = b * b - a * c;
        if (discriminant < 0 || a == 0)
            return false;
        const float range = (-b - std::sqrt(discriminant)) / a;
        if (range <= 0 || range > ray.range)
            return false;
        results.append_result(owner, range, 42, (ray.flags & CDB::OPT_ONLYNEAREST) != 0);
        return true;
    }
};

void CheckDynamicQueries(CObjectSpace& space, ISpatial_DB& spatial)
{
    const auto center = Vector(.25f, .25f, -.5f);
    TestObject object(spatial, center);
    SphereForm form(object, center);
    const auto start = Vector(.25f, .25f, -1), direction = Vector(0, 0, 1);
    const collide::ray_defs ray(start, direction, 10, CDB::OPT_CULL, collide::rqtBoth);
    collide::rq_results results;
    Require(space.RayQuery(results, ray, nullptr, nullptr, nullptr, nullptr), "Combined query missed");
    Require(results.r_count() == 4, "Combined query lost hits");
    Require(results.r_begin()->O == &object && std::abs(results.r_begin()->range - .4f) < 1.e-5f, "Combined nearest hit wrong");
    Require(std::is_sorted(results.r_get()->begin(), results.r_get()->end(),
                [](const auto& a, const auto& b)
                {
                    return a.range < b.range;
                }),
        "Combined hit order changed");
    Require(space.RayQuery(results, ray, nullptr, nullptr, nullptr, &object) && results.r_count() == 3, "Object exclusion ignored");
    u32 tested = 0;
    auto reject = [](const collide::ray_defs&, IGameObject*, void* data)
    {
        (*static_cast<u32*>(data))++;
        return false;
    };
    Require(space.RayQuery(results, ray, nullptr, &tested, reject, nullptr), "Filtering lost static hits");
    Require(tested == 1 && results.r_count() == 3, "Dynamic filter ignored");
    collide::rq_result nearest;
    Require(space.RayPick(start, direction, 10, collide::rqtBoth, nearest, nullptr) && nearest.O == &object, "Nearest dynamic hit missed");
    Require(space.RayPick(start, direction, 10, collide::rqtBoth, nearest, &object) && nearest.O == nullptr, "Nearest exclusion ignored");
    Require(space.RayTest(start, direction, .75f, collide::rqtBoth, nullptr, nullptr), "Dynamic occlusion missed");
    Require(!space.RayTest(start, direction, .75f, collide::rqtBoth, nullptr, &object), "Ignored object blocked visibility");
}
