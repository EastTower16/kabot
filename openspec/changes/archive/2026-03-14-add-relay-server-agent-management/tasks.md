## 1. 配置模型与校验

- [x] 1.1 扩展 `config_schema`，新增 relay server 与 relay-managed agent 的统一配置结构
- [x] 1.2 更新 `config_loader` 以加载共享 relay 配置、每个 managed agent 声明以及默认值归一化逻辑
- [x] 1.3 扩展 `ValidateConfig`，校验 managed agent 名称唯一、`agentId/token` 必填、绑定的本地 agent 存在且连接参数有效
- [x] 1.4 补充或更新配置示例，展示如何以统一配置管理多个 relay-managed agent

## 2. Relay 运行时集成

- [x] 2.1 新增 relay 模块，封装 WebSocket URL 构造、消息编解码和连接生命周期管理
- [x] 2.2 实现 relay 连接的心跳、自动重连和重连后状态恢复逻辑
- [x] 2.3 实现 `activity.update`、`command.ack`、`command.result` 的发送能力，并覆盖成功与失败路径
- [x] 2.4 为 relay 连接与协议异常补充结构化日志，便于按 managed agent 定位故障

## 3. 本地 agent 桥接与路由

- [x] 3.1 新增 relay command bridge，把 `command.dispatch` 映射到已配置的本地 agent 执行入口
- [x] 3.2 调整 `AgentRegistry` 或相关适配层，使 relay-managed identity 能绑定到现有本地 agent 而不重复创建执行 loop
- [x] 3.3 扩展多 agent 路由逻辑，使 relay-managed agent 能稳定解析到目标本地 agent
- [x] 3.4 确保 relay 命令执行期间的 busy/idle/error 状态切换与执行结果保持一致

## 4. 验证与兼容性

- [x] 4.1 为配置归一化和校验增加测试，覆盖缺失凭据、重复名称、未知本地 agent 和禁用 managed agent 场景
- [x] 4.2 为 relay 协议处理增加测试或可重复验证脚本，覆盖连接建立、心跳、断线重连、ack 与 result 回传
- [x] 4.3 验证现有单 agent / 多 agent / 多 channel 配置在未启用 relay 时仍保持兼容
- [x] 4.4 更新接入文档，说明新的 relay-managed agent 配置方式和迁移建议
