#include "xrEngine/stdafx.h"
#include "Light_DB.h"
#include "VulkanSettings.h"
#include "Common/LevelStructure.hpp"
namespace xray::render::vulkan
{
void CLight_DB::Load(IReader& level)
{
    Unload();
    std::string error;
    if(IReader* chunk=level.open_chunk(fsL_LIGHT_DYNAMIC))
    {
        if(!decode_level_lights({static_cast<const uint8_t*>(chunk->pointer()),chunk->length()},false,static_,error))
            Msg("! [renderer-vulkan] level.lights chunk=6 rejected: %s",error.c_str());
        chunk->close();
    }
    if(FS.exist("$level$","build.lights"))
    {
        if(IReader* file=FS.r_open("$level$","build.lights"))
        {
            if(IReader* chunk=file->open_chunk(fsL_HEADER))
            {
                std::vector<VulkanLightSnapshot> hemi;
                if(decode_level_lights({static_cast<const uint8_t*>(chunk->pointer()),chunk->length()},true,hemi,error))
                    hemi_.reset(std::move(hemi));
                else Msg("! [renderer-vulkan] level.hemi build.lights rejected: %s",error.c_str());
                chunk->close();
            }
            FS.r_close(file);
        }
    }
    Msg("[renderer-vulkan] level.lights direct=%zu hemi=%zu",static_.size(),hemi_.size());
}
void CLight_DB::Unload()
{
    static_.clear(); dynamic_.clear(); scene_.clear(); hemi_.reset();
}
void CLight_DB::Update(const std::vector<VulkanLight*>& dynamic)
{
    dynamic_.clear();
    for (const VulkanLight* light : dynamic)
        if (light && light->snapshot().active) dynamic_.push_back(light->snapshot());
    collect_scene_lights(scene_, dynamic_, static_, legacy_level_lights != 0);
}
}
