# SDLE Second Assignment

SDLE Second Assignment of group T02G12.

Group members:

1. Carolina Gonçalves (up202108781@up.pt)
2. João Costa (up202108714@up.pt)
3. Marco Costa (up202108821@up.pt)
4. Rafael Teixeira (up202108831@up.pt)

## How to run the code

This project was tested using WSL2 with Ubuntu 20.04 LTS, and on Arch Linux.

In order to run the application, you need to have the following dependencies installed:
- Nlohmann JSON ([github](https://github.com/nlohmann/json?tab=readme-ov-file#creating-json-objects-from-json-literals))
- Make
- g++
- ZMQ
- Pip (for installing dependencies such as Nlohmann JSON)

In order to install the dependencies, you can run the following commands:

```bash
make system-requirements
```

After installing the dependencies, you can compile the application by running the following command:

```bash
make
```

To run the application, you can run the following command:

```bash
make runClient1
make runClient2
# or directly
./src/client/client <broker_ip/port> <path/to/database>

make runBroker

make runWorker1
make runWorker2
make runWorker3
make runWorker4
# or directly
./src/worker/worker <broker> <service> <worker_pull_port> <worker_connect_address> <database_path>
```