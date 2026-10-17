#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

void RegisterJoltTrace();

namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS = 2;
}

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override;
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl();
    virtual JPH::uint GetNumBroadPhaseLayers() const override;
    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer) const override;
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer) const override;
private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override;
};