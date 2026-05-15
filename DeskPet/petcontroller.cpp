#include "petcontroller.h"
#include <QGuiApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QWidget>
#include <QLineF>
#include <QtMath>
#include <QDebug>
#include <QVector>

PetController::PetController(QObject *parent)
    : QObject{parent},
      m_currentState(PetState::IDLE),
      m_currentFrameIndex(0),
      m_timer(nullptr),
      m_actionTimer(nullptr),
      m_isPlayingAction(false),
      m_sleepLoopsRemaining(0),
      m_forcedActionActive(false),
      m_forcedState(PetState::IDLE)
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

    // 进入拖拽时停止移动
    if(m_currentState == PetState::DRAGGED){
        m_isMoving = false;
    }

    // 进入WALKING时重置移动状态
    if(m_currentState == PetState::WALKING){
        m_isMoving = false;
    }

    // 如果进入 SLEEPING，并且不是被强制锁定（forced），就设为 3 次循环；否则清零
    if (newState == PetState::SLEEPING && !m_forcedActionActive) {
        m_sleepLoopsRemaining = 3;
    } else {
        m_sleepLoopsRemaining = 0;
    }

    // adjust timer interval for the new state's preferred frame duration
    int interval = m_animManager.getFrameInterval(newState);
    if(m_timer) m_timer->setInterval(interval > 0 ? interval : 150);

    emit frameUpdated();
}

PetState PetController::getCurrentState() const{
    return m_currentState;
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

void PetController::chooseRandomTarget(){
    QScreen *screen = QGuiApplication::primaryScreen();
    if(!screen){
        m_isMoving = false;
        return;
    }

    QRect geo = screen->availableGeometry();
    QWidget *w = qobject_cast<QWidget*>(parent());
    QSize winSize = w ? w->size() : QSize(180, 180);

    // 防止窗口尺寸超过可用屏幕时产生非法随机区间
    const int maxX = geo.right() - winSize.width();
    const int maxY = geo.bottom() - winSize.height();

    int left = geo.left();
    int top = geo.top();
    int right = qMax(left, maxX);
    int bottom = qMax(top, maxY);

    // QRandomGenerator::bounded(low, high) 的 high 是开区间，需要 +1
    int x = QRandomGenerator::global()->bounded(left, right + 1);
    int y = QRandomGenerator::global()->bounded(top, bottom + 1);

    m_targetPos = QPoint(x, y);
    m_isMoving = true;
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

    // 保留随机行走逻辑
    if (m_currentState == PetState::WALKING) {
        QWidget *w = qobject_cast<QWidget*>(parent());
        if (!w) {
            emit frameUpdated();
            return;
        }

        if (!m_isMoving) {
            chooseRandomTarget();
        }

        if (m_isMoving) {
            QPoint cur = w->frameGeometry().topLeft();
            QLineF line(cur, m_targetPos);
            double dist = line.length();

            if (dist <= m_speed || qFuzzyIsNull(dist)) {
                emit positionChanged(m_targetPos);
                m_isMoving = false;
                m_isPlayingAction = false;
                changeState(PetState::IDLE);
                if (m_actionTimer) m_actionTimer->start(5000);
            } else {
                double ratio = m_speed / dist;
                double nx = cur.x() + (m_targetPos.x() - cur.x()) * ratio;
                double ny = cur.y() + (m_targetPos.y() - cur.y()) * ratio;
                emit positionChanged(QPoint(qRound(nx), qRound(ny)));
            }
        }
    }

    emit frameUpdated();

}

void PetController::startRandomAction(){
    if(m_isPlayingAction || m_forcedActionActive) return; // already playing or forced

    // choose candidate action states (include IDLE, exclude DRAGGED)
    QVector<PetState> candidates;
    PetState choices[] = { PetState::IDLE, PetState::WALKING, PetState::SLEEPING, PetState::WORKING, PetState::CELEBRATING, PetState::SAD };
    for(PetState s : choices){
        if(s == PetState::WALKING || m_animManager.getFrameCount(s) > 0) candidates.append(s);
    }
    if(candidates.isEmpty()) return;

    int idx = QRandomGenerator::global()->bounded(candidates.size());
    PetState chosen = candidates.at(idx);

    m_isPlayingAction = (chosen != PetState::WALKING);
    if(m_actionTimer) m_actionTimer->stop();

    // 如果随机选择的就是当前状态，需要手动重置帧索引并处理状态相关计数
    if (chosen == m_currentState) {
        m_currentFrameIndex = 0;
        if (chosen == PetState::SLEEPING && !m_forcedActionActive) {
            m_sleepLoopsRemaining = 3;
        }
        int interval = m_animManager.getFrameInterval(chosen);
        if(m_timer) m_timer->setInterval(interval > 0 ? interval : 150);
        emit frameUpdated();
    } else {
        changeState(chosen);
    }
}

void PetController::setForcedState(PetState state){
    m_forcedActionActive = true;
    m_forcedState = state;
    m_isPlayingAction = false;
    if(m_actionTimer) m_actionTimer->stop();
    changeState(state);
}

void PetController::clearForcedState(){
    m_forcedActionActive = false;
    m_forcedState = PetState::IDLE;
    changeState(PetState::IDLE);
    if(m_actionTimer) m_actionTimer->start(5000);
}

bool PetController::hasForcedState() const {
    return m_forcedActionActive;
}

PetState PetController::forcedState() const {
    return m_forcedState;
}