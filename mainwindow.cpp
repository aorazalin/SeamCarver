#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include "SeamCarver.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(tr("Seam carving"));

    ui->graphicsView->setScene(&scene);
    ui->graphicsView->show();

    connect(ui->graphicsView, &drawingview::draw_image, this, &MainWindow::receive_and_draw);

    ui->actionRemove->setVisible(false);
    ui->actionCancel->setVisible(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionImport_image_triggered()
{

    QString filename = QFileDialog::getOpenFileName(this, "Import image", QDir::currentPath(), "Images (*.png *.jpg *.bmp)");
    if (filename == "") return;

    delete_picture();
    String filepath = filename.toStdString();

    scene.addPixmap(QPixmap(filename));

    Mat image = imread(filepath, IMREAD_COLOR);
    height = image.rows;
    width = image.cols;
    ui->graphicsView->setFixedSize(width + 10, height + 10);

    can_rescale = true;
    can_export = false;

    seam_carver = new SeamCarver<Vec3b, uint8_t>(filepath, IMREAD_COLOR);
}

void MainWindow::create_marker() {
    chosen = new bool* [height];
    for (int i = 0; i < height; ++i) {
        chosen[i] = new bool[width];
        for (int j = 0; j < width; j++) {
            chosen[i][j] = false;
        }
    }
}


void MainWindow::delete_marker() {
    if (chosen == nullptr) return;
    for (int i = 0; i < height; ++i) {
        delete[] chosen[i];
    }
    delete[] chosen;
    chosen = nullptr;
    for (int i = 0; i < marker_tracked.size(); ++i) {
        delete marker_tracked[i];
    }
    marker_tracked.clear();
}

void MainWindow::receive_and_draw(QPoint p1, QPoint p2, QPen pen) {
    if (p1.x() >= ui->graphicsView->width() - 10 || p1.y() >= ui->graphicsView->height() - 10
        || p2.x() >= (ui->graphicsView->width() - 10) || p2.y() >= (ui->graphicsView->height() - 10)
        || p1.x() <= 10 || p1.y() <= 10 || p2.x() <= 10 || p2.y() <= 10
        || marker_disabled) return;
    marker_tracked.push_back(scene.addLine(QLine(p1, p2), pen));
    chosen[p1.y()][p1.x()] = true;
}



void MainWindow::on_actionClear_image_triggered()
{
    delete_marker();
    delete_picture();
    scene.clear();
    can_export = can_rescale = false;
}

void MainWindow::delete_picture() {
    if (!can_rescale) return;
    delete seam_carver;
    seam_carver = nullptr;
}

void MainWindow::on_actionResize_triggered()
{
    if (!can_rescale) return;
    dialog = new Dialog();
    dialog->setModal(true);
    dialog->show();
    connect(dialog, &Dialog::signal_to_resize, this, &MainWindow::rescale_image);
}

void MainWindow::rescale_image() {
    seam_carver->rescale(dialog->get_width(), dialog->get_height());
    seam_carver->get_pic().copyTo(image_to_export); 
    delete_picture();
    scene.clear();
    can_rescale = false;
    can_export = true;
    imshow("scaled", image_to_export);
}

void MainWindow::on_actionExport_image_triggered()
{
    if (!can_export) return;
    QString filename = QFileDialog::getSaveFileName(this, "Export image", QDir::currentPath(), "Images (*png *.jpg *bmp)");
    if (filename == "") return;
    imwrite(filename.toStdString(), image_to_export);
    can_export = false;
}

void MainWindow::on_actionObject_removal_triggered()
{
    if (!can_rescale) return;

    create_marker();
    ui->actionRemove->setVisible(true);
    ui->actionCancel->setVisible(true);

    ui->actionExport_image->setDisabled(true);
    ui->actionImport_image->setDisabled(true);
    ui->actionClear_image->setDisabled(true);
    ui->actionResize->setDisabled(true);

    // turn on the marker
    marker_disabled = false;
}

void MainWindow::cancel_drawing() {
    ui->actionRemove->setVisible(false);
    ui->actionCancel->setVisible(false);

    ui->actionExport_image->setDisabled(false);
    ui->actionImport_image->setDisabled(false);
    ui->actionClear_image->setDisabled(false);
    ui->actionResize->setDisabled(false);

    // turn of the marker
    marker_disabled = true;

    // delete marked stuff
    delete_marker();
}

void MainWindow::on_actionRemove_triggered() {
    // assert can_rescale = true
    seam_carver->remove_object(chosen);
    seam_carver->get_pic().copyTo(image_to_export);
    imshow("removed", image_to_export);
    delete_picture();
    can_rescale = false;
    can_export = true;
    ui->actionRemove->setVisible(false);
    cancel_drawing();
    scene.clear();
}

void MainWindow::on_actionCancel_triggered() {
    cancel_drawing();
}

void MainWindow::on_actionProject_triggered()
{
    QMessageBox::information(this, tr("About the project"),
        tr("This project implements <br> seam carving algorithm <br> for content-aware resizing"));
}

void MainWindow::on_actionAuthors_triggered()
{
    QMessageBox::information(this, tr("Authors"),
        tr("This project was done by <br> Alibek, Thanic, Rubab"));
}


