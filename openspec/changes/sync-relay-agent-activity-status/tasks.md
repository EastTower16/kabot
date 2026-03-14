## 1. Agent lifecycle event surface

- [x] 1.1 Define a small phase-based execution observer interface for direct agent processing that can be passed through `AgentRegistry::ProcessDirect` into `AgentLoop::ProcessDirect`
- [x] 1.2 Emit observer callbacks from `AgentLoop::ProcessDirect` for relay-relevant transitions: received, processing, calling_tools, and replying
- [x] 1.3 Ensure lifecycle callbacks fire only on meaningful phase changes and preserve current non-relay behavior when no observer is provided

## 2. Relay activity synchronization

- [x] 2.1 Implement a relay-side observer in `RelayManager::Worker` that maps agent lifecycle phases to `activity.update` payloads with the active `commandId`
- [x] 2.2 Integrate the observer into relay-dispatched `agents_.ProcessDirect(...)` execution without changing existing `command.ack` and terminal `command.result` handling
- [x] 2.3 Preserve final relay state transitions so successful commands return to `idle` and failed commands report `error`

## 3. Regression coverage

- [x] 3.1 Review relay-managed execution paths for duplicate or missing activity updates across tool-call loops and terminal completion
- [x] 3.2 Verify the implementation continues to handle non-relay direct processing calls without requiring relay-specific dependencies
- [x] 3.3 Update any relevant documentation or configuration notes if relay activity semantics become externally observable
