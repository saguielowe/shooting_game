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
        bool p1LeftOfP2 = p1->getHitbox().left() < p2->getHitbox().left();
        QString dirToP2 = p1LeftOfP2 ? "right" : "left";
        QString dirToP1 = p1LeftOfP2 ? "left" : "right";

        // p1 攻击命中 p2
        if (p1->canAttack()) {
            float dmg = p1->getAttackDamage() * p2->getDefenseMultiplier(p1->getWeaponType()); // 考虑 p2 的防御加成 和 p1 的攻击加成
            p2->receiveHit(dmg, p1->getWeaponType(), dirToP1);
        }
        // p2 攻击命中 p1
        if (p2->canAttack()) {
            float dmg = p2->getAttackDamage() * p1->getDefenseMultiplier(p2->getWeaponType());
            p1->receiveHit(dmg, p2->getWeaponType(), dirToP2);
        }
    }
}

void CombatManager::checkEntityVsPlayerCollision(Entity *e, PlayerController *p){
    if (e->hitbox().intersects(p->getHitbox())) {
        bool p1LeftOfP2 = e->getDir();
        QString dirToP2 = p1LeftOfP2 ? "left" : "right"; // 被撞后应该往哪飞
        e->onCollideWithPlayer(); // 多态自动决定子类行为
        float dmg = e->getDamage(p->getId());
        qDebug()<<"first cal:"<<dmg;
        p->receiveHit(dmg * p->getDefenseMultiplier(e->getWeaponType()), e->getWeaponType(), dirToP2); // 远程攻击考虑防御加成
    }
}
