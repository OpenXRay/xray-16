#include "xrCDB/RayQueryAudit.h"
#include <cassert>

int main()
{
    using V = CDB::RayQueryAudit::Vec;
    CDB::RayQueryAudit down{{0, 2, 0}, {0, -1, 0}};
    const V a{-2, 0, -2}, b{2, 0, -2}, c{0, 0, 2};
    down.test(2, {-2, -5, -2}, {2, -5, -2}, {0, -5, 2});
    assert(down.nearest == 2 && down.distance == 7);
    down.test(1, a, b, c);
    assert(down.nearest == 1 && down.distance == 2);
    down.test(3, {-2, 3, -2}, {2, 3, -2}, {0, 3, 2});
    assert(down.nearest == 1); // Behind the ray origin.
    CDB::RayQueryAudit up{{0, 2, 0}, {0, 1, 0}};
    up.test(1, a, b, c);
    assert(up.nearest == -1);
    up.test(3, {-2, 3, -2}, {2, 3, -2}, {0, 3, 2});
    assert(up.nearest == 3 && up.distance == 1);
    CDB::RayQueryAudit reversed{{0, 2, 0}, {0, -1, 0}};
    reversed.test(1, c, b, a);
    assert(reversed.nearest == 1); // Sector picking is two-sided.
    CDB::RayQueryAudit outside{{10, 2, 0}, {0, -1, 0}};
    outside.test(1, a, b, c);
    assert(outside.nearest == -1);
    CDB::RayQueryAudit parallel{{0, 2, 0}, {1, 0, 0}};
    parallel.test(1, a, b, c);
    parallel.test(1, a, a, a);
    assert(parallel.nearest == -1);
}
