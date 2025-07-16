#ifndef PLAYER_H
#define PLAYER_H
#include<QPainter>
#include<QRectF>
#include "animation.h"
#include "map.h"

class Player
{
public:
    Player(float x, float y, int id); // id=1 左，id=2 右
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
    float hp = maxHp;
    int ballCount = 0, bulletCount = 0;
    bool hpRegenerate = false; // 回血状态
    bool direction = 1; // 1：右，0：左
    float regenerateTime = 0;
    const int id;
    QRect hitbox;

    enum class WeaponType{
        punch, knife, ball, rifle, sniper
    };
    enum class ArmorType{
        chainmail, vest, noArmor
    };

    WeaponType weapon = WeaponType::punch;
    ArmorType armor = ArmorType::noArmor;
    int vestHardness = 100;

    void draw(QPainter& painter); // 可以隐藏的场景如草地
    static void initSettings(bool canHide, float v, float maxHP, int balls){
        Player::canHide = canHide;
        velocityratio = v;
        maxHp = maxHP;
        maxBalls = balls;
    } // 调整游戏基础数值
    /* mode=0 正常模式
     * mode=1 投掷模式：实心球可投掷数量翻倍
     * mode=2 枪战模式：生命值提高至200，手枪可以射击5次
     */

private:
    Animation animation;
    float width = 40, height = 60;
    static bool canHide;

    const float gravity = 1500.0f;
    const float moveSpeed = 400.0f; // 最大移速
    const float jumpSpeed = 800.0f;
    const float resistance = 20.0f; // 不按按键时，左右移动速度的衰减
    const float airResistance = 30.0f; // 不按按键时，空中左右移动速度的衰减
    const float acceleration = 30.0f; // 左右移动时，速度的增加
    static float velocityratio; // 最大移速倍率，全局生效
    float selfvelocityratio = 1; // 受肾上腺素影响的倍率

    static float maxHp;
    static int maxBalls;
    const int maxBullets = 3;

};

#endif // PLAYER_H
