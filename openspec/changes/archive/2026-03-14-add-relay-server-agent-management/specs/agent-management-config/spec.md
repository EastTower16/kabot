## ADDED Requirements

### Requirement: Support a unified relay agent management configuration model
The system SHALL support a structured configuration model for relay-managed agents that separates shared relay connection settings from per-agent identity and binding settings.

#### Scenario: Load shared relay defaults and per-agent overrides
- **WHEN** the configuration defines shared relay connection settings and multiple relay-managed agent entries
- **THEN** the runtime loads the shared settings once and applies them to each relay-managed agent unless explicitly overridden

#### Scenario: Keep local agent configuration independent
- **WHEN** relay-managed agent settings are loaded
- **THEN** the runtime preserves existing local agent execution settings independently from relay connection parameters

### Requirement: Validate relay-managed agent declarations before startup
The system MUST validate relay-managed agent configuration before startup completes, including required credentials, unique managed agent names, and valid bindings to local agent instances.

#### Scenario: Reject missing relay credentials
- **WHEN** a relay-managed agent omits `agentId` or `token`
- **THEN** startup fails with a validation error that identifies the invalid managed agent entry

#### Scenario: Reject duplicate managed agent names
- **WHEN** two relay-managed agent entries declare the same management name
- **THEN** startup fails with a validation error for the duplicate name

#### Scenario: Reject unknown local agent binding
- **WHEN** a relay-managed agent references a local agent name that is not defined in local agent configuration
- **THEN** startup fails with a validation error that identifies the managed agent and unknown local agent reference

### Requirement: Normalize simplified configuration into a stable runtime model
The system SHALL normalize legacy or simplified relay agent configuration forms into a single runtime representation before managers and registries are initialized.

#### Scenario: Fill default relay timing values
- **WHEN** relay configuration omits optional heartbeat or reconnect timing values
- **THEN** the runtime supplies documented default values in the normalized configuration

#### Scenario: Preserve disabled entries in normalized config
- **WHEN** relay-managed agent entries are present but disabled
- **THEN** the normalized runtime model retains those entries without starting connections for them
