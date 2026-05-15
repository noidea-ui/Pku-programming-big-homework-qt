#ifndef PETCONTROLLER_H
#define PETCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include "animationmanager.h"
class PetController : public QObject
{
    Q_OBJECT
public:
    explicit PetController(QObject *parent = nullptr);

    void changeState(PetState newState);
    PetState getCurrentState() const;
    QPixmap getCurrentFrame();
    // 强制动作接口：设定一个持续的动作（直到 clearForcedState 被调用）
    void setForcedState(PetState state);
    void clearForcedState();
    bool hasForcedState() const;
    PetState forcedState() const;

signals:
    void frameUpdated();
private slots:
    void updateLogic();
    void startRandomAction();
private:
    PetState m_currentState;
    int m_currentFrameIndex;
    QTimer *m_timer;
    QTimer *m_actionTimer;
    bool m_isPlayingAction;
    int m_sleepLoopsRemaining;
    bool m_forcedActionActive;
    PetState m_forcedState;
    QPixmap m_lionPixmap;
    AnimationManager m_animManager;

};

#endif // PETCONTROLLER_H
