#ifndef BULLET_H
#define BULLET_H
#include "entity.h"
class Bullet : public Entity
{
public:
    Bullet(QPointF pos, float vx, int type); // type=1：rifle，type=2：sniper
    void update() override;
    void draw(QPainter &painter) override;
    void onCollideWithPlayer() override;
    float getDamage() override;
    QRect hitbox() override;
    bool shouldBeRemoved() override;

private:
    QPixmap pixmap;
    bool toBeRemoved = false;
    float lifetime = 0;
    int type;
};

#endif // BULLET_H
