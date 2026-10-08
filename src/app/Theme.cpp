#include "app/Theme.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

namespace {

struct Colors {
    const char *window, *panel, *field, *text, *muted, *accent, *accentText, *hover, *line, *danger, *success, *warning;
};

constexpr Colors DARK = {"#202020", "#2b2b2b", "#1c1c1c", "#ffffff", "#9d9d9d", "#3fc8bd",
                         "#000000", "#383838", "#3d3d3d", "#ff99a4", "#6ccb5f", "#fce100"};
constexpr Colors LIGHT = {"#f3f3f3", "#ffffff", "#fbfbfb", "#1a1a1a", "#5f5f5f", "#0f7676",
                          "#ffffff", "#e5e5e5", "#d4d4d4", "#c42b1c", "#0f7b0f", "#9d5d00"};

bool g_dark = true;

const Colors &current()
{
    return g_dark ? DARK : LIGHT;
}

} // namespace

namespace Theme {

void apply(QApplication &app, bool dark)
{
    g_dark = dark;
    const Colors &c = current();
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(c.window));
    palette.setColor(QPalette::WindowText, QColor(c.text));
    palette.setColor(QPalette::Base, QColor(c.field));
    palette.setColor(QPalette::AlternateBase, QColor(c.panel));
    palette.setColor(QPalette::Text, QColor(c.text));
    palette.setColor(QPalette::Button, QColor(c.panel));
    palette.setColor(QPalette::ButtonText, QColor(c.text));
    palette.setColor(QPalette::Highlight, QColor(c.accent));
    palette.setColor(QPalette::HighlightedText, QColor(c.accentText));
    palette.setColor(QPalette::ToolTipBase, QColor(c.panel));
    palette.setColor(QPalette::ToolTipText, QColor(c.text));
    palette.setColor(QPalette::PlaceholderText, QColor(c.muted));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(c.muted));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(c.muted));
    app.setPalette(palette);

    // %1… yer tutucuları renklerle doldurulur
    app.setStyleSheet(QString(R"(
        QWidget { font-family: "Segoe UI"; font-size: 10pt; }
        QPushButton { background: %2; border: 1px solid %4; border-radius: 4px; padding: 6px 14px; }
        QPushButton:hover { background: %3; }
        QPushButton:disabled { color: %5; }
        QPushButton[flat="true"] { background: transparent; border: none; }
        QPushButton[flat="true"]:hover { background: %3; }
        QPushButton[accent="true"] { background: %1; color: %6; border: none; font-weight: 600; }
        QPushButton[accent="true"]:disabled { background: %2; color: %5; border: 1px solid %4; font-weight: normal; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit, QPlainTextEdit {
            background: %7; border: 1px solid %4; border-radius: 4px; padding: 4px 6px; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QDateEdit:focus,
        QPlainTextEdit:focus { border: 1px solid %1; }
        QTableWidget { background: %7; border: 1px solid %4; gridline-color: %4; }
        QHeaderView::section { background: %2; color: %5; border: none; border-bottom: 1px solid %4;
            padding: 6px; font-weight: 600; }
        QListWidget#navigation { background: transparent; border: none; font-size: 11pt; }
        QListWidget#navigation::item { padding: 9px 12px; border-radius: 4px; }
        QListWidget#navigation::item:selected { background: %2; color: %1; }
        QListWidget#navigation::item:hover:!selected { background: %3; }
        QListWidget#slots { background: transparent; border: none; }
        QListWidget#slots::item { border: 1px solid %4; border-radius: 4px; margin: 3px; color: %1; font-weight: 600; }
        QListWidget#slots::item:hover { background: %3; }
        QListWidget#slots::item:selected { background: %1; color: %6; border: none; }
        QListWidget#slots::item:disabled { border: 1px dashed %4; color: %5; font-weight: normal; }
        QFrame#card, QPushButton#card { background: %2; border: 1px solid %4; border-radius: 6px; text-align: left; }
        QPushButton#card:hover { border: 1px solid %5; }
        QPushButton#card:checked { border: 2px solid %1; }
        QLabel#title { font-size: 18pt; font-weight: 600; }
        QLabel#cardValue { font-size: 20pt; font-weight: 600; }
        QLabel#muted, QLabel#cardLabel { color: %5; }
        QLabel#heading { font-size: 14pt; font-weight: 600; }
        QLabel#code { font-family: Consolas; font-size: 20pt; font-weight: 600; color: %1; }
        QLabel#mono, QLineEdit#mono { font-family: Consolas; font-size: 11pt; }
        QFrame#panel { background: %2; border: 1px solid %4; border-radius: 8px; }
        QProgressBar { background: %7; border: 1px solid %4; border-radius: 3px; max-height: 6px; }
        QToolButton { background: transparent; border: none; padding: 4px; border-radius: 4px; }
        QToolButton:hover { background: %3; }
        QSplitter::handle { background: transparent; }
    )")
                          .arg(c.accent, c.panel, c.hover, c.line, c.muted, c.accentText, c.field));
}

bool isDark()
{
    return g_dark;
}

QColor accent()
{
    return QColor(current().accent);
}

QColor muted()
{
    return QColor(current().muted);
}

QColor danger()
{
    return QColor(current().danger);
}

QColor success()
{
    return QColor(current().success);
}

QColor warning()
{
    return QColor(current().warning);
}

} // namespace Theme
