#include "MainWindow.hpp"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    // ── 现代深色 QSS 主题 ──
    app.setStyleSheet(R"(
        QMainWindow, QDialog, QWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
        }
        QPushButton {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3c3c3c;
            padding: 6px 16px;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #3c3c3c;
            border-color: #007acc;
        }
        QPushButton:pressed {
            background-color: #505050;
        }
        QTableWidget {
            background-color: #1e1e20;
            color: #d4d4d4;
            gridline-color: #2d2d30;
            border: 1px solid #3c3c3c;
            alternate-background-color: #252528;
            selection-background-color: #1a3d5c;
            selection-color: #ffffff;
            border-radius: 6px;
        }
        QTableWidget::item {
            padding: 6px 10px;
            border-bottom: 1px solid #2d2d30;
        }
        QTableWidget::item:hover {
            background-color: #2a3a4a;
        }
        QTableWidget::item:selected {
            background-color: #094771;
            border-left: 3px solid #007acc;
        }
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #333340, stop:1 #2a2a30);
            color: #c0c0c0;
            border: none;
            border-right: 1px solid #3c3c3c;
            border-bottom: 2px solid #007acc;
            padding: 8px 10px;
            font-weight: bold;
            font-size: 13px;
        }
        QPlainTextEdit {
            background-color: #1e1e1e;
            color: #cccccc;
            border: 1px solid #3c3c3c;
        }
        QLineEdit, QComboBox, QDateEdit, QSpinBox {
            background-color: #3c3c3c;
            color: #d4d4d4;
            border: 1px solid #555555;
            padding: 6px 10px;
            border-radius: 4px;
        }
        QLineEdit:focus, QComboBox:focus {
            border-color: #007acc;
        }
        QTextEdit {
            background-color: #1a1a1c;
            color: #c8c8c8;
            border: 1px solid #333333;
            border-radius: 4px;
            font-family: "Consolas", "Courier New", monospace;
            font-size: 12px;
        }
        QComboBox::drop-down { border: none; width: 20px; }
        QComboBox QAbstractItemView {
            background-color: #3c3c3c;
            color: #d4d4d4;
            selection-background-color: #007acc;
        }
        QLabel, QGroupBox { color: #d4d4d4; }
        QGroupBox {
            border: 1px solid #3c3c3c;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 16px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }
        QScrollBar:vertical {
            background-color: #1e1e1e;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background-color: #555555;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar:horizontal {
            background-color: #1e1e1e;
            height: 10px;
        }
        QScrollBar::handle:horizontal {
            background-color: #555555;
            border-radius: 5px;
            min-width: 20px;
        }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal { width: 0px; }
        QDialog QPushButton {
            min-width: 80px;
        }
    )");

    MainWindow w;
    w.resize(1200, 750);
    w.show();

    return app.exec();
}
