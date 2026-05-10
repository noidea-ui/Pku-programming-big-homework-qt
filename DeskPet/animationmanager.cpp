#include "animationmanager.h"
#include<QDebug>

AnimationManager::AnimationManager() {
    registerAnimations();
}

void AnimationManager::loadLionSheet(const QString&filePath){
    if(!m_lionSheet.load(filePath)){
        qWarning()<<"Failed to load lion sheet from: "<<filePath;
    }
}

void AnimationManager::registerAnimations(){

    //IDLE 待机状态，先完成这个然后再考虑其它的东西
    QList<QRect> idleFrames;
    idleFrames.append(QRect(0,0,100,100));
    idleFrames.append(QRect(100,0,100,100));
    m_animationFrames.insert(PetState::IDLE,idleFrames);

    //DEAGGED 被拖拽的状态
    QList<QRect>dragFrames;
    dragFrames.append(QRect(0,100,100,100));
    m_animationFrames.insert(PetState::DRAGGED,dragFrames);

    //SLEEPING 睡觉状态
    QList<QRect> sleepFrames;
    sleepFrames.append(QRect(400,300,100,100));
    sleepFrames.append(QRect(500,300,100,100));
    m_animationFrames.insert(PetState::SLEEPING,sleepFrames);

    //剩下的先暂时不做了，等到其它的做好了之后再进行完成

}

QPixmap AnimationManager::getFrame(PetState state,int frameIndex){
    if(m_lionSheet.isNull()||!m_animationFrames.contains(state))  {
        return QPixmap();
    }

    return m_lionSheet;

}

int AnimationManager::getFrameCount(PetState state) const{
    if(m_animationFrames.contains(state)){
        return m_animationFrames.value(state).size();
    }
    return 0;
}


