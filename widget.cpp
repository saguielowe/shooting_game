#include "widget.h"
#include "soundmanager.h"
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include "./ui_widget.h"

Widget::Widget(const QString& scene, int maxHp, int maxBalls, int maxBullets, int maxSnipers, QWidget *parent)
    : QWidget(parent), currentScene(scene), m_maxHp(maxHp), m_maxBalls(maxBalls), m_maxBullets(maxBullets), m_maxSnipers(maxSnipers)
    , ui(new Ui::Widget) // 初始化已定义但未赋值的变量，大括号内是要做的操作
{
    setFocusPolicy(Qt::StrongFocus);  // 获取键盘焦点
    if (currentScene == "ice"){
        Player::initSettings(false, 1.5, maxHp, maxBalls, maxBullets, maxSnipers);
    }
    else if (currentScene == "grass"){
        Player::initSettings(true, 1.0, maxHp, maxBalls, maxBullets, maxSnipers);
    }
    else{
        Player::initSettings(false, 1.0, maxHp, maxBalls, maxBullets, maxSnipers);
    }
    scenePixmap = Map::getInstance().loadScene(scene);
    //qDebug() <<"scene loaded";
    SoundManager::instance().preload("crash");
    SoundManager::instance().preload("rifle");
    SoundManager::instance().preload("knife");
    SoundManager::instance().preload("sniper");
    SoundManager::instance().preload("hit");
    intent[0].moveIntent = MoveIntent::NONE;
    intent[0].attackIntent = false;
    intent[1].moveIntent = MoveIntent::NONE;
    intent[1].attackIntent = false;
    auto p1 = std::make_shared<Player>(100, 600, 0);
    auto p2 = std::make_shared<Player>(800, 600, 1);

    auto c1 = std::make_shared<PlayerController>();
    auto c2 = std::make_shared<PlayerController>();

    c1->bindPlayer(p1);  // 用 weak_ptr 持有
    c2->bindPlayer(p2);

    players.push_back(p1);
    players.push_back(p2);

    controllers.push_back(c1);
    controllers.push_back(c2);
    //qDebug() <<"player spawned";

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::gameLoop);
    connect(c1.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);
    connect(c2.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);
    timer->start(16);  // 约 60fps
    lastTime.start();
    //qDebug() <<"timer started";
    ai->setAIPlayer(players[1]);        // AI控制的玩家
    ai->setTargetPlayer(players[0]);    // 人类玩家
    ai->setFollowDistance(80.0f);    // 跟随距离
    ai->setDifficulty(5);            // 难度1-10
    ui->setupUi(this);
}
void Widget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    if (!scenePixmap.isNull()) {
        painter.drawPixmap(0, 0, scenePixmap);
    }

    if (players.size() > 0 && players[0]) {
        players[0]->draw(painter);
    }
    if (players.size() > 1 && players[1]) {
        players[1]->draw(painter);
    }

    // 保护 entities
    for (const auto& entity : entities) {
        if (entity) {
            entity->draw(painter);
        }
    }

    ai->draw(painter);
}
/*
inputIntent: 输入意图。
输入意图与按键相关，与物理场景无关，例如下落不会是输入意图，但确实存在这一状态；
输入意图并不一定有效，例如跳起时不能下蹲，需要进行初步处理，忽略掉矛盾的意图。
但要注意，这里只是对按键矛盾做处理，例如空中按下蹲的无效判定不在此处处理，因为widget不会去查询player是否在空中。
同一时间可能存在多个合法输入意图，例如跳跃射击，但动作意图和射击意图只能存在其一。
其中，边跑边跳是由于既有运动状态导致的，不是玩家输入意图所必然导致的，此处不考虑也不存储这一意图。
*/
void Widget::keyPressEvent(QKeyEvent* event) {
    emit keyPressed();
    if (event->key() == Qt::Key_A) {
        intent[0].moveIntent = MoveIntent::MOVING_LEFT; // 同时按下左右键优先处理左键
    }
    else if (event->key() == Qt::Key_D) {
        intent[0].moveIntent = MoveIntent::MOVING_RIGHT;
    }
    else if (event->key() == Qt::Key_W) {
        intent[0].moveIntent = MoveIntent::JUMP;
    }
    else if (event->key() == Qt::Key_S) { // player部分默认已经在widget处检查过合法状态转移
        intent[0].moveIntent = MoveIntent::CROUCH;
    }
    else {
        intent[0].moveIntent = MoveIntent::NONE;
    }

    if (event->key() == Qt::Key_E) {
        intent[0].attackIntent = true;
    }
    else{
        intent[0].attackIntent = false;
    }

    if (event->key() == Qt::Key_J) {
        intent[1].moveIntent = MoveIntent::MOVING_LEFT; // 同时按下左右键优先处理左键
    }
    else if (event->key() == Qt::Key_L) {
        intent[1].moveIntent = MoveIntent::MOVING_RIGHT;
    }
    else if (event->key() == Qt::Key_I) {
        intent[1].moveIntent = MoveIntent::JUMP;
    }
    else if (event->key() == Qt::Key_K) { // player部分默认已经在widget处检查过合法状态转移
        intent[1].moveIntent = MoveIntent::CROUCH;
    }
    else {
        intent[1].moveIntent = MoveIntent::NONE;
    }

    if (event->key() == Qt::Key_O) {
        intent[1].attackIntent = true;
    }

}

void Widget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_A || event->key() == Qt::Key_S || event->key() == Qt::Key_W || event->key() == Qt::Key_D) {
        intent[0].moveIntent = MoveIntent::NONE;
    }

    if (event->key() == Qt::Key_J || event->key() == Qt::Key_K || event->key() == Qt::Key_L || event->key() == Qt::Key_I) {
        intent[1].moveIntent = MoveIntent::NONE;
    }

}

void Widget::gameLoop() {
    float dt = lastTime.restart() / 1000.0f;
    if (dt > 0.05) qDebug() << 1 / dt;
    if (QRandomGenerator::global()->bounded(1000) < 3) {
        spawnDrop(); // 每帧尝试生成掉落物
    }
    updateDrops(dt);
    players.erase(std::remove_if(players.begin(), players.end(),
                                 [](const std::shared_ptr<Player>& p) {
                                     return p->isDead();
                                 }), players.end());
    controllers.erase(std::remove_if(controllers.begin(), controllers.end(),
                                     [](const std::shared_ptr<PlayerController>& c) {
                                         return c->isOrphaned();  // ✅ 玩家已被销毁
                                     }), controllers.end());
    if (players.length() < 2){
        gameEnd(players[0]->id);
        return;
    }
    players[0]->setDt(dt); // 万一卡顿根据真实帧数设置dt
    players[1]->setDt(dt);

    MoveIntent moveIntent = MoveIntent::NONE;
    AttackIntent attackIntent = false;

    ai->updateIntent(moveIntent, attackIntent);
    if (currentScene == "grass" && moveIntent == MoveIntent::NONE){
        moveIntent = MoveIntent::CROUCH; // 草地背景下不动就蹲下
    }

    intent[1].moveIntent = moveIntent;
    intent[1].attackIntent = attackIntent;
    //qDebug() << moveIntent << attackIntent;
    controllers[0]->handleIntent(intent[0].moveIntent, intent[0].attackIntent);
    controllers[1]->handleIntent(intent[1].moveIntent, intent[1].attackIntent);
    cm.checkPlayerVsPlayerCollision(controllers[0].get(), controllers[1].get());
    updateBalls(dt);
    //qDebug() << "Entity count: " << entities.size();
    entities.erase(std::remove_if(entities.begin(), entities.end(),
                               [](const std::shared_ptr<Entity>& e) {
                                   return e->shouldBeRemoved();
                               }), entities.end());
    drops.erase(std::remove_if(drops.begin(), drops.end(), [](auto& w) {
                    return w.expired(); // ✅ 清除掉失效的 weak_ptr
                }), drops.end());
    balls.erase(std::remove_if(balls.begin(), balls.end(), [](auto& w) {
                    return w.expired(); // ✅ 清除掉失效的 weak_ptr
                }), balls.end());
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](auto& w) {
                    return w.expired(); // ✅ 清除掉失效的 weak_ptr
                }), bullets.end());
    update();  // 触发 paintEvent()
    intent[1].attackIntent = false;
    intent[0].attackIntent = false;
}

void Widget::spawnDrop() {
    float x = QRandomGenerator::global()->bounded(100, 900); // 视口宽度

    // Step 1: 生成主类（武器/护甲/药品）
    int categoryRoll = QRandomGenerator::global()->bounded(100); // [0, 99]
    QString itemName;

    if (categoryRoll < 50) { // 武器 50%
        int weaponRoll = QRandomGenerator::global()->bounded(100);
        if (weaponRoll < 40) {
            itemName = "knife"; // 小刀 40%
        } else if (weaponRoll < 70) {
            itemName = "ball"; // 实心球 30%
        } else if (weaponRoll < 90) {
            itemName = "rifle"; // 步枪 20%
        } else {
            itemName = "sniper"; // 狙击枪 10%
        }
    } else if (categoryRoll < 70) { // 护甲 20%
        int armorRoll = QRandomGenerator::global()->bounded(100);
        if (armorRoll < 50) {
            itemName = "chainmail"; // 锁子甲 50%
        } else {
            itemName = "vest"; // 防弹衣 50%
        }
    } else { // 药品 30%
        int medicineRoll = QRandomGenerator::global()->bounded(100);
        if (medicineRoll < 50) {
            itemName = "bandage"; // 绷带 50%
        } else if (medicineRoll < 70) {
            itemName = "medkit"; // 医疗箱 20%
        } else {
            itemName = "adrenaline"; // 肾上腺素 30%
        }
    }
    //qDebug() << "decided to spawn:" << itemName;
    auto drop = std::make_shared<DropItem>(x, itemName);
    entities.push_back(drop);
    drops.push_back(drop);
}

void Widget::updateDrops(float dt){
    for (auto& drop : drops) {
        if (drop.lock() == nullptr){
            qDebug() <<"here is a fucking nullptr";
            return;
        }
        if (drop.lock()->isCollectedBy(players[0].get()) && intent[0].moveIntent == MoveIntent::CROUCH) {
            drop.lock()->markForDeletion();  // ⚠️ 不立即删，留给后面统一处理
            players[0]->weaponControll(drop.lock()->itemType);
        }
        else if (drop.lock()->isCollectedBy(players[1].get()) && intent[1].moveIntent == MoveIntent::CROUCH) {
            drop.lock()->markForDeletion();  // ⚠️ 不立即删，留给后面统一处理
            players[1]->weaponControll(drop.lock()->itemType);
        }
        drop.lock()->setDt(dt);
        drop.lock()->update();
    }
    updateAIInfo();
}

void Widget::updateBalls(float dt){
    for (auto& ball : balls) {
        if (ball.lock() == nullptr) return;
        ball.lock()->setDt(dt);
        ball.lock()->update();
        cm.checkEntityVsPlayerCollision(ball.lock().get(), controllers[0].get());
        cm.checkEntityVsPlayerCollision(ball.lock().get(), controllers[1].get());
    }
    for (auto& bullet : bullets) {
        if (bullet.lock() == nullptr) return;
        bullet.lock()->setDt(dt);
        bullet.lock()->update();
        cm.checkEntityVsPlayerCollision(bullet.lock().get(), controllers[0].get());
        cm.checkEntityVsPlayerCollision(bullet.lock().get(), controllers[1].get());
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::gameEnd(int id)
{
    // 1. 停掉计时器，断掉信号，避免旧逻辑还在跑
    if (timer) {
        timer->stop();
        disconnect(timer, nullptr, this, nullptr);
    }
    disconnect(this, nullptr, nullptr, nullptr);

    // 2. 不要提前清空 players/entities！
    //    先留着，paintEvent 还能画最后一帧完整画面
    // controllers.clear();
    // players.clear();
    // entities.clear();

    // 3. 叠加 Game Over 遮罩
    QLabel *overlay = new QLabel(this);
    overlay->setGeometry(rect());
    overlay->setStyleSheet(
        "background: rgba(0, 0, 0, 160);"
        "color: white;"
        "font-size: 40px;"
        "font-weight: bold;"
        );
    overlay->setAlignment(Qt::AlignCenter);
    overlay->setText("玩家"+ QString::number(2-id) +"死亡！\n\nGAME OVER\n\n按任意键重新开始");
    overlay->show();

    // 4. 抢键盘焦点，捕捉“任意键”
    grabKeyboard();

    // ✅ 用一次性连接，避免多次触发
    connect(this, &Widget::keyPressed, this, [=]() mutable {
        releaseKeyboard();  // 释放键盘

        // 5. 延迟一帧再新建，保证事件栈安全退出
        QTimer::singleShot(0, this, [=]() mutable {
            // ✅ 新建干净的 Widget（它自己会加载地图/初始化状态）
            Widget *newGame = new Widget(currentScene, m_maxHp, m_maxBalls, m_maxBullets, m_maxSnipers);
            newGame->setFixedSize(this->size());
            newGame->show();
            newGame->setFocus();   // ✅ 立刻接收输入
        });
    });
}

void Widget::onPlayerRequest(float x, float y, float vx, float vy, Player::WeaponType weapon, int id) {
    qDebug() <<"【onplayer】发射者： "<<id;
    if (weapon == Player::WeaponType::ball){
        if (vx > 0){
            vx += 500;
        }
        else if (vx < 0){
            vx -= 500;
        }
        /*if (vy > 0){
        vy += 200;
        } // 改动y值反而失去了趣味性，命中率感人
        else{
            vy -= 500;
        }*/
        auto ball = std::make_shared<Ball>(QPointF(x, y), QPointF(vx, vy), id);
        entities.push_back(ball);
        balls.push_back(ball);
    }
    else if (weapon == Player::WeaponType::rifle){
        if (vx >= 0){
            vx = 2000;
        }
        else if (vx < 0){
            vx = -2000;
        }
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 1, id);
        entities.push_back(bullet);
        bullets.push_back(bullet);
    }
    else if (weapon == Player::WeaponType::sniper){
        if (vx >= 0){
            vx = 3000;
        }
        else if (vx < 0){
            vx = -3000;
        }
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 2, id);
        entities.push_back(bullet);
        bullets.push_back(bullet);
    }
}

void Widget::updateAIInfo()
{
    if (!ai) return;

    // 收集掉落物信息
    QVector<GameAI::DropInfo> dropInfos;
    for (const auto& weakDrop : drops) {
        if (auto drop = weakDrop.lock()) {
            GameAI::DropInfo info;
            info.position = drop->getPos();
            info.itemType = drop->itemType;
            dropInfos.append(info);
        }
    }

    // 更新AI的掉落物信息
    ai->updateDropsInfo(dropInfos);

    // 更新AI玩家的当前武器（假设player2是AI）
    auto wp = players[1]->weapon;
    if (wp == Player::WeaponType::punch){
        ai->setCurrentWeapon("punch");
    }
    else if (wp == Player::WeaponType::knife){
        ai->setCurrentWeapon("knife");
    }
    else if (wp == Player::WeaponType::ball){
        ai->setCurrentWeapon("ball");
    }
    else if (wp == Player::WeaponType::rifle){
        ai->setCurrentWeapon("rifle");
    }
    else if (wp == Player::WeaponType::sniper){
        ai->setCurrentWeapon("sniper");
    }

}
