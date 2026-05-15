#include "petcontroller.h"
#include <QRandomGenerator>
#include <QDebug>

PetController::PetController(QObject *parent)
    : QObject{parent},m_currentState(PetState::IDLE),m_currentFrameIndex(0),m_actionTimer(nullptr),m_isPlayingAction(false),m_sleepLoopsRemaining(0)
{
    // Load per-state frames from resources (will fall back to sprite sheet if needed)
    m_animManager.loadFromResources();

    // load default lion pixmap from resources
    if(!m_lionPixmap.load(":/lion.png")){
        qWarning() << "Failed to load :/lion.png";
    }

    m_timer = new QTimer(this);
    connect(m_timer,&QTimer::timeout,this,&PetController::updateLogic);

    // set initial interval based on current state
    int initialInterval = m_animManager.getFrameInterval(m_currentState);
    m_timer->start(initialInterval > 0 ? initialInterval : 150);

    // action timer: every 5 seconds pick a random action to play
    m_actionTimer = new QTimer(this);
    connect(m_actionTimer, &QTimer::timeout, this, &PetController::startRandomAction);
    m_actionTimer->start(5000);
}

void PetController::changeState(PetState newState){
    if (m_currentState == newState) {
        return;
    }

    m_currentState = newState;
    m_currentFrameIndex = 0;

    // 如果进入 SLEEPING，就设为 3 次循环；否则清零
    if (newState == PetState::SLEEPING) {
        m_sleepLoopsRemaining = 3;
    } else {
        m_sleepLoopsRemaining = 0;
    }

    // adjust timer interval for the new state's preferred frame duration
    int interval = m_animManager.getFrameInterval(newState);
    if(m_timer) m_timer->setInterval(interval > 0 ? interval : 150);

    emit frameUpdated();
}

QPixmap PetController::getCurrentFrame() {
    if(m_currentState == PetState::IDLE && !m_isPlayingAction){
        if(!m_lionPixmap.isNull()) return m_lionPixmap;
        // fallback to idle frames if lion resource missing
        if(m_animManager.getFrameCount(PetState::IDLE) > 0)
            return m_animManager.getFrame(PetState::IDLE, m_currentFrameIndex);
        return QPixmap();
    }
    return m_animManager.getFrame(m_currentState,m_currentFrameIndex);
}


void PetController::updateLogic(){
    int frameCount = m_animManager.getFrameCount(m_currentState);
    if (frameCount > 0) {
        int nextIndex = (m_currentFrameIndex + 1) % frameCount;
        m_currentFrameIndex = nextIndex;

        // 如果当前状态是 SLEEPING，使用循环计数控制停留次数（每次回到第0帧计为一圈）
        if (m_currentState == PetState::SLEEPING && m_currentFrameIndex == 0 && m_sleepLoopsRemaining > 0) {
            --m_sleepLoopsRemaining;
            if (m_sleepLoopsRemaining == 0) {
                m_isPlayingAction = false;
                changeState(PetState::IDLE);
                if (m_actionTimer) m_actionTimer->start(5000);
            }
        }
        // 其他一次性动作播放结束的判断（回到第0帧表示动作完成）
        else if (m_isPlayingAction && m_currentFrameIndex == 0) {
            m_isPlayingAction = false;
            changeState(PetState::IDLE);
            if (m_actionTimer) m_actionTimer->start(5000);
        }
    }

    // 其它的一些用于状态转换的逻辑用于后来的拓展部分

    emit frameUpdated();

}

void PetController::startRandomAction(){
    if(m_isPlayingAction) return; // already playing

    // choose candidate action states (exclude IDLE and DRAGGED)
    QVector<PetState> candidates;
    PetState choices[] = { PetState::SLEEPING, PetState::WORKING, PetState::CELEBRATING, PetState::SAD };
    for(PetState s : choices){
        if(m_animManager.getFrameCount(s) > 0) candidates.append(s);
    }
    if(candidates.isEmpty()) return;

    int idx = QRandomGenerator::global()->bounded(candidates.size());
    PetState chosen = candidates.at(idx);

    m_isPlayingAction = true;
    if(m_actionTimer) m_actionTimer->stop();
    changeState(chosen);
}