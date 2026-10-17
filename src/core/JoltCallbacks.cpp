#include "JoltCallbacks.h"
#include <iostream>
#include <cstdarg>
#include <cstdio>
using namespace JPH;

static void TraceImpl(const char* inFMT, ...) {
    va_list list; va_start(list, inFMT);
    char buffer[1024]; vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list); std::cout << buffer << std::endl;
}
#ifdef JPH_ENABLE_ASSERTS
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine) {
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") "
              << (inMessage != nullptr ? inMessage : "") << std::endl;
    return true;
}
#endif
void RegisterJoltTrace() {
    Trace = TraceImpl;
#ifdef JPH_ENABLE_ASSERTS
    AssertFailed = AssertFailedImpl;
#endif
}
bool ObjectLayerPairFilterImpl::ShouldCollide(ObjectLayer a, ObjectLayer b) const {
    switch (a) {
    case Layers::NON_MOVING: return b == Layers::MOVING;
    case Layers::MOVING:     return true;
    default: JPH_ASSERT(false); return false;
    }
}
BPLayerInterfaceImpl::BPLayerInterfaceImpl() {
    mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
    mObjectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
}
uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const { return BroadPhaseLayers::NUM_LAYERS; }
BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(ObjectLayer l) const {
    JPH_ASSERT(l < Layers::NUM_LAYERS); return mObjectToBroadPhase[l];
}
const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(BroadPhaseLayer l) const {
    if (l == BroadPhaseLayers::NON_MOVING) return "NON_MOVING";
    if (l == BroadPhaseLayers::MOVING)     return "MOVING";
    JPH_ASSERT(false); return "INVALID";
}
bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(ObjectLayer a, BroadPhaseLayer b) const {
    switch (a) {
    case Layers::NON_MOVING: return b == BroadPhaseLayers::MOVING;
    case Layers::MOVING:     return true;
    default: JPH_ASSERT(false); return false;
    }
}