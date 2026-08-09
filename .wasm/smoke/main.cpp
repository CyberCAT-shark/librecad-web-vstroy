#include <QApplication>
#include <QPushButton>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QWidget window;
    window.resize(300, 200);
    window.setWindowTitle("WASM Smoke Test");
    QPushButton *btn = new QPushButton("Hello from Qt/WASM!", &window);
    btn->move(50, 80);
    window.show();
    return app.exec();
}
