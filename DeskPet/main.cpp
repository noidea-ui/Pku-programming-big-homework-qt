#include"PetWindow.h"
#include<QApplication>

int main(int argc,char *argv[]){
#if QT_VERSION<QT_VERSION_CHECK(6,0,0)
QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
QApplication a(argc,argv);
PetWindow w;
w.show();
return a.exec();

}