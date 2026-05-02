#include "stdafx.h"
#include "FontPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/fgFontRender.h"
#include "xrEngine/GameFont.h"
#include "xrUICore/ui_base.h"
#include "xrUICore/FontManager/FontManager.h"

namespace xray::render::fg::passes {

struct FontPassData {
    framegraph::VirtualResourceHandle output;
};

framegraph::VirtualResourceHandle setupFontPass(
    framegraph::FrameGraph& fg,
    framegraph::VirtualResourceHandle uiTarget)
{
    using namespace framegraph;

    auto& passData = fg.addCallbackPass<FontPassData>(
        "Font",
        [uiTarget](FrameGraph& builder, PassHandle passHandle, FontPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.output = passBuilder.readWrite(uiTarget, ResourceState::RenderTarget);
        },
        [](const FontPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            if (GEnv.isDedicatedServer || !GEnv.UI)
                return;
            CFontManager* fontMgr = GEnv.UI->m_pFontManager;
            if (!fontMgr)
                return;

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            auto* uiRT = fg.GetPhysicalTexture(data.output);
            if (!cmdList || !uiRT)
                return;

            nvrhi::FramebufferDesc fbDesc;
            fbDesc.addColorAttachment(uiRT);
            auto framebuffer = cmdList->getDevice()->createFramebuffer(fbDesc);

            for (auto fontPtrPtr : fontMgr->m_all_fonts)
            {
                CGameFont* font = *fontPtrPtr;
                if (!font || !font->pFontRender || font->strings.empty())
                    continue;
                auto* fr = static_cast<FGFontRender*>(font->pFontRender);
                fr->OnRender(*font);
                if (fr->HasWork())
                    fr->Draw(cmdList, framebuffer);
            }
        });

    return passData.output;
}

}
