## Why

当前 agent 接入仍以单个 `agentId` 和 `token` 的手工配置为中心，接入 relay server 的协议约束与本地 agent 管理逻辑也尚未被统一建模。这使得新增或维护多个 agent 时配置分散、易出错，也提高了后续扩展 relay 能力和排查在线状态问题的成本。

## What Changes

- 新增 relay server agent 连接能力，定义 agent 如何基于配置建立认证 WebSocket 连接、维持心跳、上报活动状态、处理命令下发与结果回传。
- 新增统一的 agent 管理配置模型，用结构化配置描述 relay server 地址、认证信息、重连/心跳参数以及多个可管理 agent 的映射关系。
- 调整 agent 生命周期管理逻辑，使 channel 或 manager 层能够基于统一配置创建、跟踪并更新 relay 连接中的 agent 实例。
- 为 relay 接入和 agent 配置提供可验证的行为约束，覆盖连接建立、断线重连、状态同步和配置校验等场景。

## Capabilities

### New Capabilities
- `relay-agent-integration`: 定义 agent 与 relay server 之间的连接、消息协议、状态上报和重连行为。
- `agent-management-config`: 定义统一的 agent 管理配置结构以及从配置到 agent 实例的加载与校验行为。

### Modified Capabilities
- `multi-agent-routing`: 扩展多 agent 路由能力，使其可以消费统一 agent 管理配置并感知 relay 在线状态。

## Impact

- 受影响代码可能包括 `src/agent/`、`src/channels/`、channel manager、配置加载逻辑以及消息/工具桥接层。
- 需要新增或调整 relay server WebSocket 客户端、agent 注册与状态同步流程。
- 需要更新配置格式与示例，可能影响现有多 agent 配置的兼容策略。
- 需要补充针对配置解析、协议消息处理和重连行为的测试或验证步骤。
