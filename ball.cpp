#include "ball.h"
#include "map.h"
#include "CollisionResult.h"
#include "soundmanager.h"
#include <QPixmap>
#include <QDebug>
Ball::Ball(QPointF pos, QPointF v, int id) : Entity(pos, id) {
    qDebug() <<"【ball】："<<id;
    velocity = v;
    lifetime = 0;
    QString path = ":/items/assets/items/ball.png";
    pixmap = QPixmap(path).scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void Ball::draw(QPainter& painter) {
    painter.drawPixmap(position.x(), position.y(), pixmap);
    painter.setBrush(Qt::NoBrush);
    //painter.drawRect(position.x(), position.y(), pixmap.width(), pixmap.height());
}

void Ball::update(){
    dt = fmin(dt, 0.033f);
    applyGravity();
    //stop();
    QPointF newPoint = position + velocity * dt;
    QRect newHitbox = QRect(newPoint.x(), newPoint.y(), pixmap.width(), pixmap.height());
    CollisionResult collision_ret = Map::getInstance().checkCollision(newHitbox, velocity.x(), velocity.y());
    if (collision_ret.direction == "ground"){
        onGround = true;
        velocity.setY(- velocity.y() / 2);
        if (lifetime < 5) SoundManager::instance().play("crash", 0.2);
    }
    else{
        onGround = false;
    }

    if (collision_ret.direction == "ceiling") {
        velocity.setY(- velocity.y() / 2);
        SoundManager::instance().play("crash", 0.2);
    }

    if (collision_ret.direction == "right") {
        velocity.setX(- velocity.x() / 2);
        if (lifetime < 5) SoundManager::instance().play("crash", 0.2);
    }

    if (collision_ret.direction == "left") {
        velocity.setX(- velocity.x() / 2);
        if (lifetime < 5) SoundManager::instance().play("crash", 0.2);
    }

    position += velocity * dt;
    lifetime += dt;
}

void Ball::onCollideWithPlayer(){
    basicDamage /= 1.5; // 多次撞击伤害减小
}

float Ball::getDamage(int id){
    qDebug()<<"受伤者: "<<id;
    if (parentId == id){
        return 0; // 投掷者在投掷后短时间内豁免撞击
    }
    //qDebug() << "削减前伤害："<<basicDamage * (velocity.x() * velocity.x() + velocity.y() * velocity.y());
    return fmin(40, basicDamage * (velocity.x() * velocity.x() + velocity.y() * velocity.y())); // 实心球是破甲伤害，削弱其最大伤害
}

bool Ball::shouldBeRemoved(){
    if (lifetime >= 15) return true;
    return false;
}

QRect Ball::hitbox(){
    return QRect(position.x(), position.y(), pixmap.width(), pixmap.height());
}
