#include "petcontroller.h"
#include <QGuiApplication>
#include <QScreen>
#include <QRandomGenerator>
#include <QWidget>
#include <QLineF>
#include <QtMath>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QStringList>

PetController::PetController(QObject *parent)
    : QObject{parent},m_currentState(PetState::IDLE),m_currentFrameIndex(0)
{
    // 尝试多个可能的路径加载 lion.png
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList possiblePaths;
    
    // 路径1: build 的父目录（项目根）
    possiblePaths << QDir(appDir).absoluteFilePath("../lion.png");
    
    // 路径2: build 同级目录
    possiblePaths << QFileInfo(appDir).dir().absoluteFilePath("lion.png");
    
    // 路径3: 当前工作目录
    possiblePaths << QDir::currentPath() + "/lion.png";
    
    // 路径4: 应用程序所在目录
    possiblePaths << appDir + "/lion.png";
    
    QString loadedPath;
    for(const QString &path : possiblePaths){
        QFileInfo fi(path);
        qDebug() << "Trying to load from:" << fi.absoluteFilePath() << "Exists:" << fi.exists();
        if(fi.exists()){
            m_animManager.loadLionSheet(fi.absoluteFilePath());
            loadedPath = fi.absoluteFilePath();
            qDebug() << "Successfully loaded from:" << loadedPath;
            break;
        }
    }
    
    if(loadedPath.isEmpty()){
        qWarning() << "Failed to load lion.png from any path!";
        qWarning() << "App dir:" << appDir;
        qWarning() << "Current dir:" << QDir::currentPath();
    }

    m_timer = new QTimer(this);
    connect(m_timer,&QTimer::timeout,this,&PetController::updateLogic);

    m_timer->start(150); // 150ms 更新一次
}

void PetController::changeState(PetState newState){
    if(m_currentState == newState){
        return;
    }

    m_currentState = newState;
    m_currentFrameIndex = 0;
    m_stateTimerCounter = 0;

    // 进入拖拽时停止移动
    if(m_currentState == PetState::DRAGGED){
        m_isMoving = false;
    }

    // 进入WALKING时重置移动状态
    if(m_currentState == PetState::WALKING){
        m_isMoving = false;
        m_walkStepCount = 0;
    }

    emit frameUpdated();
}

PetState PetController::getCurrentState() const{
    return m_currentState;
}

QPixmap PetController::getCurrentFrame() {
    return m_animManager.getFrame(m_currentState,m_currentFrameIndex);
}

// 随机选择一个非DRAGGED状态（IDLE、SLEEPING、WALKING等概率）
PetState getRandomState(){
    int choice = QRandomGenerator::global()->bounded(3);
    if(choice == 0) return PetState::IDLE;
    if(choice == 1) return PetState::WALKING;//It errors when switch to SLEEPING,so i temporarily deleted it
    return PetState::WALKING;
}

// 随机选择目标点，避免靠近屏幕中心
void chooseRandomTarget_internal(PetController *self){
    QScreen *screen = QGuiApplication::primaryScreen();
    if(!screen) return;
    QRect geo = screen->availableGeometry();

    QWidget *w = qobject_cast<QWidget*>(self->parent());
    QSize winSize = w ? w->size() : QSize(180,180);

    // 可选区域为屏幕减去窗口尺寸
    QRect allowed(geo.topLeft(), geo.size());
    allowed.setRight(allowed.right() - winSize.width());
    allowed.setBottom(allowed.bottom() - winSize.height());

    QPoint center = geo.center();
    double centerRadius = qMin(geo.width(), geo.height()) * 0.25; // 中心排除半径

    // 多次尝试以找到符合条件的点
    for(int i=0;i<30;i++){
        int x = QRandomGenerator::global()->bounded(allowed.left(), allowed.right()+1);
        int y = QRandomGenerator::global()->bounded(allowed.top(), allowed.bottom()+1);
        QPoint cand(x,y);
        QLineF line(cand, center);
        if(line.length() >= centerRadius){
            self->m_targetPos = cand;
            self->m_isMoving = true;
            return;
        }
    }

    // 如果没找到合适的点，就退而求其次随机选一个
    int x = QRandomGenerator::global()->bounded(allowed.left(), allowed.right()+1);
    int y = QRandomGenerator::global()->bounded(allowed.top(), allowed.bottom()+1);
    self->m_targetPos = QPoint(x,y);
    self->m_isMoving = true;
}

void PetController::updateLogic(){
    // 帧切换：每状态两帧间循环
    int frameCount = m_animManager.getFrameCount(m_currentState);
    if(frameCount > 0){
        m_currentFrameIndex = (m_currentFrameIndex + 1) % frameCount;
    }

    // 行走状态逻辑：走一次（到达目标）后随机切换
    if(m_currentState == PetState::WALKING){
        QWidget *w = qobject_cast<QWidget*>(parent());
        if(!w){
            emit frameUpdated();
            return;
        }

        if(!m_isMoving){
            // 选择新目标
            chooseRandomTarget_internal(this);
        }

        if(m_isMoving){
            QPoint cur = w->frameGeometry().topLeft();
            QLineF line(cur, m_targetPos);
            double dist = line.length();
            if(dist <= m_speed || qFuzzyIsNull(dist)){
                // 到达目标，随机切换到三个状态之一
                emit positionChanged(m_targetPos);
                m_isMoving = false;
                PetState nextState = getRandomState();
                changeState(nextState);
                m_walkStepCount = 0;
            } else {
                double ratio = m_speed / dist;
                double nx = cur.x() + (m_targetPos.x() - cur.x()) * ratio;
                double ny = cur.y() + (m_targetPos.y() - cur.y()) * ratio;
                QPoint newPos(qRound(nx), qRound(ny));
                emit positionChanged(newPos);
            }
        }
    }
    // 空闲状态逻辑：每隔约10秒（约67次更新）随机切换到三个状态之一
    else if(m_currentState == PetState::IDLE){
        m_stateTimerCounter++;
        const int TEN_SECONDS_UPDATES = 67; // 10000ms / 150ms ≈ 67
        if(m_stateTimerCounter >= TEN_SECONDS_UPDATES){
            m_stateTimerCounter = 0;
            PetState nextState = getRandomState();
            changeState(nextState);
        }
    }
    // 睡觉状态逻辑：每隔约10秒随机切换到三个状态之一
    else if(m_currentState == PetState::SLEEPING){
        m_stateTimerCounter++;
        const int TEN_SECONDS_UPDATES = 67; // 10000ms / 150ms ≈ 67
        if(m_stateTimerCounter >= TEN_SECONDS_UPDATES){
            m_stateTimerCounter = 0;
            PetState nextState = getRandomState();
            changeState(nextState);
        }
    }
    // 拖拽状态：仅显示，无需额外逻辑

    emit frameUpdated();

}