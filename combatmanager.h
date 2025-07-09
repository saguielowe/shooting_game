#ifndef COMBATMANAGER_H
#define COMBATMANAGER_H
#include "playercontroller.h"

class CombatManager
{
public:
    CombatManager();
    void checkPlayerVsPlayerCollision(PlayerController *p1, PlayerController *p2);
};

#endif // COMBATMANAGER_H
