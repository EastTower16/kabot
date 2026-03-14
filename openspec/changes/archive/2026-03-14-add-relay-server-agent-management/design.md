## Context

当前运行时以 `Config -> AgentRegistry -> AgentLoop` 和 `Config -> ChannelManager -> ChannelBase` 两条主路径组织 agent 与 channel 生命周期。`config_loader` 已经支持多 agent 与多 channel 实例，但 agent 仍主要面向本地执行回路建模，尚未提供 relay server 连接配置、协议收发状态、重连控制和远端在线状态追踪等一等能力。

本次变更需要在不破坏现有多 agent 路由和单实例兼容行为的前提下，引入 relay server 接入能力，并把分散的 agent 管理配置收敛到统一模型。设计必须兼顾：配置可读性、运行时职责边界、重连稳定性，以及未来扩展多个 relay agent 的可维护性。

## Goals / Non-Goals

**Goals:**
- 提供独立的 relay agent 集成层，封装 WebSocket 连接、心跳、消息编解码、活动状态上报与自动重连。
- 提供统一的 agent 管理配置模型，显式表达 relay server 端点、认证信息、连接参数和多个 agent 的声明方式。
- 让现有 `AgentRegistry`、`ChannelManager` 与配置校验逻辑可以消费统一配置，并保留现有多 agent 路由行为。
- 为配置校验、状态同步、命令处理和兼容迁移定义明确边界，便于后续实施与测试。

**Non-Goals:**
- 不在本次设计中定义 relay server 服务端实现或修改服务端协议。
- 不改变现有 LLM provider、tool registry 或 session manager 的核心行为。
- 不在本次变更中引入新的 channel 类型以外的消息通道抽象。
- 不要求一次性移除现有旧配置入口，而是提供兼容与归一化策略。

## Decisions

### Decision: 新增独立 `relay` 运行时模块而不是把协议逻辑并入 `AgentLoop`
将 relay server 连接、重连、心跳和消息协议处理放入独立模块，例如 `src/relay/` 下的 connection manager、protocol codec、status reporter 与 command bridge。`AgentLoop` 保持专注于本地 prompt/tool 执行，relay 层只负责把远端 `command.dispatch` 转换为本地可执行请求，并把执行状态转换回 `command.ack` / `command.result` / `activity.update`。

这样可以保持文档中建议的窄协议层边界，避免在 WebSocket 回调中直接堆积阻塞执行逻辑，也便于后续为多个 relay agent 复用同一连接管理框架。

备选方案是直接在 `AgentRegistry` 或 `AgentLoop` 中加入 WebSocket 客户端。该方案改动入口更少，但会把远端连接状态与本地执行生命周期耦合在一起，后续难以支持多 relay 连接和独立测试，因此不采用。

### Decision: 统一 agent 管理配置为“声明式 relay 配置 + agent 实例映射”
在现有 `Config` 中新增专门的 relay/agent management 配置块，用于描述：服务器地址、TLS 策略、心跳间隔、重连退避、默认活动上报行为，以及多个 relay agent 实例的 `agentId`、`token`、目标本地 agent 名称和启用状态。配置加载阶段负责把旧格式和简化写法归一化成统一运行时结构，再由 registry/manager 消费。

这样可以避免把 relay 参数分散到环境变量、单 agent 配置和 channel binding 中，也让多个 agent 的管理方式与 `channels.instances` 的声明式模型保持一致。

备选方案是仅为每个 `AgentInstanceConfig` 增加零散字段，如 `relayHost`、`relayToken`。该方案改动看似更小，但会导致共享连接参数重复定义、校验分散、示例难懂，因此不采用。

### Decision: `AgentRegistry` 继续作为本地 agent 实例权威来源，新增 relay manager 负责远端连接生命周期
`AgentRegistry` 仍根据本地 agent 配置创建 `AgentLoop` 并负责按 agent 名称解析执行目标。新增的 relay manager 在启动时根据统一配置创建多个 relay session，并在收到 `command.dispatch` 时解析目标本地 agent，然后委托给 `AgentRegistry` 或适配层执行。这样可以最大化复用现有消息/会话/工具逻辑，同时把“远端 agent 身份”和“本地执行 agent 身份”清晰分离。

备选方案是让每个 relay agent 直接拥有一套新的执行器实例。该方案会重复构建 session、tool 与 provider 依赖，增加资源占用和状态同步复杂度，因此不采用。

### Decision: 保留向后兼容，通过归一化和验证保障迁移
`config_loader` 应继续支持现有 `agents.defaults`、`agents.instances`、`channels.instances` 等格式，并在存在 relay 配置时做补全和校验。验证规则至少覆盖：relay agent 名称唯一、`agentId/token` 必填、引用的本地 agent 必须存在、心跳与重连参数范围合理，以及禁用项不会启动连接。

这能让现有单 agent 或多 agent 配置逐步迁移到新的统一模型，而不会要求用户一次性重写全部配置。

## Risks / Trade-offs

- 配置模型变复杂 → 通过归一化层和清晰示例降低直接暴露给用户的复杂度。
- relay 在线状态与本地 agent 状态可能不一致 → 通过显式活动状态机和重连后重新上报策略降低漂移。
- WebSocket 客户端引入线程与资源管理复杂度 → 通过独立 manager 和可停止的退避循环限制并发风险。
- 同一 relay server 管理多个 agent 时认证或限流失败会放大影响 → 通过按 agent 隔离连接状态与日志上下文减小故障面。

## Migration Plan

1. 扩展配置 schema 与加载/校验逻辑，引入统一 relay agent 管理配置，同时保留现有配置入口。
2. 新增 relay 模块，先实现协议编解码、连接管理和状态上报，不改动现有 channel 行为。
3. 将 relay `command.dispatch` 桥接到现有 agent 执行入口，并补充运行日志与错误路径。
4. 提供示例配置与验证步骤，确认旧配置仍能启动，新配置能管理多个 relay agent。
5. 如需回滚，可禁用 relay 配置块并恢复到纯本地 agent/channel 启动路径，不影响现有多 agent routing 数据结构。

## Verification

1. 使用包含 `relay.defaults` 与 `relay.managedAgents` 的配置启动 gateway，并确认启动日志中出现每个 enabled managed agent 的连接日志。
2. 在 relay server 侧向某个 `agentId` 下发 `command.dispatch`，确认客户端按顺序回传 `command.ack`、`command.result(status=running)`、`command.result(status=completed|failed)`。
3. 在命令执行前后观察 `activity.update`，确认状态按 `idle -> busy -> idle|error` 切换，且 `currentCommandId` 仅在执行中或失败时携带。
4. 断开 relay 连接后确认客户端输出 reconnect 日志，并在重连成功后重新发送 `activity.update(idle)` 与后续心跳。
5. 运行 `config_tests` 与 `routing_tests`，确认 relay 配置加载/校验、本地 agent 桥接与既有多 agent routing 行为保持通过。

## Open Questions

- 是否需要为 relay agent 的活动状态维护更细粒度状态，例如 `connecting` 或 `reconnecting`，以及这些状态是否需要对外暴露。
- 环境变量覆盖是否也要支持新的 relay 配置块，如果支持，命名约定需要进一步统一。
- 对于多个 relay agent 指向同一个本地 agent 的情况，是否需要额外的并发或配额控制。
