#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "agent/agent_registry.hpp"
#include "config/config_schema.hpp"

namespace kabot::relay {

class RelayManager {
public:
    RelayManager(const kabot::config::Config& config,
                 kabot::agent::AgentRegistry& agents);
    ~RelayManager();

    void Start();
    void Stop();

private:
    class Worker;

    kabot::config::Config config_;
    kabot::agent::AgentRegistry& agents_;
    std::vector<std::unique_ptr<Worker>> workers_;
    std::atomic<bool> running_{false};
};

}  // namespace kabot::relay
