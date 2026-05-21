# Large Scale Shopping App
Distributed shopping app prototype with a broker-based load-balancing layer and worker pool. Clients send shopping list operations to a broker, which routes requests to workers that persist data and return results. The design showcases message-based coordination, fault tolerance patterns, and scalable request handling.

## Highlights
- Broker/worker architecture for load distribution
- Client UI flow with local database persistence
- ZeroMQ messaging and JSON payloads
- CRDT data structures for merge-friendly state

## Architecture (high level)
Client -> Broker -> Worker Pool -> Database
The broker owns routing and health checks. Workers process requests and write to storage. Clients interact via a simple CLI-driven UI.

## Requirements
Tested on WSL2 (Ubuntu 20.04 LTS) and Arch Linux.

Dependencies:
- g++
- Make
- ZeroMQ
- Nlohmann JSON
- Pip (used by the provided dependency script)

Install system requirements:

```bash
make system-requirements
```

## Build

```bash
make
```

## Run

Start the broker:

```bash
make runBroker
```

Start one or more clients:

```bash
make runClient1
make runClient2
```

Or run a client directly:

```bash
mkdir -p database/local/user_name/
# Example: mkdir -p database/local/joao/
./src/client/client <broker_ip/port> <broker_heartbeat_ip/port> <path/to/database>
```

Start worker instances:

```bash
make runWorker1
make runWorker2
make runWorker3
make runWorker4
make runWorker5
make runWorker6
make runWorker7
make runWorker8
```

Or run a worker directly:

```bash
mkdir -p database/cloud/database_name/
# Example: mkdir -p database/cloud/my_database/
./src/worker/worker <broker> <service> <worker_pull_port> <worker_connect_address> <database_path>
```

Shutdown notes:
- Broker/worker processes: press CTRL+C in the terminal
- Client: choose the Exit option in the UI

## Repo layout
- src/broker: broker entry point and routing logic
- src/worker: worker entry point and request handling
- src/client: client UI and local storage
- src/crdt: CRDT implementations
- test: unit tests
- doc: presentation and demo video

## Documentation
Click on the links to view the [Presentation](doc/report.pdf) slides and [Demo Video](doc/demo.mp4).
