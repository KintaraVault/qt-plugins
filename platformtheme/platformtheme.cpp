#include "platformtheme.h"
#include "filedialoghelper.h"
#include "touchpadscrolling.h"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QString>
#include <QVariant>
#include <QWindow>

// Qt Private
#include <private/qiconloader_p.h>
#include <qpa/qwindowsysteminterface.h>

static QPalette cutefishPalette(bool dark)
{
    const QColor window = QColor::fromRgb(dark ? 0x292929 : 0xF6F6F6);
    const QColor base = QColor::fromRgb(dark ? 0x1E1E1E : 0xFFFFFF);
    const QColor text = QColor::fromRgb(dark ? 0xFFFFFF : 0x262626);
    const QColor disabledText = QColor::fromRgb(dark ? 0x969696 : 0x707070);
    const QColor button = QColor::fromRgb(dark ? 0x383838 : 0xEEEEEE);
    // Selection text needs more contrast than the brighter accent used for links.
    const QColor accent = QColor::fromRgb(dark ? 0x0091FF : 0x0069DD);
    const QColor highlight = QColor::fromRgb(0x0069FA);

    QPalette palette(window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, dark ? window : button);
    palette.setColor(QPalette::ToolTipBase, window);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::PlaceholderText, disabledText);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::Accent, accent);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Light, window.lighter(150));
    palette.setColor(QPalette::Midlight, window.lighter(125));
    palette.setColor(QPalette::Mid, window.darker(125));
    palette.setColor(QPalette::Dark, window.darker(150));
    palette.setColor(QPalette::Shadow, Qt::black);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, window.lighter(130));
    return palette;
}

PlatformTheme::PlatformTheme()
    : m_hints(new HintsSettings)
{
    connect(m_hints, &HintsSettings::systemFontChanged, this, &PlatformTheme::onFontChanged);
    connect(m_hints, &HintsSettings::systemFontPointSizeChanged, this,
            &PlatformTheme::onFontChanged);
    connect(m_hints, &HintsSettings::iconThemeChanged, this, &PlatformTheme::onIconThemeChanged);
    connect(m_hints, &HintsSettings::cursorThemeChanged, this, [] {
        QWindowSystemInterface::handleThemeChange();
    });
    connect(m_hints, &HintsSettings::darkModeChanged, this, [] {
        QWindowSystemInterface::handleThemeChange();
    });

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, false);
    // The application object is still being constructed.
    QMetaObject::invokeMethod(this, &TouchpadScrolling::install, Qt::QueuedConnection);
}

PlatformTheme::~PlatformTheme()
{
    delete m_hints;
}

QVariant PlatformTheme::themeHint(QPlatformTheme::ThemeHint hintType) const
{
    QVariant hint = m_hints->hint(hintType);
    if (hint.isValid()) {
        return hint;
    } else {
        return QGenericUnixTheme::themeHint(hintType);
    }
}

Qt::ColorScheme PlatformTheme::colorScheme() const
{
    return m_hints->darkMode() ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light;
}

const QPalette *PlatformTheme::palette(Palette type) const
{
    if (type == SystemPalette) {
        static const QPalette light = cutefishPalette(false);
        static const QPalette dark = cutefishPalette(true);
        return m_hints->darkMode() ? &dark : &light;
    }

    return QGenericUnixTheme::palette(type);
}

const QFont *PlatformTheme::font(Font type) const
{
    if (type == FixedFont) {
        const QString &fontName = m_hints->systemFixedFont();
        qreal fontSize = m_hints->systemFontPointSize();
        static QFont font = QFont(QString());
        font.setFamily(fontName);
        font.setPointSizeF(fontSize);
        return &font;
    }

    const QString &fontName = m_hints->systemFont();
    qreal fontSize = m_hints->systemFontPointSize();
    static QFont font = QFont(QString());
    font.setFamily(fontName);
    font.setPointSizeF(fontSize);
    return &font;
}

bool PlatformTheme::usePlatformNativeDialog(DialogType type) const
{
    return type == FileDialog;
}

QPlatformDialogHelper *PlatformTheme::createPlatformDialogHelper(DialogType type) const
{
    if (type == FileDialog)
        return new FileDialogHelper;
    return QGenericUnixTheme::createPlatformDialogHelper(type);
}

void PlatformTheme::onFontChanged()
{
    QFont font;
    font.setFamily(m_hints->systemFont());
    font.setPointSizeF(m_hints->systemFontPointSize());

    // Change font
    if (qobject_cast<QApplication *>(QCoreApplication::instance()))
        QApplication::setFont(font);
    else if (qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
        QGuiApplication::setFont(font);
}

void PlatformTheme::onIconThemeChanged()
{
    QIconLoader::instance()->updateSystemTheme();

    QEvent update(QEvent::UpdateRequest);
    for (QWindow *window : qGuiApp->allWindows()) {
        if (window->type() == Qt::Desktop)
            continue;

        qApp->sendEvent(window, &update);
    }
}
