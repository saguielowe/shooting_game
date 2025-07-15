#include "widget.h"
#include "./ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget) // 初始化已定义但未赋值的变量，大括号内是要做的操作
{
    setFocusPolicy(Qt::StrongFocus);  // 获取键盘焦点
    currentScene = "ice";
    Player::initSettings(false, 1.5);
    intent[0].moveIntent = "null";
    intent[0].attackIntent = false;
    intent[1].moveIntent = "null";
    intent[1].attackIntent = false;
    auto p1 = std::make_shared<Player>(100, 600);
    auto p2 = std::make_shared<Player>(800, 600);

    auto c1 = std::make_shared<PlayerController>();
    auto c2 = std::make_shared<PlayerController>();

    c1->bindPlayer(p1);  // 用 weak_ptr 持有
    c2->bindPlayer(p2);

    players.push_back(p1);
    players.push_back(p2);

    controllers.push_back(c1);
    controllers.push_back(c2);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::gameLoop);
    connect(c1.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);
    connect(c2.get(), &PlayerController::requestThrowBall,
            this, &Widget::onPlayerRequest);
    timer->start(16);  // 约 60fps
    lastTime.start();
    ui->setupUi(this);
}
void Widget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    Map::getInstance().loadScene(currentScene, painter); // 需要绘制的内容：地图（交给map）、玩家（交给player）
    players[0]->draw(painter);
    players[1]->draw(painter);
    for (const auto& entity : entities) {
        entity->draw(painter);
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
    if (event->key() == Qt::Key_A) {
        intent[0].moveIntent = "moving_left"; // 同时按下左右键优先处理左键
    }
    else if (event->key() == Qt::Key_D) {
        intent[0].moveIntent = "moving_right";
    }
    else if (event->key() == Qt::Key_W) {
        intent[0].moveIntent = "jump";
    }
    else if (event->key() == Qt::Key_S) { // player部分默认已经在widget处检查过合法状态转移
        intent[0].moveIntent = "crouch";
    }
    else {
        intent[0].moveIntent = "null";
    }

    if (event->key() == Qt::Key_E) {
        intent[0].attackIntent = true;
    }
    else{
        intent[0].attackIntent = false;
    }

    if (event->key() == Qt::Key_J) {
        intent[1].moveIntent = "moving_left"; // 同时按下左右键优先处理左键
    }
    else if (event->key() == Qt::Key_L) {
        intent[1].moveIntent = "moving_right";
    }
    else if (event->key() == Qt::Key_I) {
        intent[1].moveIntent = "jump";
    }
    else if (event->key() == Qt::Key_K) { // player部分默认已经在widget处检查过合法状态转移
        intent[1].moveIntent = "crouch";
    }
    else {
        intent[1].moveIntent = "null";
    }

    if (event->key() == Qt::Key_O) {
        intent[1].attackIntent = true;
    }

}

void Widget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_A || event->key() == Qt::Key_S || event->key() == Qt::Key_W || event->key() == Qt::Key_D) {
        intent[0].moveIntent = "null";
    }

    if (event->key() == Qt::Key_J || event->key() == Qt::Key_K || event->key() == Qt::Key_L || event->key() == Qt::Key_I) {
        intent[1].moveIntent = "null";
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
    // TODO:
    // 如果只有一个玩家结束游戏循环，否则造成访问越界
    if (players.length() < 2){
        gameEnd();
        return;
    }
    players[0]->setDt(dt); // 万一卡顿根据真实帧数设置dt
    players[1]->setDt(dt);
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
        if (drop.lock()->isCollectedBy(players[0].get()) && intent[0].moveIntent == "crouch") {
            drop.lock()->markForDeletion();  // ⚠️ 不立即删，留给后面统一处理
            players[0]->weaponControll(drop.lock()->itemType);
        }
        else if (drop.lock()->isCollectedBy(players[1].get()) && intent[1].moveIntent == "crouch") {
            drop.lock()->markForDeletion();  // ⚠️ 不立即删，留给后面统一处理
            players[1]->weaponControll(drop.lock()->itemType);
        }
        drop.lock()->setDt(dt);
        drop.lock()->update();
    }
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

void Widget::gameEnd(){
    timer->stop();
    qDebug() << "game is over";
}

void Widget::onPlayerRequest(float x, float y, float vx, float vy, Player::WeaponType weapon) {
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
        auto ball = std::make_shared<Ball>(QPointF(x, y), QPointF(vx, vy));
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
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 1);
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
        auto bullet = std::make_shared<Bullet>(QPointF(x, y), vx, 2);
        entities.push_back(bullet);
        bullets.push_back(bullet);
    }
}
