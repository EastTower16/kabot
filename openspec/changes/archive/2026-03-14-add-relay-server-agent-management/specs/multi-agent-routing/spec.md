## MODIFIED Requirements

### Requirement: Support multiple named agent instances
The system SHALL support configuring multiple named agent instances. Each agent instance MUST have a stable unique name and MAY override default agent settings, including workspace and model-related parameters. The runtime MAY additionally load relay-managed agent declarations that bind remote relay identities to those local named agent instances without creating duplicate local execution loops.

#### Scenario: Load multiple configured agents
- **WHEN** the configuration defines more than one agent instance
- **THEN** the system creates a distinct runtime agent instance for each configured agent name

#### Scenario: Apply default values to agent instances
- **WHEN** an agent instance omits optional settings
- **THEN** the system inherits those settings from the global agent defaults

#### Scenario: Override workspace per agent
- **WHEN** two configured agent instances specify different workspaces
- **THEN** each agent uses its own configured workspace without affecting the other agent

#### Scenario: Bind a relay-managed identity to an existing local agent
- **WHEN** the configuration defines a relay-managed agent that targets an existing local agent name
- **THEN** the runtime keeps a single local execution loop for that local agent and maps the relay-managed identity to it

### Requirement: Route inbound messages to the correct agent
The system SHALL route every inbound message to exactly one target agent based on the configured `binding.agent` for the receiving channel instance, any explicit valid agent metadata attached to the message, and any validated relay-managed agent binding that maps a remote relay identity to a local agent instance.

#### Scenario: Route to the bound agent
- **WHEN** an inbound message arrives on a channel instance that declares `binding.agent`
- **THEN** the message is routed to that bound agent

#### Scenario: Respect explicit valid agent metadata
- **WHEN** an inbound message carries a valid `agent_name` that matches a configured agent
- **THEN** the system routes the message to that agent

#### Scenario: Route relay-dispatched work to the bound local agent
- **WHEN** a relay-managed agent receives a `command.dispatch` and its management binding targets a configured local agent
- **THEN** the runtime executes the command through that bound local agent rather than creating a separate anonymous execution target
