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
    void receiveHit(float damage, QString direction);
    QRect getHitbox();
    bool canAttack() const { return player.lock()->state.shootState; }
    void triggerAttackCooldown(float time=1) { cooldowns["attack"] = time; }
    int getId() { return player.lock()->id; }
    Player::WeaponType getWeaponType();

signals:
    void requestThrowBall(float x, float y, float vx, float vy, Player::WeaponType weapon, int id); // 🟩 投掷请求（不需要传目标）
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
