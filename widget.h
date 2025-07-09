#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QPainter>
#include "player.h"
#include "map.h"
#include "playercontroller.h"
#include "combatmanager.h"
#include "dropitem.h"
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    void updateDrops(float dt);
    void drawDrops(QPainter& painter);
    ~Widget();

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private slots:
    void gameLoop();

private:
    void spawnDrop();
    void gameEnd();
    QVector<std::shared_ptr<DropItem>> drops;
    struct inputIntent {
        QString moveIntent;
        bool attackIntent;
    };
    inputIntent intent[2];

    QVector<std::shared_ptr<Player>> players;
    QVector<std::shared_ptr<PlayerController>> controllers;

    QTimer* timer;
    QElapsedTimer lastTime;
    CombatManager cm;

    QString currentScene;
    bool keyLeft = false, keyRight = false, crouching = false; // 辅助变量来判断玩家的按键输入对应哪种状态
    Ui::Widget *ui;
};
#endif // WIDGET_H
