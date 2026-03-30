#include "combatmanager.h"
#include "entity.h"
#include <QDebug>
static QString weaponName(Player::WeaponType w) {
    switch (w) {
    case Player::WeaponType::punch:  return "拳击";
    case Player::WeaponType::knife:  return "小刀";
    case Player::WeaponType::ball:   return "实心球";
    case Player::WeaponType::rifle:  return "步枪";
    case Player::WeaponType::sniper: return "狙击枪";
    }
    return "未知";
}
// 血量加成需要更新逻辑；添加一些音效。
// 枪械远程定身伤害加成仍不确定有效，实心球多次判定需要改（硬直较短可多次触发）
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
            float baseDmg = p1->getAttackDamage();
            if (p2->isFrozen())  // 受击者在定身状态下的伤害加成，必须在计算最终伤害前处理
                baseDmg *= p1->getFreezeDmgMultiplier();
            float defMult  = p2->getDefenseMultiplier(p1->getWeaponType());
            float finalDmg = baseDmg * defMult;

            qDebug().noquote() << QString("[伤害] P%1(%2) → P%3 | 基础:%4 | 防御x%5 | 最终:%6")
                                      .arg(p1->getId() + 1)
                                      .arg(weaponName(p1->getWeaponType()))
                                      .arg(p2->getId() + 1)
                                      .arg(baseDmg,   0, 'f', 1)
                                      .arg(defMult,   0, 'f', 2)
                                      .arg(finalDmg,  0, 'f', 1);

            p2->receiveHit(finalDmg, p1->getWeaponType(), dirToP1);
        }

        // p2 攻击命中 p1
        if (p2->canAttack()) {
            float baseDmg  = p2->getAttackDamage();
            if (p1->isFrozen())
                baseDmg *= p2->getFreezeDmgMultiplier();
            float defMult  = p1->getDefenseMultiplier(p2->getWeaponType());
            float finalDmg = baseDmg * defMult;

            qDebug().noquote() << QString("[伤害] P%1(%2) → P%3 | 基础:%4 | 防御x%5 | 最终:%6")
                                      .arg(p2->getId() + 1)
                                      .arg(weaponName(p2->getWeaponType()))
                                      .arg(p1->getId() + 1)
                                      .arg(baseDmg,   0, 'f', 1)
                                      .arg(defMult,   0, 'f', 2)
                                      .arg(finalDmg,  0, 'f', 1);

            p1->receiveHit(finalDmg, p2->getWeaponType(), dirToP2);
        }
    }
}

void CombatManager::checkEntityVsPlayerCollision(Entity *e, PlayerController *p) {
    if (e->hitbox().intersects(p->getHitbox())) {
        bool leftOfP = e->getDir();
        QString dir  = leftOfP ? "left" : "right";

        e->onCollideWithPlayer();

        float rawDmg   = e->getDamage(p->getId());
        if (p->isFrozen()) {
            qDebug()<<"using frozenbonus";
            rawDmg *= e->frozenBonus; // 如果玩家被定身，先乘以实体的定身伤害加成
        }
        float defMult  = p->getDefenseMultiplier(e->getWeaponType());
        float finalDmg = rawDmg * defMult;

        qDebug().noquote() << QString("[伤害] %1 → P%2 | 来源:%3 | 原始:%4 | 防御x%5 | 最终:%6")
                                  .arg(weaponName(e->getWeaponType()))
                                  .arg(p->getId() + 1)
                                  .arg(weaponName(e->getWeaponType()))
                                  .arg(rawDmg,   0, 'f', 1)
                                  .arg(defMult,  0, 'f', 2)
                                  .arg(finalDmg, 0, 'f', 1);

        p->receiveHit(finalDmg, e->getWeaponType(), dir);
    }
}
