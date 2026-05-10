#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include<QPixmap>
#include<QMap>
#include<QList>
#include<QRect>

enum class PetState{
    IDLE,
    DRAGGED,
    WALKING,
    SLEEPING
};

class AnimationManager
{
public:
    AnimationManager();

    //加载所有的素材
    void loadLionSheet(const QString &filePath);

    QPixmap getFrame(PetState state,int frameIndex);

    int getFrameCount(PetState state) const;

private:
    void registerAnimations();
    QPixmap m_lionSheet;
    QMap<PetState,QList<QRect>>m_animationFrames;
};

#endif // ANIMATIONMANAGER_H
