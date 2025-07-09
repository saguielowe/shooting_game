#include "dropitem.h"
#include "player.h"
#include <QPainter>
#include <QDebug>
#include <QPixmap>
#include "CollisionResult.h"

DropItem::DropItem(float startX, const QString type)
    : position(startX, 0), itemType(type)
{
    velocity = QPointF(0, 0);

    QString path = ":/items/assets/items/" + type + ".png";
    pixmap = QPixmap(path).scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void DropItem::update(float dt) {
    if (!onGround){
        velocity.ry() += 1500 * dt; // 模拟重力
    }

    float newX = position.x() + velocity.x() * dt;
    float newY = position.y() + velocity.y() * dt;
    QRect newHitbox = QRect(newX, newY, hitbox().width(), hitbox().height());

    // 简易落地检测
    if (Map::getInstance().checkCollision(newHitbox, 0, velocity.y()).direction == "ground") {
        velocity.ry() = 0;
        onGround = true;
    }
    else{
        onGround = false;
    }

    lifetime -= dt;
    position += velocity * dt;
}

void DropItem::draw(QPainter& painter) {
    if (isNearExpire() && (lifetime / 10) % 2 == 0) return; // 闪烁

    painter.drawPixmap(position.x(), position.y(), pixmap);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(hitbox());
}

QRect DropItem::hitbox(){
    return QRect(position.x(), position.y(), pixmap.width(), pixmap.height());
}

bool DropItem::isExpired() const {
    return lifetime <= 0;
}

bool DropItem::isNearExpire() const {
    return lifetime <= flickerStart;
}

bool DropItem::isCollectedBy(Player* player){
    return player->hitbox.intersects(hitbox());
}

bool DropItem::isMarkedForDeletion() const {
    return toBeRemoved;
}

void DropItem::markForDeletion() {
    qDebug() <<"to be removed";
    toBeRemoved = true;
}

QString DropItem::getType() const {
    return itemType;
}

