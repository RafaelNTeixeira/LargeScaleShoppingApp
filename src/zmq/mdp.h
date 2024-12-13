//
//  mdp.h
//  Majordomo Protocol definitions
//
#ifndef __MDP_H_INCLUDED__
#define __MDP_H_INCLUDED__
#include <string_view>
#include <string>

//  This is the version of MDP/Client we implement
static constexpr std::string_view k_mdp_client= "MDPC01";
static constexpr std::string_view k_mdpc_heartbeat="\x06";

//  This is the version of MDP/Worker we implement
static constexpr std::string_view k_mdpw_worker= "MDPW01";
//  MDP/Server commands, as strings
static constexpr std::string_view k_mdpw_ready = "\x01";
static constexpr std::string_view k_mdpw_request ="\x02";
static constexpr std::string_view k_mdpw_reply ="\x03";
static constexpr std::string_view k_mdpw_heartbeat="\x04";
static constexpr std::string_view k_mdpw_disconnect="\x05";
static constexpr std::string_view k_mdpw_join_ring="\x07";
static constexpr std::string_view k_mdpw_broadcast_ring="\x08";
static constexpr std::string_view k_mdpw_recv_work="\x09";

static constexpr std::string_view mdps_commands [] = {
    "", "READY", "REQUEST", "REPLY", "HEARTBEAT", "DISCONNECT", "CLIENT_HEARTBEAT", "JOIN_RING", "BROADCAST_RING", "RECV_WORK"
};


using ustring = std::basic_string<unsigned char>;
#endif

