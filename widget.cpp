#include "widget.h"
#include "soundmanager.h"
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include "./ui_widget.h"
#include <QScreen>

Widget::Widget(const GameSession& session, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget) // 初始化已定义但未赋值的变量，大括号内是要做的操作
{

    this->setFixedSize(1024, 1024);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::gameLoop);
    applySession(session);
    initGame();
    qDebug() <<"game initialized.";
    setFocusPolicy(Qt::StrongFocus);  // 获取键盘焦点

    SoundManager::instance().preload("crash");
    SoundManager::instance().preload("rifle");
    SoundManager::instance().preload("knife");
    SoundManager::instance().preload("sniper");
    SoundManager::instance().preload("hit");

    qDebug() <<"scene loaded";

    ui->setupUi(this);
}

void Widget::applySession(const GameSession& session) {
    currentSession = session;
    currentScene   = session.scene;

    // 场景决定速度倍率，其他参数直接从 session 读
    // 替代原来的 Player::initSettings（static，之后要改掉）
    // 现在先保留兼容，等 static 改成实例成员后这里直接删
    float speedRatio = (session.scene == "ice") ? 1.5f : 1.0f;

    scenePixmap = Map::getInstance().loadScene(session.scene);
    qDebug() << "session loaded.";
}

void Widget::initGame() {
    // ---- 重置 intent ------------------------------------
    intent[0] = {MoveIntent::NONE, false};
    intent[1] = {MoveIntent::NONE, false};

    bool   canHide    = (currentSession.scene == "grass");
    float  speedRatio = (currentSession.scene == "ice") ? 1.5f : 1.0f;

    auto p1 = std::make_shared<Player>(
        100, 600, 0,
        currentSession.maxHp,
        currentSession.maxBalls,
        currentSession.maxBullets,
        currentSession.maxSnipers,
        canHide, speedRatio
        );
    auto p2 = std::make_shared<Player>(
        800, 600, 1,
        currentSession.maxHp,
        currentSession.maxBalls,
        currentSession.maxBullets,
        currentSession.maxSnipers,
        canHide, speedRatio
        );

    auto c1 = std::make_shared<PlayerController>();
    auto c2 = std::make_shared<PlayerController>();

    c1->bindPlayer(p1);
    c2->bindPlayer(p2);

    players.push_back(p1);
    players.push_back(p2);
    controllers.push_back(c1);
    controllers.push_back(c2);

    qDebug() << "player spawned";

    // ---- 信号槽 -----------------------------------------
    connect(c1.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);
    connect(c2.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);

    // ---- AI（人机/无尽模式启动，双人不启动）------------
    if (currentSession.mode != GameSession::Mode::PVP) {
        ai->setAIPlayer(players[1]);
        ai->setTargetPlayer(players[0]);
        ai->setFollowDistance(80.0f);
        ai->setDifficulty(5);
        ai->startAI();
    }

    qDebug() << "ai is ready";

    // ---- 计时器 -----------------------------------------
    lastTime.restart();
    timer->start(16);
    qDebug()<<"timer started";
}

// widget.cpp
void Widget::showMatchResult(const QString& text) {
    QLabel* overlay = new QLabel(this);
    overlay->setGeometry(rect());
    overlay->setStyleSheet(
        "background: rgba(0, 0, 0, 180);"
        "color: white;"
        "font-size: 36px;"
        "font-weight: bold;"
    );
    overlay->setAlignment(Qt::AlignCenter);
    overlay->setText(text);
    overlay->show();

    grabKeyboard();
    connect(this, &Widget::keyPressed, this, [=]() {
        releaseKeyboard();
        overlay->hide();
        QTimer::singleShot(0, this, [=]() {
            emit matchResultConfirmed();  // 新信号，玩家按键确认后
        });
    }, Qt::SingleShotConnection);
}

void Widget::cleanupGame() {
    timer->stop();
    ai->stopAI();

    // 断开 controller 的信号，防止 clear 后还有回调
    for (auto& c : controllers)
        disconnect(c.get(), nullptr, this, nullptr);

    players.clear();
    controllers.clear();
    entities.clear();
    drops.clear();
    balls.clear();
    bullets.clear();
}

// 新增 resetGame，供 MainWindow 调用
// 流程：gameEnd -- roundend signal -- resetGame -- cleanup -- apply+init -- start timer
void Widget::resetGame(const GameSession& session) {
    qDebug()<<"game reset for next round";
    cleanupGame();

    // 重新应用 session 参数并初始化
    applySession(session);
    // 上一轮结束后终止了timer连接，需要复原。
    connect(timer, &QTimer::timeout, this, &Widget::gameLoop);
    initGame();

    timer->start(16);
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
    pressedKeys.insert(event->key());
    updateIntents();
}
void Widget::updateIntents() {
    // P1
    if (pressedKeys.contains(Qt::Key_W)) intent[0].moveIntent = MoveIntent::JUMP; // 优先处理跳跃下蹲，允许同时按下双键
    else if (pressedKeys.contains(Qt::Key_S)) { // player部分默认已经在widget处检查过合法状态转移
        intent[0].moveIntent = MoveIntent::CROUCH;
    }
    else if (pressedKeys.contains(Qt::Key_A)) intent[0].moveIntent = MoveIntent::MOVING_LEFT;
    else if (pressedKeys.contains(Qt::Key_D)) intent[0].moveIntent = MoveIntent::MOVING_RIGHT;
    else intent[0].moveIntent = MoveIntent::NONE;
    intent[0].attackIntent = pressedKeys.contains(Qt::Key_E);
    // P2同理
    if (pressedKeys.contains(Qt::Key_I)) intent[1].moveIntent = MoveIntent::JUMP;
    else if (pressedKeys.contains(Qt::Key_K)) { // player部分默认已经在widget处检查过合法状态转移
        intent[1].moveIntent = MoveIntent::CROUCH;
    }
    else if (pressedKeys.contains(Qt::Key_J)) intent[1].moveIntent = MoveIntent::MOVING_LEFT;
    else if (pressedKeys.contains(Qt::Key_L)) intent[1].moveIntent = MoveIntent::MOVING_RIGHT;
    else intent[1].moveIntent = MoveIntent::NONE;
    intent[1].attackIntent = pressedKeys.contains(Qt::Key_O);
    //qDebug()<<intent[1].moveIntent;
}


void Widget::keyReleaseEvent(QKeyEvent* event) { // 按键 --> 意图
    pressedKeys.remove(event->key());
    updateIntents(); // 可以边走边打，但移动时不能跳起/蹲下，需要处理。
    if (event->key() == Qt::Key_A || event->key() == Qt::Key_S || event->key() == Qt::Key_W || event->key() == Qt::Key_D) {
        intent[0].moveIntent = MoveIntent::NONE;
    }

    if (event->key() == Qt::Key_J || event->key() == Qt::Key_K || event->key() == Qt::Key_L || event->key() == Qt::Key_I) {
        intent[1].moveIntent = MoveIntent::NONE;
    }

}

void Widget::gameLoop() {
    if (players.size() < 2) { qDebug() << "players not ready"; return; }

    if (!controllers[0] || !controllers[1]) { qDebug() << "controllers null"; return; }

    float dt = lastTime.restart() / 1000.0f;
    //qDebug() << dt;
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



    if(currentSession.mode == GameSession::Mode::AI){
        MoveIntent moveIntent = MoveIntent::NONE;
        AttackIntent attackIntent = false;
        ai->updateIntent(moveIntent, attackIntent);
        if (currentScene == "grass" && moveIntent == MoveIntent::NONE){
            moveIntent = MoveIntent::CROUCH; // 草地背景下不动就蹲下
        }
        intent[1].moveIntent = moveIntent;
        intent[1].attackIntent = attackIntent;
        //qDebug() << moveIntent << attackIntent;
    }
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
        if (weaponRoll < 10) {
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

    connect(this, &Widget::keyPressed, this, [=]() {
        releaseKeyboard();
        overlay->hide(); // 重新开始后遮罩解除
        QTimer::singleShot(0, this, [=]() {
            emit roundEnded(id);
        });
    }, Qt::SingleShotConnection);
}

void Widget::onPlayerRequest(float x, float y, float vx, float vy, Player::WeaponType weapon, float initDamage, int id) {
    //qDebug() <<"【onplayer】发射者： "<<id;
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
        auto ball = std::make_shared<Ball>(QPointF(x, y), QPointF(vx, vy), initDamage, id);
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
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 1, initDamage, id);
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
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 2, initDamage, id);
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
