#ifndef PETWINDOW_H
#define PETWINDOW_H

#include<QWidget>
#include<QMouseEvent>
#include<QContextMenuEvent>
#include"petcontroller.h"
#include"traymenumanager.h"
class PetWindow : public QWidget
{
    Q_OBJECT
public:
    PetWindow(QWidget *parent = nullptr);
    ~PetWindow();

protected:
    void paintEvent(QPaintEvent *event)override ;

    void mousePressEvent(QMouseEvent *event)override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onFrameUpdated();

private:
    PetController *m_controller;
    TrayMenuManager *m_trayManager;
    QPoint m_dragPosition;
    bool m_isDragging;

};

#endif // PETWINDOW_H
