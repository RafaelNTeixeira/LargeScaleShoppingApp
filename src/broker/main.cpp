#include <iostream>
#include <deque>
#include <unordered_set>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "../zmq/zmsg.hpp"
#include "../zmq/mdp.h"
#include "majBroker.cpp"

using json = nlohmann::json;

int main(int argc, char *argv []) {
    int verbose = (argc > 1 && strcmp (argv [1], "-v") == 0);

    s_version_assert (4, 0);
    s_catch_signals ();
    broker brk(verbose);
    brk.bind ("tcp://*:5555", "tcp://*:5556", "tcp://*:5557", "tcp://localhost:5558");

    std::cout << "HERE" << std::endl;

    brk.start_brokering();

    if (s_interrupted)
        printf ("W: interrupt received, shutting down...\n");

    return 0;
}
