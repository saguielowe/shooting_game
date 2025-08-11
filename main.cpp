#include "widget.h"
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // === 设置对话框 ===
    QDialog dialog;
    dialog.setWindowTitle("游戏设置");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 场景选择
    QLabel *sceneLabel = new QLabel("选择游戏场景：");
    QComboBox *sceneCombo = new QComboBox;
    sceneCombo->addItems({"default", "grass", "ice"});
    layout->addWidget(sceneLabel);
    layout->addWidget(sceneCombo);

    // 血条上限
    QLabel *hpLabel = new QLabel("血条上限：");
    QSpinBox *hpSpin = new QSpinBox;
    hpSpin->setRange(10, 1000);
    hpSpin->setValue(100);
    layout->addWidget(hpLabel);
    layout->addWidget(hpSpin);

    // 投掷上限
    QLabel *ballLabel = new QLabel("实心球投掷上限：");
    QSpinBox *ballSpin = new QSpinBox;
    ballSpin->setRange(1, 50);
    ballSpin->setValue(3);
    layout->addWidget(ballLabel);
    layout->addWidget(ballSpin);

    // 射击上限
    QLabel *bulletLabel = new QLabel("手枪射击上限：");
    QSpinBox *bulletSpin = new QSpinBox;
    bulletSpin->setRange(1, 50);
    bulletSpin->setValue(3);
    layout->addWidget(bulletLabel);
    layout->addWidget(bulletSpin);

    // 射击上限
    QLabel *sniperLabel = new QLabel("狙击枪射击上限：");
    QSpinBox *sniperSpin = new QSpinBox;
    sniperSpin->setRange(1, 50);
    sniperSpin->setValue(1);
    layout->addWidget(sniperLabel);
    layout->addWidget(sniperSpin);


    // OK / Cancel 按钮
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 显示对话框
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedScene = sceneCombo->currentText();
        int maxHp = hpSpin->value();
        int maxBalls = ballSpin->value();
        int maxBullets = bulletSpin->value();
        int maxSnipers = sniperSpin->value();

        // 创建游戏窗口并传参
        Widget w(selectedScene, maxHp, maxBalls, maxBullets, maxSnipers);
        w.setFixedSize(1024, 956);
        w.show();
        return a.exec();
    } else {
        // 用户取消 -> 直接退出
        return 0;
    }
}
/*
 * 推荐游戏场景设置：
 * 默认场景，一切按照默认值：经典，熟悉操作，与AI切磋；
 * 冰地场景，选择ice：脚滑，难以操控，AI狂暴；
 * 草地场景，选择grass：隐身，你看不见AI，但AI看得见你！
 * 投掷基础，选择ice，最高血量200，实心球可投掷5次：熟悉实心球弹道，提高命中率！
 * 投掷进阶，选择grass，最高血量300，实心球可投掷7次：在AI隐身的状态下投掷！（最高血量越高，实心球能造成的最大伤害越高）
 * 枪械作战，选择grass，最高血量500，步枪可发射10次，狙击枪可发射5次：只有用枪才能制胜！
 */
