#include "petcontroller.h"

PetController::PetController(QObject *parent)
    : QObject{parent},m_currentState(PetState::IDLE),m_currentFrameIndex(0)
{
    m_animManager.loadLionSheet(":/lion.png");//在这个地方放入小狮子的图片资源,这个地方出了很长时间的错误，后来发现很有可能是因为使用cmake构建项目的时候需要显示的在camkelist之中添加

        m_timer = new QTimer(this);
    connect(m_timer,&QTimer::timeout,this,&PetController::updateLogic);

    m_timer->start(150);
}

void PetController::changeState(PetState newState){
    if(m_currentState == newState){
        return;
    }

    m_currentState = newState;
    m_currentFrameIndex = 0;

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