#include "ui/AboutDialog.h"

#include "app/I18n.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>
#include <QVBoxLayout>

AboutDialog::AboutDialog(const QString &dataFolder, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::t("about"));
    auto *layout = new QVBoxLayout(this);
    auto *icon = new QLabel(this);
    icon->setPixmap(QPixmap(":/icon.png").scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setAlignment(Qt::AlignCenter);
    auto *title = new QLabel(I18n::t("app_name"), this);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    auto *version = new QLabel(I18n::t("version").replace("{0}", APP_VERSION), this);
    version->setAlignment(Qt::AlignCenter);
    auto *text = new QLabel(I18n::t("about_text"), this);
    text->setWordWrap(true);
    text->setAlignment(Qt::AlignCenter);
    auto *folder = new QLabel(I18n::t("data_folder") + ": " + QDir::toNativeSeparators(dataFolder), this);
    folder->setObjectName("muted");
    folder->setWordWrap(true);
    folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
    // Link sabit; Qt sadece bu adresi tarayıcıda açar
    auto *link = new QLabel("<a href='https://github.com/MrcDprm/hospital-management-system'>"
                                + I18n::t("view_on_github") + "</a>",
                            this);
    link->setOpenExternalLinks(true);
    link->setAlignment(Qt::AlignCenter);
    auto *credits = new QLabel(I18n::t("credits"), this);
    credits->setObjectName("muted");
    credits->setWordWrap(true);
    credits->setAlignment(Qt::AlignCenter);
    auto *copyright = new QLabel("© 2026 Miraç Deprem", this);
    copyright->setAlignment(Qt::AlignCenter);
    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(I18n::t("close"), QDialogButtonBox::RejectRole);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    for (QWidget *widget :
         std::initializer_list<QWidget *>{icon, title, version, text, folder, link, credits, copyright, buttons})
        layout->addWidget(widget);
    setFixedWidth(440);
}
