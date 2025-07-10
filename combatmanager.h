#ifndef COMBATMANAGER_H
#define COMBATMANAGER_H
#include "playercontroller.h"
#include "entity.h"

class CombatManager
{
public:
    CombatManager();
    void checkPlayerVsPlayerCollision(PlayerController *p1, PlayerController *p2);
    void checkEntityVsPlayerCollision(Entity *e, PlayerController *p);
};

#endif // COMBATMANAGER_H
