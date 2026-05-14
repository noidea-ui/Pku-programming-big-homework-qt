#include "petcontroller.h"

PetController::PetController(QObject *parent)
    : QObject{parent},m_currentState(PetState::IDLE),m_currentFrameIndex(0)
{
    // Load per-state frames from resources (will fall back to sprite sheet if needed)
    m_animManager.loadFromResources();

    m_timer = new QTimer(this);
    connect(m_timer,&QTimer::timeout,this,&PetController::updateLogic);

    // set initial interval based on current state
    int initialInterval = m_animManager.getFrameInterval(m_currentState);
    m_timer->start(initialInterval > 0 ? initialInterval : 150);
}

void PetController::changeState(PetState newState){
    if(m_currentState == newState){
        return;
    }

    m_currentState = newState;
    m_currentFrameIndex = 0;

    // adjust timer interval for the new state's preferred frame duration
    int interval = m_animManager.getFrameInterval(newState);
    if(m_timer) m_timer->setInterval(interval > 0 ? interval : 150);

    emit frameUpdated();
}

QPixmap PetController::getCurrentFrame() {
    return m_animManager.getFrame(m_currentState,m_currentFrameIndex);
}


void PetController::updateLogic(){
    int frameCount = m_animManager.getFrameCount(m_currentState);
    if(frameCount >0){
        m_currentFrameIndex = (m_currentFrameIndex +1)%frameCount;

    }

    //其它的一些用于状态转换的逻辑用于后来的拓展部分

    emit frameUpdated();


}