#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H
#include "player.h"
#include "CollisionResult.h"
class PlayerController
{
public:
    PlayerController(Player* player);
    void handleIntent(QString moveIntent, bool attackIntent);

private:
    Player* player;
    QMap <QString, float> cooldowns;
    void applyCollision();
};

#endif // PLAYERCONTROLLER_H
