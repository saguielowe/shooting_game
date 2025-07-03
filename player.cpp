#include "player.h"
#include <QDebug>
Player::Player(float x, float y) : x(x), y(y) {
    qDebug()<<"loading animation assets……";
    animation.load("idle", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Idle.png", 120, 10);
    animation.load("run", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Run.png", 120, 10);
    animation.load("jump", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Jump.png", 120, 3);
    animation.load("crouch", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Crouch.png", 120, 1);
    animation.load("fall", ":/assets/assets/FreeKnight_v1/Colour1/Outline/120x80_PNGSheets/_Fall.png", 120, 3);

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
    if (state.moveState == "stop" && vx == 0){
        state.moveState = "null"; // 如果减速至0则变为静止
    }
    if (state.moveState == "jump" && vy > 0 && onGround == false){
        state.moveState = "fall";
    }
    //qDebug() << "state" << state.moveState;
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

    animation.update();
    hitbox = animation.getTargetRect();
}

void Player::draw(QPainter& painter) { // 传窗口painter的引用
    if (vx >= 0){
        animation.draw(painter, x, y);
    }
    else{
        animation.draw(painter, x, y, true); // 播放动画
    }
}
