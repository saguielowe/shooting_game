#ifndef BALL_H
#define BALL_H
#include "entity.h"
class Ball : public Entity
{
public:
    Ball(QPointF pos, QPointF v, float initDamage, int id);
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
};

#endif // BALL_H
