#include "widget.h"
#include "./ui_widget.h"
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QElapsedTimer>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget) // 初始化已定义但未赋值的变量，大括号内是要做的操作
{
    setFocusPolicy(Qt::StrongFocus);  // 获取键盘焦点
    currentScene = "default";
    intent[0].moveIntent = "null";
    intent[0].attackIntent = false;
    intent[1].moveIntent = "null";
    intent[1].attackIntent = false;
    player1 = std::make_unique<Player>(100, 100);
    player2 = std::make_unique<Player>(500, 100);
    controller1 = std::make_unique<PlayerController>(player1.get());
    controller2 = std::make_unique<PlayerController>(player2.get());

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Widget::gameLoop);
    timer->start(16);  // 约 60fps
    lastTime.start();
    ui->setupUi(this);
}
void Widget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    Map::getInstance().loadScene(currentScene, painter); // 需要绘制的内容：地图（交给map）、玩家（交给player）
    player1->draw(painter);
    player2->draw(painter);
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
}

void Widget::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_A || event->key() == Qt::Key_S || event->key() == Qt::Key_W || event->key() == Qt::Key_D) {
        intent[0].moveIntent = "null";
    }
    if (event->key() == Qt::Key_E) {
        intent[0].attackIntent = true;
    }
    if (event->key() == Qt::Key_J || event->key() == Qt::Key_K || event->key() == Qt::Key_L || event->key() == Qt::Key_I) {
        intent[1].moveIntent = "null";
    }
    if (event->key() == Qt::Key_O) {
        intent[1].attackIntent = true;
    }
}

void Widget::gameLoop() {
    float dt = lastTime.restart() / 1000.0f;
    player1->setDt(dt); // 万一卡顿根据真实帧数设置dt
    player2->setDt(dt);
    controller1->handleIntent(intent[0].moveIntent, intent[0].attackIntent);
    controller2->handleIntent(intent[1].moveIntent, intent[1].attackIntent);
    update();  // 触发 paintEvent()，绘制player位置
}

Widget::~Widget()
{
    delete ui;
}
