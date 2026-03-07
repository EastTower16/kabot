#include "channels/channel_manager.hpp"

#include <chrono>
#include <thread>

#include "channels/lark_channel.hpp"
#include "channels/telegram_channel.hpp"
#include "utils/logging.hpp"

namespace kabot::channels {

namespace {

constexpr int kMaxSendAttempts = 3;
constexpr auto kRetryDelay = std::chrono::milliseconds(800);

}  // namespace

ChannelManager::ChannelManager(const kabot::config::Config& config,
                               kabot::bus::MessageBus& bus)
    : bus_(bus)
    , config_(config) {
    InitChannels();
}

void ChannelManager::InitChannels() {
    RegisterTelegram(config_.channels.telegram);
    RegisterLark(config_.channels.lark);
}

void ChannelManager::Register(std::unique_ptr<ChannelBase> channel) {
    auto name = channel->Name();
    channels_.emplace(std::move(name), std::move(channel));
}

ChannelBase* ChannelManager::GetChannel(const std::string& name) {
    auto it = channels_.find(name);
    if (it == channels_.end()) {
        return nullptr;
    }
    return it->second.get();
}

void ChannelManager::DispatchOutbound(const kabot::bus::OutboundMessage& msg) {
    SendWithRetry(msg);
}

void ChannelManager::StartAll() {
    if (!dispatch_running_) {
        dispatch_running_ = true;
        dispatch_thread_ = std::thread([this] { RunOutboundDispatcher(); });
    }
    for (auto& [_, channel] : channels_) {
        channel->Start();
    }
}

void ChannelManager::StopAll() {
    dispatch_running_ = false;
    if (dispatch_thread_.joinable()) {
        dispatch_thread_.join();
    }
    for (auto& [_, channel] : channels_) {
        channel->Stop();
    }
}

std::unordered_map<std::string, bool> ChannelManager::Status() const {
    std::unordered_map<std::string, bool> status;
    for (const auto& [name, channel] : channels_) {
        status[name] = channel->IsRunning();
    }
    return status;
}

bool ChannelManager::SendWithRetry(const kabot::bus::OutboundMessage& msg) {
    auto channel = GetChannel(msg.channel);
    if (!channel) {
        LOG_ERROR("[channel] outbound send failed: unknown channel={} chat_id={}",
                  msg.channel,
                  msg.chat_id);
        return false;
    }

    const bool is_typing = [&]() {
        auto it = msg.metadata.find("action");
        return it != msg.metadata.end() && it->second == "typing";
    }();
    const int max_attempts = is_typing ? 1 : kMaxSendAttempts;

    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        const bool sent = channel->Send(msg);
        if (sent) {
            if (attempt > 1) {
                LOG_WARN("[channel] outbound send recovered after retry channel={} chat_id={} attempt={}",
                         msg.channel,
                         msg.chat_id,
                         attempt);
            }
            return true;
        }

        LOG_ERROR("[channel] outbound send failed channel={} chat_id={} attempt={}/{} reply_to={} content_size={} media_count={}",
                  msg.channel,
                  msg.chat_id,
                  attempt,
                  max_attempts,
                  msg.reply_to,
                  msg.content.size(),
                  msg.media.size());

        if (attempt < max_attempts) {
            std::this_thread::sleep_for(kRetryDelay);
        }
    }

    return false;
}

void ChannelManager::RegisterTelegram(const kabot::config::TelegramConfig& config) {
    if (!config.enabled) {
        return;
    }
    Register(std::make_unique<TelegramChannel>(config, bus_));
}

void ChannelManager::RegisterLark(const kabot::config::LarkConfig& config) {
    if (!config.enabled) {
        return;
    }
    Register(std::make_unique<LarkChannel>(config, bus_));
}

void ChannelManager::RunOutboundDispatcher() {
    while (dispatch_running_) {
        kabot::bus::OutboundMessage msg{};
        if (!bus_.TryConsumeOutbound(msg, std::chrono::milliseconds(1000))) {
            continue;
        }
        SendWithRetry(msg);
    }
}

}  // namespace kabot::channels
