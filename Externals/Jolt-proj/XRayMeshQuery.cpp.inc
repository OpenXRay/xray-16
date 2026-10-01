// Included in the generated MeshShape translation unit, alongside its codecs.
class XRayMeshQueries
{
public:
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
