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
make runBroker # to run the broker

make runClient1 # to run the client 1
make runClient2 # to run the client 2

# or to directly run the client
mkdir -p database/local/user_name/ # user_name can be any name
# Example: mkdir -p database/local/joao/
./src/client/client <broker_ip/port> <broker_heartbeat_ip/port> <path/to/database>

make runWorker1 # to run the worker 1
make runWorker2 # to run the worker 2
make runWorker3 # to run the worker 3
make runWorker4 # to run the worker 4
make runWorker5 # to run the worker 5
make runWorker6 # to run the worker 6
make runWorker7 # to run the worker 7
make runWorker8 # to run the worker 8

# or to directly run the worker
mkdir -p database/cloud/database_name/  # database_name can be any name
# Example: mkdir -p database/cloud/my_database/
./src/worker/worker <broker> <service> <worker_pull_port> <worker_connect_address> <database_path>
```

To close the Broker and the Workers, you can just `CTRL+C` in the terminal.

To close the Client, you need choose the `Exit` option to close. 

## Documentation

In the `docs` folder you can find the presentation and the video demo for the project.