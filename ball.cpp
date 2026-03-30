#include "ball.h"
#include "map.h"
#include "CollisionResult.h"
#include "player.h"
#include "soundmanager.h"
#include <QPixmap>
#include <QDebug>
Ball::Ball(QPointF pos, QPointF v, float initDamage, float frozenBonus, int id) : Entity(pos, id, frozenBonus) {
    qDebug() <<"【ball】："<<frozenBonus;
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

void Ball::update() {
    dt = fmin(dt, 0.033f);

    // 命中冷却倒计时
    for (auto& key : hitCooldowns.keys()) {
        hitCooldowns[key] = fmax(0.f, hitCooldowns[key] - dt);
    }
    parentImmunityTime = fmax(0.f, parentImmunityTime - dt);

    applyGravity();
    QPointF newPoint = position + velocity * dt;
    QRect newHitbox = QRect(newPoint.x(), newPoint.y(), pixmap.width(), pixmap.height());
    CollisionResult collision_ret = Map::getInstance().checkCollision(newHitbox, velocity.x(), velocity.y());

    bool bounced = false;
    if (collision_ret.direction == "ground") {
        onGround = true;
        velocity.setY(-velocity.y() / 2);
        bounced = true;
        if (lifetime < 5) SoundManager::instance().play("crash", 0.2);
    } else {
        onGround = false;
    }
    if (collision_ret.direction == "ceiling") {
        velocity.setY(-velocity.y() / 2);
        bounced = true;
        SoundManager::instance().play("crash", 0.2);
    }
    if (collision_ret.direction == "right") {
        velocity.setX(-velocity.x() / 2);
        bounced = true;
        if (lifetime < 5) SoundManager::instance().play("crash", 0.2);
    }
    if (collision_ret.direction == "left") {
        velocity.setX(-velocity.x() / 2);
        bounced = true;
        if (lifetime < 5) SoundManager::instance().play("crash", 0.2);
    }

    // 弹开后重置命中冷却（可以再次命中同一玩家）
    if (bounced) hitCooldowns.clear();

    position += velocity * dt;
    lifetime += dt;
}

float Ball::getDamage(int id) {
    // 施放者豁免（仅限初始短时间）
    if (parentId == id && parentImmunityTime > 0.f) return 0;

    // 命中冷却检查
    if (hitCooldowns.value(id, 0.f) > 0.f) return 0;

    // 设置命中冷却
    hitCooldowns[id] = 0.5f;

    float maxDamage = 50.f;
    int kineticEnergy = basicDamage * velocity.x() * velocity.x() + velocity.y() * velocity.y();
    float x = (kineticEnergy - 1500000.f) / 300000.f;
    float sig = 1.0f / (1.0f + expf(-x));
    float damage = maxDamage * 2.0f * sig;
    qDebug() << "基础伤害：" << basicDamage << " 动能：" << kineticEnergy;
    return qMax(0.f, damage);
}

void Ball::onCollideWithPlayer() {
    if (lifetime >= 1) basicDamage /= 1.5f;
}

bool Ball::shouldBeRemoved(){
    if (lifetime >= 15) return true;
    return false;
}

QRect Ball::hitbox(){
    return QRect(position.x(), position.y(), pixmap.width(), pixmap.height());
}
