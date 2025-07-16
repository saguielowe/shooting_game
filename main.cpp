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

        // 创建游戏窗口并传参
        Widget w(selectedScene, maxHp, maxBalls);
        w.setFixedSize(1024, 956);
        w.show();
        return a.exec();
    } else {
        // 用户取消 -> 直接退出
        return 0;
    }
}
