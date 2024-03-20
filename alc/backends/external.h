#ifndef BACKENDS_EXTERNAL_H
#define BACKENDS_EXTERNAL_H

#include "base.h"

struct ExternalBackendFactory final : public BackendFactory {
  public:
    bool init() override;
    bool querySupport(BackendType type) override;
    std::string probe(BackendType type) override;
    BackendPtr createBackend(DeviceBase * device, BackendType type) override;
    static BackendFactory &getFactory();
};

#endif /* BACKENDS_EXTERNAL_H */
