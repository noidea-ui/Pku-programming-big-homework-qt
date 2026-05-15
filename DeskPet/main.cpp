#include "PetWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
	QApplication a(argc, argv);
	// 保持程序在窗口关闭/隐藏后仍驻留（由托盘控制退出）
	a.setQuitOnLastWindowClosed(false);
	// 设置全局应用图标（同时用于任务栏/窗口和托盘图标）
	a.setWindowIcon(QIcon(":/lion.png"));

	PetWindow w;
	w.show();
	return a.exec();
}