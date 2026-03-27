#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H
#include "player.h"
#include <QObject>
#include "CollisionResult.h"
class PlayerController : public QObject
{
    Q_OBJECT
public:
    PlayerController();
    void bindPlayer(const std::shared_ptr<Player>& p) {
        player = p; // ✅ 不增加引用计数
    }

    bool isOrphaned() const {
        return player.lock() == nullptr; // ✅ 玩家已被销毁
    }
    void handleIntent(MoveIntent moveIntent, bool attackIntent);
    void receiveHit(float damage, Player::WeaponType weaponType, QString direction);
    QRect getHitbox();
    bool canAttack() const { return player.lock()->state.shootState; }
    void triggerAttackCooldown(float time=1) { cooldowns["attack"] = time; }
    int getId() { return player.lock()->id; }
    Player::WeaponType getWeaponType();
    float getAttackDamage() const {
        return player.lock()->getAttackDamage(); // 纯转发，没加任何逻辑
    }
    float getDefenseMultiplier(Player::WeaponType weaponType) const{
        return player.lock()->getDefenseMultiplier(weaponType);
    }
    bool isInHurt() const { return cooldowns.value("hurt", 0) > 0; }
    bool  isFrozen()            const { return player.lock()->spellState.isFrozen; }
    float getFreezeDmgMultiplier() const {
        return player.lock()->modifiers.freezeDmgMultiplier;
    }

signals:
    void requestThrowBall(float x, float y, float vx, float vy, Player::WeaponType weapon, float damage, int id); // 🟩 投掷请求（不需要传目标）
    void frozenBroken(int victimId);  // 定身被打破，通知Widget处理CDR词条
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
