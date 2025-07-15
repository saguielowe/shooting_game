#include "combatmanager.h"
#include "entity.h"
#include <QDebug>

CombatManager::CombatManager() {}

void CombatManager::checkPlayerVsPlayerCollision(PlayerController *p1, PlayerController *p2)
{
    if (p1->canAttack() == false && p2->canAttack() == false) return;
    // 获取碰撞箱
    QRectF hitbox1 = p1->getHitbox();
    QRectF hitbox2 = p2->getHitbox();

    // 检测是否碰撞
    if (hitbox1.intersects(hitbox2)) {
        // 缓存位置和方向判断
        bool p1LeftOfP2 = p1->getHitbox().right() < p2->getHitbox().left();
        QString dirToP2 = p1LeftOfP2 ? "right" : "left";
        QString dirToP1 = p1LeftOfP2 ? "left" : "right";

        // p1 攻击命中 p2
        if (p1->canAttack()) {
            int damage = (p1->getWeaponType() == Player::WeaponType::knife) ? 15 : 5;
            p2->receiveHit(damage, dirToP2);
        }

        // p2 攻击命中 p1
        if (p2->canAttack()) {
            int damage = (p2->getWeaponType() == Player::WeaponType::knife) ? 15 : 5;
            p1->receiveHit(damage, dirToP1);
        }
    }
}

void CombatManager::checkEntityVsPlayerCollision(Entity *e, PlayerController *p){
    if (e->hitbox().intersects(p->getHitbox())) {
        bool p1LeftOfP2 = e->getDir();
        QString dirToP2 = p1LeftOfP2 ? "left" : "right"; // 被撞后应该往哪飞
        e->onCollideWithPlayer(); // 多态自动决定子类行为
        p->receiveHit(e->getDamage(), dirToP2);
    }
}
