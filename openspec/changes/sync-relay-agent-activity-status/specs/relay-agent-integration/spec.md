## MODIFIED Requirements

### Requirement: Report agent activity to the relay server
The system SHALL publish `activity.update` messages that reflect the relay-managed agent's execution state during connection startup, command execution progress, command execution success, and command execution failure.

#### Scenario: Report idle after connection opens
- **WHEN** a relay connection is established successfully
- **THEN** the runtime sends an `activity.update` message with `activityStatus` set to `idle`

#### Scenario: Report busy when command execution starts
- **WHEN** the relay runtime accepts a `command.dispatch` for execution
- **THEN** it sends an `activity.update` message with `activityStatus` set to `busy` before starting local execution

#### Scenario: Report lifecycle progress during local agent execution
- **WHEN** a relay-dispatched command enters local agent execution
- **THEN** the runtime sends `activity.update` messages for meaningful lifecycle transitions including message receipt, active processing, tool invocation, and reply generation before the command reaches a terminal state

#### Scenario: Correlate lifecycle updates to the active command
- **WHEN** the runtime emits an in-flight `activity.update` for a relay-dispatched command
- **THEN** the message includes the active `commandId` so the relay server can associate the update with the running command

#### Scenario: Report error after failed execution
- **WHEN** command execution fails for a relay-dispatched command
- **THEN** the runtime sends an `activity.update` message with `activityStatus` set to `error`
