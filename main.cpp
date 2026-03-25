#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
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
