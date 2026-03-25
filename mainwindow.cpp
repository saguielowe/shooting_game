#include "mainwindow.h"
#include "setupwidget.h"
#include "widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QPushButton>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Shooting Game");
    //setFixedSize(1000, 700);  // 和原来 Widget 保持一致，按需调整

    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // 按顺序建好三个页面，索引固定
    buildMenu();   // PAGE_MENU  = 0
    buildSetup();  // PAGE_SETUP = 1

    // GameWidget 还不能在这里建，需要等 session 确定后才能初始化
    // 先占个位置，之后 onSessionReady 里再创建
    gamePage = nullptr;
    stack->addWidget(new QWidget(this));  // PAGE_GAME 先占位

    stack->setCurrentIndex(PAGE_MENU);

    // 背景色
    setStyleSheet("QMainWindow { background: #1a1a2e; }");
}

// ============================================================
//  主菜单页面构建
// ============================================================
void MainWindow::buildMenu() {
    menuPage = new QWidget(this);
    menuPage->setStyleSheet("background: #1a1a2e;");

    auto* layout = new QVBoxLayout(menuPage);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    auto* title = new QLabel("SHOOTING GAME", menuPage);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 42px; font-weight: bold; color: #e74c3c; letter-spacing: 4px;");
    layout->addWidget(title);

    layout->addSpacing(40);

    auto makeBtn = [&](const QString& text) {
        auto* btn = new QPushButton(text, menuPage);
        btn->setFixedSize(240, 52);
        btn->setStyleSheet(
            "QPushButton { background: #2c2c54; color: white; border-radius: 10px;"
            "              font-size: 16px; border: 1px solid #444; }"
            "QPushButton:hover { background: #40407a; }"
            "QPushButton:pressed { background: #706fd3; }"
        );
        layout->addWidget(btn, 0, Qt::AlignCenter);
        return btn;
    };

    auto* btnStart = makeBtn("开始游戏");
    auto* btnQuit  = makeBtn("退出");

    connect(btnStart, &QPushButton::clicked, this, &MainWindow::goToSetup);
    connect(btnQuit,  &QPushButton::clicked, qApp, &QApplication::quit);

    stack->addWidget(menuPage);
}

// ============================================================
//  设置页面构建
// ============================================================
void MainWindow::buildSetup() {
    setupPage = new SetupWidget(this);
    setupPage->setStyleSheet("background: #1a1a2e;");

    connect(setupPage, &SetupWidget::sessionReady,
            this, &MainWindow::onSessionReady);
    connect(setupPage, &SetupWidget::backToMenu,
            this, &MainWindow::goToMenu);

    stack->addWidget(setupPage);
}

// ============================================================
//  页面切换
// ============================================================
void MainWindow::goToSetup() {
    stack->setCurrentIndex(PAGE_SETUP);
}

void MainWindow::goToMenu() {
    stack->setCurrentIndex(PAGE_MENU);
}

// ============================================================
//  开始游戏：收到 SetupWidget 的 session，启动第一局
// ============================================================
void MainWindow::onSessionReady(GameSession s) {
    session = s;
    session.resetScores();

    if (gamePage == nullptr) {
        // 首次创建 GameWidget
        gamePage = new Widget(session, this);

        // 替换掉占位的空 widget
        auto* placeholder = stack->widget(PAGE_GAME);
        stack->removeWidget(placeholder);
        placeholder->deleteLater();
        stack->insertWidget(PAGE_GAME, gamePage);

        // 连接局结束信号
        connect(gamePage, &Widget::roundEnded,
                this, &MainWindow::onRoundEnd);
    } else {
        // 已有 GameWidget，直接 reset
        gamePage->resetGame(session);
    }

    stack->setCurrentIndex(PAGE_GAME);
    gamePage->setFocus();
}

// ============================================================
//  一局结束：更新比分，判断是否继续
// ============================================================
void MainWindow::onRoundEnd(int winnerId) {
    qDebug()<<"round ended!";
    session.addScore(winnerId);

    if (session.mode == GameSession::Mode::ENDLESS) {
        // 无尽模式：玩家死亡就是结束
        showResult();
        return;
    }

    if (session.isMatchOver()) {
        showResult();
    } else {
        // 还有下一局，直接 reset 继续
        startRound();
    }
}

void MainWindow::startRound() {
    gamePage->resetGame(session);
    gamePage->setFocus();
    gamePage->show();
}

// ============================================================
//  整场结束：显示结果，提供"再来一局"和"返回菜单"
// ============================================================
void MainWindow::showResult() {
    int winner = (session.scoreP1 > session.scoreP2) ? 1 : 2;

    QString msg = QString("玩家%1 获胜！\n\n最终比分：%2 : %3")
                      .arg(winner)
                      .arg(session.scoreP1)
                      .arg(session.scoreP2);

    // 用 QMessageBox 简单显示，后续可以换成自定义 ResultWidget
    QMessageBox box(this);
    box.setWindowTitle("比赛结束");
    box.setText(msg);
    box.setStyleSheet("QMessageBox { background: #1a1a2e; color: white; }");

    QPushButton* btnAgain = box.addButton("再来一局", QMessageBox::AcceptRole);
    QPushButton* btnMenu  = box.addButton("返回菜单", QMessageBox::RejectRole);
    Q_UNUSED(btnAgain);

    box.exec();

    if (box.clickedButton() == btnMenu) {
        goToMenu();
    } else {
        // 再来一局：重置比分，重新开始
        session.resetScores();
        startRound();
    }
}
