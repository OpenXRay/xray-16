// Included in the generated MeshShape translation unit, alongside its codecs.
class XRayMeshQueries
{
public:
    template <typename Bounds>
    static bool VisitSpecialized(const MeshShape& mesh, void* context,
        XRayMeshTriangleCallback triangle, Bounds bounds)
    {
        struct Visitor {
            void* context;
            XRayMeshTriangleCallback triangle;
            Bounds bounds;
            float distances[NodeCodec::StackSize]{};
            bool aborted = false;
            bool ShouldAbort() const { return aborted; }
            bool ShouldVisitNode(int top) const { return bounds.Accept(distances[top]); }
            int VisitNodes(Vec4Arg minX, Vec4Arg minY, Vec4Arg minZ, Vec4Arg maxX,
                Vec4Arg maxY, Vec4Arg maxZ, UVec4& properties, int top)
            {
                const Vec4 entries = bounds.Test(minX, minY, minZ, maxX, maxY, maxZ);
                const UVec4 original = properties;
                uint32 selected[4]{};
                int count = 0;
                for (int child = 0; child < 4; ++child) {
                    if ((original[child] >> NodeCodec::TRIANGLE_COUNT_SHIFT) == NodeCodec::TRIANGLE_COUNT_MASK ||
                        !bounds.Accept(entries[child])) continue;
                    selected[count] = original[child];
                    distances[top + count++] = entries[child];
                }
                properties = UVec4::sLoadInt4(selected);
                return count;
            }
            void VisitTriangles(const TriangleCodec::DecodingContext& decoder, const void* data, int count, uint32)
            {
                for (int index = 0; index < count; ++index)
                    if (triangle(context, decoder.GetUserData(data, index))) { aborted = true; break; }
            }
        } visitor{context, triangle, bounds};
        mesh.WalkTree(visitor);
        return visitor.aborted;
    }
    static bool Ray(const MeshShape& mesh, void* context, Vec3Arg origin, Vec3Arg direction,
        float& range, XRayMeshTriangleCallback triangle)
    {
        struct Bounds {
            Vec3 origin;
            RayInvDirection inverse;
            float& range;
            bool Accept(float distance) const { return distance <= range && distance != FLT_MAX; }
            Vec4 Test(Vec4Arg minX, Vec4Arg minY, Vec4Arg minZ, Vec4Arg maxX, Vec4Arg maxY, Vec4Arg maxZ) const {
                return RayAABox4(origin, inverse, minX, minY, minZ, maxX, maxY, maxZ);
            }
        } bounds{origin, RayInvDirection(direction), range};
        const auto& root = mesh.GetLocalBounds();
        if (!bounds.Accept(RayAABox(origin, bounds.inverse, root.mMin, root.mMax))) return false;
        return VisitSpecialized(mesh, context, triangle, bounds);
    }
    static bool Box(const MeshShape& mesh, void* context, Vec3Arg minimum, Vec3Arg maximum,
        XRayMeshTriangleCallback triangle)
    {
        struct Bounds {
            Vec3 minimum, maximum;
            bool Accept(float distance) const { return distance == 0; }
            Vec4 Test(Vec4Arg minX, Vec4Arg minY, Vec4Arg minZ, Vec4Arg maxX, Vec4Arg maxY, Vec4Arg maxZ) const {
                const auto outsideX = UVec4::sOr(Vec4::sGreater(minX, maximum.SplatX()), Vec4::sLess(maxX, minimum.SplatX()));
                const auto outsideY = UVec4::sOr(Vec4::sGreater(minY, maximum.SplatY()), Vec4::sLess(maxY, minimum.SplatY()));
                const auto outsideZ = UVec4::sOr(Vec4::sGreater(minZ, maximum.SplatZ()), Vec4::sLess(maxZ, minimum.SplatZ()));
                return Vec4::sSelect(Vec4::sZero(), Vec4::sReplicate(FLT_MAX),
                    UVec4::sOr(UVec4::sOr(outsideX, outsideY), outsideZ));
            }
        } bounds{minimum, maximum};
        if (!AABox(minimum, maximum).Overlaps(mesh.GetLocalBounds())) return false;
        return VisitSpecialized(mesh, context, triangle, bounds);
    }
    static bool Visit(const MeshShape& mesh, void* context, XRayMeshBoundsCallback bounds,
        XRayMeshTriangleCallback triangle, uint32 mask)
    {
        struct Visitor
        {
            void* context;
            XRayMeshBoundsCallback bounds;
            XRayMeshTriangleCallback triangle;
            uint32 masks[NodeCodec::StackSize]{};
            bool aborted = false;
            bool ShouldAbort() const { return aborted; }
            bool ShouldVisitNode(int) const { return true; }
            int VisitNodes(Vec4Arg minX, Vec4Arg minY, Vec4Arg minZ, Vec4Arg maxX,
                Vec4Arg maxY, Vec4Arg maxZ, UVec4& properties, int top)
            {
                const auto inheritedMask = masks[top];
                const UVec4 original = properties;
                uint32 selected[4]{};
                int count = 0;
                for (int child = 0; child < 4; ++child)
                {
                    if ((original[child] >> NodeCodec::TRIANGLE_COUNT_SHIFT) == NodeCodec::TRIANGLE_COUNT_MASK)
                        continue;
                    uint32 childMask = inheritedMask;
                    const AABox box(Vec3(minX[child], minY[child], minZ[child]), Vec3(maxX[child], maxY[child], maxZ[child]));
                    if (bounds(context, box, childMask))
                    {
                        selected[count] = original[child];
                        masks[top + count] = childMask;
                        ++count;
                    }
                }
                properties = UVec4::sLoadInt4(selected);
                return count;
            }
            void VisitTriangles(const TriangleCodec::DecodingContext& decoder, const void* data, int count, uint32)
            {
                for (int index = 0; index < count; ++index)
                    if (triangle(context, decoder.GetUserData(data, index))) { aborted = true; break; }
            }
        } visitor{context, bounds, triangle};
        if (!bounds(context, mesh.GetLocalBounds(), mask)) return false;
        visitor.masks[0] = mask;
        mesh.WalkTree(visitor);
        return visitor.aborted;
    }
};
bool XRayVisitMesh(const MeshShape& mesh, void* context, XRayMeshBoundsCallback bounds,
    XRayMeshTriangleCallback triangle, uint32 mask)
{
    return XRayMeshQueries::Visit(mesh, context, bounds, triangle, mask);
}
bool XRayVisitMeshRay(const MeshShape& mesh, void* context, Vec3Arg origin,
    Vec3Arg direction, float& range, XRayMeshTriangleCallback triangle)
{
    return XRayMeshQueries::Ray(mesh, context, origin, direction, range, triangle);
}
bool XRayVisitMeshBox(const MeshShape& mesh, void* context, Vec3Arg minimum,
    Vec3Arg maximum, XRayMeshTriangleCallback triangle)
{
    return XRayMeshQueries::Box(mesh, context, minimum, maximum, triangle);
}
