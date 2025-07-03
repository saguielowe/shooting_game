#ifndef MAP_H
#define MAP_H
#include <QVector>
#include <QRect>
#include <QPainter>
#include "CollisionResult.h"
class Map
{
public:
    // 获取单例实例的全局访问点
    static Map& getInstance();

    // 根据场景名称加载地图数据
    void loadScene(const QString& sceneName, QPainter& painter);

    // 获取碰撞箱数据供其他类使用
    const QVector<QRect>& getCollisionBoxes() const;

    // 假设提供一个方法来检查碰撞
    CollisionResult checkCollision(const QRect& playerRect, const float playerVx, const float playerVy);
private:
    QVector <QRect> collisionBox;
    QString currentSceneName;
    QPixmap map;
    Map();
};

#endif // MAP_H
