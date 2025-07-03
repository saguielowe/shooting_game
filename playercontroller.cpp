#include "playercontroller.h"
#include <QDebug>
PlayerController::PlayerController(Player* player)
    : player(player) {
    cooldowns["attack"] = 0; // =0代表冷却已到，单位毫秒
    cooldowns["hurt"] = 0;
    cooldowns["left"] = 0;
    cooldowns["right"] = 0;
}

void PlayerController::applyCollision(){ // 预测dt后坐标及速度
    float newX = player->x + player->vx * player->dt;
    float newY = player->y + player->vy * player->dt;
    QRect newHitbox = QRect(newX - player->hitbox.width() / 2.0, newY, player->hitbox.width(), player->hitbox.height());
    CollisionResult collision_ret = Map::getInstance().checkCollision(newHitbox, player->vx, player->vy);
    if (collision_ret.direction == "ground"){ // 如果竖直方向无速度不要修正坐标
        player->onGround = true;
        player->vy = 0;
        if (player->state.moveState == "fall")
            player->state.moveState = "stop";
    }
    else{
        player->onGround = false; // onGround判定交给controller，在player不再更改
    }

    if (collision_ret.direction == "ceiling") {
        player->vy = 0;
        player->state.moveState = "fall";
    }

    if (collision_ret.direction == "right") {
        cooldowns["right"] = 0.5;
        player->vx = 0;
        player->x = collision_ret.correctedPosition - player->hitbox.width() / 2.0;
    }

    if (collision_ret.direction == "left") {
        cooldowns["left"] = 0.5;
        player->vx = 200;
        player->x = collision_ret.correctedPosition + player->hitbox.width() / 2.0;
    }
}

void PlayerController::handleIntent(QString moveIntent, bool attackIntent){
    for (auto& key : cooldowns.keys()) {
        if (cooldowns[key] > 0)
            cooldowns[key] = fmin(0, cooldowns[key] - player->dt);
    }
    if (moveIntent != "null")
        qDebug() << "Intent" <<moveIntent;

    if (moveIntent == "crouch" && player->onGround == true){
        player->state.moveState = "crouch"; // 不在地面忽略
    }

    if (moveIntent == "jump" && player->onGround){
        player->state.moveState = "jump"; // 用户在地面的jump意图会使得状态变为jump，直到自动变成fall
    }

    if (moveIntent == "moving_left" && cooldowns["left"] == 0){
        player->state.moveState = "moving_left";
    }
    else if (moveIntent == "moving_right" && cooldowns["right"] == 0){
        player->state.moveState = "moving_right";
    }

    if (moveIntent == "null" && player->onGround && player->vx != 0){ // 表明用户此时没有任何操作
        player->state.moveState = "stop"; // 如果是jump或fall状态放他去
    }
    if (moveIntent == "null" && player->onGround && player->vx == 0){ // 表明用户此时没有任何操作
        player->state.moveState = "null"; // 如果是jump或fall状态放他去
    }

    if (attackIntent){
        player->state.shootState = true;
    }
    else{
        player->state.shootState = false;
    }
    player->applyGravity();
    applyCollision();
    player->update();

}
