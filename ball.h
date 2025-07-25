#ifndef BALL_H
#define BALL_H
#include "entity.h"
class Ball : public Entity
{
public:
    Ball(QPointF pos, QPointF v);
    void update() override;
    void draw(QPainter &painter) override;
    void onCollideWithPlayer() override;
    float getDamage() override;
    QRect hitbox() override;
    bool shouldBeRemoved() override;
private:
    QPixmap pixmap;
    float lifetime, basicDamage=0.001;
};

#endif // BALL_H
