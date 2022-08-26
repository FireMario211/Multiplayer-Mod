// this code is bad, if you want to improve it, create a PR kthx
#include "PlayLayer.h"

int reconnectionDelay = 1000;
int reconnectionDelayMax = 5000;
int reconnectionAttempts = 0;
int renderDist = 300;
// -------------------------------------------- //
bool connect_finish = false;
std::mutex lock;
std::unique_lock<std::mutex> unique_lock(lock);
std::condition_variable cond;
HANDLE hThread;
sio::socket::ptr current_socket;
int current_level_id = NULL;

// performance reasons, this is so it doesnt update players outside the players screen
float calculateDist(float x1, float y1, float x2, float y2) {
    // sqrt((x1-y1)^2 + (x2-y2)^2))
    return sqrt(pow(x1-y1, 2) + pow(x2-y2, 2));
}

namespace ConnectionHandler {
    void onSuccess() {
        std::cout << "Connection successful!" << std::endl;
        connect_finish = true;
        cond.notify_all();
    }
    void onClose(sio::client::close_reason const& reason) {
        std::cout << "Connection closed: " << reason << std::endl;
    }
    void onFail() {
        std::cout << "Connection failed." << std::endl;
    }
    void onError(sio::message::ptr const& message) {
        std::cout << "Sock error: " << message->get_string() << std::endl;
    }
}

// Emits a join event of the level so the server knows who to give the player info to 
bool PlayLayer::join() {
    sio::message::ptr object = sio::object_message::create();
    if (current_level_id == NULL) {
        current_level_id = 1;
    }
    std::cout << "emitting join" << std::endl;
    //object->get_map()["level"] = sio::string_message::create(std::to_string(current_level_id));
    current_socket->emit("join", std::to_string(current_level_id));
    std::cout << "emitted join" << std::endl;
    return true;
}


// probably not a good idea but I have no other solution
int playerTypeToModeID(gd::PlayerObject* player) {
    // Gamemode IDs:
    // 0 = Cube
    // 1 = Ship
    // 2 = Ball
    // 3 = UFO (Bird)
    // 4 = Wave
    // 5 = Robot
    // 6 = Spider

    // bad coding
    if (player->m_isShip) return 1;
    if (player->m_isBall) return 2;
    if (player->m_isBird) return 3;
    if (player->m_isDart) return 4;
    if (player->m_isRobot) return 5;
    if (player->m_isSpider) return 6;
    return 0;
}

// also bad coding
gd::IconType intToEnum(int gamemodeID) {
    switch (gamemodeID) {
    default:
        return gd::kIconTypeCube;
    case 1:
        return gd::kIconTypeShip;
    case 2:
        return gd::kIconTypeBall;
    case 3:
        return gd::kIconTypeUfo;
    case 4:
        return gd::kIconTypeWave;
    case 5:
        return gd::kIconTypeRobot;
    case 6:
        return gd::kIconTypeSpider;
    }
}

void updatePlayer(gd::PlayLayer* self, sio::message::ptr const& data) {
    auto objsLayer = reinterpret_cast<CCLayer*>(self->getChildren()->objectAtIndex(3));
    int tag = (int)data->get_map()["tag"]->get_int();
    double x = data->get_map()["x"]->get_double();
    double y = data->get_map()["y"]->get_double();
    double rot = data->get_map()["rotation"]->get_double();
    double scale = data->get_map()["scale"]->get_double();
    int gamemodeID = (int)data->get_map()["gamemode"]->get_int();
    auto getPlayerObj = objsLayer->getChildByTag(tag);

    float pX = self->m_pPlayer1->getPositionX();
    float pY = self->m_pPlayer2->getPositionY();

    float calcDistance = calculateDist(pX, pY, (float)x, (float)y);
    // If the code below is false, dont update the players position as to prevent lag (even though this wouldnt even cause lag at all)
    if (calcDistance <= renderDist) {
        if (getPlayerObj != NULL) {
            //getPlayerObj->updatePlayerFrame(1, gd::kIconTypeBall);
            //getPlayerObj->updatePlayerFrame(1, intToEnum(gamemodeID));
            std::cout << "updating player frame" << std::endl;
            auto player = reinterpret_cast<gd::SimplePlayer*>(getPlayerObj->getChildren()->objectAtIndex(0));
            std::cout << gamemodeID << std::endl;
            player->updatePlayerFrame(1, intToEnum(gamemodeID));
            getPlayerObj->setPosition(ccp(x, y));
            getPlayerObj->setRotation((float)rot);
            getPlayerObj->setScale((float)scale);
        } else {
            std::cout << "player not found, creating new one" << std::endl;
            CCNode* playersObj = CCNode::create();
            playersObj->setTag(tag);
            gd::SimplePlayer* player = gd::SimplePlayer::create(1);
            player->updatePlayerFrame(1, intToEnum(gamemodeID));
            playersObj->addChild(player);
            playersObj->setPosition(ccp(x, y));
            playersObj->setRotation((float)rot);
            playersObj->setScale((float)scale);
            objsLayer->addChild(playersObj);
            std::cout << "Player created" << std::endl;
        }
    }
}

bool PlayLayer::setSocket(gd::PlayLayer* self, sio::socket::ptr sock) {
    current_socket = sock;
    //auto player = self->getChildByTag(3982);
    //auto player = static_cast<CCSprite*>(self->getChildByTag(3982));
    /*auto playersObj = reinterpret_cast<CCNode*>(self->getChildren()->objectAtIndex(4));
    std::cout << playersObj->getChildrenCount() << std::endl;
    std::cout << "---" << std::endl;
    auto player = reinterpret_cast<CCSprite*>(playersObj->getChildren()->objectAtIndex(0));*/
    std::cout << self->getChildrenCount() << std::endl;
    PlayLayer::join();
    std::cout << "listening for events" << std::endl;
    std::cout << self->getChildrenCount() << std::endl;
    std::cout << self << std::endl;
    current_socket->on("update", sio::socket::event_listener_aux([&](std::string const& user, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
        /*auto objsLayer = reinterpret_cast<CCLayer*>(self->getChildren()->objectAtIndex(3));
        int tag = (int)data->get_map()["tag"]->get_int();
        double x = data->get_map()["x"]->get_double();
        double y = data->get_map()["y"]->get_double();
        double rot = data->get_map()["rotation"]->get_double();
        auto getPlayerObj = objsLayer->getChildByTag(tag);
        if (getPlayerObj != NULL) {
            getPlayerObj->setPosition(ccp(x, y));
            getPlayerObj->setRotation((float)rot);
        } else {
            std::cout << "player not found, creating new one" << std::endl;
            CCNode* playersObj = CCNode::create();
            playersObj->setTag(tag);
            CCSprite* player = gd::SimplePlayer::create(1);
            playersObj->addChild(player);
            playersObj->setPosition(ccp(x, y));
            playersObj->setRotation((float)rot);
            objsLayer->addChild(playersObj);
            std::cout << "Player created" << std::endl;
        }*/
        updatePlayer(self, data);
    }));
    
    current_socket->on("player", sio::socket::event_listener_aux([&](std::string const& user, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
        /*int tag = (int)data->get_map()["tag"]->get_int();
        double x = data->get_map()["x"]->get_double();
        double y = data->get_map()["y"]->get_double();
        double rot = data->get_map()["rotation"]->get_double();
        auto objsLayer = reinterpret_cast<CCLayer*>(self->getChildren()->objectAtIndex(3));
        auto player = objsLayer->getChildByTag(tag);
        if (player != NULL) {
            player->setPosition(ccp(x, y));
            player->setRotation((float)rot);
        } else {
            // create the player
            CCNode* playersObj = CCNode::create();
            playersObj->setTag(tag);
            CCSprite* player = gd::SimplePlayer::create(1);
            playersObj->addChild(player);
            objsLayer->addChild(playersObj);
            std::cout << "Player Joined" << std::endl;
        }*/
        updatePlayer(self, data);
    }));
    return true;
}

DWORD WINAPI start_socket(void* self) {
    std::cout << "Starting socket..." << std::endl;
    sio::client sock;
    sock.set_reconnect_delay(reconnectionDelay);
    sock.set_reconnect_delay_max(reconnectionDelayMax);
    sock.set_reconnect_attempts(reconnectionAttempts);
    sock.set_open_listener(&ConnectionHandler::onSuccess);
    sock.set_close_listener(&ConnectionHandler::onClose);
    sock.set_fail_listener(&ConnectionHandler::onFail);
    sock.connect("wss://127.0.0.1:8000/socket.io/");
    if (!connect_finish) {
        cond.wait(unique_lock);
    }
    std::cout << "started socket" << std::endl;
    sock.socket()->on_error(ConnectionHandler::onError);
    std::cout << "set socket" << std::endl;
    PlayLayer::setSocket((gd::PlayLayer*)self, sock.socket());
    while (true) {
        Sleep(1000); // this code is for some reason the reason why the socket client still runs, even though it looks very wrong
    }
    return true;
}

// Nothing special here
bool __fastcall PlayLayer::hook(gd::PlayLayer* self, int, gd::GJGameLevel* level) {
    bool result = init(self, level);
    connect_finish = false;
    current_socket = sio::socket::ptr();
    current_level_id = level->levelID;
    auto director = cocos2d::CCDirector::sharedDirector();
    auto size = director->getWinSize();
    // probably couldve also used pthread
    hThread = CreateThread(NULL, 0, start_socket, (void*)self, 0, NULL);
    return result;
}

// Emits player data (such as position, rotation, gamemode, scale, etc)
bool __fastcall PlayLayer::hookUpdate(gd::PlayLayer* self, float dt) {
    bool result = update(self, dt);
    if (!current_socket) return result;
    sio::message::ptr object = sio::object_message::create();
    object->get_map()["x"] = sio::double_message::create(self->m_pPlayer1->getPositionX());
    object->get_map()["y"] = sio::double_message::create(self->m_pPlayer1->getPositionY());
    object->get_map()["rotation"] = sio::double_message::create(self->m_pPlayer1->getRotationX());
    object->get_map()["gamemode"] = sio::int_message::create(playerTypeToModeID(self->m_pPlayer1));
    object->get_map()["scale"] = sio::double_message::create(self->m_pPlayer1->getScaleX());
    current_socket->emit("update", object);
    return result;
}

// Terminates the current socket connection and closes the thread.
bool __fastcall PlayLayer::hookExit(gd::PlayLayer* self) {
    bool result = exit(self);
    // Stop the hThread
    if (hThread) {
        if (current_socket) {
            current_socket->emit("disconnect");
            current_socket->close();
        }
        current_level_id = NULL;
        current_socket = sio::socket::ptr();
        connect_finish = false;
        TerminateThread(hThread, 0);
        CloseHandle(hThread);
        hThread = NULL;
        std::cout << "Thread closed" << std::endl;
    }
    return result;
}