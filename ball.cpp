#include "ball.h"
#include "map.h"
#include "CollisionResult.h"
#include "player.h"
#include "soundmanager.h"
#include <QPixmap>
#include <QDebug>
Ball::Ball(QPointF pos, QPointF v, float initDamage, int id) : Entity(pos, id) {
    //qDebug() <<"【ball】："<<id;
    velocity = v;
    lifetime = 0;
    basicDamage = initDamage;
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
    if (lifetime >= 1) basicDamage /= 1.5; // 多次撞击伤害减小，前提是真的打到别人了
}

float Ball::getDamage(int id){
    //qDebug()<<"受伤者: "<<id;
    if (parentId == id){
        return 0; // 投掷者在投掷后短时间内豁免撞击
    }
    float maxDamage = 50.f;  // 上限25%血
    int kineticEnergy = basicDamage * velocity.x() * velocity.x() + velocity.y() * velocity.y(); // 基础攻击力作用于动能，不至于影响过大
    float x = (kineticEnergy - 1500000.f) / 300000.f;
    float sig = 1.0f / (1.0f + expf(-x)); // 用sigmoid函数将伤害根据动能平滑映射到0-maxDamage范围，动能在90万到150万之间时伤害增长较快，超过150万后趋近于maxDamage
    
    // 微调：偏移0.047，系数2.2
    float damage = maxDamage * 2.0f * sig ;
    qDebug() << "基础伤害："<<basicDamage <<" 动能："<< kineticEnergy;
    return qMax(0.f, damage);
    
    // 这里只计算削减前的伤害，削减在combat manager里根据玩家状态进行
}

bool Ball::shouldBeRemoved(){
    if (lifetime >= 15) return true;
    return false;
}

QRect Ball::hitbox(){
    return QRect(position.x(), position.y(), pixmap.width(), pixmap.height());
}
