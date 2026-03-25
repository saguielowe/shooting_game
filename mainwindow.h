#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include "gamesession.h"

class SetupWidget;
class Widget;

// ============================================================
//  MainWindow — 程序入口窗口
//  持有 GameSession（跨局数据）
//  持有 QStackedWidget 管理页面切换
//  自己不画任何游戏内容，只负责数据和页面调度
// ============================================================

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

private slots:
    void onSessionReady(GameSession session);  // SetupWidget 发出，开始一局
    void onRoundEnd(int winnerId);             // Widget 发出，一局结束
    void goToSetup();                          // 切到设置页
    void goToMenu();                           // 切回主菜单

private:
    void buildMenu();     // 构建主菜单页
    void buildSetup();    // 构建设置页
    void startRound();    // 启动/重置 GameWidget 开始新的一局
    void showResult();    // 整场比赛结束，显示结果

    QStackedWidget* stack;

    // 页面指针
    QWidget*     menuPage;   // page 0：主菜单
    SetupWidget* setupPage;  // page 1：游戏设置
    Widget*      gamePage;   // page 2：游戏本体（复用，reset而不是new）

    // 持久状态
    GameSession session;

    // 页面索引常量
    static constexpr int PAGE_MENU  = 0;
    static constexpr int PAGE_SETUP = 1;
    static constexpr int PAGE_GAME  = 2;
};

#endif // MAINWINDOW_H
