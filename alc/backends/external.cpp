#include "config.h"

#include <atomic>
#include <cassert>

#include <AL/alc.h>

#include "external.h"
#include "almalloc.h"

namespace {

struct ExternalBackend final : public BackendBase {
    ExternalBackend(DeviceBase* device) noexcept : BackendBase{device}
    {
        if (sm_instance.load()) {
            assert(false);
        }
        sm_instance = this;
    }

    ~ExternalBackend() noexcept { sm_instance = nullptr; }

    void open(const char *) override { reset(); }

    bool reset() override
    {
        mDevice->DeviceName = "external";
        mDevice->Frequency = 48000;
        mDevice->FmtChans = DevFmtStereo;
        mDevice->FmtType = DevFmtShort;
        mDevice->UpdateSize = 512;
        mDevice->BufferSize = 2 * mDevice->UpdateSize;
        mDevice->RealOut.ChannelIndex.fill(InvalidChannelIndex);
        mDevice->RealOut.ChannelIndex[FrontLeft] = 0;
        mDevice->RealOut.ChannelIndex[FrontRight] = 1;
        return true;
    }

    void start() override {}
    void stop() override {}

    DEF_NEWDEL(ExternalBackend)

    static std::atomic<ExternalBackend*> sm_instance;
};

std::atomic<ExternalBackend*> ExternalBackend::sm_instance;

} // namespace

BackendFactory &ExternalBackendFactory::getFactory()
{
    static ExternalBackendFactory factory{};
    return factory;
}

bool ExternalBackendFactory::init() { return true; }

bool ExternalBackendFactory::querySupport(BackendType type) { return type == BackendType::Playback; }

std::string ExternalBackendFactory::probe(BackendType type)
{
    if (type == BackendType::Playback) {
        return "external";
    } else {
        return "";
    }
}

BackendPtr ExternalBackendFactory::createBackend(DeviceBase *device, BackendType type)
{
    if (type == BackendType::Playback) {
        return BackendPtr(new ExternalBackend{device});
    } else {
        return nullptr;
    }
}

extern "C" ALC_API void ALC_APIENTRY alcExternalBackendGetSamples(ALCshort* out, ALCuint numSamples)
{
    if (ExternalBackend *externalBackend = ExternalBackend::sm_instance.load()) {
        DeviceBase *device = externalBackend->mDevice;
        device->renderSamples(out, numSamples, device->channelsFromFmt());
    } else {
        memset(out, 0, sizeof(int16_t) * 2 /* channels */ * numSamples);
    }
}
