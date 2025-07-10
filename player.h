#ifndef PLAYER_H
#define PLAYER_H
#include<QPainter>
#include<QRectF>
#include "animation.h"
#include "map.h"

class Player
{
public:
    Player(float x, float y);
    void update();         // 每帧更新位置
    void jump();
    void crouch();
    void moveLeft();
    void moveRight();
    void stop();
    void applyGravity();
    void setDt(float t);
    void weaponControll(QString type);
    bool isDead() const;
    void modifyvelocity(float ratio){
        velocityratio = ratio;
    }

    bool isShooting = false;
    bool onGround = false;

    struct State{
        QString moveState;
        bool shootState;
    };
    State state; // 玩家待改变状态，注意区分惯性状态（动画绘制的是新状态，惯性是被位置使用的）

    float x, y, dt;
    float vx = 0, vy = 0;
    float hp = 100;
    int armor = 0, ballCount = 0, bulletCount = 0;
    bool hpRegenerate = false; // 回血状态
    float regenerateTime = 0;
    QRect hitbox;

    enum class WeaponType{
        punch, knife, ball, rifle, sniper
    };
    WeaponType weapon = WeaponType::punch;

    void draw(QPainter& painter);

private:
    Animation animation;
    float width = 40, height = 60;

    const float gravity = 1500.0f;
    const float moveSpeed = 400.0f;
    const float jumpSpeed = 800.0f;
    const float groundY = 400; // 顶=0
    const float resistance = 20.0f; // 不按按键时，左右移动速度的衰减
    const float airResistance = 30.0f; // 不按按键时，空中左右移动速度的衰减
    const float acceleration = 30.0f; // 左右移动时，速度的增加
    float velocityratio = 1;

    const float maxHp = 100;
    const int maxBalls = 3;
    const int maxBullets = 3;
};

#endif // PLAYER_H
