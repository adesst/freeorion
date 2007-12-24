#include "ServerFSM.h"

#include "SaveLoad.h"
#include "ServerApp.h"
#include "../Empire/Empire.h"
#include "../network/ServerNetworking.h"
#include "../network/Message.h"
#include "../util/Directories.h"
#include "../util/OrderSet.h"

#include <GG/Font.h>


namespace {
    const bool TRACE_EXECUTION = false;

    void RebuildSaveGameEmpireData(std::map<int, SaveGameEmpireData>& save_game_empire_data, const std::string& save_game_filename)
    {
        save_game_empire_data.clear();
        std::vector<PlayerSaveGameData> player_save_game_data;
        int current_turn;
        Universe universe;
        LoadGame((GetLocalDir() / "save" / save_game_filename).native_file_string().c_str(),
                 current_turn, player_save_game_data, universe);
        for (unsigned int i = 0; i < player_save_game_data.size(); ++i) {
            Empire* empire = player_save_game_data[i].m_empire;
            SaveGameEmpireData& data = save_game_empire_data[empire->EmpireID()];
            data.m_id = empire->EmpireID();
            data.m_name = empire->Name();
            data.m_player_name = empire->PlayerName();
            data.m_color = empire->Color();
            delete empire;
        }
    }
}

////////////////////////////////////////////////////////////
// MessageEventBase
////////////////////////////////////////////////////////////
Disconnection::Disconnection(PlayerConnectionPtr& player_connection) :
    m_player_connection(player_connection)
{}

////////////////////////////////////////////////////////////
// MessageEventBase
////////////////////////////////////////////////////////////
MessageEventBase::MessageEventBase(Message& message, PlayerConnectionPtr& player_connection) :
    m_message(),
    m_player_connection(player_connection)
{ swap(m_message, message); }


////////////////////////////////////////////////////////////
// ServerFSM
////////////////////////////////////////////////////////////
ServerFSM::ServerFSM(ServerApp &server) :
    m_server(server)
{}

void ServerFSM::unconsumed_event(const boost::statechart::event_base &event)
{
    std::string most_derived_message_type_str = "[ERROR: Unknown Event]";
    const boost::statechart::event_base* event_ptr = &event;
    if (dynamic_cast<const Disconnection*>(event_ptr))
        most_derived_message_type_str = "Disconnection";
#define MESSAGE_EVENT_CASE(r, data, name)                               \
    else if (dynamic_cast<const name*>(event_ptr))                      \
        most_derived_message_type_str = BOOST_PP_STRINGIZE(name);
    BOOST_PP_SEQ_FOR_EACH(MESSAGE_EVENT_CASE, _, MESSAGE_EVENTS);
#undef MESSAGE_EVENT_CASE
    Logger().errorStream() << "ServerFSM : A " << most_derived_message_type_str << " event was passed to "
        "the ServerFSM.  This event is illegal in the FSM's current state.  It is being ignored.";
}

ServerApp& ServerFSM::Server()
{ return m_server; }

void ServerFSM::HandleNonLobbyDisconnection(const Disconnection& d)
{
    PlayerConnectionPtr& player_connection = d.m_player_connection;

    int id = player_connection->ID();
    // this will not usually happen, since the host client process usually owns the server process, and will usually take it down if it fails
    if (player_connection->Host()) {
        // if the host dies, there's really nothing else we can do -- the game's over
        std::string message = player_connection->PlayerName();
        for (ServerNetworking::const_established_iterator it = m_server.m_networking.established_begin(); it != m_server.m_networking.established_end(); ++it) {
            if ((*it)->ID() != id) {
                (*it)->SendMessage(EndGameMessage((*it)->ID(), Message::HOST_DISCONNECTED, player_connection->PlayerName()));
            }
        }
        Logger().debugStream() << "ServerFSM::HandleNonLobbyDisconnection : Host player disconnected; server terminating.";
        Sleep(2000); // HACK! Pause for a bit to let the player disconnected and end game messages propogate.
        m_server.Exit(1);
    } else if (m_server.m_losers.find(id) == m_server.m_losers.end()) { // player abnormally disconnected during a regular game
        Logger().debugStream() << "ServerFSM::HandleNonLobbyDisconnection : Lost connection to player #" << boost::lexical_cast<std::string>(id) 
                               << ", named \"" << player_connection->PlayerName() << "\"; server terminating.";
        std::string message = player_connection->PlayerName();
        for (ServerNetworking::const_established_iterator it = m_server.m_networking.established_begin(); it != m_server.m_networking.established_end(); ++it) {
            if ((*it)->ID() != id) {
                // in the future we may find a way to recover from this, but for now we will immediately send a game ending message as well
                (*it)->SendMessage(EndGameMessage((*it)->ID(), Message::NONHOST_DISCONNECTED, player_connection->PlayerName()));
            }
        }
    }

    // independently of everything else, if there are no humans left, it's time to terminate
    if (m_server.m_networking.empty() || m_server.m_ai_clients.size() == m_server.m_networking.NumPlayers()) {
        Logger().debugStream() << "ServerFSM::HandleNonLobbyDisconnection : All human players disconnected; server terminating.";
        Sleep(2000); // HACK! Pause for a bit to let the player disconnected and end game messages propogate.
        m_server.Exit(1);
    }
}


////////////////////////////////////////////////////////////
// Idle
////////////////////////////////////////////////////////////
Idle::Idle() :
    Base()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) Idle"; }

Idle::~Idle()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~Idle"; }

boost::statechart::result Idle::react(const HostMPGame& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) Idle.HostMPGame";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    std::string host_player_name = message.Text();
    int player_id = Networking::HOST_PLAYER_ID;
    player_connection->EstablishPlayer(player_id, host_player_name, true);
    player_connection->SendMessage(HostMPAckMessage(player_id));
    server.m_single_player_game = false;

    return transit<MPLobby>();
}

boost::statechart::result Idle::react(const HostSPGame& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) Idle.HostSPGame";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    boost::shared_ptr<SinglePlayerSetupData> setup_data(new SinglePlayerSetupData);
    ExtractMessageData(message, *setup_data);

    int player_id = Networking::HOST_PLAYER_ID;
    player_connection->EstablishPlayer(player_id, setup_data->m_host_player_name, true);
    player_connection->SendMessage(HostSPAckMessage(player_id));
    server.m_single_player_game = true;
    context<ServerFSM>().m_setup_data = setup_data;

    return transit<WaitingForSPGameJoiners>();
}


////////////////////////////////////////////////////////////
// MPLobby
////////////////////////////////////////////////////////////
MPLobby::MPLobby(my_context c) :
    Base(c),
    m_lobby_data (new MultiplayerLobbyData(true))
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby";
    ServerApp& server = Server();
    int player_id = Networking::HOST_PLAYER_ID;
    const PlayerConnectionPtr& player_connection = *server.m_networking.GetPlayer(player_id);
    PlayerSetupData& player_setup_data = m_lobby_data->m_players[player_id];
    player_setup_data.m_player_id = player_id;
    player_setup_data.m_player_name = player_connection->PlayerName();
    player_setup_data.m_empire_color = EmpireColors().at(0);
    server.m_networking.SendMessage(ServerLobbyUpdateMessage(player_id, *m_lobby_data));
}

MPLobby::~MPLobby()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~MPLobby"; }

boost::statechart::result MPLobby::react(const Disconnection& d)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby.Disconnection";
    ServerApp& server = Server();
    PlayerConnectionPtr& player_connection = d.m_player_connection;

    int id = player_connection->ID();
    if (m_lobby_data->m_players.find(id) != m_lobby_data->m_players.end()) {
        // this will not usually happen, since the host process usually owns the server process, and will usually take it down if it fails
        if (player_connection->Host()) {
            for (ServerNetworking::const_iterator it = server.m_networking.begin(); it != server.m_networking.end(); ++it) {
                if (*it != player_connection)
                    (*it)->SendMessage(ServerLobbyHostAbortMessage((*it)->ID()));
            }
            Logger().debugStream() << "MPLobby.Disconnection : Host player disconnected; server terminating.";
            server.Exit(1);
        } else {
            m_lobby_data->m_players.erase(id);
            for (ServerNetworking::const_iterator it = server.m_networking.begin(); it != server.m_networking.end(); ++it) {
                if (*it != player_connection)
                    (*it)->SendMessage(ServerLobbyExitMessage(id, (*it)->ID()));
            }
        }

        // independently of everything else, if there are no humans left, it's time to terminate
        if (server.m_networking.empty() || server.m_ai_clients.size() == server.m_networking.NumPlayers()) {
            Logger().debugStream() << "MPLobby.Disconnection : All human players disconnected; server terminating.";
            server.Exit(1);
        }
    }

    return discard_event();
}

boost::statechart::result MPLobby::react(const JoinGame& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby.JoinGame";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    std::string player_name = message.Text();
    int player_id = server.m_networking.GreatestPlayerID() + 1;
    player_connection->EstablishPlayer(player_id, player_name, false);
    player_connection->SendMessage(JoinAckMessage(player_id));
    PlayerSetupData& player_setup_data = m_lobby_data->m_players[player_id];
    player_setup_data.m_player_id = player_id;
    player_setup_data.m_player_name = player_name;
    player_setup_data.m_empire_color = EmpireColors().at(0);
    for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ++it) {
        (*it)->SendMessage(ServerLobbyUpdateMessage((*it)->ID(), *m_lobby_data));
    }

    return discard_event();
}

boost::statechart::result MPLobby::react(const LobbyUpdate& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby.LobbyUpdate";
    ServerApp& server = Server();
    const Message& message = msg.m_message;

    MultiplayerLobbyData incoming_lobby_data;
    ExtractMessageData(message, incoming_lobby_data);

    // NOTE: The client is only allowed to update certain of these, so those are the only ones we'll copy into m_lobby_data.
    m_lobby_data->m_new_game = incoming_lobby_data.m_new_game;
    m_lobby_data->m_size = incoming_lobby_data.m_size;
    m_lobby_data->m_shape = incoming_lobby_data.m_shape;
    m_lobby_data->m_age = incoming_lobby_data.m_age;
    m_lobby_data->m_starlane_freq = incoming_lobby_data.m_starlane_freq;
    m_lobby_data->m_planet_density = incoming_lobby_data.m_planet_density;
    m_lobby_data->m_specials_freq = incoming_lobby_data.m_specials_freq;

    bool new_save_file_selected = false;
    if (incoming_lobby_data.m_save_file_index != m_lobby_data->m_save_file_index &&
        0 <= incoming_lobby_data.m_save_file_index &&
        incoming_lobby_data.m_save_file_index < static_cast<int>(m_lobby_data->m_save_games.size())) {
        m_lobby_data->m_save_file_index = incoming_lobby_data.m_save_file_index;
        RebuildSaveGameEmpireData(m_lobby_data->m_save_game_empire_data, m_lobby_data->m_save_games[m_lobby_data->m_save_file_index]);
        // reset the current choice of empire for each player, since the new save game's empires may not have the same IDs
        for (unsigned int i = 0; i < m_lobby_data->m_players.size(); ++i) {
            m_lobby_data->m_players[i].m_save_game_empire_id = -1;
        }
        new_save_file_selected = true;
    }
    m_lobby_data->m_players = incoming_lobby_data.m_players;

    for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ++it) {
        if ((*it)->ID() != message.SendingPlayer() || new_save_file_selected)
            (*it)->SendMessage(ServerLobbyUpdateMessage((*it)->ID(), *m_lobby_data));
    }

    return discard_event();
}

boost::statechart::result MPLobby::react(const LobbyChat& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby.LobbyChat";
    ServerApp& server = Server();
    const Message& message = msg.m_message;

    if (message.ReceivingPlayer() == -1) { // the receiver is everyone (except the sender)
        for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ++it) {
            if ((*it)->ID() != message.SendingPlayer())
                (*it)->SendMessage(ServerLobbyChatMessage(message.SendingPlayer(), (*it)->ID(), message.Text()));
        }
    } else {
        server.m_networking.SendMessage(ServerLobbyChatMessage(message.SendingPlayer(), message.ReceivingPlayer(), message.Text()));
    }

    return discard_event();
}

boost::statechart::result MPLobby::react(const LobbyHostAbort& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby.LobbyHostAbort";
    ServerApp& server = Server();
    const Message& message = msg.m_message;

    for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ++it) {
        if ((*it)->ID() != message.SendingPlayer()) {
            (*it)->SendMessage(ServerLobbyHostAbortMessage((*it)->ID()));
        }
    }

    Sleep(1000); // HACK! Add a delay here so the messages can propagate; setting socket linger does not appear to work

    for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ) {
        PlayerConnectionPtr player_connection = *it++;
        if (player_connection->ID() != message.SendingPlayer())
            server.m_networking.Disconnect(player_connection);
    }

    return discard_event();
}

boost::statechart::result MPLobby::react(const LobbyNonHostExit& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby.LobbyNonHostExit";
    ServerApp& server = Server();
    const Message& message = msg.m_message;

    m_lobby_data->m_players.erase(message.SendingPlayer());
    for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ++it) {
        if ((*it)->ID() != message.SendingPlayer())
            (*it)->SendMessage(ServerLobbyExitMessage(message.SendingPlayer(), (*it)->ID()));
    }

    return discard_event();
}

boost::statechart::result MPLobby::react(const StartMPGame& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) MPLobby.StartMPGame";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    if (player_connection->Host()) {
        if (m_lobby_data->m_new_game) {
            int expected_players = m_lobby_data->m_players.size();
            if (expected_players == static_cast<int>(server.m_networking.NumPlayers())) {
                server.NewGameInit(m_lobby_data);
                return transit<PlayingGame>();
            }
        } else {
            LoadGame((GetLocalDir() / "save" / m_lobby_data->m_save_games[m_lobby_data->m_save_file_index]).native_file_string(),
                     server.m_current_turn, m_player_save_game_data, GetUniverse());
            int expected_players = m_player_save_game_data.size();
            int needed_AI_clients = expected_players - server.m_networking.NumPlayers();
            if (!needed_AI_clients) {
                server.LoadGameInit(m_lobby_data, m_player_save_game_data);
                return transit<PlayingGame>();
            }
        }
    } else {
        Logger().errorStream() << "(ServerFSM) MPLobby.StartMPGame : Player #" << message.SendingPlayer()
                               << " attempted to initiate a game load, but is not the host.  Terminating connection.";
        server.m_networking.Disconnect(player_connection);
        return discard_event();
    }

    context<ServerFSM>().m_lobby_data = m_lobby_data;
    context<ServerFSM>().m_player_save_game_data = m_player_save_game_data;

    return transit<WaitingForMPGameJoiners>();
}


////////////////////////////////////////////////////////////
// WaitingForSPGameJoiners
////////////////////////////////////////////////////////////
WaitingForSPGameJoiners::WaitingForSPGameJoiners(my_context c) :
    Base(c),
    m_setup_data(context<ServerFSM>().m_setup_data),
    m_num_expected_players(0)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForSPGameJoiners";

    context<ServerFSM>().m_setup_data.reset();
    ServerApp& server = Server();

    if (m_setup_data->m_new_game) {
        m_num_expected_players = m_setup_data->m_AIs + 1;
        server.CreateAIClients(std::vector<PlayerSetupData>(m_setup_data->m_AIs), m_expected_ai_player_names);
    } else {
        LoadGame(m_setup_data->m_filename, server.m_current_turn, m_player_save_game_data, GetUniverse());
        assert(!m_player_save_game_data.empty());
        m_num_expected_players = m_player_save_game_data.size();
        server.CreateAIClients(std::vector<PlayerSetupData>(m_num_expected_players - 1), m_expected_ai_player_names);
    }
}

WaitingForSPGameJoiners::~WaitingForSPGameJoiners()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~WaitingForSPGameJoiners"; }

boost::statechart::result WaitingForSPGameJoiners::react(const JoinGame& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForSPGameJoiners.JoinGame";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    std::string player_name = message.Text();
    int player_id = server.m_networking.GreatestPlayerID() + 1;
    std::set<std::string>::iterator it = m_expected_ai_player_names.find(player_name);
    if (it != m_expected_ai_player_names.end()) { // incoming AI player connection
        // let the networking system know what socket this player is on
        player_connection->EstablishPlayer(player_id, player_name, false);
        player_connection->SendMessage(JoinAckMessage(player_id));
        m_expected_ai_player_names.erase(player_name); // only allow one connection per AI
        server.m_ai_IDs.insert(player_id);
    } else { // non-AI player connection
        if (static_cast<int>(m_expected_ai_player_names.size() + server.m_networking.NumPlayers()) < m_num_expected_players) {
            player_connection->EstablishPlayer(player_id, player_name, false);
            player_connection->SendMessage(JoinAckMessage(player_id));
        } else {
            Logger().errorStream() << "WaitingForSPGameJoiners.JoinGame : A human player attempted to join "
                "the game but there was not enough room.  Terminating connection.";
            server.m_networking.Disconnect(player_connection);
        }
    }

    if (static_cast<int>(server.m_networking.NumPlayers()) == m_num_expected_players) {
        if (m_setup_data->m_new_game)
            server.NewGameInit(m_setup_data);
        else
            server.LoadGameInit(m_player_save_game_data);
        return transit<PlayingGame>();
    }

    return discard_event();
}


////////////////////////////////////////////////////////////
// WaitingForMPGameJoiners
////////////////////////////////////////////////////////////
WaitingForMPGameJoiners::WaitingForMPGameJoiners(my_context c) :
    Base(c),
    m_lobby_data(context<ServerFSM>().m_lobby_data),
    m_player_save_game_data(context<ServerFSM>().m_player_save_game_data),
    m_num_expected_players(0)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForMPGameJoiners";
    context<ServerFSM>().m_lobby_data.reset();
    context<ServerFSM>().m_player_save_game_data.clear();
    ServerApp& server = Server();
    if (m_lobby_data->m_new_game) {
        m_num_expected_players = m_lobby_data->m_players.size();
        std::vector<PlayerSetupData> AI_clients;
        for (std::set<int>::const_iterator it = m_lobby_data->m_AI_player_ids.begin(); it != m_lobby_data->m_AI_player_ids.end(); ++it) {
            assert(m_lobby_data->m_players.find(*it) != m_lobby_data->m_players.end());
            AI_clients.push_back(m_lobby_data->m_players[*it]);
        }
        server.CreateAIClients(AI_clients, m_expected_ai_player_names);
    } else {
        m_num_expected_players = m_player_save_game_data.size();
        int num_AI_clients = m_num_expected_players - server.m_networking.NumPlayers();
        server.CreateAIClients(std::vector<PlayerSetupData>(num_AI_clients), m_expected_ai_player_names);
    }
}

WaitingForMPGameJoiners::~WaitingForMPGameJoiners()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~WaitingForMPGameJoiners"; }

boost::statechart::result WaitingForMPGameJoiners::react(const JoinGame& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForMPGameJoiners.JoinGame";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    std::string player_name = message.Text();
    int player_id = server.m_networking.GreatestPlayerID() + 1;
    std::set<std::string>::iterator it = m_expected_ai_player_names.find(player_name);
    if (it != m_expected_ai_player_names.end()) { // incoming AI player connection
        // let the networking system know what socket this player is on
        player_connection->EstablishPlayer(player_id, player_name, false);
        player_connection->SendMessage(JoinAckMessage(player_id));
        m_expected_ai_player_names.erase(player_name); // only allow one connection per AI
        server.m_ai_IDs.insert(player_id);
    } else { // non-AI player connection
        if (static_cast<int>(m_expected_ai_player_names.size() + server.m_networking.NumPlayers()) < m_num_expected_players) {
            player_connection->EstablishPlayer(player_id, player_name, false);
            player_connection->SendMessage(JoinAckMessage(player_id));
        } else {
            Logger().errorStream() << "WaitingForMPGameJoiners.JoinGame : A human player attempted to join "
                "the game but there was not enough room.  Terminating connection.";
            server.m_networking.Disconnect(player_connection);
        }
    }

    if (static_cast<int>(server.m_networking.NumPlayers()) == m_num_expected_players) {
        if (m_player_save_game_data.empty())
            server.NewGameInit(m_lobby_data);
        else
            server.LoadGameInit(m_lobby_data, m_player_save_game_data);
        return transit<PlayingGame>();
    }

    return discard_event();
}


////////////////////////////////////////////////////////////
// PlayingGame
////////////////////////////////////////////////////////////
PlayingGame::PlayingGame() :
    Base()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) PlayingGame"; }

PlayingGame::~PlayingGame()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~PlayingGame"; }


////////////////////////////////////////////////////////////
// WaitingForTurnEnd
////////////////////////////////////////////////////////////
WaitingForTurnEnd::WaitingForTurnEnd() :
    Base()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEnd"; }

WaitingForTurnEnd::~WaitingForTurnEnd()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~WaitingForTurnEnd"; }

boost::statechart::result WaitingForTurnEnd::react(const HostSPGame& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEnd.HostSPGame";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    boost::shared_ptr<SinglePlayerSetupData> setup_data(new SinglePlayerSetupData);
    ExtractMessageData(message, *setup_data);

    if (player_connection->Host() && !setup_data->m_new_game) {
        Empires().RemoveAllEmpires();
        player_connection->SendMessage(HostSPAckMessage(player_connection->ID()));
        player_connection->SendMessage(JoinAckMessage(player_connection->ID()));
        server.m_single_player_game = true;
        context<ServerFSM>().m_setup_data = setup_data;
        return transit<WaitingForSPGameJoiners>();
    }

    if (!player_connection->Host()) {
        Logger().errorStream() << "WaitingForTurnEnd.HostSPGame : Player #" << message.SendingPlayer()
                               << " attempted to initiate a new game or game load, but is not the host. "
                               << "Terminating connection.";
    }
    if (setup_data->m_new_game) {
        Logger().errorStream() << "WaitingForTurnEnd.HostSPGame : Player #" << message.SendingPlayer()
                               << " attempted to start a new game without ending the current one. "
                               << "Terminating connection.";
    }
    server.m_networking.Disconnect(player_connection);

    return discard_event();
}

boost::statechart::result WaitingForTurnEnd::react(const TurnOrders& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEnd.TurnOrders";
    ServerApp& server = Server();
    const Message& message = msg.m_message;

    OrderSet* order_set = new OrderSet;
    ExtractMessageData(message, *order_set);

    // check order validity -- all orders must originate from this empire in order to be considered valid
    Empire* empire = server.GetPlayerEmpire(message.SendingPlayer());
    assert(empire);
    for (OrderSet::const_iterator it = order_set->begin(); it != order_set->end(); ++it) {
        Order* order = it->second;
        assert(order);
        if (empire->EmpireID() != order->EmpireID()) {
            throw std::runtime_error(
                "WaitingForTurnEnd.TurnOrders : Player \"" + empire->PlayerName() +
                "\" attempted to issue an order for player \"" +
                Empires().Lookup(order->EmpireID())->PlayerName() + "\"!  Terminating...");
        }
    }

    server.m_networking.SendMessage(TurnProgressMessage(message.SendingPlayer(), Message::WAITING_FOR_PLAYERS, -1));

    Logger().debugStream() << "WaitingForTurnEnd.TurnOrders : Received orders from player " << message.SendingPlayer();

    server.SetEmpireTurnOrders(server.GetPlayerEmpire(message.SendingPlayer())->EmpireID(), order_set);

    if (server.AllOrdersReceived()) {
        Logger().debugStream() << "WaitingForTurnEnd.TurnOrders : All orders received.  Processing turn....";
        server.ProcessTurns();
        return transit<WaitingForTurnEnd>();
    }

    return discard_event();
}

boost::statechart::result WaitingForTurnEnd::react(const RequestObjectID& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEnd.RequestObjectID";
    Server().m_networking.SendMessage(DispatchObjectIDMessage(msg.m_message.SendingPlayer(), GetUniverse().GenerateObjectID()));
    return discard_event();
}

boost::statechart::result WaitingForTurnEnd::react(const RequestDesignID& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEnd.RequestDesignID";
    Server().m_networking.SendMessage(DispatchDesignIDMessage(msg.m_message.SendingPlayer(), GetUniverse().GenerateDesignID()));
    return discard_event();
}

boost::statechart::result WaitingForTurnEnd::react(const PlayerChat& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEnd.PlayerChat";
    ServerApp& server = Server();
    for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ++it) {
        if (msg.m_message.ReceivingPlayer() == -1 || msg.m_message.ReceivingPlayer() == (*it)->ID())
            (*it)->SendMessage(SingleRecipientChatMessage(msg.m_message.SendingPlayer(), (*it)->ID(), msg.m_message.Text()));
    }
    return discard_event();
}


////////////////////////////////////////////////////////////
// WaitingForTurnEndIdle
////////////////////////////////////////////////////////////
WaitingForTurnEndIdle::WaitingForTurnEndIdle() :
    Base()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEndIdle"; }

WaitingForTurnEndIdle::~WaitingForTurnEndIdle()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~WaitingForTurnEndIdle"; }

boost::statechart::result WaitingForTurnEndIdle::react(const SaveGameRequest& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForTurnEndIdle.SaveGameRequest";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    if (!player_connection->Host()) {
        Logger().errorStream() << "WaitingForTurnEndIdle.SaveGameRequest : Player #" << message.SendingPlayer()
                               << " attempted to initiate a game save, but is not the host.  Terminating connection.";
        server.m_networking.Disconnect(player_connection);
    }

    context<WaitingForTurnEnd>().m_save_filename = message.Text();

    return transit<WaitingForSaveData>();
}


////////////////////////////////////////////////////////////
// WaitingForSaveData
////////////////////////////////////////////////////////////
WaitingForSaveData::WaitingForSaveData(my_context c) :
    Base(c)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForSaveData";

    ServerApp& server = Server();
    for (ServerNetworking::const_established_iterator it = server.m_networking.established_begin(); it != server.m_networking.established_end(); ++it) {
        (*it)->SendMessage(ServerSaveGameMessage((*it)->ID(), (*it)->ID() == Networking::HOST_PLAYER_ID));
        m_needed_reponses.insert((*it)->ID());
    }
}

WaitingForSaveData::~WaitingForSaveData()
{ if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) ~WaitingForSaveData"; }

boost::statechart::result WaitingForSaveData::react(const ClientSaveData& msg)
{
    if (TRACE_EXECUTION) Logger().debugStream() << "(ServerFSM) WaitingForSaveData.ClientSaveData";
    ServerApp& server = Server();
    const Message& message = msg.m_message;
    PlayerConnectionPtr& player_connection = msg.m_player_connection;

    boost::shared_ptr<OrderSet> order_set(new OrderSet);
    boost::shared_ptr<SaveGameUIData> ui_data(new SaveGameUIData);
    if (!ExtractMessageData(message, *order_set, *ui_data))
        ui_data.reset();
    m_player_save_game_data.push_back(PlayerSaveGameData(player_connection->PlayerName(), server.GetPlayerEmpire(message.SendingPlayer()), order_set, ui_data));
    m_players_responded.insert(message.SendingPlayer());

    if (m_players_responded == m_needed_reponses) {
        SaveGame(context<WaitingForTurnEnd>().m_save_filename, server.m_current_turn, m_player_save_game_data, GetUniverse());
        context<WaitingForTurnEnd>().m_save_filename = "";
        return transit<WaitingForTurnEndIdle>();
    }

    return discard_event();
}