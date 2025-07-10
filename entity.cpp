#include "entity.h"

Entity::Entity(QPointF pos, int layer): position(pos), layer(layer) {}

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
