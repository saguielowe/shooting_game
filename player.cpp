#include "player.h"
#include <QDebug>
Player::Player(float x, float y) : x(x), y(y) {
    qDebug()<<"loading animation assets……";
    animation.load("idle", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Idle.png", 120, 10);
    animation.load("run", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Run.png", 120, 10);
    animation.load("jump", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Jump.png", 120, 3);
    animation.load("crouch", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Crouch.png", 120, 1);
    animation.load("fall", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Fall.png", 120, 3);
    animation.load("hurt", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_SlideFull.png", 120, 4);
    animation.load("knife", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Attack2NoMovement.png", 120, 6);

    animation.play("idle");
}
void Player::setDt(float t){
    dt = t;
}

void Player::applyGravity(){
    if (!onGround){
        vy += gravity * dt;
    }
}

void Player::crouch(){
    vx = 0;
}

void Player::stop(){
    const float resistanceUsed = onGround == true ? resistance : airResistance;
    if (vx > 0){
        vx = fmax(0, vx - resistanceUsed);
    }
    else if (vx < 0){
        vx = fmin(0, vx + resistanceUsed);
    }
}

void Player::moveLeft(){
    if (vx > 0){ // 如果人物在向右，给它快速拉回左。
        vx -= 4 * acceleration;
    }
    else{
        vx = fmax(-moveSpeed, vx - acceleration); // 往左不能快于moveSpeed
    }
}

void Player::moveRight(){
    if (vx < 0){
        vx += 4 * acceleration;
    }
    else{
        vx = fmin(moveSpeed, vx + acceleration);
    }
}

void Player::update(){ // player更新自身位置，animation更新自身动画
    if (hpRegenerate){
        if (regenerateTime > 0){
            regenerateTime -= dt;
            hp = fmin(100, hp + 3 * dt);
        }
        else{
            hpRegenerate = false;
            regenerateTime = 0;
        }
    }
    bool loop = true;
    if (state.moveState == "hurt"){
        animation.play("hurt");
        loop = false;
    }
    if (state.shootState){
        if (weapon == WeaponType::knife){
            animation.play("knife", true);
        }
    }
    if (state.moveState == "stop" && vx == 0){
        state.moveState = "null"; // 如果减速至0则变为静止
    }
    if (state.moveState == "jump" && vy > 0 && onGround == false){
        state.moveState = "fall";
    }

    if (state.moveState == "jump" && onGround){ // 在地面才给推一把
        vy = - jumpSpeed;
        animation.play("jump");
    }
    if (state.moveState == "fall"){
        animation.play("fall");
    }
    if (state.moveState == "crouch"){
        crouch();
        animation.play("crouch");
    }
    if (state.moveState == "moving_left"){
        moveLeft();
        animation.play("run");
    }
    if (state.moveState == "moving_right"){ // 这里的right和实际运动方向无关，只是加速度方向
        moveRight();
        animation.play("run");
    }
    if (state.moveState == "stop"){
        stop();
        animation.play("run");
    }
    if (state.moveState == "null"){
        animation.play("idle");
    }
    y += vy * dt;
    x += vx * dt;

    animation.update(loop);
    hitbox = animation.getTargetRect();
}

void Player::draw(QPainter& painter) { // 传窗口painter的引用
    if (vx >= 0){
        animation.draw(painter, x, y);
    }
    else{
        animation.draw(painter, x, y, true); // 播放动画
    }

    QRect backgroundRect(hitbox.left(), hitbox.top() - 20, 100, 15);
    painter.setBrush(Qt::gray);
    painter.drawRect(backgroundRect);

    // 绘制红色血条
    int barWidth = static_cast<int>(100 * (fmax(0, hp) / maxHp));
    QRect healthRect(hitbox.left(), hitbox.top() - 20, barWidth, 15);
    painter.setBrush(Qt::red);
    painter.drawRect(healthRect);
    painter.drawText(healthRect, QString::number(int(hp)));
}

void Player::weaponControll(QString type){
    if (type == "knife"){
        weapon = WeaponType::knife;
    }
    else if (type == "ball"){
        weapon = WeaponType::ball;
        ballCount = maxBalls;
    }
    else if (type == "bandage"){
        hp = fmin(hp + 30, 100);
    }
    else if (type == "rifle"){
        weapon = WeaponType::rifle;
        bulletCount = maxBullets;
    }
    else if (type == "medkit"){
        hp = 100;
    }
    else if (type == "sniper"){
        weapon = WeaponType::sniper;
    }
    else if (type == "adrenaline"){
        velocityratio = 1.5;
        hpRegenerate = true;
        regenerateTime = 20;
    }
}

bool Player::isDead() const{
    return hp<=0 ;
}
