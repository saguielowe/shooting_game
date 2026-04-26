#ifndef BULLET_H
#define BULLET_H
#include "entity.h"
class Bullet : public Entity
{
public:
    Bullet(QPointF pos, float vx, int type, float initDamage, float frozenBonus, int id); // type=1：rifle，type=2：sniper
    void update() override;
    void draw(QPainter &painter) override;
    void onCollideWithPlayer() override;
    float getDamage(int id) override;
    void reflectByIronBody(float speedScale, float damageScale, int newParentId);
    QRect hitbox() override;
    bool shouldBeRemoved() override;
    virtual Player::WeaponType getWeaponType() override {
        if (type == 1) return Player::WeaponType::rifle;
        else if (type == 2) return Player::WeaponType::sniper;
        else return Player::WeaponType::punch; // 默认返回拳击，虽然不太合理但总得有个默认值
    }

private:
    QPixmap pixmap;
    bool toBeRemoved = false;
    float lifetime = 0;
    int type;
    float basicDamage; // 攻击者的伤害倍率
};

#endif // BULLET_H
