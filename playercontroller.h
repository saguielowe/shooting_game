#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H
#include "player.h"
#include "CollisionResult.h"
class PlayerController
{
public:
    PlayerController();
    void bindPlayer(const std::shared_ptr<Player>& p) {
        player = p; // ✅ 不增加引用计数
    }

    bool isOrphaned() const {
        return player.lock() == nullptr; // ✅ 玩家已被销毁
    }
    void handleIntent(QString moveIntent, bool attackIntent);
    void receiveHit(float damage, QString direction);
    QRect getHitbox();
    bool canAttack() const { return player.lock()->state.shootState; }
    void triggerAttackCooldown() { cooldowns["attack"] = 1; }
    Player::WeaponType getWeaponType();

private:
    std::weak_ptr<Player> player;
    QMap <QString, float> cooldowns;
    void applyMapCollision();
    std::shared_ptr<Player> getPlayer() {
        return player.lock();
    }
    void applyEntityCollision();
};

#endif // PLAYERCONTROLLER_H
