#ifndef PETCONTROLLER_H
#define PETCONTROLLER_H

#include <QObject>
#include<QTimer>
#include <QPoint>
#include"animationmanager.h"
class PetController : public QObject
{
    Q_OBJECT
public:
    explicit PetController(QObject *parent = nullptr);

    void changeState(PetState newState);
    PetState getCurrentState() const;
    QPixmap getCurrentFrame();
    // 行走相关
    QPoint m_targetPos;
    bool m_isMoving{false};
    double m_speed{4.0}; // 每次更新移动像素数（可调）
    // 状态切换计时器
    int m_stateTimerCounter{0}; // 每个状态的计时计数器
    int m_walkStepCount{0};     // 行走距离计数（一次行走后切换）

signals:
    void positionChanged(const QPoint &pos);

signals:
    void frameUpdated();
private slots:
    void updateLogic();
private:
    PetState m_currentState;
    int m_currentFrameIndex;
    QTimer *m_timer;
    AnimationManager m_animManager;
    

};

#endif // PETCONTROLLER_H
