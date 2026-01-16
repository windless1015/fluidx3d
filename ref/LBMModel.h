#pragma once

#include "LBMConfig.h"
#include "LBMCore.h"
#include "FreeSurfaceModule.h"
#include <memory>

namespace lbm {

class LBMModel {
public:
    LBMModel();
    ~LBMModel();

    // Disable copy/move
    LBMModel(const LBMModel&) = delete;
    LBMModel& operator=(const LBMModel&) = delete;

    // Configuration
    void configure(const LBMConfig& config);
    
    // Module Control
    void enableFreeSurface(bool enable);

    // Simulation Control
    void initialize();
    void step();

    // Component Access
    LBMCore* core() { return core_.get(); }
    FreeSurfaceModule* freeSurface() { return fsModule_.get(); }
    
    // Status
    bool isFreeSurfaceEnabled() const { return useFreeSurface_; }

private:
    LBMConfig config_;
    bool useFreeSurface_;
    
    std::unique_ptr<LBMCore> core_;
    std::unique_ptr<FreeSurfaceModule> fsModule_;
};

} // namespace lbm
