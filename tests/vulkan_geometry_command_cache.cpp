#include "src/Layers/xrRenderVK/GeometryCommandCache.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <type_traits>
#include <vector>
using namespace xray::render::vulkan;
namespace {
template<class T> T h(uintptr_t n) { if constexpr(std::is_pointer_v<T>) return reinterpret_cast<T>(n); else return T(n); }
struct State {
    VkPipeline pipeline{}; VkBuffer vertices{}, indices{}; VkDescriptorSet material{};
    VkExtent2D viewport{}, scissor{};
    std::array<std::array<uint8_t,256>,2> constants{};
    bool operator==(const State& b) const { return pipeline==b.pipeline && vertices==b.vertices &&
        indices==b.indices && material==b.material && viewport.width==b.viewport.width &&
        viewport.height==b.viewport.height && scissor.width==b.scissor.width &&
        scissor.height==b.scissor.height && constants==b.constants; }
} current;
unsigned commands{};
void VKAPI_PTR pipeline(VkCommandBuffer,VkPipelineBindPoint,VkPipeline p) { current.pipeline=p; ++commands; }
void VKAPI_PTR viewport(VkCommandBuffer,uint32_t,uint32_t,const VkViewport* p) { current.viewport={uint32_t(p->width),uint32_t(p->height)}; ++commands; }
void VKAPI_PTR scissor(VkCommandBuffer,uint32_t,uint32_t,const VkRect2D* p) { current.scissor=p->extent; ++commands; }
void VKAPI_PTR vertices(VkCommandBuffer,uint32_t,uint32_t,const VkBuffer* p,const VkDeviceSize*) {current.vertices=*p;++commands;}
void VKAPI_PTR indices(VkCommandBuffer,VkBuffer p,VkDeviceSize,VkIndexType) {current.indices=p;++commands;}
void VKAPI_PTR descriptor(VkCommandBuffer,VkPipelineBindPoint,VkPipelineLayout,uint32_t,uint32_t,const VkDescriptorSet* p,uint32_t,const uint32_t*) {current.material=*p;++commands;}
void VKAPI_PTR push(VkCommandBuffer,VkPipelineLayout,VkShaderStageFlags flags,uint32_t offset,uint32_t size,const void* data) {
    for(unsigned stage=0;stage<2;++stage) if(flags & (stage==0?VK_SHADER_STAGE_VERTEX_BIT:VK_SHADER_STAGE_FRAGMENT_BIT))
        std::memcpy(current.constants[stage].data()+offset,data,size);
    ++commands;
}
}
int main() {
    ScenePassDispatch vk{}; vk.cmd_bind_pipeline=pipeline;vk.cmd_set_viewport=viewport;
    vk.cmd_set_scissor=scissor;vk.cmd_bind_vertex_buffers=vertices;vk.cmd_bind_index_buffer=indices;
    vk.cmd_bind_descriptor_sets=descriptor;vk.cmd_push_constants=push;
    FrameRecordingContext frame{h<VkCommandBuffer>(1),h<VkRenderPass>(2),h<VkFramebuffer>(3),{640,480}};
    std::vector<State> reference;
    unsigned baseline{};
    for(bool cached:{false,true}) {
        GeometryCommandCache cache;
        // Keep the same state across thousands of draws, then vary every input,
        // overlap stage-specific constants and restart a batch/command buffer.
        current={};commands=0;
        auto run=[&] {
            for(unsigned i=0;i<5000;++i) {
                auto f=frame;if(i>=4900)f.extent={320,240};if(i>=4950)f.command_buffer=h<VkCommandBuffer>(9);
                cache.bind(vk,f,h<VkPipeline>(i/1250+10),h<VkPipelineLayout>(20+i/4800),
                    h<VkBuffer>(30+i/2000),h<VkBuffer>(40+i/3000),h<VkDescriptorSet>(50+i/100));
                float mvp[16]{};mvp[0]=float(i/500);
                float surface[12]{};surface[0]=float(i/100);
                float matrices[28]{};matrices[0]=1.f;
                cache.push(vk,f.command_buffer,h<VkPipelineLayout>(20+i/4800),VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(mvp),mvp);
                cache.push(vk,f.command_buffer,h<VkPipelineLayout>(20+i/4800),VK_SHADER_STAGE_FRAGMENT_BIT,80,sizeof(surface),surface);
                cache.push(vk,f.command_buffer,h<VkPipelineLayout>(20+i/4800),VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,128,sizeof(matrices),matrices);
                if(cached) assert(current==reference[i]); else reference.push_back(current);
            }
        };
        if(cached) { GeometryCommandCache::Batch scope(cache);run(); } else run();
        if(cached) {
            std::cout<<"5000 draws: state commands "<<baseline<<" -> "<<commands<<" (identical draw states)\n";
            assert(commands<baseline/20);
            // Outside the scope state may have been changed by another renderer.
            const auto count=commands;
            cache.bind(vk,frame,h<VkPipeline>(10),h<VkPipelineLayout>(20),h<VkBuffer>(30),h<VkBuffer>(40),h<VkDescriptorSet>(50));
            assert(commands==count+6);
            GeometryCommandCache::Batch scope(cache);
            float value=1,other=2;
            cache.bind(vk,frame,h<VkPipeline>(10),h<VkPipelineLayout>(20),h<VkBuffer>(30),h<VkBuffer>(40),h<VkDescriptorSet>(50));
            cache.push(vk,frame.command_buffer,h<VkPipelineLayout>(20),VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,80,4,&value);
            cache.push(vk,frame.command_buffer,h<VkPipelineLayout>(20),VK_SHADER_STAGE_FRAGMENT_BIT,80,4,&other);
            cache.push(vk,frame.command_buffer,h<VkPipelineLayout>(20),VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,80,4,&value);
            assert(std::memcmp(current.constants[0].data()+80,&value,4)==0 &&
                std::memcmp(current.constants[1].data()+80,&value,4)==0);
        } else baseline=commands;
    }
}
