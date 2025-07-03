#include "map.h"
#include <QDebug>
#include <QPixmap>
Map::Map() {}

Map& Map::getInstance() {
    static Map instance; // 静态局部变量，C++11保证线程安全地只创建一次
    return instance;
}

void Map::loadScene(const QString& sceneName, QPainter& painter) {
    // qDebug() << "Loading scene: " << sceneName;
    currentSceneName = sceneName;
    map = QPixmap(":/Maps/assets/default_map.png");
    painter.drawPixmap(QRect(0, 0, map.width(), map.height()), map);
    collisionBox.clear(); // 清除旧的碰撞箱

    // 根据场景名称加载不同的碰撞箱数据
    if (sceneName == "default") {
        collisionBox << QRect(372, 668, 253, 178) << QRect(238, 248, 534, 44)
                     << QRect(592, 475, 490, 48) << QRect(0, 846, 1024, 56)
                     << QRect(0, 474, 425, 48) << QRect(0, 0, 20, 956)
                     << QRect(1004, 0, 20, 956);
        painter.drawRects(collisionBox);
    }

}

const QVector<QRect>& Map::getCollisionBoxes() const {
    return collisionBox;
}

CollisionResult Map::checkCollision(const QRect& playerHitbox, const float playerVx, const float playerVy) {
    const float tolerance = 1;
    CollisionResult result;
    for (const QRect& box : collisionBox) {
        // 首先进行粗略的 AABB 碰撞检测
        if (playerHitbox.intersects(box)) {

            // 如果发生相交，进一步判断碰撞方向
            QRect overlap = playerHitbox.intersected(box);
            // 1. 判断是否是垂直方向的主导碰撞 (地面或天花板)
            if (playerVy > 0) { // 玩家向下移动 (可能碰到地面)
                // 如果重叠的垂直高度远小于水平宽度，则更可能是垂直碰撞
                if (overlap.height() < overlap.width() + tolerance &&
                        (playerHitbox.bottom() - box.top() >= -tolerance && playerHitbox.bottom() <= box.bottom()))
                {
                    result.direction = "ground";
                    result.correctedPosition = box.top();
                    qDebug() << "Collision: Ground with obstacle at " << box;
                }
            } else if (playerVy < 0) { // 玩家向上移动 (可能碰到天花板)
                    // 如果重叠的垂直高度远小于水平宽度，则更可能是垂直碰撞
                    if (overlap.height() < overlap.width() + tolerance &&
                        (playerHitbox.top() - box.bottom() <= tolerance && playerHitbox.top() >= box.top()))
                    {
                        result.direction = "ceiling";
                        result.correctedPosition = box.bottom();
                        qDebug() << "Collision: Ceiling with obstacle at " << box;
                    }
                }

                // 2. 判断是否是水平方向的主导碰撞 (左右墙壁)
                if (playerVx > 0) { // 玩家向右移动 (可能碰到右墙)
                    // 如果重叠的水平宽度远小于垂直高度，则更可能是水平碰撞
                    if (overlap.width() < overlap.height() + tolerance &&
                        (playerHitbox.right() - box.left() >= -tolerance && playerHitbox.right() <= box.right()))
                    {
                        result.direction = "right";
                        result.correctedPosition = box.left();
                        qDebug() << "Collision: Right wall with obstacle at " << box;
                    }
                }
                if (playerVx < 0) { // 玩家向左移动 (可能碰到左墙)
                    // 如果重叠的水平宽度远小于垂直高度，则更可能是水平碰撞
                    if (overlap.width() < overlap.height() + tolerance &&
                        (playerHitbox.left() - box.right() <= tolerance && playerHitbox.left() >= box.left()))
                    {
                        result.direction = "left";
                        result.correctedPosition = box.right();
                        qDebug() << "Collision: Left wall with obstacle at " << box;
                    }
                }

                // 如果到达这里，说明发生了重叠，但可能无法明确判断是哪个主要方向的碰撞，
                // 或者同时发生了多方向碰撞。你可能需要更复杂的逻辑来处理这种情况。
                // 这里为了简洁，可以返回第一个检测到的主要方向，或者返回一个通用的"overlap"。
                // 例如，你可以选择在检测到第一个碰撞后就返回，这取决于你的游戏逻辑。
                // 为避免多重碰撞的歧义，更高级的系统会计算 MTV (Minimum Translation Vector)。
            }
        }
    return result;
}

