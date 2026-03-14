## Context

`RelayManager::Worker::HandleMessage` currently emits relay-visible state only at the outer command boundary: acknowledge dispatch, mark the agent busy, emit a running result, execute `AgentRegistry::ProcessDirect`, then send a terminal completed or failed result. Inside `AgentLoop::ProcessDirect`, the agent progresses through several meaningful phases such as initial message handling, model completion, tool-call execution, and final reply generation, but those phases are currently only reflected in local control flow and logs.

This change spans both relay and agent modules. The relay layer owns websocket communication and `activity.update` payload formatting, while the agent layer owns the lifecycle transitions that should be surfaced. The design therefore needs a narrow integration contract that lets agent execution report progress without coupling core agent logic directly to websocket transport details.

## Goals / Non-Goals

**Goals:**
- Provide relay-visible lifecycle updates for relay-dispatched agent execution, including message receipt, active processing, tool invocation, and reply generation.
- Keep relay transport concerns centralized in relay-specific code while allowing agent code to emit lifecycle events through a small callback or observer interface.
- Preserve existing terminal relay semantics for `command.result` and final `idle` / `error` activity states.
- Ensure activity updates remain associated with the active relay `commandId`.

**Non-Goals:**
- Redesign the relay websocket protocol beyond additional `activity.update` usage.
- Introduce speculative percentage progress for every internal model step.
- Change non-relay channel behavior or external channel message formats.

## Decisions

### Introduce an execution activity sink between relay and agent layers
`AgentRegistry::ProcessDirect` should gain an optional execution observer abstraction that receives normalized agent lifecycle events. For relay-dispatched commands, `RelayManager::Worker` will provide an observer implementation that translates those events into `activity.update` payloads. Non-relay callers can continue calling `ProcessDirect` without an observer and will see unchanged behavior.

This keeps `AgentLoop` independent from relay transport and makes the integration testable at the boundary of emitted lifecycle events. The main alternative was to let `AgentLoop` call relay code directly, but that would create an undesirable dependency from agent execution into websocket transport.

### Model lifecycle updates as a small fixed phase set
The lifecycle should be expressed as a stable set of phases rather than ad hoc log strings. The minimum useful set for this change is:
- received: the relay-dispatched request has been accepted and local agent execution is starting
- processing: the agent is preparing context or waiting for / handling model output
- calling_tools: the agent is executing one or more tool calls from a model response
- replying: the agent is finalizing the assistant response for the caller

Relay payload summaries can remain human-readable, but phase identifiers should be stable so future code can map them consistently. The alternative of sending every internal micro-step would be noisier and less stable.

### Emit updates only when the phase changes
`AgentLoop::ProcessDirect` may iterate multiple times while handling model responses and tool calls. The observer should therefore be notified only on meaningful phase transitions, not on every loop iteration, to avoid flooding the relay server with redundant updates.

### Keep terminal status handling in RelayManager
The observer will cover in-flight lifecycle updates only. `RelayManager::Worker::HandleMessage` should remain responsible for `command.ack`, `command.result`, and terminal `idle` / `error` activity messages because it owns command correlation, websocket error handling, and terminal status recovery.

## Risks / Trade-offs

- [Observer API leaks relay semantics into agent code] → Keep the observer contract generic and phase-based, without websocket or JSON types.
- [Too many updates for long tool-heavy requests] → Emit only on phase transitions and avoid duplicate updates for repeated tool batches.
- [Ambiguity between outer relay `busy` and inner lifecycle phases] → Treat outer `busy` as the top-level activity status and use lifecycle-specific summaries or sub-status mapping consistently in relay payloads.
- [Future channels may want the same lifecycle stream] → The observer interface can be reused later, but this change scopes usage to relay-dispatched commands only.

## Migration Plan

1. Extend the agent direct-processing path to accept an optional lifecycle observer.
2. Implement relay-side mapping from observer events to `activity.update` payloads correlated with the active `commandId`.
3. Update relay integration specs to require lifecycle activity reporting.
4. Rollback strategy: remove the observer hookup while preserving the existing outer `busy/completed/failed` behavior.

## Open Questions

- Whether relay payloads should encode the detailed phase in `activityStatus`, `activitySummary`, or both depends on relay server expectations; the initial implementation should preserve compatibility by continuing to send valid `activity.update` messages with richer summaries.
- If the relay server expects a finite enum for `activityStatus`, the detailed lifecycle may need to remain in `activitySummary` while `activityStatus` stays at `busy` until terminal completion.
