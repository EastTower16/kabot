## ADDED Requirements

### Requirement: Connect configured relay agents to the relay server
The system SHALL allow each enabled relay-managed agent to establish an authenticated WebSocket connection to the configured relay server endpoint using its configured `agentId` and `token`.

#### Scenario: Connect an enabled relay agent
- **WHEN** startup loads an enabled relay-managed agent with a valid server endpoint, `agentId`, and `token`
- **THEN** the runtime opens a WebSocket connection to `/ws/agent` using those credentials

#### Scenario: Skip disabled relay agents
- **WHEN** a relay-managed agent is marked disabled in configuration
- **THEN** the runtime does not attempt to open a relay connection for that agent

### Requirement: Maintain relay liveness and recover connectivity
The system SHALL maintain relay liveness for each connected relay-managed agent by sending heartbeats on a configured interval and MUST automatically reconnect after connection loss using bounded backoff.

#### Scenario: Send heartbeat for active connection
- **WHEN** a relay-managed agent has an open relay connection
- **THEN** the runtime periodically sends `heartbeat` messages before the peer liveness timeout expires

#### Scenario: Reconnect after disconnect
- **WHEN** an established relay connection closes unexpectedly
- **THEN** the runtime retries connection using configured backoff and restarts heartbeat after reconnection

### Requirement: Report agent activity to the relay server
The system SHALL publish `activity.update` messages that reflect the relay-managed agent's execution state during connection startup, command execution success, and command execution failure.

#### Scenario: Report idle after connection opens
- **WHEN** a relay connection is established successfully
- **THEN** the runtime sends an `activity.update` message with `activityStatus` set to `idle`

#### Scenario: Report busy during command execution
- **WHEN** the relay runtime accepts a `command.dispatch` for execution
- **THEN** it sends an `activity.update` message with `activityStatus` set to `busy` before starting local execution

#### Scenario: Report error after failed execution
- **WHEN** command execution fails for a relay-dispatched command
- **THEN** the runtime sends an `activity.update` message with `activityStatus` set to `error`

### Requirement: Acknowledge and complete dispatched relay commands
The system SHALL accept `command.dispatch` messages from the relay server, reply with `command.ack`, and send `command.result` updates that identify the same `commandId` until the command reaches a terminal status.

#### Scenario: Acknowledge a received command
- **WHEN** a valid `command.dispatch` message is received
- **THEN** the runtime promptly sends `command.ack` with the same `commandId`

#### Scenario: Return a completed result
- **WHEN** local execution of a relay-dispatched command succeeds
- **THEN** the runtime sends `command.result` with status `completed` and the original `commandId`

#### Scenario: Return a failed result
- **WHEN** local execution of a relay-dispatched command fails
- **THEN** the runtime sends `command.result` with status `failed`, the original `commandId`, and an error summary
