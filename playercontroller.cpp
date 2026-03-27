#include "playercontroller.h"
#include "soundmanager.h"
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

void PlayerController::handleIntent(MoveIntent moveIntent, bool attackIntent){
    for (auto& key : cooldowns.keys()) {
        if (cooldowns[key] > 0)
            cooldowns[key] = fmax(0, cooldowns[key] - player.lock()->dt);
    }
    if (player.lock()->spellState.isFrozen) {
        return;
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
        if (player.lock()->weapon == Player::WeaponType::ball && player.lock()->ballCount > 0){
            emit requestThrowBall(player.lock()->x, player.lock()->y, player.lock()->vx, player.lock()->vy, Player::WeaponType::ball, getAttackDamage(), player.lock()->id);
            player.lock()->ballCount --;
            player.lock()->updateSlots();
        }
        else if (player.lock()->weapon == Player::WeaponType::rifle || player.lock()->weapon == Player::WeaponType::sniper){
            if (player.lock()->vx != 0){ // 此处步枪和狙击枪统一命令
                emit requestThrowBall(player.lock()->x, player.lock()->y, player.lock()->vx, player.lock()->vy, player.lock()->weapon, getAttackDamage(), player.lock()->id);
            }
            // 实心球伤害时高时低，有时40，有时只有个位数
            // 血量上限加减有问题
            // 准备引入法术系统
            else{
                float vvx = player.lock()->direction ? 1 : -1; // 自动根据人物在场景位置选择射击方向
                emit requestThrowBall(player.lock()->x, player.lock()->y, vvx, player.lock()->vy, player.lock()->weapon, getAttackDamage(), player.lock()->id);
            }
            if (player.lock()->weapon == Player::WeaponType::rifle){
                SoundManager::instance().play("rifle", 0.4);
                player.lock()->rifleCount --;
            }
            else if (player.lock()->weapon == Player::WeaponType::sniper){
                SoundManager::instance().play("sniper", 0.6);
                player.lock()->sniperCount --;
            }
            player.lock()->updateSlots();
        }
        else if (player.lock()->weapon == Player::WeaponType::knife){
            SoundManager::instance().play("knife", 0.4);
        }
    }
    else{
        player.lock()->state.shootState = false;
    }

    if (moveIntent == MoveIntent::CROUCH && player.lock()->onGround == true){
        player.lock()->state.moveState = "crouch"; // 不在地面忽略
    }

    if (moveIntent == MoveIntent::JUMP && player.lock()->onGround){
        player.lock()->state.moveState = "jump"; // 用户在地面的jump意图会使得状态变为jump，直到自动变成fall
    }

    if (moveIntent == MoveIntent::MOVING_LEFT && cooldowns["left"] == 0){
        player.lock()->state.moveState = "moving_left";
    }
    else if (moveIntent == MoveIntent::MOVING_RIGHT && cooldowns["right"] == 0){
        player.lock()->state.moveState = "moving_right";
    }

    if (moveIntent == MoveIntent::NONE && player.lock()->onGround && player.lock()->vx != 0){ // 表明用户此时没有任何操作
        player.lock()->state.moveState = "stop"; // 如果是jump或fall状态放他去
    }
    if (moveIntent == MoveIntent::NONE && player.lock()->onGround && player.lock()->vx == 0){ // 表明用户此时没有任何操作
        player.lock()->state.moveState = "null"; // 如果是jump或fall状态放他去
    }
    player.lock()->applyGravity();
    applyMapCollision();
    player.lock()->update();

}

void PlayerController::receiveHit(float damage, Player::WeaponType weaponType, QString direction) {

    // ── 定身状态受击 ─────────────────────────────────────────
    if (player.lock()->spellState.isFrozen) {
        if (damage < 1) return;

        // ── 护甲减伤（和普通受击一样处理）──────────────────
        if (player.lock()->armor == Player::ArmorType::chainmail) {
            if (weaponType == Player::WeaponType::punch) return; // 免疫拳击
            else if (weaponType == Player::WeaponType::knife) damage /= 2;
        }
        else if (player.lock()->armor == Player::ArmorType::vest
                 && player.lock()->vestHardness > 0) {
            if (weaponType == Player::WeaponType::rifle
                || weaponType == Player::WeaponType::sniper) {
                damage /= 2;
                player.lock()->vestHardness -= damage;
                if (player.lock()->vestHardness <= 0)
                    player.lock()->armor = Player::ArmorType::noArmor;
            }
        }

        // ── 音效 ────────────────────────────────────────────
        SoundManager::instance().play("hit", 0.5);

        // ── 扣血打印 ────────────────────────────────────────
        qDebug().noquote() << QString("[定身受击] P%1 | 伤害:%2 | HP:%3 → %4")
                                  .arg(player.lock()->id + 1)
                                  .arg(damage, 0, 'f', 1)
                                  .arg(player.lock()->hp, 0, 'f', 1)
                                  .arg(player.lock()->hp - damage, 0, 'f', 1);

        player.lock()->hp -= damage;

        // ── 累计判断是否破定身 ──────────────────────────────
        auto& ss = player.lock()->spellState;
        ss.frozenBreakHpLeft -= damage;

        if (ss.frozenBreakHpLeft <= 0) {
            ss.isFrozen     = false;
            ss.frozenRemain = 0.f;

            float stunTime = 1.f + player.lock()->modifiers.freezeStunBonus;
            cooldowns["hurt"] = stunTime;

            // 击退
            if (direction == "left") {
                player.lock()->vx = 60;
                player.lock()->vy = -400;
            } else {
                player.lock()->vx = -60;
                player.lock()->vy = -400;
            }
            player.lock()->state.moveState = "hurt";
            player.lock()->update();

            qDebug().noquote() << QString("[定身破除] P%1 | 硬直:%2s")
                                      .arg(player.lock()->id + 1)
                                      .arg(stunTime, 0, 'f', 1);

            emit frozenBroken(player.lock()->id);
        }
        return; // 不走后续普通受击逻辑
    }

    // ── 普通受击：原有逻辑完全不变 ──────────────────────────
    if (cooldowns["hurt"] != 0) return;
    if (damage < 1) return;

    if (weaponType == Player::WeaponType::ball) {
        damage = fmin(damage, 0.4 * player.lock()->maxHp);
        qDebug() << "ball final damage:" << damage;
    }

    if (player.lock()->armor == Player::ArmorType::chainmail) {
        if (weaponType == Player::WeaponType::punch) return;
        else if (weaponType == Player::WeaponType::knife) damage /= 2;
    }
    else if (player.lock()->armor == Player::ArmorType::vest
             && player.lock()->vestHardness > 0) {
        if (weaponType == Player::WeaponType::rifle
            || weaponType == Player::WeaponType::sniper) {
            damage /= 2;
            player.lock()->vestHardness -= damage;
            if (player.lock()->vestHardness <= 0)
                player.lock()->armor = Player::ArmorType::noArmor;
        }
    }

    SoundManager::instance().play("hit", 0.5);

    qDebug().noquote() << QString("[受击] P%1 | 护甲后:%2 | HP:%3 → %4")
                              .arg(player.lock()->id + 1)
                              .arg(damage, 0, 'f', 1)
                              .arg(player.lock()->hp, 0, 'f', 1)
                              .arg(player.lock()->hp - damage, 0, 'f', 1);

    player.lock()->hp -= damage;
    cooldowns["hurt"] = 1;

    if (direction == "left") {
        player.lock()->vx = 60;
        player.lock()->vy = -400;
    } else {
        player.lock()->vx = -60;
        player.lock()->vy = -400;
    }
    player.lock()->state.moveState = "hurt";
    player.lock()->update();
}

QRect PlayerController::getHitbox(){
    return player.lock()->hitbox;
}

Player::WeaponType PlayerController::getWeaponType(){
    return player.lock()->weapon;
}
