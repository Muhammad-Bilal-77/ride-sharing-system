#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Set window size
    this->resize(1000, 700);

    // Set gradient background
    this->setStyleSheet(
        "QMainWindow { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #FFFFFF, stop:1 #E8F5E9);"
        "}"
    );

    // Style the title
    ui->labelTitle->setStyleSheet(
        "color: #2E7D32;"
        "font-size: 32px;"
        "font-weight: bold;"
        "padding: 20px;"
        "background: transparent;"
    );

    // Create container for button and image
    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background: transparent;");
    
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(60, 30, 60, 60);
    contentLayout->setSpacing(60);

    // LEFT: Button
    chooseUserButton = new QPushButton("Choose User");
    chooseUserButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 15px;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    min-width: 240px;"
        "    min-height: 80px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #66BB6A;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #388E3C;"
        "}"
    );
    chooseUserButton->setCursor(Qt::PointingHandCursor);

    // RIGHT: Image
    imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    
    // Load image
    QString imgPath = QCoreApplication::applicationDirPath() + "/Maskgroup.png";
    QFileInfo imgInfo(imgPath);
    
    if (!imgInfo.exists()) {
        QDir exeDir(QCoreApplication::applicationDirPath());
        exeDir.cdUp();
        exeDir.cdUp();
        imgPath = exeDir.absoluteFilePath("Maskgroup.png");
        imgInfo.setFile(imgPath);
    }
    
    qDebug() << "Loading image from:" << imgPath << "Exists:" << imgInfo.exists();
    
    if (imgInfo.exists()) {
        QPixmap pixmap(imgPath);
        if (!pixmap.isNull()) {
            imageLabel->setPixmap(pixmap.scaled(500, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            qDebug() << "Image loaded successfully, size:" << pixmap.size();
        } else {
            imageLabel->setText("Failed to load image");
            qDebug() << "Failed to load pixmap";
        }
    } else {
        imageLabel->setText("Image not found");
        imageLabel->setStyleSheet("color: red; font-size: 18px;");
        qDebug() << "Image file not found";
    }

    // Add to content layout with equal spacing
    contentLayout->addStretch(1);
    contentLayout->addWidget(chooseUserButton, 0, Qt::AlignCenter);
    contentLayout->addStretch(1);
    contentLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
    contentLayout->addStretch(1);

    // Add content to main vertical layout (from ui file)
    ui->verticalLayout->addWidget(contentWidget, 1);
}

MainWindow::~MainWindow()
{
    delete ui;
}
