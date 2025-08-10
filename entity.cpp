#include "entity.h"
#include <QDebug>

Entity::Entity(QPointF pos, int id): position(pos), parentId(id){
    qDebug() << "【entity】："<<id;
}

void Entity::setDt(float t){
    dt = t;
}

void Entity::applyGravity(){
    if (!onGround){
        velocity.setY(velocity.y() + gravity * dt);
    }
}

void Entity::stop(){
    const float resistanceUsed = onGround == true ? resistance : airResistance;
    if (velocity.x() > 0){
        velocity.setX(fmax(0, velocity.x() - resistanceUsed));
    }
    else if (velocity.x() < 0){
        velocity.setX(fmin(0, velocity.x() + resistanceUsed));
    }
}
