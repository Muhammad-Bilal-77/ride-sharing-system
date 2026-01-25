#include "mainwindow.h"
#include "resourcemanager.h"

#include <QApplication>
#include <QWidget>
#include <QVideoWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVBoxLayout>
#include <QTimer>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Create preloader window
    QWidget *preloader = new QWidget();
    preloader->setWindowTitle("Loading...");
    preloader->setFixedSize(800, 600);
    preloader->setStyleSheet("background-color: black;");
    preloader->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);  // Remove window frame for cleaner look
    
    // Create video widget
    QVideoWidget *videoWidget = new QVideoWidget(preloader);
    videoWidget->setAspectRatioMode(Qt::IgnoreAspectRatio);  // Fill entire window
    
    // Create layout
    QVBoxLayout *layout = new QVBoxLayout(preloader);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(videoWidget);
    preloader->setLayout(layout);
    
    // Add fade-in animation
    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(preloader);
    preloader->setGraphicsEffect(opacityEffect);
    
    QPropertyAnimation *fadeIn = new QPropertyAnimation(opacityEffect, "opacity");
    fadeIn->setDuration(1000);  // 1 second fade-in
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);
    
    // Create media player
    QMediaPlayer *player = new QMediaPlayer(preloader);
    QAudioOutput *audioOutput = new QAudioOutput(preloader);
    player->setAudioOutput(audioOutput);
    player->setVideoOutput(videoWidget);
    
    // Get absolute path to video file using portable resource manager
    QString videoPath = ResourceManager::getResourcePath("preloader_ride_sharing_system.mp4");
    
    if (!videoPath.isEmpty()) {
        qDebug() << "Video file found at:" << videoPath;
        player->setSource(QUrl::fromLocalFile(videoPath));
    } else {
        qDebug() << "Video file NOT found!";
        qDebug() << "Current directory:" << QDir::currentPath();
        qDebug() << "Application directory:" << QCoreApplication::applicationDirPath();
    }
    
    // Connect error handling
    QObject::connect(player, &QMediaPlayer::errorOccurred, [](QMediaPlayer::Error error, const QString &errorString) {
        qDebug() << "Media Player Error:" << error << errorString;
    });
    
    // Create main window
    MainWindow *w = new MainWindow();
    
    // Create fade-out animation for preloader
    QPropertyAnimation *fadeOut = new QPropertyAnimation(opacityEffect, "opacity");
    fadeOut->setDuration(800);  // 0.8 second fade-out
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InOutQuad);
    
    // Connect to media status changed to detect when video finishes
    QObject::connect(player, &QMediaPlayer::mediaStatusChanged, [player, fadeOut, w, preloader, fadeIn](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            qDebug() << "Video finished playing";
            // Start fade-out animation
            fadeOut->start();
        }
    });
    
    // Connect fade-out finished to close preloader and show main window
    QObject::connect(fadeOut, &QPropertyAnimation::finished, [preloader, player, w, fadeIn, fadeOut]() {
        preloader->close();
        delete fadeIn;
        delete fadeOut;
        delete player;
        delete preloader;
        w->show();
    });
    
    // Show preloader and start animations
    preloader->show();
    fadeIn->start();
    player->setPlaybackRate(1.5);  // 1.5x speed
    player->play();
    
    return a.exec();
}
