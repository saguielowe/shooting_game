#include "playercontroller.h"
#include <QDebug>
PlayerController::PlayerController(){
    cooldowns["attack"] = 0; // =0代表冷却已到，单位秒
    cooldowns["hurt"] = 0;
    cooldowns["left"] = 0;
    cooldowns["right"] = 0;
}

void PlayerController::applyMapCollision(){ // 判断与地图的碰撞
    float newX = player.lock()->x + player.lock()->vx * player.lock()->dt;
    float newY = player.lock()->y + player.lock()->vy * player.lock()->dt;
    QRect newHitbox = QRect(newX - player.lock()->hitbox.width() / 2.0, newY, player.lock()->hitbox.width(), player.lock()->hitbox.height());
    CollisionResult collision_ret = Map::getInstance().checkCollision(newHitbox, player.lock()->vx, player.lock()->vy);
    if (collision_ret.direction == "ground"){ // 如果竖直方向无速度不要修正坐标
        player.lock()->onGround = true;
        player.lock()->vy = 0;
        if (player.lock()->state.moveState == "fall")
            player.lock()->state.moveState = "stop";
    }
    else{
        player.lock()->onGround = false; // onGround判定交给controller，在player不再更改
    }

    if (collision_ret.direction == "ceiling") {
        player.lock()->vy = 0;
        player.lock()->state.moveState = "fall";
    }

    if (collision_ret.direction == "right") {
        cooldowns["right"] = 0.5;
        player.lock()->vx = 0;
        player.lock()->x = collision_ret.correctedPosition - player.lock()->hitbox.width() / 2.0;
    }

    if (collision_ret.direction == "left") {
        cooldowns["left"] = 0.5;
        player.lock()->vx = 200;
        player.lock()->x = collision_ret.correctedPosition + player.lock()->hitbox.width() / 2.0;
    }
}

void PlayerController::handleIntent(QString moveIntent, bool attackIntent){
    for (auto& key : cooldowns.keys()) {
        if (cooldowns[key] > 0)
            cooldowns[key] = fmax(0, cooldowns[key] - player.lock()->dt);
    }

    if (cooldowns["hurt"] != 0){
        player.lock()->applyGravity();
        applyMapCollision();
        player.lock()->update();
        return; // 受击状态不处理请求
    }

    if (cooldowns["attack"] == 0 && attackIntent){ // 无需判定是否击中，空挥也有动画
        player.lock()->state.shootState = true; // 当按下攻击键且不处于冷却，判定可以攻击，打不打到无所谓
        triggerAttackCooldown();
    }
    else{
        player.lock()->state.shootState = false;
    }

    if (moveIntent == "crouch" && player.lock()->onGround == true){
        player.lock()->state.moveState = "crouch"; // 不在地面忽略
    }


    if (moveIntent == "jump" && player.lock()->onGround){
        player.lock()->state.moveState = "jump"; // 用户在地面的jump意图会使得状态变为jump，直到自动变成fall
    }

    if (moveIntent == "moving_left" && cooldowns["left"] == 0){
        player.lock()->state.moveState = "moving_left";
    }
    else if (moveIntent == "moving_right" && cooldowns["right"] == 0){
        player.lock()->state.moveState = "moving_right";
    }

    if (moveIntent == "null" && player.lock()->onGround && player.lock()->vx != 0){ // 表明用户此时没有任何操作
        player.lock()->state.moveState = "stop"; // 如果是jump或fall状态放他去
    }
    if (moveIntent == "null" && player.lock()->onGround && player.lock()->vx == 0){ // 表明用户此时没有任何操作
        player.lock()->state.moveState = "null"; // 如果是jump或fall状态放他去
    }

    player.lock()->applyGravity();
    applyMapCollision();
    player.lock()->update();

}

void PlayerController::receiveHit(float damage, QString direction) {
    player.lock()->hp -= damage;
    cooldowns["hurt"] = 1;
    if (direction == "left"){
        player.lock()->vx = 60;
        player.lock()->vy = -400;
    }
    else{
        player.lock()->vx = -60;
        player.lock()->vy = -400;
    }
    qDebug() << cooldowns["hurt"] << "Player 受到伤害：" << player.lock()->hp;
    player.lock()->state.moveState = "hurt";
    player.lock()->update();
}

QRect PlayerController::getHitbox(){
    return player.lock()->hitbox;
}

Player::WeaponType PlayerController::getWeaponType(){
    return player.lock()->weapon;
}
