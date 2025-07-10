#include "bullet.h"
#include "CollisionResult.h"
#include "map.h"
#include <QDebug>

Bullet::Bullet(QPointF pos, float vx, int type) : Entity(pos), type(type) {
    velocity = QPointF(vx, 0);
    QString path = ":/items/assets/items/bullet.png";
    pixmap = QPixmap(path).scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void Bullet::draw(QPainter& painter) {
    painter.drawPixmap(position.x(), position.y(), pixmap);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(position.x(), position.y(), pixmap.width(), pixmap.height());
}

void Bullet::update(){
    //applyGravity(); 重力对子弹来说太大，影响弹道
    stop();
    position += velocity * dt;
    lifetime += dt;
    if (position.x() > 2000 || position.x() < -100){
        toBeRemoved = true; // 无碰撞判断，飞跃即判移除
    }
}

void Bullet::onCollideWithPlayer(){
    if (lifetime <= 0.08) return;
    toBeRemoved = true;
}

float Bullet::getDamage(){
    if (lifetime <= 0.08) return 0;
    if (type == 1){
        return 30;
    }
    else if (type == 2){
        return 50;
    }
}

bool Bullet::shouldBeRemoved(){
    return toBeRemoved;
}

QRect Bullet::hitbox(){
    return QRect(position.x(), position.y(), pixmap.width(), pixmap.height());
}
