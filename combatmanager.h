#ifndef COMBATMANAGER_H
#define COMBATMANAGER_H
#include "playercontroller.h"
#include "entity.h"

class CombatManager
{
public:
    struct DamageStats {
        float byPlayerAndWeapon[2][5] = { {0} };
    };

    CombatManager();
    void checkPlayerVsPlayerCollision(PlayerController *p1, PlayerController *p2);
    void checkEntityVsPlayerCollision(Entity *e, PlayerController *p);
    void resetStats() { m_stats = DamageStats{}; }
    const DamageStats& stats() const { return m_stats; }

private:
    void recordDamage(int attackerId, Player::WeaponType weapon, float damage);
    DamageStats m_stats;
};

#endif // COMBATMANAGER_H
