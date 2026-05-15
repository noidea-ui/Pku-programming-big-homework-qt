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
    QPixmap m_lionPixmap;
    AnimationManager m_animManager;

};

#endif // PETCONTROLLER_H
