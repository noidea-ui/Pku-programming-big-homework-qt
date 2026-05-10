#include "animationmanager.h"
#include<QDebug>

AnimationManager::AnimationManager() {
    registerAnimations();
}

void AnimationManager::loadLionSheet(const QString&filePath){
    bool loaded = m_lionSheet.load(filePath);
    if(loaded){
        qDebug() << "Successfully loaded lion sheet:" << filePath 
                 << "Size:" << m_lionSheet.size();
    } else {
        qWarning() << "Failed to load lion sheet from:" << filePath;
    }
}

void AnimationManager::registerAnimations(){
    // 暂时跳过注册，改为在 getFrame 时动态返回整张图片
    // 这样可以兼容后续的多帧动画
}

QPixmap AnimationManager::getFrame(PetState /*state*/, int /*frameIndex*/){
    // 直接返回整张图片，不处理帧切片
    // 帧切换由调用者在 PetController 中处理（改变 m_currentFrameIndex）
    if(m_lionSheet.isNull()){
        qDebug() << "Lion sheet is null in getFrame()";
        return QPixmap();
    }
    qDebug() << "Returning lion image, size:" << m_lionSheet.size();
    return m_lionSheet;
}

int AnimationManager::getFrameCount(PetState /*state*/) const{
    // 返回2表示两帧（虽然实际上只返回一张图片）
    // 这样 frameIndex % 2 可以在 0 和 1 间循环
    return m_lionSheet.isNull() ? 0 : 2;
}


