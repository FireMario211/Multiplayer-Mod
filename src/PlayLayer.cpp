// this code is bad, if you want to improve it, create a PR kthx
// also I just realized this could crash because there arent any checks so technically players could crash other players if they wanted to.
// I'll do those fixes later, or you could contribute to do those fixes but for now here's the code
#include "PlayLayer.h"

#include <queue>
std::queue<sio::message::ptr> dataQueue;
std::queue<sio::message::ptr> leftQueue;
std::queue<sio::message::ptr> emptyQueue; // probably a bad idea but idk
bool checkSocketEvents = false;

int reconnectionDelay = 1000;
int reconnectionDelayMax = 5000;
int reconnectionAttempts = 0;
int renderDist = 1000;

constexpr int PLAYERSID = 9998;
int myTag = 0;

// -------------------------------------------- //
bool connect_finish = false;
std::mutex lock;
std::unique_lock<std::mutex> unique_lock(lock);
std::condition_variable cond;
HANDLE hThread;
sio::socket::ptr current_socket;
int current_level_id = NULL;

// alk dont say anything about this ok, this is old code
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
    //object->get_map()["level"] = sio::string_message::create(std::to_string(current_level_id));
    current_socket->emit("join", std::to_string(current_level_id));
    return true;
}


// probably not a good idea but I have no other solution
int playerTypeToModeID(gd::PlayerObject* player) {
    // Gamemode IDs:
    // 0 = Cube
    // 1 = Ship
    // 2 = Ball
    // 3 = UFO (Bird)
    // 4 = Wave (Dart)
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

// probably a bad way of doing this but too lazy to impl PlayerObject
gd::SimplePlayer* renderDome(int cubeID, int ufoID) {
    auto domePlayer = gd::SimplePlayer::create(1);
    domePlayer->updatePlayerFrame(ufoID, gd::IconType::kIconTypeUfo);
    auto player = gd::SimplePlayer::create(cubeID);
    player->setScale(0.55F);
    player->setPositionY(5);
    domePlayer->addChild(player);
    return domePlayer;
}

// Removes a player if they left.
void removePlayer(gd::PlayLayer* self, sio::message::ptr const& data) {
    if (myTag == 0) return;
    auto objsLayer = static_cast<CCNode*>(self->getChildren()->objectAtIndex(3));
    if (objsLayer == nullptr) return;
    auto playersNode = objsLayer->getChildByTag(PLAYERSID);
    if (playersNode == nullptr) return;
    int tag = (int)data->get_map()["tag"]->get_int();
    if (tag == myTag) return; // no need to update based on that
    auto getPlayerObj = playersNode->getChildByTag(tag);
    if (getPlayerObj == nullptr) return;
    if (getPlayerObj != nullptr) {
        std::cout << "Removed Player " << tag << std::endl;
        playersNode->removeChildByTag(tag); // player has left, so we remove it to clean up.
    }
}

// Updates the other players positions
// TODO: Create "buffering" or client-side interpolation for the player positioning
// Possibly use PlayerObject with a scheduler running (or CCAction) to mimic as if it were another player
// This would "reduce latency" and make the movement look smoother.
void updatePlayer(gd::PlayLayer* self, sio::message::ptr const& data) {
    if (myTag == 0) return;
    auto objsLayer = static_cast<CCNode*>(self->getChildren()->objectAtIndex(3));
    if (objsLayer == nullptr) return;
    auto playersNode = objsLayer->getChildByTag(PLAYERSID);
    if (playersNode == nullptr) return;
    //auto objsLayer = self->m_pPlayer1->getParent()->getParent(); // more reliable ig
    int tag = (int)data->get_map()["tag"]->get_int();
    if (tag == myTag) return; // no need to update based on that
    double x = data->get_map()["x"]->get_double();
    double y = data->get_map()["y"]->get_double();
    double rot = data->get_map()["rotation"]->get_double();
    double scale = data->get_map()["scale"]->get_double();
    bool flipped = data->get_map()["flip"]->get_bool();
    int gamemodeID = data->get_map()["gamemode"]->get_int();

    int cubeIcon = data->get_map()["cube"]->get_int();
    int shipIcon = data->get_map()["ship"]->get_int();
    int ballIcon = data->get_map()["ball"]->get_int();
    int ufoIcon = data->get_map()["ufo"]->get_int();
    int waveIcon = data->get_map()["wave"]->get_int();
    int robotIcon = data->get_map()["robot"]->get_int();
    int spiderIcon = data->get_map()["spider"]->get_int();

    auto col1 = data->get_map()["col1"]->get_vector();
    auto col2 = data->get_map()["col2"]->get_vector();
    bool glow = data->get_map()["glow"]->get_bool();

    int iconFrame = 1;
    switch (gamemodeID) {
        case 0: // cube
            iconFrame = cubeIcon;
            break;
        case 1: // ship
            iconFrame = shipIcon;
            break;
        case 2: // ball
            iconFrame = ballIcon;
            break;
        case 3: // ufo
            iconFrame = ufoIcon;
            break;
        case 4: // wave
            iconFrame = waveIcon;
            break;
        case 5: // robot
            iconFrame = robotIcon;
            break;
        case 6: // spider
            iconFrame = spiderIcon;
            break;
    }

    auto getPlayerObj = playersNode->getChildByTag(tag);

    // TODO: do render distance
    //float pX = self->m_pPlayer1->getPositionX();
    //float pY = self->m_pPlayer2->getPositionY();
    if (getPlayerObj == nullptr && y <= -100) return;
    // Player exists, continue updating it.
    if (getPlayerObj != nullptr) {
        if (y <= -100) { // player has left
            playersNode->removeChildByTag(tag);
            return;
        }
        auto player = static_cast<gd::SimplePlayer*>(getPlayerObj->getChildren()->objectAtIndex(0));
        player->updatePlayerFrame(iconFrame, intToEnum(gamemodeID));
        getPlayerObj->setPosition(ccp(x, y));
        player->setRotation((float)rot);
        player->setScale((float)scale);
        if (flipped) { // setFlipY not working sadge
            player->setScaleY(-player->getScaleX());
        }

        if (!col1.empty() && col1.size() >= 3) {
            ccColor3B color1;
            color1.r = col1[0]->get_int();
            color1.g = col1[1]->get_int();
            color1.b = col1[2]->get_int();
            player->setColor(color1);
        }
        if (!col2.empty() && col2.size() >= 3) {
            ccColor3B color2;
            color2.r = col2[0]->get_int();
            color2.g = col2[1]->get_int();
            color2.b = col2[2]->get_int();
            player->setSecondColor(color2);
        }
        auto smallerPlayer = static_cast<gd::SimplePlayer*>(player->getChildren()->objectAtIndex(2));
        if (smallerPlayer != nullptr) {
            smallerPlayer->setVisible((gamemodeID == 1 || gamemodeID == 3));
            if (gamemodeID == 1) { // Ship
                smallerPlayer->setPositionY(10);
                player->setPositionY(-5);
            } else {
                smallerPlayer->setPositionY(6);
                player->setPositionY(0);
            }
        }
        
        if (gamemodeID == 5) { // Robot
            auto robot = player->getRobotSprite();
            robot->runAnimation("run");
            robot->updateTweenAction(0.2F, "run");
            player->setRobotSprite(robot); // this will probably crash LOL
        }
        if (gamemodeID == 6) { // Spider
            auto spider = player->getSpiderSprite();
            spider->runAnimation("run");
            spider->updateTweenAction(0.2F, "run");
            player->setSpiderSprite(spider);
        }
        player->setGlowOutline(glow);
    } else { // Player doesn't exist, create a new one.
        std::cout << "Creating new player." << std::endl;
        CCNode* playersObj = CCNode::create();
        playersObj->setTag(tag);
        gd::SimplePlayer* player = gd::SimplePlayer::create(iconFrame);
        auto userNameID = CCLabelBMFont::create(std::to_string(tag).c_str(), "bigFont.fnt");
        userNameID->setScale(0.5F);
        userNameID->setPositionY(20.0F);
        player->updatePlayerFrame(iconFrame, intToEnum(gamemodeID));

        auto smallerPlayer = gd::SimplePlayer::create(1);
        smallerPlayer->updatePlayerFrame(cubeIcon, gd::IconType::kIconTypeCube);
        smallerPlayer->setScale(0.55F);
        smallerPlayer->setPositionY(5);
        smallerPlayer->setVisible(false);
        player->addChild(smallerPlayer);

        if (!col1.empty() && col1.size() >= 3) {
            ccColor3B color1;
            color1.r = col1[0]->get_int();
            color1.g = col1[1]->get_int();
            color1.b = col1[2]->get_int();
            player->setColor(color1);
            smallerPlayer->setColor(color1);
        }
        if (!col2.empty() && col2.size() >= 3) {
            ccColor3B color2;
            color2.r = col2[0]->get_int();
            color2.g = col2[1]->get_int();
            color2.b = col2[2]->get_int();
            player->setSecondColor(color2);
            smallerPlayer->setSecondColor(color2);
        }
        player->setGlowOutline(glow);
        smallerPlayer->setGlowOutline(glow);
        playersObj->addChild(player);
        playersObj->setPosition(ccp(x, y));
        player->setRotationX((float)rot);
        player->setScale((float)scale);
        if (flipped) {
            player->setScaleY(-player->getScaleX());
        }
        playersObj->addChild(userNameID);
        
        playersNode->addChild(playersObj);
        // probably will switch to a switch statement later on
        // TODO: -6px for ship (do not change smallerPlayer)
        if (gamemodeID == 1 || gamemodeID == 3) { // Ship + UFO
            smallerPlayer->setVisible(true);
            if (gamemodeID == 1) { // Ship
                smallerPlayer->setPositionY(10);
                player->setPositionY(-5);
            }
        }
        if (gamemodeID == 5) { // Robot
            auto robot = player->getRobotSprite();
            robot->runAnimation("run");
            robot->updateTweenAction(0.2F, "run");
            player->setRobotSprite(robot); // this will probably crash LOL
        }
        if (gamemodeID == 6) { // Spider
            auto spider = player->getSpiderSprite();
            spider->runAnimation("run");
            spider->updateTweenAction(0.2F, "run");
            player->setSpiderSprite(spider);
        }
        std::cout << "Player created" << std::endl;
    }
}

bool PlayLayer::setSocket(gd::PlayLayer* self, sio::socket::ptr sock) {
    current_socket = sock;
    PlayLayer::join();
    std::cout << "listening for events" << std::endl;
    current_socket->on("update", sio::socket::event_listener_aux([&](std::string const& user, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
        if (!checkSocketEvents) return;
        dataQueue.push(data);
    }));
    current_socket->on("left", sio::socket::event_listener_aux([&](std::string const& user, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
        std::cout << "add to left queue" << std::endl;
        leftQueue.push(data);
    }));
    current_socket->on("tag", sio::socket::event_listener_aux([&](std::string const& user, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
        myTag = data->get_int();
        std::cout << "Updated Current Tag to " << myTag << std::endl;
    }));
    /*current_socket->on("player", sio::socket::event_listener_aux([&](std::string const& user, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp) {
        dataQueue.push(data);
    }));*/
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
    std::cout << "Started socket" << std::endl;
    sock.socket()->on_error(ConnectionHandler::onError);
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
    checkSocketEvents = true;
    current_socket = sio::socket::ptr();
    current_level_id = level->m_nLevelID;
    auto director = cocos2d::CCDirector::sharedDirector();
    auto size = director->getWinSize();

    auto playersNode = CCNode::create();
    playersNode->setTag(PLAYERSID); // stores all players
    playersNode->setPosition({0, 0});
    auto objsLayer = static_cast<CCNode*>(self->getChildren()->objectAtIndex(3));
    objsLayer->addChild(playersNode);

    hThread = CreateThread(NULL, 0, start_socket, (void*)self, 0, NULL);
    return result;
}

void processEvent(gd::PlayLayer* self) {
    if (!leftQueue.empty()) {
        std::cout << "step 1" << std::endl;
        auto data = leftQueue.front();
        leftQueue.pop();
        removePlayer(self, data);
    }
    if (!dataQueue.empty()) {
        auto data = dataQueue.front();
        dataQueue.pop();
        updatePlayer(self, data);
    }
}

// Emits player data (such as position, rotation, gamemode, scale, etc)
bool __fastcall PlayLayer::hookUpdate(gd::PlayLayer* self, float dt) {
    bool ret = update(self, dt);
    if (!current_socket) return ret;
    auto objsLayer = static_cast<CCNode*>(self->getChildren()->objectAtIndex(3));
    if (objsLayer != nullptr) { // so many nullptr checks
        auto getPlayersNode = objsLayer->getChildByTag(PLAYERSID);
        if (getPlayersNode != nullptr) {
            // the reason why im doing this is because robs "effects" mess up the whole players
            getPlayersNode->setPosition({0, 0});
            getPlayersNode->setRotation(0);
            getPlayersNode->setScale(1.0F);
            getPlayersNode->setSkewX(0);
            getPlayersNode->setSkewY(0);
        }
    }
    if (!checkSocketEvents) { // This fixes the issue where if you unpause, but another player is paused, you wont be able to see their "latest position"
        std::cout << "Getting all other players positions" << std::endl;
        current_socket->emit("receive");
    }
    checkSocketEvents = true;
    sio::message::ptr object = sio::object_message::create();

    // Position, Rotation, Scale
    object->get_map()["x"] = sio::double_message::create(self->m_pPlayer1->getPositionX());
    object->get_map()["y"] = sio::double_message::create(self->m_pPlayer1->getPositionY());
    object->get_map()["rotation"] = sio::double_message::create(self->m_pPlayer1->getRotationX());
    object->get_map()["gamemode"] = sio::int_message::create(playerTypeToModeID(self->m_pPlayer1));
    object->get_map()["scale"] = sio::double_message::create(self->m_pPlayer1->getScaleX());
    object->get_map()["flip"] = sio::bool_message::create((self->m_pPlayer1->getScaleY() < 0));

    // Gamemode Icons
    auto gm = gd::GameManager::sharedState();
    object->get_map()["cube"] = sio::int_message::create(gm->getPlayerFrame());
    object->get_map()["ship"] = sio::int_message::create(gm->getPlayerShip());
    object->get_map()["ball"] = sio::int_message::create(gm->getPlayerBall());
    object->get_map()["ufo"] = sio::int_message::create(gm->getPlayerBird());
    object->get_map()["wave"] = sio::int_message::create(gm->getPlayerDart());
    object->get_map()["robot"] = sio::int_message::create(gm->getPlayerRobot());
    object->get_map()["spider"] = sio::int_message::create(gm->getPlayerSpider());

    auto colorArray = [](const ccColor3B color) {
        auto array = sio::array_message::create();
        array->get_vector().push_back(sio::int_message::create(color.r));
        array->get_vector().push_back(sio::int_message::create(color.g));
        array->get_vector().push_back(sio::int_message::create(color.b));
        return array;
    };
    object->get_map()["col1"] = colorArray(self->m_pPlayer1->m_playerColor1);
    object->get_map()["col2"] = colorArray(self->m_pPlayer1->m_playerColor2);
    object->get_map()["glow"] = sio::bool_message::create(gm->getPlayerGlow());

    current_socket->emit("update", object);
    if (dataQueue.empty() && leftQueue.empty()) return ret; // Will not process any events if it's empty.
    processEvent(self);
    return ret;
}

// Terminates the current socket connection and closes the thread.
// Cleans up everything else.
bool __fastcall PlayLayer::hookExit(gd::PlayLayer* self) {
    bool result = exit(self);
    // Stop the hThread
    if (hThread) {
        if (current_socket) {
            current_socket->emit("disconnect");
            current_socket->close();
        }
        current_level_id = NULL;
        dataQueue.swap(emptyQueue); // Swaps it to an empty queue. This is probably not efficient.
        leftQueue.swap(emptyQueue); // Swaps it to an empty queue. This is probably not efficient.
        current_socket = sio::socket::ptr();
        connect_finish = false;
        checkSocketEvents = true;
        myTag = 0;
        TerminateThread(hThread, 0);
        CloseHandle(hThread);
        hThread = NULL;
        std::cout << "Thread closed" << std::endl;
    }
    return result;
}

// Pauses socket events from being added to the queue
bool PauseLayer::hook(CCLayer* self) {
    bool ret = init(self);
    std::cout << "Pausing any socket events." << std::endl;
    checkSocketEvents = false;
    return ret;
}
