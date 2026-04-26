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
    void clearAttackState() { player.lock()->state.shootState = false; }
    void triggerAttackCooldown(float time=1) { cooldowns["attack"] = time; }
    int getId() { return player.lock()->id; }
    Player::WeaponType getWeaponType();
    float getAttackDamage() const {
        return player.lock()->getAttackDamage(); // 纯转发，没加任何逻辑
    }
    float getDefenseMultiplier(Player::WeaponType weaponType) const{
        return player.lock()->getDefenseMultiplier(weaponType);
    }
    void forceHurt(float stunTime, const QString& direction);
    void consumeIronBodyAndStun(const QString& direction, float stunTime = 1.0f);
    void consumeIronBodyWithNormalHurt(float damageAfterDefense, const QString& direction);
    void reduceSpellCooldown(float sec);
    bool hasUsedIronBodyReflectCdr() const;
    void markIronBodyReflectCdrUsed();
    void takeReflectDamage(float damage);
    bool isInHurt() const { return cooldowns.value("hurt", 0) > 0; }
    bool  isFrozen()            const { return player.lock()->spellState.isFrozen; }
    float getFreezeDmgMultiplier() const {
        return player.lock()->modifiers.freezeDmgMultiplier;
    }
    bool  isStealthed() const {
        return player.lock()->spellState.stealthActive;
    }
    const Player::ModifierState& getModifiers() const {
        return player.lock()->modifiers;
    }
    const SpellState& getSpellState() const {
        return player.lock()->spellState;
    }
    void triggerStealthFirstHit() {
        auto p = player.lock();
        p->spellState.stealthFirstHitUsed = true;
        p->spellState.stealthActive       = false;
        p->spellState.stealthRemain       = 0.f;
    }

signals:
    void requestThrowBall(float x, float y, float vx, float vy, Player::WeaponType weapon, float damage, float frozenBonus, int id); // 🟩 投掷请求（不需要传目标）
    void frozenBroken(int victimId);  // 定身被打破，通知Widget处理CDR词条
private:
    std::weak_ptr<Player> player;
    QMap <QString, float> cooldowns;
    float hurtPeakDamage = 0.f; // 当前硬直链中已记录的最大单次伤害（护甲后）
    void applyMapCollision();
    std::shared_ptr<Player> getPlayer() {
        return player.lock();
    }
    void applyEntityCollision();
};

#endif // PLAYERCONTROLLER_H
