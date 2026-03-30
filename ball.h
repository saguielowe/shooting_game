#ifndef BALL_H
#define BALL_H
#include "entity.h"
class Ball : public Entity
{
public:
    Ball(QPointF pos, QPointF v, float initDamage, float frozenBonus, int id);
    void update() override;
    void draw(QPainter &painter) override;
    void onCollideWithPlayer() override;
    float getDamage(int id) override;
    Player::WeaponType getWeaponType() override { return Player::WeaponType::ball; }
    QRect hitbox() override;
    bool shouldBeRemoved() override;
private:
    QPixmap pixmap;
    float lifetime, basicDamage=0;
    QMap<int, float> hitCooldowns;
    float parentImmunityTime = 0.3f; // 施放者豁免时间（秒）
};

#endif // BALL_H
