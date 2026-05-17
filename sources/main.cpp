#include "mainwindow.h"
#include <QApplication>
#include <ryisp.h>
#include <QFont>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));  //使用 Fusion 样式,for mac 解决MAC下进度条不显示百分比问题
#ifdef Q_OS_WIN
    a.setFont(QFont("SimSun", 9));
#endif
    MainWindow w;
    w.show();

    return a.exec();
}

// 编译说明：
// 1. release版本注销QT_DEBUG定义, ryesp32isp.h
// 2. 版本历史见 git log 或 GitHub Releases
