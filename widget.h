#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include "player.h"
#include "map.h"
#include "playercontroller.h"

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
    ~Widget();

protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;

private slots:
    void gameLoop();

private:
    struct inputIntent {
        QString moveIntent;
        bool attackIntent;
    };
    inputIntent intent[2];

    std::unique_ptr<Player> player1;
    std::unique_ptr<Player> player2;
    std::unique_ptr<PlayerController> controller1;
    std::unique_ptr<PlayerController> controller2;

    QTimer* timer;
    QElapsedTimer lastTime;

    QString currentScene;
    bool keyLeft = false, keyRight = false, crouching = false; // 辅助变量来判断玩家的按键输入对应哪种状态
    Ui::Widget *ui;
};
#endif // WIDGET_H
