#include "petwindow.h"
#include <QPainter>
#include <QPixmap>
#include <QApplication>
#include <QMenu>
#include <QAction>

PetWindow::PetWindow(QWidget *parent)
    : QWidget(parent),
    m_isDragging(false)
{
    // 【核心属性】：设置为无边框窗口，并置顶
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);

    // 【核心属性】：设置窗口背景完全透明
    setAttribute(Qt::WA_TranslucentBackground);

    // 初始化窗口大小(与你的单帧素材大小一致)
    resize(180, 180);

    // 实例化控制器
    m_controller = new PetController(this);
    connect(m_controller, &PetController::frameUpdated, this, &PetWindow::onFrameUpdated);

    // 实例化托盘管理器
    m_trayManager = new TrayMenuManager(this);
}

PetWindow::~PetWindow()
{
}

void PetWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    // 开启抗锯齿
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QPixmap currentFrame = m_controller->getCurrentFrame();

    if (!currentFrame.isNull()) {
        // 如果成功获取到了图片，就绘制图片
        painter.drawPixmap(rect(), currentFrame);
    } else {
        static QPixmap s_lionPixmap(QStringLiteral(":/lion.png"));
        if(!s_lionPixmap.isNull()){
            painter.drawPixmap(rect(), s_lionPixmap);
        } else {
            // 回退到原来的黄色占位（仅在资源缺失时）
            painter.setBrush(QColor(255, 200, 0, 200)); // 半透明的黄色
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(rect());

            painter.setPen(Qt::black); // 这里必须设置画笔颜色为黑色，否则文字画不出来
            painter.drawText(rect(), Qt::AlignCenter, "载入\n素材中...");
        }
    }
}

void PetWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        // 记录鼠标相对窗口左上角的偏移量 (兼容Qt5和Qt6的写法)
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
#else
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
#endif

        // 状态切换：通知控制器进入“被拖拽”状态
        m_controller->startDrag();
        event->accept();
    }
}

void PetWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // 移动窗口到鼠标当前位置减去刚才计算的偏移量
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        move(event->globalPosition().toPoint() - m_dragPosition);
#else
        move(event->globalPos() - m_dragPosition);
#endif
        event->accept();
    }
}

void PetWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        // 状态切换：松开鼠标后恢复拖拽前状态
        m_controller->stopDrag();
        event->accept();
    }
}

void PetWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 示例：双击可以在这里改变状态，比如变成开心/举牌等
        // m_controller->changeState(PetState::CELEBRATING);
    }
}

void PetWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *defaultAct = menu.addAction("Default");
    menu.addSeparator();

    QAction *sleepAct = menu.addAction("SLEEPING");
    QAction *workAct = menu.addAction("WORKING");
    QAction *celebrateAct = menu.addAction("CELEBRATING");
    QAction *sadAct = menu.addAction("SAD");

    // 标记当前选择
    defaultAct->setCheckable(true);
    sleepAct->setCheckable(true);
    workAct->setCheckable(true);
    celebrateAct->setCheckable(true);
    sadAct->setCheckable(true);

    bool hasForced = m_controller->hasForcedState();
    defaultAct->setChecked(!hasForced);
    if (hasForced) {
        PetState fs = m_controller->forcedState();
        sleepAct->setChecked(fs == PetState::SLEEPING);
        workAct->setChecked(fs == PetState::WORKING);
        celebrateAct->setChecked(fs == PetState::CELEBRATING);
        sadAct->setChecked(fs == PetState::SAD);
    }

    QAction *selected = menu.exec(event->globalPos());
    if (!selected) return;

    if (selected == defaultAct) {
        m_controller->clearForcedState();
    } else if (selected == sleepAct) {
        m_controller->setForcedState(PetState::SLEEPING);
    } else if (selected == workAct) {
        m_controller->setForcedState(PetState::WORKING);
    } else if (selected == celebrateAct) {
        m_controller->setForcedState(PetState::CELEBRATING);
    } else if (selected == sadAct) {
        m_controller->setForcedState(PetState::SAD);
    }

    event->accept();
}

void PetWindow::onFrameUpdated()
{
    // 收到控制器发出的刷新信号后，触发 paintEvent 重绘
    update();
}