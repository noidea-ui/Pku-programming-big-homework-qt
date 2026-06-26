#include "animationmanager.h"
#include<QDebug>

#include <QDir>

static QString stateToString(PetState s){
    switch(s){
    case PetState::IDLE: return QStringLiteral("idle");
    case PetState::DRAGGED: return QStringLiteral("dragged");
    case PetState::WALKING: return QStringLiteral("walking");
    case PetState::SLEEPING: return QStringLiteral("sleeping");
    case PetState::WORKING: return QStringLiteral("working");
    case PetState::CELEBRATING: return QStringLiteral("celebrating");
    case PetState::SAD: return QStringLiteral("sad");
    }
    return QString();
}

AnimationManager::AnimationManager() {
    registerAnimations();
}

void AnimationManager::loadLionSheet(const QString &filePath){
    if(!m_lionSheet.load(filePath)){
        qWarning() << "Failed to load lion sheet from: " << filePath;
    }
}

void AnimationManager::addAnimationFromResourceDir(PetState state, const QString &resourceDir, int intervalMs){
    QDir dir(resourceDir);
    if(!dir.exists()){

        qWarning() << "Resource dir does not exist:" << resourceDir;
        return;
    }

    QStringList nameFilters;
    nameFilters << "*.png" << "*.jpg" << "*.bmp";
    QStringList files = dir.entryList(nameFilters, QDir::Files, QDir::Name);

    QList<QPixmap> frames;
    for(const QString &f : files){
        QString path = resourceDir + "/" + f;
        QPixmap px;
        if(!px.load(path)){
            qWarning() << "Failed to load frame:" << path;
            continue;
        }
        frames.append(px);
    }

    if(!frames.isEmpty()){
        m_animationPixmaps.insert(state, frames);
        m_frameIntervalsMs.insert(state, intervalMs);
        qDebug() << "Loaded" << frames.size() << "frames for state" << stateToString(state);
    }
}

void AnimationManager::addAnimation(PetState state, const QStringList &resourcePaths, int intervalMs){
    QList<QPixmap> frames;
    for(const QString &p : resourcePaths){
        QPixmap px;
        if(!px.load(p)){
            qWarning() << "Failed to load frame:" << p;
            continue;
        }
        frames.append(px);
    }
    if(!frames.isEmpty()){
        m_animationPixmaps.insert(state, frames);
        m_frameIntervalsMs.insert(state, intervalMs);
    }
}

void AnimationManager::loadFromResources(){

    PetState states[] = { PetState::IDLE, PetState::DRAGGED, PetState::WALKING, PetState::SLEEPING, PetState::WORKING, PetState::CELEBRATING, PetState::SAD };
    QMap<PetState,int> defaults;
    defaults.insert(PetState::IDLE, 200);
    defaults.insert(PetState::DRAGGED, 150);
    defaults.insert(PetState::WALKING, 120);
    defaults.insert(PetState::SLEEPING, 500);
    defaults.insert(PetState::WORKING, 120);
    defaults.insert(PetState::CELEBRATING, 100);
    defaults.insert(PetState::SAD, 250);

    for(PetState s : states){
        QString dir = QStringLiteral(":/images/%1").arg(stateToString(s));
        addAnimationFromResourceDir(s, dir, defaults.value(s, 150));
    }
}

void AnimationManager::registerAnimations(){

    loadFromResources();
}

QPixmap AnimationManager::getFrame(PetState state,int frameIndex) const{
    if(!m_animationPixmaps.contains(state)){
        return QPixmap();
    }
    const QList<QPixmap> &list = m_animationPixmaps.value(state);
    if(frameIndex < 0 || frameIndex >= list.size()){
        return QPixmap();
    }
    return list.at(frameIndex);
}

int AnimationManager::getFrameCount(PetState state) const{
    if(m_animationPixmaps.contains(state)){
        return m_animationPixmaps.value(state).size();
    }
    return 0;
}

int AnimationManager::getFrameInterval(PetState state) const{
    return m_frameIntervalsMs.value(state, 150);
}


