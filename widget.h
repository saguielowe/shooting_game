#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QPainter>
#include "player.h"
#include "ball.h"
#include "map.h"
#include "playercontroller.h"
#include "combatmanager.h"
#include "dropitem.h"
#include "bullet.h"
#include <QRandomGenerator>
#include "gameai.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(const QString& scene, int maxHp, int maxBalls, int maxBullets, int maxSnipers, QWidget *parent = nullptr);
    void updateDrops(float dt);
    void updateBalls(float dt);
    void drawDrops(QPainter& painter);
    ~Widget();

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private slots:
    void gameLoop();
    void onPlayerRequest(float x, float y, float vx, float vy, Player::WeaponType weapon);
signals:
    void keyPressed();
private:
    void spawnDrop();
    void gameEnd(int id);

    struct inputIntent {
        MoveIntent moveIntent;
        bool attackIntent;
    };
    inputIntent intent[2];

    QVector<std::shared_ptr<Player>> players;
    QVector<std::shared_ptr<PlayerController>> controllers;

    QVector<std::weak_ptr<Ball>> balls;
    QVector<std::weak_ptr<DropItem>> drops;
    QVector<std::weak_ptr<Bullet>> bullets;
    QVector<std::shared_ptr<Entity>> entities;

    QTimer* timer;
    QElapsedTimer lastTime;
    CombatManager cm;

    QString currentScene;
    bool keyLeft = false, keyRight = false, crouching = false; // 辅助变量来判断玩家的按键输入对应哪种状态
    std::shared_ptr<GameAI> ai = std::make_shared<GameAI>(); // 赋空初始值
    Ui::Widget *ui;

    int m_maxHp, m_maxBalls, m_maxBullets, m_maxSnipers;
    QPixmap scenePixmap;
};
#endif // WIDGET_H
