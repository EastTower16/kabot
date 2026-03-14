## Why

Relay-managed agents currently report only coarse-grained relay activity such as `idle`, `busy`, `completed`, and `error`. When an agent receives a channel message and progresses through message intake, reasoning, tool invocation, and reply generation, those intermediate states are not propagated to the relay server, leaving remote observers without accurate execution visibility.

## What Changes

- Extend relay-managed command execution to publish finer-grained `activity.update` messages that track the agent lifecycle while processing a dispatched request.
- Define a stable mapping between agent-side execution phases and relay activity payloads so the relay server can observe message receipt, active processing, tool execution, and reply generation.
- Ensure relay activity updates remain correlated to the active relay `commandId` and converge back to `idle` or `error` on terminal completion.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `relay-agent-integration`: expand activity reporting requirements so relay-managed agents must emit lifecycle state transitions during local agent execution, not just connection, start, and terminal outcomes.

## Impact

- Affected specs: `openspec/specs/relay-agent-integration/spec.md`
- Likely affected code: `src/relay/relay_manager.cpp`, `src/relay/relay_manager.hpp`, `src/agent/agent_registry.*`, `src/agent/agent_loop.*`
- External system impact: relay server consumers will receive more frequent `activity.update` events for relay-dispatched agent work
