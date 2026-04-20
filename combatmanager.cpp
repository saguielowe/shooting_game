#include "combatmanager.h"
#include "entity.h"
#include "ball.h"
#include "bullet.h"
#include "soundmanager.h"
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

static int weaponIndex(Player::WeaponType w) {
    switch (w) {
    case Player::WeaponType::punch:  return 0;
    case Player::WeaponType::knife:  return 1;
    case Player::WeaponType::ball:   return 2;
    case Player::WeaponType::rifle:  return 3;
    case Player::WeaponType::sniper: return 4;
    }
    return 0;
}

static int otherIndex() {
    return 5;
}
// 添加一些音效；铜头铁臂法术（定身可破）；无尽模式制作
CombatManager::CombatManager() {}

void CombatManager::recordDamage(int attackerId, Player::WeaponType weapon, float damage) {
    if (attackerId < 0 || attackerId > 1) return;
    if (damage <= 0.f) return;
    m_stats.byPlayerAndWeapon[attackerId][weaponIndex(weapon)] += damage;
}

void CombatManager::recordOtherDamage(int attackerId, float damage) {
    if (attackerId < 0 || attackerId > 1) return;
    if (damage <= 0.f) return;
    m_stats.byPlayerAndWeapon[attackerId][otherIndex()] += damage;
}

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
            if (p2->getSpellState().ironBodyActive &&
                (p1->getWeaponType() == Player::WeaponType::punch ||
                 p1->getWeaponType() == Player::WeaponType::knife)) {
                float meleeIncoming = p1->getAttackDamage();
                p1->clearAttackState();
                p1->forceHurt(1.0f, dirToP2);
                if (p2->getModifiers().ironBodyReflectCDR && !p2->hasUsedIronBodyReflectCdr()) {
                    p2->reduceSpellCooldown(5.f);
                    p2->markIronBodyReflectCdrUsed();
                }
                if (p2->getModifiers().ironBodyThorns) {
                    p1->takeReflectDamage(meleeIncoming * 0.5f);
                    recordOtherDamage(p2->getId(), meleeIncoming * 0.5f);
                }
                SoundManager::instance().play("tanfan", 0.7);
                qDebug().noquote() << QString("[铜头铁臂] 近战反制 P%1 被硬直")
                                          .arg(p1->getId() + 1);
            } else {
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

            if (p2->getSpellState().ironBodyActive && finalDmg >= 40.f) {
                p2->consumeIronBodyWithNormalHurt(finalDmg, dirToP1);
                qDebug().noquote() << QString("[铜头铁臂] 高伤免疫 P%1 免疫伤害并进入硬直")
                                          .arg(p2->getId() + 1);
            } else {
                p2->receiveHit(finalDmg, p1->getWeaponType(), dirToP1);
                recordDamage(p1->getId(), p1->getWeaponType(), finalDmg);
            }
            }
        }

        // p2 攻击命中 p1
        if (p2->canAttack()) {
            if (p1->getSpellState().ironBodyActive &&
                (p2->getWeaponType() == Player::WeaponType::punch ||
                 p2->getWeaponType() == Player::WeaponType::knife)) {
                float meleeIncoming = p2->getAttackDamage();
                p2->clearAttackState();
                p2->forceHurt(1.0f, dirToP1);
                if (p1->getModifiers().ironBodyReflectCDR && !p1->hasUsedIronBodyReflectCdr()) {
                    p1->reduceSpellCooldown(5.f);
                    p1->markIronBodyReflectCdrUsed();
                }
                if (p1->getModifiers().ironBodyThorns) {
                    p2->takeReflectDamage(meleeIncoming * 0.5f);
                    recordOtherDamage(p1->getId(), meleeIncoming * 0.5f);
                }
                SoundManager::instance().play("tanfan", 0.7);
                qDebug().noquote() << QString("[铜头铁臂] 近战反制 P%1 被硬直")
                                          .arg(p2->getId() + 1);
            } else {
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

            if (p1->getSpellState().ironBodyActive && finalDmg >= 40.f) {
                p1->consumeIronBodyWithNormalHurt(finalDmg, dirToP2);
                qDebug().noquote() << QString("[铜头铁臂] 高伤免疫 P%1 免疫伤害并进入硬直")
                                          .arg(p1->getId() + 1);
            } else {
                p1->receiveHit(finalDmg, p2->getWeaponType(), dirToP2);
                recordDamage(p2->getId(), p2->getWeaponType(), finalDmg);
            }
            }
        }
    }
}

void CombatManager::checkEntityVsPlayerCollision(Entity *e, PlayerController *p) {
    if (e->hitbox().intersects(p->getHitbox())) {
        bool leftOfP = e->getDir();
        QString dir  = leftOfP ? "left" : "right";
        const bool ironBodyOn = p->getSpellState().ironBodyActive;
        const auto weaponType = e->getWeaponType();
        const bool isSelfProjectile = (e->parentId == p->getId());

        e->onCollideWithPlayer();

        if (ironBodyOn && !isSelfProjectile && weaponType == Player::WeaponType::ball) {
            auto* ball = dynamic_cast<Ball*>(e);
            if (ball != nullptr) {
                ball->reflectByIronBody(1.0f, p->getId());
                if (p->getModifiers().ironBodyReflectCDR && !p->hasUsedIronBodyReflectCdr()) {
                    p->reduceSpellCooldown(5.f);
                    p->markIronBodyReflectCdrUsed();
                }
                SoundManager::instance().play("tanfan", 0.7);
                qDebug().noquote() << QString("[铜头铁臂] P%1 反弹实心球")
                                          .arg(p->getId() + 1);
                return;
            }
        }

        if (ironBodyOn && !isSelfProjectile &&
            (weaponType == Player::WeaponType::rifle || weaponType == Player::WeaponType::sniper)) {
            auto* bullet = dynamic_cast<Bullet*>(e);
            if (bullet != nullptr) {
                float rawDmg = e->getDamage(p->getId());
                if (p->isFrozen()) {
                    rawDmg *= e->frozenBonus;
                }
                float defMult = p->getDefenseMultiplier(e->getWeaponType());
                float incomingFinalDmg = rawDmg * defMult;

                bullet->reflectByIronBody(0.5f, 0.5f, p->getId());
                if (p->getModifiers().ironBodyReflectCDR && !p->hasUsedIronBodyReflectCdr()) {
                    p->reduceSpellCooldown(5.f);
                    p->markIronBodyReflectCdrUsed();
                }
                SoundManager::instance().play("tanfan", 0.7);
                if (incomingFinalDmg >= 40.f) {
                    p->consumeIronBodyWithNormalHurt(incomingFinalDmg, dir);
                    qDebug().noquote() << QString("[铜头铁臂] P%1 反弹高伤子弹并消耗铜头")
                                              .arg(p->getId() + 1);
                } else {
                    qDebug().noquote() << QString("[铜头铁臂] P%1 反弹低伤子弹，铜头保留")
                                              .arg(p->getId() + 1);
                }
                return;
            }
        }

        float rawDmg   = e->getDamage(p->getId());
        if (p->isFrozen()) {
            qDebug()<<"using frozenbonus";
            rawDmg *= e->frozenBonus; // 如果玩家被定身，先乘以实体的定身伤害加成
        }
        float defMult  = p->getDefenseMultiplier(e->getWeaponType());
        float finalDmg = rawDmg * defMult;

        if (rawDmg >= 1.f) {
            qDebug().noquote() << QString("[伤害] %1 → P%2 | 来源:%3 | 原始:%4 | 防御x%5 | 最终:%6")
                                      .arg(weaponName(e->getWeaponType()))
                                      .arg(p->getId() + 1)
                                      .arg(weaponName(e->getWeaponType()))
                                      .arg(rawDmg,   0, 'f', 1)
                                      .arg(defMult,  0, 'f', 2)
                                      .arg(finalDmg, 0, 'f', 1);
        }
        if (ironBodyOn && finalDmg >= 40.f) {
            p->consumeIronBodyWithNormalHurt(finalDmg, dir);
            qDebug().noquote() << QString("[铜头铁臂] 高伤免疫 P%1 免疫伤害并进入硬直")
                                      .arg(p->getId() + 1);
            return;
        }

        p->receiveHit(finalDmg, e->getWeaponType(), dir);
        int attackerId = e->parentId;
        recordDamage(attackerId, e->getWeaponType(), finalDmg);
    }
}
