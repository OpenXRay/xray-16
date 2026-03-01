// xrRender/FrameGraphPasses/RibbonPassSetup.cpp
// Ribbon trail rendering — Stride-compatible camera-facing quad strip
#include "stdafx.h"
#include "RibbonPassSetup.h"
#include "PassCommon.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/Backend/D3D12Backend.h"
#include "Layers/xrRender/Bindless/MaterialBuffer.h"

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using namespace bindless;

// ═══════════════════════════════════════════════════════
//  Trail point management
// ═══════════════════════════════════════════════════════

static void UpdateTrailPoints(RibbonPassState& state, const Fvector& emitterPos, float dt, float pointSize)
{
    // Age existing points & remove expired ones
    u32 writeIdx = 0;
    for (u32 i = 0; i < state.pointCount; i++) {
        state.points[i].age += dt;
        if (state.points[i].age < state.maxAge) {
            if (writeIdx != i)
                state.points[writeIdx] = state.points[i];
            writeIdx++;
        }
    }
    state.pointCount = writeIdx;

    // Insert new point at front if moved enough from the previous head
    bool shouldInsert = true;
    if (state.pointCount > 0) {
        Fvector diff;
        diff.sub(emitterPos, state.points[0].position);
        if (diff.magnitude() < RIBBON_MIN_SEGMENT_DIST)
            shouldInsert = false;
    }

    if (shouldInsert) {
        u32 count = std::min(state.pointCount, RIBBON_MAX_POINTS - 1);
        for (u32 i = count; i > 0; i--)
            state.points[i] = state.points[i - 1];

        state.points[0].position = emitterPos;
        state.points[0].age = 0.f;
        state.points[0].size = pointSize;
        state.points[0].order = (u32(state.currentGroupID) << 16) | u32(state.nextSpawnOrder);
        state.nextSpawnOrder++;
        state.pointCount = count + 1;
    }
}

// ═══════════════════════════════════════════════════════
//  Math helpers
// ═══════════════════════════════════════════════════════

static Fvector CatmullRom(const Fvector& p0, const Fvector& p1, const Fvector& p2, const Fvector& p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    Fvector r;
    r.x = 0.5f * ((2.f * p1.x) + (-p0.x + p2.x) * t + (2.f * p0.x - 5.f * p1.x + 4.f * p2.x - p3.x) * t2 + (-p0.x + 3.f * p1.x - 3.f * p2.x + p3.x) * t3);
    r.y = 0.5f * ((2.f * p1.y) + (-p0.y + p2.y) * t + (2.f * p0.y - 5.f * p1.y + 4.f * p2.y - p3.y) * t2 + (-p0.y + 3.f * p1.y - 3.f * p2.y + p3.y) * t3);
    r.z = 0.5f * ((2.f * p1.z) + (-p0.z + p2.z) * t + (2.f * p0.z - 5.f * p1.z + 4.f * p2.z - p3.z) * t2 + (-p0.z + 3.f * p1.z - 3.f * p2.z + p3.z) * t3);
    return r;
}

// Circumcenter of triangle ABC in 3D (Stride's formula)
static Fvector Circumcenter(const Fvector& A, const Fvector& B, const Fvector& C)
{
    Fvector a, b;
    a.sub(A, C);
    b.sub(B, C);

    Fvector crossAB;
    crossAB.crossproduct(a, b);
    float denom = 2.0f * crossAB.square_magnitude();

    if (denom < 1e-12f)
        return C;  // Degenerate: collinear points

    float a2 = a.square_magnitude();
    float b2 = b.square_magnitude();

    // numerator = cross(a2*b - b2*a, cross(a,b))
    Fvector scaled_b, scaled_a, diff, num;
    scaled_b.set(b.x * a2, b.y * a2, b.z * a2);
    scaled_a.set(a.x * b2, a.y * b2, a.z * b2);
    diff.sub(scaled_b, scaled_a);
    num.crossproduct(diff, crossAB);

    float invDenom = 1.0f / denom;
    Fvector result;
    result.set(C.x + num.x * invDenom, C.y + num.y * invDenom, C.z + num.z * invDenom);
    return result;
}

// Circumcircle-based smooth interpolation between P1 and P2 (Stride's BestFit)
static Fvector CircumcircleSmooth(const Fvector& P0, const Fvector& P1, const Fvector& P2, const Fvector& P3, float t)
{
    Fvector O1 = Circumcenter(P0, P1, P2);
    float R1 = O1.distance_to(P1);

    Fvector O2 = Circumcenter(P1, P2, P3);
    float R2 = O2.distance_to(P2);

    // Linear guess
    Fvector guess;
    guess.lerp(P1, P2, t);

    // Project onto circumcircle 1
    Fvector d1;
    d1.sub(guess, O1);
    float len1 = d1.magnitude();
    if (len1 > EPS_S) d1.div(len1); else d1.set(0.f, 1.f, 0.f);

    // Project onto circumcircle 2
    Fvector d2;
    d2.sub(guess, O2);
    float len2 = d2.magnitude();
    if (len2 > EPS_S) d2.div(len2); else d2.set(0.f, 1.f, 0.f);

    // Points on each circle
    Fvector p1OnCircle, p2OnCircle;
    p1OnCircle.mad(O1, d1, R1);
    p2OnCircle.mad(O2, d2, R2);

    // Blend
    Fvector result;
    result.lerp(p1OnCircle, p2OnCircle, t);
    return result;
}

static Fvector2 ApplyUVTransform(float u, float v, const RibbonUVTransform& xform)
{
    float xP = xform.flipX ? (1.0f - u) : u;
    float yP = xform.flipY ? (1.0f - v) : v;
    Fvector2 result;
    if (xform.rotate90)
        result.set(yP, xP);
    else
        result.set(xP, yP);
    return result;
}

// ═══════════════════════════════════════════════════════
//  Order field group splitting
// ═══════════════════════════════════════════════════════

struct RibbonGroup {
    u32 startIdx;
    u32 count;
};

static xr_vector<RibbonGroup> SplitRibbonGroups(const RibbonPoint* points, u32 pointCount)
{
    xr_vector<RibbonGroup> groups;
    if (pointCount == 0)
        return groups;

    u16 currentGroup = u16(points[0].order >> 16);
    u32 groupStart = 0;

    for (u32 i = 1; i < pointCount; i++) {
        u16 g = u16(points[i].order >> 16);
        if (g != currentGroup) {
            groups.push_back({groupStart, i - groupStart});
            groupStart = i;
            currentGroup = g;
        }
    }
    groups.push_back({groupStart, pointCount - groupStart});
    return groups;
}

// ═══════════════════════════════════════════════════════
//  Core geometry generation for one ribbon chain
// ═══════════════════════════════════════════════════════

static void GenerateRibbonForGroup(
    const RibbonPoint* ctrlPoints,
    u32 ctrlCount,
    const RibbonPassState& state,
    const Fmatrix& viewProj,
    const Fvector& invViewX,   // camera right (from mInvView.i)
    const Fvector& invViewY,   // camera up    (from mInvView.j)
    const Fvector& camPos,
    xr_vector<ParticleVertex>& outVerts,
    xr_vector<u16>& outIndices)
{
    if (ctrlCount < 2)
        return;

    // --- Step 1: Mirror-reflection virtual first point (Stride boundary) ---
    // virtualFirst = 2 * P0 - P1
    Fvector virtualFirst;
    virtualFirst.sub(ctrlPoints[0].position, ctrlPoints[1].position);
    virtualFirst.add(ctrlPoints[0].position);

    // --- Step 2: Subdivision ---
    u32 segments = ctrlCount - 1;
    u32 smoothCount = segments * RIBBON_SUBDIVISIONS + 1;

    xr_vector<Fvector> smoothPos;
    xr_vector<float> smoothSize;
    xr_vector<float> smoothT;
    smoothPos.reserve(smoothCount);
    smoothSize.reserve(smoothCount);
    smoothT.reserve(smoothCount);

    for (u32 seg = 0; seg < segments; seg++) {
        Fvector p0, p1, p2, p3;
        float s1, s2;

        p1 = ctrlPoints[seg].position;
        p2 = ctrlPoints[seg + 1].position;
        s1 = ctrlPoints[seg].size;
        s2 = ctrlPoints[seg + 1].size;

        // P0: virtual mirror for first segment, else previous
        p0 = (seg == 0) ? virtualFirst : ctrlPoints[seg - 1].position;
        // P3: clamp to last for final segment
        p3 = (seg + 2 < ctrlCount) ? ctrlPoints[seg + 2].position : p2;

        u32 startSub = (seg == 0) ? 0 : 1;
        for (u32 sub = startSub; sub <= RIBBON_SUBDIVISIONS; sub++) {
            float lt = (float)sub / (float)RIBBON_SUBDIVISIONS;

            Fvector pos;
            if (state.smoothing == RibbonSmoothingMode::Circumcircle)
                pos = CircumcircleSmooth(p0, p1, p2, p3, lt);
            else
                pos = CatmullRom(p0, p1, p2, p3, lt);

            float sz = s1 + (s2 - s1) * lt;  // linear size interpolation (matches Stride)
            float globalT = ((float)seg + lt) / (float)segments;

            smoothPos.push_back(pos);
            smoothSize.push_back(sz);
            smoothT.push_back(globalT);
        }
    }

    u32 pointCount = (u32)smoothPos.size();
    if (pointCount < 2)
        return;

    // --- Step 3: Accumulate distance (for UV) ---
    xr_vector<float> cumDist(pointCount, 0.0f);
    float totalDist = 0.0f;
    for (u32 i = 1; i < pointCount; i++) {
        totalDist += smoothPos[i].distance_to(smoothPos[i - 1]);
        cumDist[i] = totalDist;
    }

    // --- Step 4: Width computation + vertex emission ---
    u32 baseVertex = (u32)outVerts.size();
    outVerts.resize(baseVertex + pointCount * 2);

    // Screen-space tangent averaging state (Stride approach)
    float prevAxisX = 0.f, prevAxisY = 0.f;
    bool hasPrevAxis = false;

    for (u32 i = 0; i < pointCount; i++) {
        Fvector widthVec;
        float halfWidth = smoothSize[i];

        if (state.useScreenSpaceWidth) {
            // --- Stride screen-space width with tangent averaging ---
            // Project current and adjacent point to NDC
            u32 adjIdx = (i + 1 < pointCount) ? i + 1 : i - 1;

            Fvector4 clipA, clipB;
            Fvector4 wA = {smoothPos[i].x, smoothPos[i].y, smoothPos[i].z, 1.0f};
            Fvector4 wB = {smoothPos[adjIdx].x, smoothPos[adjIdx].y, smoothPos[adjIdx].z, 1.0f};
            viewProj.transform(clipA, wA);
            viewProj.transform(clipB, wB);

            float invWA = (_abs(clipA.w) > EPS_S) ? (1.0f / clipA.w) : 0.0f;
            float invWB = (_abs(clipB.w) > EPS_S) ? (1.0f / clipB.w) : 0.0f;

            // Screen-space tangent: from current toward adjacent (Stride: projPt0 - projPt1)
            float dx = clipA.x * invWA - clipB.x * invWB;
            float dy = clipA.y * invWA - clipB.y * invWB;
            float screenLen = _sqrt(dx * dx + dy * dy);
            if (screenLen > EPS_S) { dx /= screenLen; dy /= screenLen; }
            else { dx = 1.0f; dy = 0.0f; }

            // Average with previous tangent (Stride: axisAvg = normalize(axis0 + axis1))
            float avgX, avgY;
            if (hasPrevAxis) {
                avgX = prevAxisX + dx;
                avgY = prevAxisY + dy;
                float avgLen = _sqrt(avgX * avgX + avgY * avgY);
                if (avgLen > EPS_S) { avgX /= avgLen; avgY /= avgLen; }
                else { avgX = dx; avgY = dy; }
            } else {
                avgX = dx;
                avgY = dy;
            }
            prevAxisX = dx;
            prevAxisY = dy;
            hasPrevAxis = true;

            // Perpendicular in screen space → world space (Stride: unitX.Y * invViewX - unitX.X * invViewY)
            // avgX/avgY is the screen-space tangent; perpendicular = (avgY, -avgX)
            // world = avgY * invViewX - avgX * invViewY
            widthVec.set(
                avgY * invViewX.x - avgX * invViewY.x,
                avgY * invViewX.y - avgX * invViewY.y,
                avgY * invViewX.z - avgX * invViewY.z
            );
            // widthVec is already unit length (invViewX/invViewY are orthonormal, avg is unit)
            // halfWidth stays as smoothSize[i]
        } else {
            // --- World-space cross-product width ---
            Fvector tangent;
            if (i + 1 < pointCount)
                tangent.sub(smoothPos[i + 1], smoothPos[i]);
            else
                tangent.sub(smoothPos[i], smoothPos[i - 1]);
            float tangentLen = tangent.magnitude();
            if (tangentLen > EPS_S) tangent.div(tangentLen);
            else tangent.set(0.f, 0.f, 1.f);

            Fvector toCamera;
            toCamera.sub(camPos, smoothPos[i]);
            toCamera.normalize_safe();

            widthVec.crossproduct(tangent, toCamera);
            float wLen = widthVec.magnitude();
            if (wLen > EPS_S) widthVec.div(wLen);
            else widthVec.set(0.f, 1.f, 0.f);
        }

        // Tail fade (our custom feature)
        float tailFade = 1.0f;
        if (state.enableTailFade) {
            float t = smoothT[i];
            tailFade = 1.0f - t * t;
        }

        float hw = halfWidth * tailFade;

        // --- UV computation ---
        float v;
        switch (state.uvPolicy) {
        case RibbonUVPolicy::Stretched:
            v = ((float)i / (float)(pointCount - 1)) * state.texCoordsFactor;
            break;
        case RibbonUVPolicy::AsIs: {
            // V resets 0->1 per original segment
            float segF = smoothT[i] * (float)(ctrlCount - 1);
            float localT = segF - floor(segF);
            // At exact segment boundaries (localT ~= 0), use 0 for start of new segment
            v = localT * state.texCoordsFactor;
            break;
        }
        case RibbonUVPolicy::DistanceBased:
        default: {
            float invTotal = (totalDist > EPS_S) ? (1.0f / totalDist) : 0.0f;
            v = cumDist[i] * invTotal * state.texCoordsFactor;
            break;
        }
        }

        Fvector2 uvLeft = ApplyUVTransform(0.0f, v, state.uvTransform);
        Fvector2 uvRight = ApplyUVTransform(1.0f, v, state.uvTransform);

        u8 alpha = (u8)(tailFade * 255.0f);
        u32 clr = (alpha << 24) | 0x00FFFFFF;

        u32 vi = baseVertex + i * 2;

        // Left vertex (position - widthVec * hw)
        outVerts[vi].p.set(
            smoothPos[i].x - widthVec.x * hw,
            smoothPos[i].y - widthVec.y * hw,
            smoothPos[i].z - widthVec.z * hw);
        outVerts[vi].color = clr;
        outVerts[vi].t = uvLeft;
        outVerts[vi].materialID = 0;
        outVerts[vi]._pad = 0;

        // Right vertex (position + widthVec * hw)
        outVerts[vi + 1].p.set(
            smoothPos[i].x + widthVec.x * hw,
            smoothPos[i].y + widthVec.y * hw,
            smoothPos[i].z + widthVec.z * hw);
        outVerts[vi + 1].color = clr;
        outVerts[vi + 1].t = uvRight;
        outVerts[vi + 1].materialID = 0;
        outVerts[vi + 1]._pad = 0;
    }

    // --- Step 5: Index generation ---
    u32 quadCount = pointCount - 1;
    u32 baseIndex = (u32)outIndices.size();
    outIndices.resize(baseIndex + quadCount * 6);

    for (u32 i = 0; i < quadCount; i++) {
        u16 tl = (u16)(baseVertex + i * 2);
        u16 tr = (u16)(baseVertex + i * 2 + 1);
        u16 bl = (u16)(baseVertex + i * 2 + 2);
        u16 br = (u16)(baseVertex + i * 2 + 3);

        u32 ii = baseIndex + i * 6;
        outIndices[ii + 0] = tl;
        outIndices[ii + 1] = tr;
        outIndices[ii + 2] = bl;
        outIndices[ii + 3] = bl;
        outIndices[ii + 4] = tr;
        outIndices[ii + 5] = br;
    }
}

// ═══════════════════════════════════════════════════════
//  Buffer management
// ═══════════════════════════════════════════════════════

static void EnsureRibbonVB(nvrhi::IDevice* nvDevice, u32 vertexCount, RibbonPassState& state)
{
    if (state.ribbonVB && state.ribbonVBCapacity >= vertexCount)
        return;

    u32 capacity = std::max(vertexCount, 64u);
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = capacity * sizeof(ParticleVertex);
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "RibbonVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = true;
    state.ribbonVB = nvDevice->createBuffer(vbDesc);
    state.ribbonVBCapacity = state.ribbonVB ? capacity : 0;
}

static void EnsureRibbonIB(nvrhi::IDevice* nvDevice, u32 indexCount, RibbonPassState& state)
{
    if (state.ribbonIB && state.ribbonIBCapacity >= indexCount)
        return;

    u32 capacity = std::max(indexCount, 128u);
    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = capacity * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "RibbonIB";
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;
    ibDesc.keepInitialState = true;
    state.ribbonIB = nvDevice->createBuffer(ibDesc);
    state.ribbonIBCapacity = state.ribbonIB ? capacity : 0;
}

// ═══════════════════════════════════════════════════════
//  Pipeline initialization
// ═══════════════════════════════════════════════════════

void InitializeRibbonResources(ng::RenderDevice* device, nvrhi::IFramebuffer* framebuffer, RibbonPassState& state)
{
    if (state.initialized)
        return;

    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice)
        return;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    auto* backend = device->GetBackend();
    nvrhi::IBindingLayout* bindlessLayout = backend ? backend->GetBindlessLayout() : nullptr;
    auto& cache = GetPassResourceCache();
    auto fbInfo = framebuffer->getFramebufferInfo();

    nvrhi::BindingLayoutDesc layoutDesc;
    layoutDesc.visibility = nvrhi::ShaderType::All;
    layoutDesc.bindings = {
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(0),   // DynamicTransforms
        nvrhi::BindingLayoutItem::VolatileConstantBuffer(2),   // StaticGlobals
        nvrhi::BindingLayoutItem::StructuredBuffer_SRV(8),     // MaterialBuffer
        nvrhi::BindingLayoutItem::Sampler(0),
    };
    state.layout = nvDevice->createBindingLayout(layoutDesc);

    auto vsResult = shaderLoader->LoadVertexShader("bindless_particle", "main");
    if (!vsResult.handle)
        return;
    state.vs = vsResult.handle;

    auto psResult = shaderLoader->LoadPixelShader("ribbon", "main");
    if (!psResult.handle)
        return;
    state.ps = psResult.handle;

    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Repeat);
    samplerDesc.setAllFilters(true);
    samplerDesc.setMaxAnisotropy(8.0f);
    state.sampler = nvDevice->createSampler(samplerDesc);

    constexpr u32 stride = sizeof(ParticleVertex);
    nvrhi::VertexAttributeDesc attribs[] = {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(0).setElementStride(stride),
        nvrhi::VertexAttributeDesc().setName("COLOR").setFormat(nvrhi::Format::BGRA8_UNORM).setBufferIndex(0).setOffset(12).setElementStride(stride),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setBufferIndex(0).setOffset(16).setElementStride(stride),
        nvrhi::VertexAttributeDesc().setName("MATERIALID").setFormat(nvrhi::Format::R32_UINT).setBufferIndex(0).setOffset(24).setElementStride(stride),
    };
    state.inputLayout = nvDevice->createInputLayout(attribs, 4, state.vs);

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = state.vs;
    pipeDesc.PS = state.ps;
    pipeDesc.inputLayout = state.inputLayout;
    pipeDesc.bindingLayouts.push_back(state.layout);
    if (bindlessLayout)
        pipeDesc.bindingLayouts.push_back(bindlessLayout);
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;
    pipeDesc.renderState.depthStencilState.depthTestEnable = true;
    pipeDesc.renderState.depthStencilState.depthWriteEnable = false;
    pipeDesc.renderState.depthStencilState.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    pipeDesc.renderState.rasterState.cullMode = nvrhi::RasterCullMode::None;
    pipeDesc.renderState.blendState.targets[0].enableBlend();
    pipeDesc.renderState.blendState.targets[0].srcBlend = nvrhi::BlendFactor::SrcAlpha;
    pipeDesc.renderState.blendState.targets[0].destBlend = nvrhi::BlendFactor::InvSrcAlpha;
    pipeDesc.renderState.blendState.targets[0].srcBlendAlpha = nvrhi::BlendFactor::One;
    pipeDesc.renderState.blendState.targets[0].destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;

    state.pipeline = cache.GetOrCreatePipeline("RibbonPass_blend", pipeDesc, fbInfo, nvDevice);

    if (state.pipeline) {
        const auto& actualDesc = state.pipeline->getDesc();
        if (!actualDesc.bindingLayouts.empty())
            state.layout = actualDesc.bindingLayouts[0];
    }

    state.initialized = true;
    Msg("* [RibbonPass] Pipeline initialization complete");
}

// ═══════════════════════════════════════════════════════
//  Pass setup
// ═══════════════════════════════════════════════════════

RibbonPassOutput setupRibbonPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    const DefaultOutputLayout& forwardInputs,
    u32 width,
    u32 height,
    RibbonPassState* state)
{
    if (state) {
        Fvector emitterPos;
        emitterPos.mad(Device.vCameraPosition, Device.vCameraDirection, 5.f);
        UpdateTrailPoints(*state, emitterPos, Device.fTimeDelta, state->defaultHalfWidth);
    }

    auto& passData = fg.addCallbackPass<RibbonPassData>(
        "Ribbon",
        [&, width, height, state](FrameGraph& builder, PassHandle passHandle, RibbonPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.passState = state;

            data.inputColor = passBuilder.read(forwardInputs.albedo);
            data.outputColor = passBuilder.write(forwardInputs.albedo, ResourceState::RenderTarget);
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);

            data.outputs.albedo = data.outputColor;
            data.outputs.normal = forwardInputs.normal;
            data.outputs.baseColor = forwardInputs.baseColor;
            data.outputs.worldPos = forwardInputs.worldPos;
            data.outputs.depth = data.depth;
        },
        [](const RibbonPassData& data, const FrameGraph& fg, ng::RenderContext* ctx) {
            if (!data.passState || data.passState->pointCount < 2)
                return;

            auto* colorRT = fg.GetPhysicalTexture(data.outputColor);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);
            if (!colorRT || !depthRT)
                return;

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            if (!nvDevice || !cmdList)
                return;

            auto& matBuffer = MaterialBuffer::Instance();
            matBuffer.Upload(ctx);

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(colorRT);
            fbDesc.setDepthAttachment(depthRT);
            auto& cache = GetPassResourceCache();
            auto framebuffer = cache.GetOrCreateFramebuffer("RibbonPass", fbDesc, nvDevice);
            if (!framebuffer)
                return;

            InitializeRibbonResources(data.device, framebuffer, *data.passState);
            if (!data.passState->initialized)
                return;

            auto& st = *data.passState;

            // Extract camera basis from inverse view matrix
            Fmatrix invView;
            invView.invert(Device.mView);
            Fvector invViewX = {invView.i.x, invView.i.y, invView.i.z};  // camera right
            Fvector invViewY = {invView.j.x, invView.j.y, invView.j.z};  // camera up

            // Split points by group ID and generate geometry per group
            auto groups = SplitRibbonGroups(st.points, st.pointCount);

            xr_vector<ParticleVertex> vertices;
            xr_vector<u16> indices;
            u32 maxSmooth = st.pointCount * RIBBON_SUBDIVISIONS;
            vertices.reserve(maxSmooth * 2);
            indices.reserve(maxSmooth * 6);

            for (const auto& group : groups) {
                if (group.count < 2)
                    continue;
                GenerateRibbonForGroup(
                    &st.points[group.startIdx], group.count,
                    st, Device.mFullTransform, invViewX, invViewY,
                    Device.vCameraPosition, vertices, indices);
            }

            if (vertices.empty())
                return;

            u32 vertCount = (u32)vertices.size();
            u32 idxCount = (u32)indices.size();

            EnsureRibbonVB(nvDevice, vertCount, st);
            EnsureRibbonIB(nvDevice, idxCount, st);
            if (!st.ribbonVB || !st.ribbonIB)
                return;

            cmdList->writeBuffer(st.ribbonVB, vertices.data(), vertCount * sizeof(ParticleVertex));
            cmdList->writeBuffer(st.ribbonIB, indices.data(), idxCount * sizeof(u16));

            // Constant buffers
            const auto& rtDesc = colorRT->getDesc();

            auto dynTransformsCB = cache.GetOrCreateVolatileCB("RibbonPass", "DynTransforms", sizeof(DynamicTransforms), 16, nvDevice).Get();
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB("RibbonPass", "StaticGlobals", sizeof(StaticGlobals), 16, nvDevice).Get();

            DynamicTransforms dynTrans = {};
            FillDynamicTransforms(dynTrans);
            cmdList->writeBuffer(dynTransformsCB, &dynTrans, sizeof(dynTrans));

            auto staticGlobals = BuildStaticGlobals();
            cmdList->writeBuffer(staticGlobalsCB, &staticGlobals, sizeof(staticGlobals));

            nvrhi::BindingSetDesc bindDesc;
            bindDesc.bindings = {
                nvrhi::BindingSetItem::ConstantBuffer(0, dynTransformsCB),
                nvrhi::BindingSetItem::ConstantBuffer(2, staticGlobalsCB),
                nvrhi::BindingSetItem::StructuredBuffer_SRV(8, matBuffer.GetBuffer()),
                nvrhi::BindingSetItem::Sampler(0, data.passState->sampler),
            };
            auto bindingSet = cache.GetOrCreateBindingSet(bindDesc, st.layout, nvDevice);

            auto* backend = data.device->GetBackend();
            nvrhi::IDescriptorTable* bindlessTable = backend ? backend->GetBindlessDescriptorTable() : nullptr;

            nvrhi::Viewport viewport(
                0.0f, static_cast<float>(rtDesc.width),
                0.0f, static_cast<float>(rtDesc.height),
                0.0f, 1.0f
            );
            nvrhi::Rect scissor(rtDesc.width, rtDesc.height);

            nvrhi::GraphicsState gfxState;
            gfxState.pipeline = st.pipeline;
            gfxState.framebuffer = framebuffer;
            gfxState.bindings = { bindingSet };
            if (bindlessTable)
                gfxState.addBindingSet(bindlessTable);
            gfxState.vertexBuffers = { {st.ribbonVB, 0, 0} };
            gfxState.indexBuffer = { st.ribbonIB, nvrhi::Format::R16_UINT, 0 };
            gfxState.viewport.addViewport(viewport);
            gfxState.viewport.addScissorRect(scissor);

            cmdList->setGraphicsState(gfxState);
            cmdList->drawIndexed(
                nvrhi::DrawArguments()
                    .setVertexCount(idxCount)
                    .setStartIndexLocation(0)
                    .setStartVertexLocation(0)
            );
        }
    );

    RibbonPassOutput output;
    output.layout.albedo = passData.outputColor;
    output.layout.normal = passData.outputs.normal;
    output.layout.baseColor = passData.outputs.baseColor;
    output.layout.worldPos = passData.outputs.worldPos;
    output.layout.depth = passData.depth;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
