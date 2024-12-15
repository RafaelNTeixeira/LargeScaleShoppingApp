#include <deque>
#include <iostream>
#include <unordered_set>
#include <zmq.hpp>

#include "../zmq/mdp.h"
#include "../zmq/zmsg.hpp"
#include "majBroker.cpp"

int main(int argc, char *argv[]) {
    int verbose = (argc > 1 && strcmp(argv[1], "-v") == 0);

    s_version_assert(4, 0);
    s_catch_signals();
    broker brk(verbose);
    brk.bind("tcp://*:5555", "tcp://*:5556", "tcp://*:5557", "tcp://*:5558", "tcp://*:5554");

    brk.start_brokering();

    if (s_interrupted)
        printf("W: interrupt received, shutting down...\n");

    return 0;
}

