//
// Copyright 2017-2023 Valve Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef STEAMAUDIO_API_INTERFACES_H
#define STEAMAUDIO_API_INTERFACES_H

#include "phonon.h"

namespace api {

class IContext;
class ISerializedObject;
class IEmbreeDevice;
class IOpenCLDeviceList;
class IOpenCLDevice;
class IRadeonRaysDevice;
class ITrueAudioNextDevice;
class IScene;
class IStaticMesh;
class IInstancedMesh;
class IHRTF;
class IPanningEffect;
class IBinauralEffect;
class IVirtualSurroundEffect;
class IAmbisonicsEncodeEffect;
class IAmbisonicsPanningEffect;
class IAmbisonicsBinauralEffect;
class IAmbisonicsRotationEffect;
class IAmbisonicsDecodeEffect;
class IDirectEffect;
class IReflectionEffect;
class IReflectionMixer;
class IPathEffect;
class IProbeArray;
class IProbeBatch;
class ISimulator;
class ISource;

class IContext
{
public:
    virtual IContext* retain() = 0;

    virtual void release() = 0;

    virtual void setProfilerContext(void* profilerContext) = 0;

    virtual IPLVector3 calculateRelativeDirection(IPLVector3 sourcePosition,
                                                  IPLVector3 listenerPosition,
                                                  IPLVector3 listenerAhead,
                                                  IPLVector3 listenerUp) = 0;

    virtual IPLerror createSerializedObject(IPLSerializedObjectSettings* settings,
                                            ISerializedObject** serializedObject) = 0;

    virtual IPLerror createEmbreeDevice(IPLEmbreeDeviceSettings* settings,
                                        IEmbreeDevice** device) = 0;

    virtual IPLerror createOpenCLDeviceList(IPLOpenCLDeviceSettings* settings,
                                            IOpenCLDeviceList** deviceList) = 0;

    virtual IPLerror createOpenCLDevice(IOpenCLDeviceList* deviceList,
                                        IPLint32 index,
                                        IOpenCLDevice** device) = 0;

    virtual IPLerror createOpenCLDeviceFromExisting(void* convolutionQueue,
                                                    void* irUpdateQueue,
                                                    IOpenCLDevice** device) = 0;

    virtual IPLerror createScene(IPLSceneSettings* settings,
                                 IScene** scene) = 0;

    virtual IPLerror loadScene(IPLSceneSettings* settings,
                               ISerializedObject* serializedObject,
                               IPLProgressCallback progressCallback,
                               void* userData,
                               IScene** scene) = 0;

    virtual IPLerror allocateAudioBuffer(IPLint32 numChannels,
                                         IPLint32 numSamples,
                                         IPLAudioBuffer* audioBuffer) = 0;

    virtual void freeAudioBuffer(IPLAudioBuffer* audioBuffer) = 0;

    virtual void interleaveAudioBuffer(IPLAudioBuffer* src,
                                       IPLfloat32* dst) = 0;

    virtual void deinterleaveAudioBuffer(IPLfloat32* src,
                                         IPLAudioBuffer* dst) = 0;

    virtual void mixAudioBuffer(IPLAudioBuffer* in,
                                IPLAudioBuffer* mix) = 0;

    virtual void downmixAudioBuffer(IPLAudioBuffer* in,
                                    IPLAudioBuffer* out) = 0;

    virtual void convertAmbisonicAudioBuffer(IPLAmbisonicsType inType,
                                             IPLAmbisonicsType outType,
                                             IPLAudioBuffer* in,
                                             IPLAudioBuffer* out) = 0;

    virtual IPLerror createHRTF(IPLAudioSettings* audioSettings,
                                IPLHRTFSettings* hrtfSettings,
                                IHRTF** hrtf) = 0;

    virtual IPLerror createPanningEffect(IPLAudioSettings* audioSettings,
                                         IPLPanningEffectSettings* effectSettings,
                                         IPanningEffect** effect) = 0;

    virtual IPLerror createBinauralEffect(IPLAudioSettings* audioSettings,
                                          IPLBinauralEffectSettings* effectSettings,
                                          IBinauralEffect** effect) = 0;

    virtual IPLerror createVirtualSurroundEffect(IPLAudioSettings* audioSettings,
                                                 IPLVirtualSurroundEffectSettings* effectSettings,
                                                 IVirtualSurroundEffect** effect) = 0;

    virtual IPLerror createAmbisonicsEncodeEffect(IPLAudioSettings* audioSettings,
                                                  IPLAmbisonicsEncodeEffectSettings* effectSettings,
                                                  IAmbisonicsEncodeEffect** effect) = 0;

    virtual IPLerror createAmbisonicsPanningEffect(IPLAudioSettings* audioSettings,
                                                   IPLAmbisonicsPanningEffectSettings* effectSettings,
                                                   IAmbisonicsPanningEffect** effect) = 0;

    virtual IPLerror createAmbisonicsBinauralEffect(IPLAudioSettings* audioSettings,
                                                    IPLAmbisonicsBinauralEffectSettings* effectSettings,
                                                    IAmbisonicsBinauralEffect** effect) = 0;

    virtual IPLerror createAmbisonicsRotationEffect(IPLAudioSettings* audioSettings,
                                                    IPLAmbisonicsRotationEffectSettings* effectSettings,
                                                    IAmbisonicsRotationEffect** effect) = 0;

    virtual IPLerror createAmbisonicsDecodeEffect(IPLAudioSettings* audioSettings,
                                                  IPLAmbisonicsDecodeEffectSettings* effectSettings,
                                                  IAmbisonicsDecodeEffect** effect) = 0;

    virtual IPLerror createDirectEffect(IPLAudioSettings* audioSettings,
                                        IPLDirectEffectSettings* effectSettings,
                                        IDirectEffect** effect) = 0;

    virtual IPLerror createReflectionEffect(IPLAudioSettings* audioSettings,
                                            IPLReflectionEffectSettings* effectSettings,
                                            IReflectionEffect** effect) = 0;

    virtual IPLerror createReflectionMixer(IPLAudioSettings* audioSettings,
                                           IPLReflectionEffectSettings* effectSettings,
                                           IReflectionMixer** mixer) = 0;

    virtual IPLerror createPathEffect(IPLAudioSettings* audioSettings,
                                      IPLPathEffectSettings* effectSettings,
                                      IPathEffect** effect) = 0;

    virtual IPLerror createProbeArray(IProbeArray** probeArray) = 0;

    virtual IPLerror createProbeBatch(IProbeBatch** probeBatch) = 0;

    virtual IPLerror loadProbeBatch(ISerializedObject* serializedObject,
                                    IProbeBatch** probeBatch) = 0;

    virtual void bakeReflections(IPLReflectionsBakeParams* params,
                                 IPLProgressCallback progressCallback,
                                 void* userData) = 0;

    virtual void cancelBakeReflections() = 0;

    virtual void bakePaths(IPLPathBakeParams* params,
                           IPLProgressCallback progressCallback,
                           void* userData) = 0;

    virtual void cancelBakePaths() = 0;

    virtual IPLerror createSimulator(IPLSimulationSettings* settings,
                                     ISimulator** simulator) = 0;

    virtual IPLfloat32 calculateDistanceAttenuation(IPLVector3 source,
                                                    IPLVector3 listener,
                                                    IPLDistanceAttenuationModel* model) = 0;

    virtual void calculateAirAbsorption(IPLVector3 source,
                                        IPLVector3 listener,
                                        IPLAirAbsorptionModel* model,
                                        IPLfloat32* airAbsorption) = 0;

    virtual IPLfloat32 calculateDirectivity(IPLCoordinateSpace3 source,
                                            IPLVector3 listener,
                                            IPLDirectivity* model) = 0;
};

class ISerializedObject
{
public:
    virtual ISerializedObject* retain() = 0;

    virtual void release() = 0;

    virtual IPLsize getSize() = 0;

    virtual IPLbyte* getData() = 0;
};

class IEmbreeDevice
{
public:
    virtual IEmbreeDevice* retain() = 0;

    virtual void release() = 0;
};

class IOpenCLDeviceList
{
public:
    virtual IOpenCLDeviceList* retain() = 0;

    virtual void release() = 0;

    virtual IPLint32 getNumDevices() = 0;

    virtual void getDeviceDesc(IPLint32 index,
                               IPLOpenCLDeviceDesc* deviceDesc) = 0;
};

class IOpenCLDevice
{
public:
    virtual IOpenCLDevice* retain() = 0;

    virtual void release() = 0;

    virtual IPLerror createRadeonRaysDevice(IPLRadeonRaysDeviceSettings* settings,
                                            IRadeonRaysDevice** device) = 0;

    virtual IPLerror createTrueAudioNextDevice(IPLTrueAudioNextDeviceSettings* settings,
                                               ITrueAudioNextDevice** device) = 0;
};

class IRadeonRaysDevice
{
public:
    virtual IRadeonRaysDevice* retain() = 0;

    virtual void release() = 0;
};

class ITrueAudioNextDevice
{
public:
    virtual ITrueAudioNextDevice* retain() = 0;

    virtual void release() = 0;
};

class IScene
{
public:
    virtual IScene* retain() = 0;

    virtual void release() = 0;

    virtual void save(ISerializedObject* serializedObject) = 0;

    virtual void saveOBJ(IPLstring fileBaseName) = 0;

    virtual void commit() = 0;

    virtual IPLerror createStaticMesh(IPLStaticMeshSettings* settings,
                                      IStaticMesh** staticMesh) = 0;

    virtual IPLerror loadStaticMesh(ISerializedObject* serializedObject,
                                    IPLProgressCallback progressCallback,
                                    void* userData,
                                    IStaticMesh** staticMesh) = 0;

    virtual IPLerror createInstancedMesh(IPLInstancedMeshSettings* settings,
                                         IInstancedMesh** instancedMesh) = 0;
};

class IStaticMesh
{
public:
    virtual IStaticMesh* retain() = 0;

    virtual void release() = 0;

    virtual void save(ISerializedObject* serializedObject) = 0;

    virtual void add(IScene* scene) = 0;

    virtual void remove(IScene* scene) = 0;
};

class IInstancedMesh
{
public:
    virtual IInstancedMesh* retain() = 0;

    virtual void release() = 0;

    virtual void add(IScene* scene) = 0;

    virtual void remove(IScene* scene) = 0;

    virtual void updateTransform(IScene* scene,
                                 IPLMatrix4x4 transform) = 0;
};

class IHRTF
{
public:
    virtual IHRTF* retain() = 0;

    virtual void release() = 0;
};

class IPanningEffect
{
public:
    virtual IPanningEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLPanningEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IBinauralEffect
{
public:
    virtual IBinauralEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLBinauralEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IVirtualSurroundEffect
{
public:
    virtual IVirtualSurroundEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLVirtualSurroundEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IAmbisonicsEncodeEffect
{
public:
    virtual IAmbisonicsEncodeEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLAmbisonicsEncodeEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IAmbisonicsPanningEffect
{
public:
    virtual IAmbisonicsPanningEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLAmbisonicsPanningEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IAmbisonicsBinauralEffect
{
public:
    virtual IAmbisonicsBinauralEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLAmbisonicsBinauralEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IAmbisonicsRotationEffect
{
public:
    virtual IAmbisonicsRotationEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLAmbisonicsRotationEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IAmbisonicsDecodeEffect
{
public:
    virtual IAmbisonicsDecodeEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLAmbisonicsDecodeEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IDirectEffect
{
public:
    virtual IDirectEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLDirectEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IReflectionEffect
{
public:
    virtual IReflectionEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLReflectionEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out,
                                      IReflectionMixer* mixer) = 0;
};

class IReflectionMixer
{
public:
    virtual IReflectionMixer* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLReflectionEffectParams* params,
                                      IPLAudioBuffer* out) = 0;
};

class IPathEffect
{
public:
    virtual IPathEffect* retain() = 0;

    virtual void release() = 0;

    virtual void reset() = 0;

    virtual IPLAudioEffectState apply(IPLPathEffectParams* params,
                                      IPLAudioBuffer* in,
                                      IPLAudioBuffer* out) = 0;
};

class IProbeArray
{
public:
    virtual IProbeArray* retain() = 0;

    virtual void release() = 0;

    virtual void generateProbes(IScene* scene,
                                IPLProbeGenerationParams* params) = 0;

    virtual IPLint32 getNumProbes() = 0;

    virtual IPLSphere getProbe(IPLint32 index) = 0;
};

class IProbeBatch
{
public:
    virtual IProbeBatch* retain() = 0;

    virtual void release() = 0;

    virtual void save(ISerializedObject* serializedObject) = 0;

    virtual IPLint32 getNumProbes() = 0;

    virtual void addProbe(IPLSphere probe) = 0;

    virtual void addProbeArray(IProbeArray* probeArray) = 0;

    virtual void removeProbe(IPLint32 index) = 0;

    virtual void commit() = 0;

    virtual void removeData(IPLBakedDataIdentifier* identifier) = 0;

    virtual IPLsize getDataSize(IPLBakedDataIdentifier* identifier) = 0;
};

class ISimulator
{
public:
    virtual ISimulator* retain() = 0;

    virtual void release() = 0;

    virtual void setScene(IScene* scene) = 0;

    virtual void addProbeBatch(IProbeBatch* probeBatch) = 0;

    virtual void removeProbeBatch(IProbeBatch* probeBatch) = 0;

    virtual void setSharedInputs(IPLSimulationFlags flags,
                                 IPLSimulationSharedInputs* sharedInputs) = 0;

    virtual void commit() = 0;

    virtual void runDirect() = 0;

    virtual void runReflections() = 0;

    virtual void runPathing() = 0;

    virtual IPLerror createSource(IPLSourceSettings* settings,
                                  ISource** source) = 0;
};

class ISource
{
public:
    virtual ISource* retain() = 0;

    virtual void release() = 0;

    virtual void add(ISimulator* simulator) = 0;

    virtual void remove(ISimulator* simulator) = 0;

    virtual void setInputs(IPLSimulationFlags flags,
                           IPLSimulationInputs* inputs) = 0;

    virtual void getOutputs(IPLSimulationFlags flags,
                            IPLSimulationOutputs* outputs) = 0;
};

}

#if !defined(STEAMAUDIO_SKIP_API_FUNCTIONS)

#if !defined(STEAMAUDIO_BUILDING_CORE)
IPLerror IPLCALL iplContextCreate(IPLContextSettings* settings,
                          IPLContext* context)
{
    return IPL_STATUS_FAILURE;
}
#endif

IPLContext IPLCALL iplContextRetain(IPLContext context)
{
    if (!context)
        return nullptr;

    return reinterpret_cast<IPLContext>(reinterpret_cast<api::IContext*>(context)->retain());
}

void IPLCALL iplContextRelease(IPLContext* context)
{
    if (!context || !*context)
        return;

    reinterpret_cast<api::IContext*>(*context)->release();

    *context = nullptr;
}

IPLVector3 IPLCALL iplCalculateRelativeDirection(IPLContext context,
                                         IPLVector3 sourcePosition,
                                         IPLVector3 listenerPosition,
                                         IPLVector3 listenerAhead,
                                         IPLVector3 listenerUp)
{
    if (!context)
        return IPLVector3{};

    return reinterpret_cast<api::IContext*>(context)->calculateRelativeDirection(sourcePosition, listenerPosition, listenerAhead, listenerUp);
}

IPLerror IPLCALL iplSerializedObjectCreate(IPLContext context,
                                   IPLSerializedObjectSettings* settings,
                                   IPLSerializedObject* serializedObject)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createSerializedObject(settings, reinterpret_cast<api::ISerializedObject**>(serializedObject));
}

IPLSerializedObject IPLCALL iplSerializedObjectRetain(IPLSerializedObject serializedObject)
{
    if (!serializedObject)
        return nullptr;

    return reinterpret_cast<IPLSerializedObject>(reinterpret_cast<api::ISerializedObject*>(serializedObject)->retain());
}

void IPLCALL iplSerializedObjectRelease(IPLSerializedObject* serializedObject)
{
    if (!serializedObject || !*serializedObject)
        return;

    reinterpret_cast<api::ISerializedObject*>(*serializedObject)->release();

    *serializedObject = nullptr;
}

IPLsize IPLCALL iplSerializedObjectGetSize(IPLSerializedObject serializedObject)
{
    if (!serializedObject)
        return 0;

    return reinterpret_cast<api::ISerializedObject*>(serializedObject)->getSize();
}

IPLbyte* IPLCALL iplSerializedObjectGetData(IPLSerializedObject serializedObject)
{
    if (!serializedObject)
        return 0;

    return reinterpret_cast<api::ISerializedObject*>(serializedObject)->getData();
}

IPLerror IPLCALL iplEmbreeDeviceCreate(IPLContext context,
                               IPLEmbreeDeviceSettings* settings,
                               IPLEmbreeDevice* device)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createEmbreeDevice(settings, reinterpret_cast<api::IEmbreeDevice**>(device));
}

IPLEmbreeDevice IPLCALL iplEmbreeDeviceRetain(IPLEmbreeDevice device)
{
    if (!device)
        return nullptr;

    return reinterpret_cast<IPLEmbreeDevice>(reinterpret_cast<api::IEmbreeDevice*>(device)->retain());
}

void IPLCALL iplEmbreeDeviceRelease(IPLEmbreeDevice* device)
{
    if (!device || !*device)
        return;

    reinterpret_cast<api::IEmbreeDevice*>(*device)->release();

    *device = nullptr;
}

IPLerror IPLCALL iplOpenCLDeviceListCreate(IPLContext context,
                                   IPLOpenCLDeviceSettings* settings,
                                   IPLOpenCLDeviceList* deviceList)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createOpenCLDeviceList(settings, reinterpret_cast<api::IOpenCLDeviceList**>(deviceList));
}

IPLOpenCLDeviceList IPLCALL iplOpenCLDeviceListRetain(IPLOpenCLDeviceList deviceList)
{
    if (!deviceList)
        return nullptr;

    return reinterpret_cast<IPLOpenCLDeviceList>(reinterpret_cast<api::IOpenCLDeviceList*>(deviceList)->retain());
}

void IPLCALL iplOpenCLDeviceListRelease(IPLOpenCLDeviceList* deviceList)
{
    if (!deviceList || !*deviceList)
        return;

    reinterpret_cast<api::IOpenCLDeviceList*>(*deviceList)->release();

    *deviceList = nullptr;
}

IPLint32 IPLCALL iplOpenCLDeviceListGetNumDevices(IPLOpenCLDeviceList deviceList)
{
    if (!deviceList)
        return 0;

    return reinterpret_cast<api::IOpenCLDeviceList*>(deviceList)->getNumDevices();
}

void IPLCALL iplOpenCLDeviceListGetDeviceDesc(IPLOpenCLDeviceList deviceList,
                                      IPLint32 index,
                                      IPLOpenCLDeviceDesc* deviceDesc)
{
    if (!deviceList)
        return;

    reinterpret_cast<api::IOpenCLDeviceList*>(deviceList)->getDeviceDesc(index, deviceDesc);
}

IPLerror IPLCALL iplOpenCLDeviceCreate(IPLContext context,
                               IPLOpenCLDeviceList deviceList,
                               IPLint32 index,
                               IPLOpenCLDevice* device)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createOpenCLDevice(reinterpret_cast<api::IOpenCLDeviceList*>(deviceList), index, reinterpret_cast<api::IOpenCLDevice**>(device));
}

IPLerror IPLCALL iplOpenCLDeviceCreateFromExisting(IPLContext context,
                                           void* convolutionQueue,
                                           void* irUpdateQueue,
                                           IPLOpenCLDevice* device)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createOpenCLDeviceFromExisting(convolutionQueue, irUpdateQueue, reinterpret_cast<api::IOpenCLDevice**>(device));
}

IPLOpenCLDevice IPLCALL iplOpenCLDeviceRetain(IPLOpenCLDevice device)
{
    if (!device)
        return nullptr;

    return reinterpret_cast<IPLOpenCLDevice>(reinterpret_cast<api::IOpenCLDevice*>(device)->retain());
}

void IPLCALL iplOpenCLDeviceRelease(IPLOpenCLDevice* device)
{
    if (!device || !*device)
        return;

    reinterpret_cast<api::IOpenCLDevice*>(*device)->release();

    *device = nullptr;
}

IPLerror IPLCALL iplRadeonRaysDeviceCreate(IPLOpenCLDevice openCLDevice,
                                   IPLRadeonRaysDeviceSettings* settings,
                                   IPLRadeonRaysDevice* rrDevice)
{
    if (!openCLDevice)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IOpenCLDevice*>(openCLDevice)->createRadeonRaysDevice(settings, reinterpret_cast<api::IRadeonRaysDevice**>(rrDevice));
}

IPLRadeonRaysDevice IPLCALL iplRadeonRaysDeviceRetain(IPLRadeonRaysDevice device)
{
    if (!device)
        return nullptr;

    return reinterpret_cast<IPLRadeonRaysDevice>(reinterpret_cast<api::IRadeonRaysDevice*>(device)->retain());
}

void IPLCALL iplRadeonRaysDeviceRelease(IPLRadeonRaysDevice* device)
{
    if (!device || !*device)
        return;

    reinterpret_cast<api::IRadeonRaysDevice*>(*device)->release();

    *device = nullptr;
}

IPLerror IPLCALL iplTrueAudioNextDeviceCreate(IPLOpenCLDevice openCLDevice,
                                      IPLTrueAudioNextDeviceSettings* settings,
                                      IPLTrueAudioNextDevice* tanDevice)
{
    if (!openCLDevice)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IOpenCLDevice*>(openCLDevice)->createTrueAudioNextDevice(settings, reinterpret_cast<api::ITrueAudioNextDevice**>(tanDevice));
}

IPLTrueAudioNextDevice IPLCALL iplTrueAudioNextDeviceRetain(IPLTrueAudioNextDevice device)
{
    if (!device)
        return nullptr;

    return reinterpret_cast<IPLTrueAudioNextDevice>(reinterpret_cast<api::ITrueAudioNextDevice*>(device)->retain());
}

void IPLCALL iplTrueAudioNextDeviceRelease(IPLTrueAudioNextDevice* device)
{
    if (!device || !*device)
        return;

    reinterpret_cast<api::ITrueAudioNextDevice*>(*device)->release();

    *device = nullptr;
}

IPLerror IPLCALL iplSceneCreate(IPLContext context,
                        IPLSceneSettings* settings,
                        IPLScene* scene)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createScene(settings, reinterpret_cast<api::IScene**>(scene));
}

IPLScene IPLCALL iplSceneRetain(IPLScene scene)
{
    if (!scene)
        return nullptr;

    return reinterpret_cast<IPLScene>(reinterpret_cast<api::IScene*>(scene)->retain());
}

void IPLCALL iplSceneRelease(IPLScene* scene)
{
    if (!scene || !*scene)
        return;

    reinterpret_cast<api::IScene*>(*scene)->release();

    *scene = nullptr;
}

IPLerror IPLCALL iplSceneLoad(IPLContext context,
                      IPLSceneSettings* settings,
                      IPLSerializedObject serializedObject,
                      IPLProgressCallback progressCallback,
                      void* progressCallbackUserData,
                      IPLScene* scene)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->loadScene(settings, reinterpret_cast<api::ISerializedObject*>(serializedObject),
                                                                progressCallback, progressCallbackUserData,
                                                                reinterpret_cast<api::IScene**>(scene));
}

void IPLCALL iplSceneSave(IPLScene scene,
                  IPLSerializedObject serializedObject)
{
    if (!scene)
        return;

    reinterpret_cast<api::IScene*>(scene)->save(reinterpret_cast<api::ISerializedObject*>(serializedObject));
}

void IPLCALL iplSceneSaveOBJ(IPLScene scene,
                     IPLstring fileBaseName)
{
    if (!scene)
        return;

    reinterpret_cast<api::IScene*>(scene)->saveOBJ(fileBaseName);
}

void IPLCALL iplSceneCommit(IPLScene scene)
{
    if (!scene)
        return;

    reinterpret_cast<api::IScene*>(scene)->commit();
}

IPLerror IPLCALL iplStaticMeshCreate(IPLScene scene,
                             IPLStaticMeshSettings* settings,
                             IPLStaticMesh* staticMesh)
{
    if (!scene)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IScene*>(scene)->createStaticMesh(settings, reinterpret_cast<api::IStaticMesh**>(staticMesh));
}

IPLStaticMesh IPLCALL iplStaticMeshRetain(IPLStaticMesh staticMesh)
{
    if (!staticMesh)
        return nullptr;

    return reinterpret_cast<IPLStaticMesh>(reinterpret_cast<api::IStaticMesh*>(staticMesh)->retain());
}

void IPLCALL iplStaticMeshRelease(IPLStaticMesh* staticMesh)
{
    if (!staticMesh || !*staticMesh)
        return;

    reinterpret_cast<api::IStaticMesh*>(*staticMesh)->release();

    *staticMesh = nullptr;
}

IPLerror IPLCALL iplStaticMeshLoad(IPLScene scene,
                           IPLSerializedObject serializedObject,
                           IPLProgressCallback progressCallback,
                           void* progressCallbackUserData,
                           IPLStaticMesh* staticMesh)
{
    if (!scene)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IScene*>(scene)->loadStaticMesh(reinterpret_cast<api::ISerializedObject*>(serializedObject),
                                                                 progressCallback, progressCallbackUserData,
                                                                 reinterpret_cast<api::IStaticMesh**>(staticMesh));
}

void IPLCALL iplStaticMeshSave(IPLStaticMesh staticMesh,
                       IPLSerializedObject serializedObject)
{
    if (!staticMesh)
        return;

    reinterpret_cast<api::IStaticMesh*>(staticMesh)->save(reinterpret_cast<api::ISerializedObject*>(serializedObject));
}

void IPLCALL iplStaticMeshAdd(IPLStaticMesh staticMesh, IPLScene scene)
{
    if (!staticMesh)
        return;

    reinterpret_cast<api::IStaticMesh*>(staticMesh)->add(reinterpret_cast<api::IScene*>(scene));
}

void IPLCALL iplStaticMeshRemove(IPLStaticMesh staticMesh, IPLScene scene)
{
    if (!staticMesh)
        return;

    reinterpret_cast<api::IStaticMesh*>(staticMesh)->remove(reinterpret_cast<api::IScene*>(scene));
}

IPLerror IPLCALL iplInstancedMeshCreate(IPLScene scene,
                                IPLInstancedMeshSettings* settings,
                                IPLInstancedMesh* instancedMesh)
{
    if (!scene)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IScene*>(scene)->createInstancedMesh(settings, reinterpret_cast<api::IInstancedMesh**>(instancedMesh));
}

IPLInstancedMesh IPLCALL iplInstancedMeshRetain(IPLInstancedMesh instancedMesh)
{
    if (!instancedMesh)
        return nullptr;

    return reinterpret_cast<IPLInstancedMesh>(reinterpret_cast<api::IInstancedMesh*>(instancedMesh)->retain());
}

void IPLCALL iplInstancedMeshRelease(IPLInstancedMesh* instancedMesh)
{
    if (!instancedMesh || !*instancedMesh)
        return;

    reinterpret_cast<api::IInstancedMesh*>(*instancedMesh)->release();

    *instancedMesh = nullptr;
}

void IPLCALL iplInstancedMeshAdd(IPLInstancedMesh instancedMesh, IPLScene scene)
{
    if (!instancedMesh)
        return;

    reinterpret_cast<api::IInstancedMesh*>(instancedMesh)->add(reinterpret_cast<api::IScene*>(scene));
}

void IPLCALL iplInstancedMeshRemove(IPLInstancedMesh instancedMesh, IPLScene scene)
{
    if (!instancedMesh)
        return;

    reinterpret_cast<api::IInstancedMesh*>(instancedMesh)->remove(reinterpret_cast<api::IScene*>(scene));
}

void IPLCALL iplInstancedMeshUpdateTransform(IPLInstancedMesh instancedMesh, IPLScene scene, IPLMatrix4x4 transform)
{
    if (!instancedMesh)
        return;

    reinterpret_cast<api::IInstancedMesh*>(instancedMesh)->updateTransform(reinterpret_cast<api::IScene*>(scene), transform);
}

IPLerror IPLCALL iplAudioBufferAllocate(IPLContext context,
                                IPLint32 numChannels,
                                IPLint32 numSamples,
                                IPLAudioBuffer* audioBuffer)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->allocateAudioBuffer(numChannels, numSamples, audioBuffer);
}

void IPLCALL iplAudioBufferFree(IPLContext context,
                        IPLAudioBuffer* audioBuffer)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->freeAudioBuffer(audioBuffer);
}

void IPLCALL iplAudioBufferInterleave(IPLContext context,
                              IPLAudioBuffer* src,
                              IPLfloat32* dst)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->interleaveAudioBuffer(src, dst);
}

void IPLCALL iplAudioBufferDeinterleave(IPLContext context,
                                IPLfloat32* src,
                                IPLAudioBuffer* dst)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->deinterleaveAudioBuffer(src, dst);
}

void IPLCALL iplAudioBufferMix(IPLContext context,
                       IPLAudioBuffer* in,
                       IPLAudioBuffer* mix)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->mixAudioBuffer(in, mix);
}

void IPLCALL iplAudioBufferDownmix(IPLContext context,
                           IPLAudioBuffer* in,
                           IPLAudioBuffer* out)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->downmixAudioBuffer(in, out);
}

void IPLCALL iplAudioBufferConvertAmbisonics(IPLContext context,
                                     IPLAmbisonicsType inType,
                                     IPLAmbisonicsType outType,
                                     IPLAudioBuffer* in,
                                     IPLAudioBuffer* out)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->convertAmbisonicAudioBuffer(inType, outType, in, out);
}

IPLerror IPLCALL iplHRTFCreate(IPLContext context,
                       IPLAudioSettings* audioSettings,
                       IPLHRTFSettings* hrtfSettings,
                       IPLHRTF* hrtf)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createHRTF(audioSettings, hrtfSettings, reinterpret_cast<api::IHRTF**>(hrtf));
}

IPLHRTF IPLCALL iplHRTFRetain(IPLHRTF hrtf)
{
    if (!hrtf)
        return nullptr;

    return reinterpret_cast<IPLHRTF>(reinterpret_cast<api::IHRTF*>(hrtf)->retain());
}

void IPLCALL iplHRTFRelease(IPLHRTF* hrtf)
{
    if (!hrtf || !*hrtf)
        return;

    reinterpret_cast<api::IHRTF*>(*hrtf)->release();

    *hrtf = nullptr;
}

IPLerror IPLCALL iplPanningEffectCreate(IPLContext context,
                                IPLAudioSettings* audioSettings,
                                IPLPanningEffectSettings* effectSettings,
                                IPLPanningEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createPanningEffect(audioSettings, effectSettings, reinterpret_cast<api::IPanningEffect**>(effect));
}

IPLPanningEffect IPLCALL iplPanningEffectRetain(IPLPanningEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLPanningEffect>(reinterpret_cast<api::IPanningEffect*>(effect)->retain());
}

void IPLCALL iplPanningEffectRelease(IPLPanningEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IPanningEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplPanningEffectReset(IPLPanningEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IPanningEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplPanningEffectApply(IPLPanningEffect effect,
                                          IPLPanningEffectParams* params,
                                          IPLAudioBuffer* in,
                                          IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IPanningEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplBinauralEffectCreate(IPLContext context,
                                 IPLAudioSettings* audioSettings,
                                 IPLBinauralEffectSettings* effectSettings,
                                 IPLBinauralEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createBinauralEffect(audioSettings, effectSettings, reinterpret_cast<api::IBinauralEffect**>(effect));
}

IPLBinauralEffect IPLCALL iplBinauralEffectRetain(IPLBinauralEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLBinauralEffect>(reinterpret_cast<api::IBinauralEffect*>(effect)->retain());
}

void IPLCALL iplBinauralEffectRelease(IPLBinauralEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IBinauralEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplBinauralEffectReset(IPLBinauralEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IBinauralEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplBinauralEffectApply(IPLBinauralEffect effect,
                                           IPLBinauralEffectParams* params,
                                           IPLAudioBuffer* in,
                                           IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IBinauralEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplVirtualSurroundEffectCreate(IPLContext context,
                                        IPLAudioSettings* audioSettings,
                                        IPLVirtualSurroundEffectSettings* effectSettings,
                                        IPLVirtualSurroundEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createVirtualSurroundEffect(audioSettings, effectSettings, reinterpret_cast<api::IVirtualSurroundEffect**>(effect));
}

IPLVirtualSurroundEffect IPLCALL iplVirtualSurroundEffectRetain(IPLVirtualSurroundEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLVirtualSurroundEffect>(reinterpret_cast<api::IVirtualSurroundEffect*>(effect)->retain());
}

void IPLCALL iplVirtualSurroundEffectRelease(IPLVirtualSurroundEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IVirtualSurroundEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplVirtualSurroundEffectReset(IPLVirtualSurroundEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IVirtualSurroundEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplVirtualSurroundEffectApply(IPLVirtualSurroundEffect effect,
                                                  IPLVirtualSurroundEffectParams* params,
                                                  IPLAudioBuffer* in,
                                                  IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IVirtualSurroundEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplAmbisonicsEncodeEffectCreate(IPLContext context,
                                         IPLAudioSettings* audioSettings,
                                         IPLAmbisonicsEncodeEffectSettings* effectSettings,
                                         IPLAmbisonicsEncodeEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createAmbisonicsEncodeEffect(audioSettings, effectSettings, reinterpret_cast<api::IAmbisonicsEncodeEffect**>(effect));
}

IPLAmbisonicsEncodeEffect IPLCALL iplAmbisonicsEncodeEffectRetain(IPLAmbisonicsEncodeEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLAmbisonicsEncodeEffect>(reinterpret_cast<api::IAmbisonicsEncodeEffect*>(effect)->retain());
}

void IPLCALL iplAmbisonicsEncodeEffectRelease(IPLAmbisonicsEncodeEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IAmbisonicsEncodeEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplAmbisonicsEncodeEffectReset(IPLAmbisonicsEncodeEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IAmbisonicsEncodeEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplAmbisonicsEncodeEffectApply(IPLAmbisonicsEncodeEffect effect,
                                                   IPLAmbisonicsEncodeEffectParams* params,
                                                   IPLAudioBuffer* in,
                                                   IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IAmbisonicsEncodeEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplAmbisonicsPanningEffectCreate(IPLContext context,
                                          IPLAudioSettings* audioSettings,
                                          IPLAmbisonicsPanningEffectSettings* effectSettings,
                                          IPLAmbisonicsPanningEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createAmbisonicsPanningEffect(audioSettings, effectSettings, reinterpret_cast<api::IAmbisonicsPanningEffect**>(effect));
}

IPLAmbisonicsPanningEffect IPLCALL iplAmbisonicsPanningEffectRetain(IPLAmbisonicsPanningEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLAmbisonicsPanningEffect>(reinterpret_cast<api::IAmbisonicsPanningEffect*>(effect)->retain());
}

void IPLCALL iplAmbisonicsPanningEffectRelease(IPLAmbisonicsPanningEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IAmbisonicsPanningEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplAmbisonicsPanningEffectReset(IPLAmbisonicsPanningEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IAmbisonicsPanningEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplAmbisonicsPanningEffectApply(IPLAmbisonicsPanningEffect effect,
                                                    IPLAmbisonicsPanningEffectParams* params,
                                                    IPLAudioBuffer* in,
                                                    IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IAmbisonicsPanningEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplAmbisonicsBinauralEffectCreate(IPLContext context,
                                           IPLAudioSettings* audioSettings,
                                           IPLAmbisonicsBinauralEffectSettings* effectSettings,
                                           IPLAmbisonicsBinauralEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createAmbisonicsBinauralEffect(audioSettings, effectSettings, reinterpret_cast<api::IAmbisonicsBinauralEffect**>(effect));
}

IPLAmbisonicsBinauralEffect IPLCALL iplAmbisonicsBinauralEffectRetain(IPLAmbisonicsBinauralEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLAmbisonicsBinauralEffect>(reinterpret_cast<api::IAmbisonicsBinauralEffect*>(effect)->retain());
}

void IPLCALL iplAmbisonicsBinauralEffectRelease(IPLAmbisonicsBinauralEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IAmbisonicsBinauralEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplAmbisonicsBinauralEffectReset(IPLAmbisonicsBinauralEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IAmbisonicsBinauralEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplAmbisonicsBinauralEffectApply(IPLAmbisonicsBinauralEffect effect,
                                                     IPLAmbisonicsBinauralEffectParams* params,
                                                     IPLAudioBuffer* in,
                                                     IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IAmbisonicsBinauralEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplAmbisonicsRotationEffectCreate(IPLContext context,
                                           IPLAudioSettings* audioSettings,
                                           IPLAmbisonicsRotationEffectSettings* effectSettings,
                                           IPLAmbisonicsRotationEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createAmbisonicsRotationEffect(audioSettings, effectSettings, reinterpret_cast<api::IAmbisonicsRotationEffect**>(effect));
}

IPLAmbisonicsRotationEffect IPLCALL iplAmbisonicsRotationEffectRetain(IPLAmbisonicsRotationEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLAmbisonicsRotationEffect>(reinterpret_cast<api::IAmbisonicsRotationEffect*>(effect)->retain());
}

void IPLCALL iplAmbisonicsRotationEffectRelease(IPLAmbisonicsRotationEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IAmbisonicsRotationEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplAmbisonicsRotationEffectReset(IPLAmbisonicsRotationEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IAmbisonicsRotationEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplAmbisonicsRotationEffectApply(IPLAmbisonicsRotationEffect effect,
                                                     IPLAmbisonicsRotationEffectParams* params,
                                                     IPLAudioBuffer* in,
                                                     IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IAmbisonicsRotationEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplAmbisonicsDecodeEffectCreate(IPLContext context,
                                         IPLAudioSettings* audioSettings,
                                         IPLAmbisonicsDecodeEffectSettings* effectSettings,
                                         IPLAmbisonicsDecodeEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createAmbisonicsDecodeEffect(audioSettings, effectSettings, reinterpret_cast<api::IAmbisonicsDecodeEffect**>(effect));
}

IPLAmbisonicsDecodeEffect IPLCALL iplAmbisonicsDecodeEffectRetain(IPLAmbisonicsDecodeEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLAmbisonicsDecodeEffect>(reinterpret_cast<api::IAmbisonicsDecodeEffect*>(effect)->retain());
}

void IPLCALL iplAmbisonicsDecodeEffectRelease(IPLAmbisonicsDecodeEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IAmbisonicsDecodeEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplAmbisonicsDecodeEffectReset(IPLAmbisonicsDecodeEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IAmbisonicsDecodeEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplAmbisonicsDecodeEffectApply(IPLAmbisonicsDecodeEffect effect,
                                                   IPLAmbisonicsDecodeEffectParams* params,
                                                   IPLAudioBuffer* in,
                                                   IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IAmbisonicsDecodeEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplDirectEffectCreate(IPLContext context,
                               IPLAudioSettings* audioSettings,
                               IPLDirectEffectSettings* effectSettings,
                               IPLDirectEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createDirectEffect(audioSettings, effectSettings, reinterpret_cast<api::IDirectEffect**>(effect));
}

IPLDirectEffect IPLCALL iplDirectEffectRetain(IPLDirectEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLDirectEffect>(reinterpret_cast<api::IDirectEffect*>(effect)->retain());
}

void IPLCALL iplDirectEffectRelease(IPLDirectEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IDirectEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplDirectEffectReset(IPLDirectEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IDirectEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplDirectEffectApply(IPLDirectEffect effect,
                                         IPLDirectEffectParams* params,
                                         IPLAudioBuffer* in,
                                         IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IDirectEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplReflectionEffectCreate(IPLContext context,
                                   IPLAudioSettings* audioSettings,
                                   IPLReflectionEffectSettings* effectSettings,
                                   IPLReflectionEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createReflectionEffect(audioSettings, effectSettings, reinterpret_cast<api::IReflectionEffect**>(effect));
}

IPLReflectionEffect IPLCALL iplReflectionEffectRetain(IPLReflectionEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLReflectionEffect>(reinterpret_cast<api::IReflectionEffect*>(effect)->retain());
}

void IPLCALL iplReflectionEffectRelease(IPLReflectionEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IReflectionEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplReflectionEffectReset(IPLReflectionEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IReflectionEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplReflectionEffectApply(IPLReflectionEffect effect,
                                             IPLReflectionEffectParams* params,
                                             IPLAudioBuffer* in,
                                             IPLAudioBuffer* out,
                                             IPLReflectionMixer mixer)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IReflectionEffect*>(effect)->apply(params, in, out, reinterpret_cast<api::IReflectionMixer*>(mixer));
}

IPLerror IPLCALL iplReflectionMixerCreate(IPLContext context,
                                  IPLAudioSettings* audioSettings,
                                  IPLReflectionEffectSettings* effectSettings,
                                  IPLReflectionMixer* mixer)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createReflectionMixer(audioSettings, effectSettings, reinterpret_cast<api::IReflectionMixer**>(mixer));
}

IPLReflectionMixer IPLCALL iplReflectionMixerRetain(IPLReflectionMixer mixer)
{
    if (!mixer)
        return nullptr;

    return reinterpret_cast<IPLReflectionMixer>(reinterpret_cast<api::IReflectionMixer*>(mixer)->retain());
}

void IPLCALL iplReflectionMixerRelease(IPLReflectionMixer* mixer)
{
    if (!mixer || !*mixer)
        return;

    reinterpret_cast<api::IReflectionMixer*>(*mixer)->release();

    *mixer = nullptr;
}

void IPLCALL iplReflectionMixerReset(IPLReflectionMixer mixer)
{
    if (!mixer)
        return;

    reinterpret_cast<api::IReflectionMixer*>(mixer)->reset();
}

IPLAudioEffectState IPLCALL iplReflectionMixerApply(IPLReflectionMixer mixer,
                                            IPLReflectionEffectParams* params,
                                            IPLAudioBuffer* out)
{
    if (!mixer)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IReflectionMixer*>(mixer)->apply(params, out);
}

IPLerror IPLCALL iplPathEffectCreate(IPLContext context,
                             IPLAudioSettings* audioSettings,
                             IPLPathEffectSettings* effectSettings,
                             IPLPathEffect* effect)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createPathEffect(audioSettings, effectSettings, reinterpret_cast<api::IPathEffect**>(effect));
}

IPLPathEffect IPLCALL iplPathEffectRetain(IPLPathEffect effect)
{
    if (!effect)
        return nullptr;

    return reinterpret_cast<IPLPathEffect>(reinterpret_cast<api::IPathEffect*>(effect)->retain());
}

void IPLCALL iplPathEffectRelease(IPLPathEffect* effect)
{
    if (!effect || !*effect)
        return;

    reinterpret_cast<api::IPathEffect*>(*effect)->release();

    *effect = nullptr;
}

void IPLCALL iplPathEffectReset(IPLPathEffect effect)
{
    if (!effect)
        return;

    reinterpret_cast<api::IPathEffect*>(effect)->reset();
}

IPLAudioEffectState IPLCALL iplPathEffectApply(IPLPathEffect effect,
                                       IPLPathEffectParams* params,
                                       IPLAudioBuffer* in,
                                       IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    return reinterpret_cast<api::IPathEffect*>(effect)->apply(params, in, out);
}

IPLerror IPLCALL iplProbeArrayCreate(IPLContext context,
                             IPLProbeArray* probeArray)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createProbeArray(reinterpret_cast<api::IProbeArray**>(probeArray));
}

IPLProbeArray IPLCALL iplProbeArrayRetain(IPLProbeArray probeArray)
{
    if (!probeArray)
        return nullptr;

    return reinterpret_cast<IPLProbeArray>(reinterpret_cast<api::IProbeArray*>(probeArray)->retain());
}

void IPLCALL iplProbeArrayRelease(IPLProbeArray* probeArray)
{
    if (!probeArray || !*probeArray)
        return;

    reinterpret_cast<api::IProbeArray*>(*probeArray)->release();

    *probeArray = nullptr;
}

void IPLCALL iplProbeArrayGenerateProbes(IPLProbeArray probeArray, IPLScene scene, IPLProbeGenerationParams* params)
{
    if (!probeArray)
        return;

    reinterpret_cast<api::IProbeArray*>(probeArray)->generateProbes(reinterpret_cast<api::IScene*>(scene), params);
}

IPLint32 IPLCALL iplProbeArrayGetNumProbes(IPLProbeArray probeArray)
{
    if (!probeArray)
        return 0;

    return reinterpret_cast<api::IProbeArray*>(probeArray)->getNumProbes();
}

IPLSphere IPLCALL iplProbeArrayGetProbe(IPLProbeArray probeArray,
                                IPLint32 index)
{
    if (!probeArray)
        return IPLSphere{};

    return reinterpret_cast<api::IProbeArray*>(probeArray)->getProbe(index);
}

IPLerror IPLCALL iplProbeBatchCreate(IPLContext context,
                             IPLProbeBatch* probeBatch)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createProbeBatch(reinterpret_cast<api::IProbeBatch**>(probeBatch));
}

IPLProbeBatch IPLCALL iplProbeBatchRetain(IPLProbeBatch probeBatch)
{
    if (!probeBatch)
        return nullptr;

    return reinterpret_cast<IPLProbeBatch>(reinterpret_cast<api::IProbeBatch*>(probeBatch)->retain());
}

void IPLCALL iplProbeBatchRelease(IPLProbeBatch* probeBatch)
{
    if (!probeBatch || !*probeBatch)
        return;

    reinterpret_cast<api::IProbeBatch*>(*probeBatch)->release();

    *probeBatch = nullptr;
}

IPLerror IPLCALL iplProbeBatchLoad(IPLContext context,
                           IPLSerializedObject serializedObject,
                           IPLProbeBatch* probeBatch)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->loadProbeBatch(reinterpret_cast<api::ISerializedObject*>(serializedObject), reinterpret_cast<api::IProbeBatch**>(probeBatch));
}

void IPLCALL iplProbeBatchSave(IPLProbeBatch probeBatch,
                       IPLSerializedObject serializedObject)
{
    if (!probeBatch)
        return;

    reinterpret_cast<api::IProbeBatch*>(probeBatch)->save(reinterpret_cast<api::ISerializedObject*>(serializedObject));
}

IPLint32 IPLCALL iplProbeBatchGetNumProbes(IPLProbeBatch probeBatch)
{
    if (!probeBatch)
        return 0;

    return reinterpret_cast<api::IProbeBatch*>(probeBatch)->getNumProbes();
}

void IPLCALL iplProbeBatchAddProbe(IPLProbeBatch probeBatch,
                           IPLSphere probe)
{
    if (!probeBatch)
        return;

    reinterpret_cast<api::IProbeBatch*>(probeBatch)->addProbe(probe);
}

void IPLCALL iplProbeBatchAddProbeArray(IPLProbeBatch probeBatch,
                                IPLProbeArray probeArray)
{
    if (!probeBatch)
        return;

    reinterpret_cast<api::IProbeBatch*>(probeBatch)->addProbeArray(reinterpret_cast<api::IProbeArray*>(probeArray));
}

void IPLCALL iplProbeBatchRemoveProbe(IPLProbeBatch probeBatch,
                              IPLint32 index)
{
    if (!probeBatch)
        return;

    reinterpret_cast<api::IProbeBatch*>(probeBatch)->removeProbe(index);
}

void IPLCALL iplProbeBatchCommit(IPLProbeBatch probeBatch)
{
    if (!probeBatch)
        return;

    reinterpret_cast<api::IProbeBatch*>(probeBatch)->commit();
}

void IPLCALL iplProbeBatchRemoveData(IPLProbeBatch probeBatch,
                             IPLBakedDataIdentifier* identifier)
{
    if (!probeBatch)
        return;

    reinterpret_cast<api::IProbeBatch*>(probeBatch)->removeData(identifier);
}

IPLsize IPLCALL iplProbeBatchGetDataSize(IPLProbeBatch probeBatch,
                                  IPLBakedDataIdentifier* identifier)
{
    if (!probeBatch)
        return 0;

    return reinterpret_cast<api::IProbeBatch*>(probeBatch)->getDataSize(identifier);
}

void IPLCALL iplReflectionsBakerBake(IPLContext context,
                             IPLReflectionsBakeParams* params,
                             IPLProgressCallback progressCallback,
                             void* userData)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->bakeReflections(params, progressCallback, userData);
}

void IPLCALL iplReflectionsBakerCancelBake(IPLContext context)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->cancelBakeReflections();
}

void IPLCALL iplPathBakerBake(IPLContext context,
                      IPLPathBakeParams* params,
                      IPLProgressCallback progressCallback,
                      void* userData)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->bakePaths(params, progressCallback, userData);
}

void IPLCALL iplPathBakerCancelBake(IPLContext context)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->cancelBakePaths();
}

IPLerror IPLCALL iplSimulatorCreate(IPLContext context,
                            IPLSimulationSettings* settings,
                            IPLSimulator* simulator)
{
    if (!context)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::IContext*>(context)->createSimulator(settings, reinterpret_cast<api::ISimulator**>(simulator));
}

IPLSimulator IPLCALL iplSimulatorRetain(IPLSimulator simulator)
{
    if (!simulator)
        return nullptr;

    return reinterpret_cast<IPLSimulator>(reinterpret_cast<api::ISimulator*>(simulator)->retain());
}

void IPLCALL iplSimulatorRelease(IPLSimulator* simulator)
{
    if (!simulator || !*simulator)
        return;

    reinterpret_cast<api::ISimulator*>(*simulator)->release();

    *simulator = nullptr;
}

void IPLCALL iplSimulatorSetScene(IPLSimulator simulator,
                          IPLScene scene)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->setScene(reinterpret_cast<api::IScene*>(scene));
}

void IPLCALL iplSimulatorAddProbeBatch(IPLSimulator simulator, IPLProbeBatch probeBatch)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->addProbeBatch(reinterpret_cast<api::IProbeBatch*>(probeBatch));
}

void IPLCALL iplSimulatorRemoveProbeBatch(IPLSimulator simulator, IPLProbeBatch probeBatch)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->removeProbeBatch(reinterpret_cast<api::IProbeBatch*>(probeBatch));
}

void IPLCALL iplSimulatorSetSharedInputs(IPLSimulator simulator,
                                 IPLSimulationFlags flags,
                                 IPLSimulationSharedInputs* sharedInputs)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->setSharedInputs(flags, sharedInputs);
}

void IPLCALL iplSimulatorCommit(IPLSimulator simulator)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->commit();
}

void IPLCALL iplSimulatorRunDirect(IPLSimulator simulator)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->runDirect();
}

void IPLCALL iplSimulatorRunReflections(IPLSimulator simulator)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->runReflections();
}

void IPLCALL iplSimulatorRunPathing(IPLSimulator simulator)
{
    if (!simulator)
        return;

    reinterpret_cast<api::ISimulator*>(simulator)->runPathing();
}

IPLerror IPLCALL iplSourceCreate(IPLSimulator simulator,
                         IPLSourceSettings* settings,
                         IPLSource* source)
{
    if (!simulator)
        return IPL_STATUS_FAILURE;

    return reinterpret_cast<api::ISimulator*>(simulator)->createSource(settings, reinterpret_cast<api::ISource**>(source));
}

IPLSource IPLCALL iplSourceRetain(IPLSource source)
{
    if (!source)
        return nullptr;

    return reinterpret_cast<IPLSource>(reinterpret_cast<api::ISource*>(source)->retain());
}

void IPLCALL iplSourceRelease(IPLSource* source)
{
    if (!source || !*source)
        return;

    reinterpret_cast<api::ISource*>(*source)->release();

    *source = nullptr;
}

void IPLCALL iplSourceAdd(IPLSource source, IPLSimulator simulator)
{
    if (!source)
        return;

    reinterpret_cast<api::ISource*>(source)->add(reinterpret_cast<api::ISimulator*>(simulator));
}

void IPLCALL iplSourceRemove(IPLSource source, IPLSimulator simulator)
{
    if (!source)
        return;

    reinterpret_cast<api::ISource*>(source)->remove(reinterpret_cast<api::ISimulator*>(simulator));
}

void IPLCALL iplSourceSetInputs(IPLSource source,
                        IPLSimulationFlags flags,
                        IPLSimulationInputs* inputs)
{
    if (!source)
        return;

    reinterpret_cast<api::ISource*>(source)->setInputs(flags, inputs);
}

void IPLCALL iplSourceGetOutputs(IPLSource source,
                         IPLSimulationFlags flags,
                         IPLSimulationOutputs* outputs)
{
    if (!source)
        return;

    reinterpret_cast<api::ISource*>(source)->getOutputs(flags, outputs);
}

IPLfloat32 IPLCALL iplDistanceAttenuationCalculate(IPLContext context,
                                           IPLVector3 source,
                                           IPLVector3 listener,
                                           IPLDistanceAttenuationModel* model)
{
    if (!context)
        return 1.0f;

    return reinterpret_cast<api::IContext*>(context)->calculateDistanceAttenuation(source, listener, model);
}

void IPLCALL iplAirAbsorptionCalculate(IPLContext context,
                               IPLVector3 source,
                               IPLVector3 listener,
                               IPLAirAbsorptionModel* model,
                               IPLfloat32* airAbsorption)
{
    if (!context)
        return;

    reinterpret_cast<api::IContext*>(context)->calculateAirAbsorption(source, listener, model, airAbsorption);
}

IPLfloat32 IPLCALL iplDirectivityCalculate(IPLContext context,
                                   IPLCoordinateSpace3 source,
                                   IPLVector3 listener,
                                   IPLDirectivity* model)
{
    if (!context)
        return 1.0f;

    return reinterpret_cast<api::IContext*>(context)->calculateDirectivity(source, listener, model);
}

#endif

#endif
