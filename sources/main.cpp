#include "mainwindow.h"
#include <QApplication>
#include <ryisp.h>
#include <QFont>
#include <QStyleFactory>

#ifdef Q_OS_WIN
#include <windows.h>
// Windows 入口点：绕过 Qt6EntryPoint 与 GCC 15+ MinGW 的兼容性问题
extern "C" int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    int argc = 0;
    char **argv = nullptr;
    // 从 Windows 命令行获取参数
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (szArglist) {
        argv = new char*[argc];
        for (int i = 0; i < argc; i++) {
            int len = WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, nullptr, 0, nullptr, nullptr);
            argv[i] = new char[len];
            WideCharToMultiByte(CP_UTF8, 0, szArglist[i], -1, argv[i], len, nullptr, nullptr);
        }
        LocalFree(szArglist);
    }
#else
int main(int argc, char *argv[])
{
#endif
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

    int result = a.exec();

#ifdef Q_OS_WIN
    // 清理 argv
    if (argv) {
        for (int i = 0; i < argc; i++) {
            delete[] argv[i];
        }
        delete[] argv;
    }
#endif
    return result;
}

// 编译说明：
// 1. release版本注销QT_DEBUG定义, ryesp32isp.h
// 2. 版本历史见 git log 或 GitHub Releases
