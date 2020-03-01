///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Planes-AABB overlap test.
 *	- original code by Ville Miettinen, from Umbra/dPVS (released on the GD-Algorithms mailing list)
 *	- almost used "as-is", I even left the comments (hence the frustum-related notes)
 *
 *	\param		center			[in] box center
 *	\param		extents			[in] box extents
 *	\param		out_clip_mask	[out] bitmask for active planes
 *	\param		in_clip_mask	[in] bitmask for active planes
 *	\return		TRUE if boxes overlap planes
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline_ BOOL PlanesCollider::PlanesAABBOverlap(
    const Point& center, const Point& extents, udword& out_clip_mask, udword in_clip_mask)
{
    // Stats
    mNbVolumeBVTests++;

    const Plane* p = mPlanes;

    // Evaluate through all active frustum planes. We determine the relation
    // between the AABB and a plane by using the concept of "near" and "far"
    // vertices originally described by Zhang (and later by Mцller). Our
    // variant here uses 3 fabs ops, 6 muls, 7 adds and two floating point
    // comparisons per plane. The routine early-exits if the AABB is found
    // to be outside any of the planes. The loop also constructs a _new_ output
    // clip mask. Most FPUs have a native single-cycle _abs() operation.

    udword Mask = 1; // current mask index (1,2,4,8,..)
    udword TmpOutClipMask = 0; // initialize output clip mask into empty.

    while (Mask <= in_clip_mask) // keep looping while we have active planes left...
    {
        if (in_clip_mask & Mask) // if clip plane is active, process it..
        {
            float NP = extents.x * _abs(p->n.x) + extents.y * _abs(p->n.y) +
                extents.z * _abs(p->n.z); // ### _abs could be precomputed
            float MP = center.x * p->n.x + center.y * p->n.y + center.z * p->n.z + p->d;

            if (NP < MP) // near vertex behind the clip plane...
                return FALSE; // .. so there is no intersection..
            if ((-NP) < MP) // near and far vertices on different sides of plane..
                TmpOutClipMask |= Mask; // .. so update the clip mask...
        }
        Mask += Mask; // mk = (1<<plane)
        p++; // advance to next plane
    }

    out_clip_mask = TmpOutClipMask; // copy output value (temp used to resolve aliasing!)
    return TRUE; // indicate that AABB intersects frustum
}
