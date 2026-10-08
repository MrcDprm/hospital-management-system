#pragma once

class QApplication;
class QColor;

// Koyu ve açık tema: Qt'nin "Fusion" stiline renk paleti ve küçük bir stil sayfası uygulanır.
namespace Theme {

void apply(QApplication &app, bool dark);
bool isDark();
QColor accent();
QColor muted();
QColor danger();
QColor success();
QColor warning();

} // namespace Theme
