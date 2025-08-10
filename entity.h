#ifndef ENTITY_H
#define ENTITY_H
#include<QPainter>
#include<QRectF>

class Entity
{
public:
    Entity(QPointF pos, int id=0); // 所有实体初始时都有位置
    virtual void update() = 0; // 每帧更新位置，人物作为实体的更新需要结合动画，故需要重载
    virtual void onCollideWithPlayer(){};
    virtual float getDamage(int id){return 0;};
    virtual QRect hitbox(){};
    bool getDir(){ return (velocity.x() > 0);}; // 右true，左false，用于撞击方向判断
    QPointF getPos(){ return position; };
    void stop(); // 施加阻力
    void applyGravity(); // 施加重力
    void setDt(float t); // 设置实时帧率

    bool onGround = false; // 是否在地面（与地面碰撞不反弹）
    float dt;
    int layer; // 子弹会被实心球阻挡，layer越低越硬
    int parentId; // 发射者id

    virtual void draw(QPainter& painter) = 0; // 实体的绘制函数（简单绘制，不考虑动画），人物的绘制要考虑方向，需要重载
    virtual bool shouldBeRemoved(){}; // 要求所有实体类成员都有何时销毁的机制

protected:
    float width = 40, height = 60; // 贴图大小
    QPointF position;
    QPointF velocity;

    const float gravity = 1500.0f; // 所有实体所受重力大小
    const float resistance = 20.0f; // 所有实体地面自然减速
    const float airResistance = 30.0f; // 所有实体空中自然减速
};

#endif // ENTITY_H
