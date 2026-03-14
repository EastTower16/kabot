#include "config/config_loader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[config_tests] " << message << std::endl;
        std::exit(1);
    }
}

void TestValidateConfigWithBindingAgent() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "ops-agent";
    agent.workspace = "workspace-ops";
    config.agents.instances.push_back(agent);

    kabot::config::ChannelInstanceConfig channel{};
    channel.name = "telegram_ops";
    channel.type = "telegram";
    channel.enabled = true;
    channel.binding.agent = "ops-agent";
    channel.telegram.name = channel.name;
    channel.telegram.enabled = true;
    channel.telegram.token = "token";
    config.channels.instances.push_back(channel);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(errors.empty(), "expected config with binding.agent to validate successfully");
}

void TestValidateConfigRejectsMissingBindingAgent() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "ops-agent";
    config.agents.instances.push_back(agent);

    kabot::config::ChannelInstanceConfig channel{};
    channel.name = "telegram_ops";
    channel.type = "telegram";
    channel.enabled = true;
    channel.telegram.name = channel.name;
    channel.telegram.enabled = true;
    channel.telegram.token = "token";
    config.channels.instances.push_back(channel);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(!errors.empty(), "expected missing binding.agent to fail validation");
}

void TestValidateConfigRejectsUnknownBindingAgent() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "ops-agent";
    config.agents.instances.push_back(agent);

    kabot::config::ChannelInstanceConfig channel{};
    channel.name = "telegram_ops";
    channel.type = "telegram";
    channel.enabled = true;
    channel.binding.agent = "missing-agent";
    channel.telegram.name = channel.name;
    channel.telegram.enabled = true;
    channel.telegram.token = "token";
    config.channels.instances.push_back(channel);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(!errors.empty(), "expected unknown binding.agent to fail validation");
}

void TestValidateConfigRejectsUnsupportedToolProfile() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "ops-agent";
    agent.tool_profile = "everything";
    config.agents.instances.push_back(agent);

    kabot::config::ChannelInstanceConfig channel{};
    channel.name = "telegram_ops";
    channel.type = "telegram";
    channel.enabled = true;
    channel.binding.agent = "ops-agent";
    channel.telegram.name = channel.name;
    channel.telegram.enabled = true;
    channel.telegram.token = "token";
    config.channels.instances.push_back(channel);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(!errors.empty(), "expected unsupported toolProfile to fail validation");
}

void TestValidateConfigAcceptsQQBotChannel() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "ops-agent";
    config.agents.instances.push_back(agent);

    kabot::config::ChannelInstanceConfig channel{};
    channel.name = "qq_ops";
    channel.type = "qqbot";
    channel.enabled = true;
    channel.binding.agent = "ops-agent";
    channel.qqbot.name = channel.name;
    channel.qqbot.enabled = true;
    channel.qqbot.app_id = "app-id";
    channel.qqbot.client_secret = "client-secret";
    config.channels.instances.push_back(channel);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(errors.empty(), "expected qqbot config to validate successfully");
}

void TestValidateConfigRejectsQQBotChannelMissingCredentials() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "ops-agent";
    config.agents.instances.push_back(agent);

    kabot::config::ChannelInstanceConfig channel{};
    channel.name = "qq_ops";
    channel.type = "qqbot";
    channel.enabled = true;
    channel.binding.agent = "ops-agent";
    channel.qqbot.name = channel.name;
    channel.qqbot.enabled = true;
    channel.qqbot.app_id = "app-id";
    config.channels.instances.push_back(channel);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(!errors.empty(), "expected qqbot config without credentials to fail validation");
}

void TestLoadConfigParsesQQBotInstance() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "kabot_config_tests_qqbot";
    std::filesystem::create_directories(temp_dir);
    const auto config_path = temp_dir / "config.json";

    std::ofstream output(config_path);
    output << R"({
  "agents": {
    "instances": [
      {
        "name": "ops-agent"
      }
    ]
  },
  "channels": {
    "instances": [
      {
        "name": "qq_ops",
        "type": "qqbot",
        "enabled": true,
        "appId": "app-id",
        "clientSecret": "client-secret",
        "sandbox": true,
        "intents": "1107296256",
        "skipTlsVerify": true,
        "binding": {
          "agent": "ops-agent"
        }
      }
    ]
  }
})";
    output.close();

    const auto config = kabot::config::LoadConfig(config_path);
    Expect(config.channels.instances.size() == 1, "expected one qqbot channel instance");
    Expect(config.channels.instances.front().type == "qqbot", "expected qqbot channel type to load");
    Expect(config.channels.instances.front().qqbot.app_id == "app-id", "expected qqbot appId to load");
    Expect(config.channels.instances.front().qqbot.client_secret == "client-secret", "expected qqbot clientSecret to load");
    Expect(config.channels.instances.front().qqbot.sandbox, "expected qqbot sandbox to load");
    Expect(config.channels.instances.front().qqbot.skip_tls_verify, "expected qqbot skipTlsVerify to load");
}

void TestLegacyConfigCompatibility() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "kabot_config_tests";
    std::filesystem::create_directories(temp_dir);
    const auto config_path = temp_dir / "config.json";

    std::ofstream output(config_path);
    output << R"({
  "agents": {
    "defaults": {
      "workspace": "legacy-workspace",
      "model": "legacy-model"
    }
  },
  "channels": {
    "telegram": {
      "enabled": true,
      "token": "legacy-token",
      "allowFrom": ["12345"]
    }
  }
})";
    output.close();

    const auto config = kabot::config::LoadConfig(config_path);
    Expect(config.agents.instances.size() == 1, "expected legacy config to create one default agent instance");
    Expect(config.agents.instances.front().name == "default", "expected legacy default agent name");
    Expect(config.agents.instances.front().workspace == "legacy-workspace", "expected legacy workspace to be preserved");
    Expect(config.channels.instances.size() == 1, "expected legacy telegram config to create one channel instance");
    Expect(config.channels.instances.front().name == "telegram", "expected legacy telegram instance name");
    Expect(config.channels.instances.front().binding.agent == "default", "expected legacy channel to bind default agent");
}

void TestAgentInstancesInheritRuntimeDefaults() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "kabot_config_tests_agent_defaults";
    std::filesystem::create_directories(temp_dir);
    const auto config_path = temp_dir / "config.json";

    std::ofstream output(config_path);
    output << R"({
  "agents": {
    "defaults": {
      "workspace": "json-workspace",
      "model": "json-model",
      "toolProfile": "full",
      "maxIterations": 20,
      "maxTokens": 8192,
      "temperature": 0.7,
      "maxToolIterations": 20,
      "maxHistoryMessages": 50
    },
    "instances": [
      {
        "name": "ops-agent"
      }
    ]
  },
  "tools": {
    "web": {
      "search": {
        "apiKey": "test-brave-key"
      }
    }
  }
})";
    output.close();

    const auto config = kabot::config::LoadConfig(config_path);
    Expect(config.agents.instances.size() == 1, "expected one agent instance");
    Expect(config.agents.instances.front().name == "ops-agent", "expected ops-agent instance name");
    Expect(config.agents.instances.front().brave_api_key == "test-brave-key",
           "expected runtime brave api key to be inherited by agent instance");
}

void TestLoadConfigParsesRelayManagedAgents() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "kabot_config_tests_relay";
    std::filesystem::create_directories(temp_dir);
    const auto config_path = temp_dir / "config.json";

    std::ofstream output(config_path);
    output << R"({
  "agents": {
    "instances": [
      {
        "name": "default"
      },
      {
        "name": "ops-agent"
      }
    ]
  },
  "relay": {
    "defaults": {
      "host": "relay.example.com",
      "port": 443,
      "path": "/ws/agent",
      "useTls": true,
      "heartbeatIntervalS": 9,
      "reconnectInitialDelayMs": 1500,
      "reconnectMaxDelayMs": 9000
    },
    "managedAgents": [
      {
        "name": "relay-default",
        "localAgent": "default",
        "agentId": "agent-default",
        "token": "token-default"
      },
      {
        "name": "relay-ops",
        "localAgent": "ops-agent",
        "agentId": "agent-ops",
        "token": "token-ops",
        "enabled": false,
        "heartbeatIntervalS": 12
      }
    ]
  }
})";
    output.close();

    const auto config = kabot::config::LoadConfig(config_path);
    Expect(config.relay.managed_agents.size() == 2, "expected two relay managed agents");
    Expect(config.relay.managed_agents.front().host == "relay.example.com", "expected relay host to inherit defaults");
    Expect(config.relay.managed_agents.front().port == 443, "expected relay port to inherit defaults");
    Expect(config.relay.managed_agents.front().path == "/ws/agent", "expected relay path to inherit defaults");
    Expect(config.relay.managed_agents.front().scheme == "wss", "expected tls relay scheme to normalize to wss");
    Expect(config.relay.managed_agents.front().local_agent == "default", "expected local agent binding to load");
    Expect(config.relay.managed_agents.front().heartbeat_interval_s == 9, "expected heartbeat interval to inherit defaults");
    Expect(!config.relay.managed_agents.back().enabled, "expected disabled relay managed agent to be preserved");
    Expect(config.relay.managed_agents.back().heartbeat_interval_s == 12, "expected per-agent relay heartbeat override to load");
}

void TestLoadConfigDefaultsRelayLocalAgentToManagedAgentName() {
    const auto temp_dir = std::filesystem::temp_directory_path() / "kabot_config_test_relay_local_default";
    std::filesystem::create_directories(temp_dir);
    const auto config_path = temp_dir / "config.json";

    std::ofstream output(config_path);
    output << R"({
  "agents": {
    "instances": [
      {
        "name": "default"
      },
      {
        "name": "ops-agent"
      }
    ]
  },
  "relay": {
    "defaults": {
      "host": "relay.example.com",
      "port": 443,
      "path": "/ws/agent",
      "useTls": true,
      "heartbeatIntervalS": 9,
      "reconnectInitialDelayMs": 1000,
      "reconnectMaxDelayMs": 5000
    },
    "managedAgents": [
      {
        "name": "ops-agent",
        "agentId": "agent-ops",
        "token": "token-ops"
      }
    ]
  }
})";
    output.close();

    const auto config = kabot::config::LoadConfig(config_path);
    Expect(config.relay.managed_agents.size() == 1, "expected one relay managed agent");
    Expect(config.relay.managed_agents.front().local_agent == "ops-agent",
           "expected relay managed agent to default localAgent to its own name");
}

void TestValidateConfigRejectsRelayManagedAgentMissingFields() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "default";
    config.agents.instances.push_back(agent);

    kabot::config::RelayManagedAgentConfig relay_agent{};
    relay_agent.name = "relay-default";
    relay_agent.local_agent = "default";
    relay_agent.host = "relay.example.com";
    relay_agent.port = 443;
    relay_agent.path = "/ws/agent";
    relay_agent.scheme = "wss";
    relay_agent.heartbeat_interval_s = 10;
    relay_agent.reconnect_initial_delay_ms = 1000;
    relay_agent.reconnect_max_delay_ms = 5000;
    config.relay.managed_agents.push_back(relay_agent);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(!errors.empty(), "expected missing relay credentials to fail validation");
}

void TestValidateConfigRejectsDuplicateRelayManagedAgentNames() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "default";
    config.agents.instances.push_back(agent);

    kabot::config::RelayManagedAgentConfig relay_a{};
    relay_a.name = "relay-default";
    relay_a.local_agent = "default";
    relay_a.agent_id = "a";
    relay_a.token = "ta";
    relay_a.host = "relay.example.com";
    relay_a.port = 443;
    relay_a.path = "/ws/agent";
    relay_a.scheme = "wss";
    relay_a.heartbeat_interval_s = 10;
    relay_a.reconnect_initial_delay_ms = 1000;
    relay_a.reconnect_max_delay_ms = 5000;
    config.relay.managed_agents.push_back(relay_a);

    auto relay_b = relay_a;
    relay_b.agent_id = "b";
    relay_b.token = "tb";
    config.relay.managed_agents.push_back(relay_b);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(!errors.empty(), "expected duplicate relay managed agent names to fail validation");
}

void TestValidateConfigRejectsUnknownRelayLocalAgent() {
    kabot::config::Config config{};

    kabot::config::AgentInstanceConfig agent{};
    agent.name = "default";
    config.agents.instances.push_back(agent);

    kabot::config::RelayManagedAgentConfig relay_agent{};
    relay_agent.name = "relay-default";
    relay_agent.local_agent = "missing-agent";
    relay_agent.agent_id = "a";
    relay_agent.token = "ta";
    relay_agent.host = "relay.example.com";
    relay_agent.port = 443;
    relay_agent.path = "/ws/agent";
    relay_agent.scheme = "wss";
    relay_agent.heartbeat_interval_s = 10;
    relay_agent.reconnect_initial_delay_ms = 1000;
    relay_agent.reconnect_max_delay_ms = 5000;
    config.relay.managed_agents.push_back(relay_agent);

    const auto errors = kabot::config::ValidateConfig(config);
    Expect(!errors.empty(), "expected unknown relay local agent to fail validation");
}

}  // namespace

int main() {
    TestValidateConfigWithBindingAgent();
    TestValidateConfigRejectsMissingBindingAgent();
    TestValidateConfigRejectsUnknownBindingAgent();
    TestValidateConfigRejectsUnsupportedToolProfile();
    TestValidateConfigAcceptsQQBotChannel();
    TestValidateConfigRejectsQQBotChannelMissingCredentials();
    TestLoadConfigParsesQQBotInstance();
    TestLegacyConfigCompatibility();
    TestAgentInstancesInheritRuntimeDefaults();
    TestLoadConfigParsesRelayManagedAgents();
    TestLoadConfigDefaultsRelayLocalAgentToManagedAgentName();
    TestValidateConfigRejectsRelayManagedAgentMissingFields();
    TestValidateConfigRejectsDuplicateRelayManagedAgentNames();
    TestValidateConfigRejectsUnknownRelayLocalAgent();
    std::cout << "config_tests passed" << std::endl;
    return 0;
}
