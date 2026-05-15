#ifndef ANIMATIONMANAGER_H
#define ANIMATIONMANAGER_H

#include <QPixmap>
#include <QMap>
#include <QList>
#include <QStringList>

enum class PetState{
    IDLE,
    DRAGGED,
    WALKING,
    SLEEPING,
    WORKING,
    CELEBRATING,
    SAD
};

class AnimationManager
{
public:
    AnimationManager();

    // 兼容旧的精灵表加载（保留以便回退）
    void loadLionSheet(const QString &filePath);

    // 从资源目录预加载按状态的图片帧
    void loadFromResources();
    void addAnimationFromResourceDir(PetState state, const QString &resourceDir, int intervalMs = 150);
    void addAnimation(PetState state, const QStringList &resourcePaths, int intervalMs = 150);

    QPixmap getFrame(PetState state,int frameIndex) const;

    int getFrameCount(PetState state) const;
    int getFrameInterval(PetState state) const;

private:
    void registerAnimations();

    QMap<PetState,QList<QPixmap>> m_animationPixmaps;
    QMap<PetState,int> m_frameIntervalsMs;
    QPixmap m_lionSheet; // fallback for backward compatibility
};

#endif // ANIMATIONMANAGER_H
