#include "ToolsModule.h"
#include "../../../gui/CollapsibleHelper.h"
#include "../../../utils/AppPaths.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSettings>
#include <QTimer>

ToolsModule::ToolsModule(const QString &tabGroup, QObject *parent)
    : QObject(parent), m_tabGroup(tabGroup)
{
}

void ToolsModule::setup(QList<CollapsibleHelper *> &helpers, std::function<void()> adjustWindowCallback)
{
    m_tab = new QWidget;

    auto *mainLayout = new QVBoxLayout(m_tab);

    // ========== Image Sequence to Video Converter ==========
    auto *videoConverterGroup = new QGroupBox(tr("Image Sequence to Video Converter"), m_tab);
    videoConverterGroup->setStyleSheet(
        "QGroupBox {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    margin-top: 1ex;"
        "    padding-top: 10px;"
        "    font-weight: bold;"
        "    background-color: #f8f9fa;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    subcontrol-position: top left;"
        "    left: 10px;"
        "    padding: 0 5px;"
        "}"
    );
    auto *converterLayout = new QGridLayout(videoConverterGroup);
    converterLayout->setColumnMinimumWidth(0, 150);
    converterLayout->setColumnStretch(1, 1);

    int row = 0;

    // Input Directory
    converterLayout->addWidget(new QLabel(tr("Input Directory:"), m_tab), row, 0);
    m_inputDirInput = new QLineEdit(m_tab);
    m_inputDirInput->setPlaceholderText(tr("Select folder containing image sequence..."));
    converterLayout->addWidget(m_inputDirInput, row, 1);
    m_inputDirBrowseButton = new QPushButton(tr("Browse..."), m_tab);
    m_inputDirBrowseButton->setFixedWidth(80);
    connect(m_inputDirBrowseButton, &QPushButton::clicked, this, &ToolsModule::browseInputDir);
    converterLayout->addWidget(m_inputDirBrowseButton, row, 2);
    row++;

    // Image Pattern - Editable ComboBox with context menu
    converterLayout->addWidget(new QLabel(tr("Image Pattern:"), m_tab), row, 0);
    m_patternSelector = new QComboBox(m_tab);
    m_patternSelector->setEditable(true);
    m_patternSelector->setFixedWidth(200);
    m_patternSelector->setToolTip(tr("Pattern for image files\nUse %04d for numbered sequences or *.jpg for wildcards\nRight-click to add or delete values"));

    // Load saved custom patterns
    QSettings patternSettings(AppPaths::globalConf(), QSettings::IniFormat);
    patternSettings.beginGroup(m_tabGroup);
    for (int i = 0; i < 10; ++i) {
        QString val = patternSettings.value(QString("Tools/CustomPattern%1").arg(i + 1)).toString();
        if (!val.isEmpty()) {
            m_patternSelector->addItem(val);
        }
    }

    // Context menu for adding/deleting patterns
    m_patternSelector->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_patternSelector, &QComboBox::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Pattern Options"), m_tab);

        QAction *addAction = contextMenu.addAction(tr("Add new pattern..."));
        connect(addAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newPattern = QInputDialog::getText(m_tab, tr("Add Pattern"),
                                                       tr("Enter image pattern (e.g., img%04d.jpg):"),
                                                       QLineEdit::Normal, "", &ok);
            if (ok && !newPattern.isEmpty()) {
                QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
                settings.beginGroup(m_tabGroup);
                QStringList customPatterns;
                for (int i = 0; i < 10; ++i) {
                    QString val = settings.value(QString("Tools/CustomPattern%1").arg(i + 1)).toString();
                    if (!val.isEmpty()) customPatterns.append(val);
                }
                if (!customPatterns.contains(newPattern)) {
                    customPatterns.append(newPattern);
                    for (int i = 0; i < 10; ++i) {
                        settings.remove(QString("Tools/CustomPattern%1").arg(i + 1));
                    }
                    for (int i = 0; i < customPatterns.size() && i < 10; ++i) {
                        settings.setValue(QString("Tools/CustomPattern%1").arg(i + 1), customPatterns[i]);
                    }
                    m_patternSelector->addItem(newPattern);
                    m_patternSelector->setCurrentText(newPattern);
                }
            }
        });

        contextMenu.addSeparator();

        QString currentValue = m_patternSelector->currentText();
        QAction *deleteAction = contextMenu.addAction(tr("Delete '%1'").arg(currentValue));
        deleteAction->setEnabled(!currentValue.isEmpty());
        connect(deleteAction, &QAction::triggered, this, [this, currentValue]() {
            QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
            settings.beginGroup(m_tabGroup);
            QStringList customPatterns;
            for (int i = 0; i < 10; ++i) {
                QString val = settings.value(QString("Tools/CustomPattern%1").arg(i + 1)).toString();
                if (!val.isEmpty()) customPatterns.append(val);
            }
            customPatterns.removeAll(currentValue);
            for (int i = 0; i < 10; ++i) {
                settings.remove(QString("Tools/CustomPattern%1").arg(i + 1));
            }
            for (int i = 0; i < customPatterns.size() && i < 10; ++i) {
                settings.setValue(QString("Tools/CustomPattern%1").arg(i + 1), customPatterns[i]);
            }
            int idx = m_patternSelector->findText(currentValue);
            if (idx >= 0) m_patternSelector->removeItem(idx);
            m_patternSelector->setCurrentIndex(0);
        });

        contextMenu.exec(m_patternSelector->mapToGlobal(pos));
    });

    converterLayout->addWidget(m_patternSelector, row, 1);

    // Auto-detect button for pattern
    QPushButton *autoDetectPatternButton = new QPushButton(tr("Auto"), m_tab);
    autoDetectPatternButton->setFixedWidth(80);
    autoDetectPatternButton->setToolTip(tr("Automatically detect image pattern from input directory"));
    connect(autoDetectPatternButton, &QPushButton::clicked, this, [this]() {
        QString inputDir = m_inputDirInput->text();
        if (inputDir.isEmpty()) {
            QMessageBox::information(m_tab, tr("No Directory"), tr("Please select an input directory first."));
            return;
        }
        QDir dir(inputDir);
        if (!dir.exists()) {
            QMessageBox::warning(m_tab, tr("Invalid Directory"), tr("The selected directory does not exist."));
            return;
        }
        QStringList imageFiles = dir.entryList(
            QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.tiff",
            QDir::Files, QDir::Name);
        if (imageFiles.isEmpty()) {
            QMessageBox::information(m_tab, tr("No Images"), tr("No image files found in the directory."));
            return;
        }
        QString detectedPattern;
        QString firstFile = imageFiles.first();
        QRegularExpression numRegex("(.*?)(\\d{3,})(.*)\\.(jpg|jpeg|png|bmp|tiff)$",
                                    QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = numRegex.match(firstFile);
        if (match.hasMatch()) {
            QString prefix = match.captured(1);
            int digits = match.captured(2).length();
            QString suffix = match.captured(3);
            QString ext = match.captured(4).toLower();
            detectedPattern = prefix + "%0" + QString::number(digits) + "d" + suffix + "." + ext;
            QString message = QString("Detected pattern: %1\n\nBased on file: %2\n\nFound %3 images.")
                .arg(detectedPattern).arg(firstFile).arg(imageFiles.count());
            QMessageBox msgBox(m_tab);
            msgBox.setWindowTitle(tr("Pattern Detected"));
            msgBox.setText(message);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            if (msgBox.exec() == QMessageBox::Ok) {
                m_patternSelector->setCurrentText(detectedPattern);
                QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
                settings.beginGroup(m_tabGroup);
                QStringList customPatterns;
                for (int i = 0; i < 10; ++i) {
                    QString val = settings.value(QString("Tools/CustomPattern%1").arg(i + 1)).toString();
                    if (!val.isEmpty()) customPatterns.append(val);
                }
                if (!customPatterns.contains(detectedPattern)) {
                    customPatterns.append(detectedPattern);
                    for (int i = 0; i < 10; ++i) {
                        settings.remove(QString("Tools/CustomPattern%1").arg(i + 1));
                    }
                    for (int i = 0; i < customPatterns.size() && i < 10; ++i) {
                        settings.setValue(QString("Tools/CustomPattern%1").arg(i + 1), customPatterns[i]);
                    }
                    m_patternSelector->addItem(detectedPattern);
                }
            }
        } else {
            QString ext = QFileInfo(firstFile).suffix().toLower();
            detectedPattern = "*." + ext;
            QString message = QString("No numbered sequence detected.\n\nSuggested wildcard pattern: %1\n\nFound %2 .%3 files.")
                .arg(detectedPattern).arg(imageFiles.count()).arg(ext);
            QMessageBox msgBox(m_tab);
            msgBox.setWindowTitle(tr("Wildcard Pattern"));
            msgBox.setText(message);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            if (msgBox.exec() == QMessageBox::Ok) {
                m_patternSelector->setCurrentText(detectedPattern);
                QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
                settings.beginGroup(m_tabGroup);
                QStringList customPatterns;
                for (int i = 0; i < 10; ++i) {
                    QString val = settings.value(QString("Tools/CustomPattern%1").arg(i + 1)).toString();
                    if (!val.isEmpty()) customPatterns.append(val);
                }
                if (!customPatterns.contains(detectedPattern)) {
                    customPatterns.append(detectedPattern);
                    for (int i = 0; i < 10; ++i) {
                        settings.remove(QString("Tools/CustomPattern%1").arg(i + 1));
                    }
                    for (int i = 0; i < customPatterns.size() && i < 10; ++i) {
                        settings.setValue(QString("Tools/CustomPattern%1").arg(i + 1), customPatterns[i]);
                    }
                    m_patternSelector->addItem(detectedPattern);
                }
            }
        }
    });
    converterLayout->addWidget(autoDetectPatternButton, row, 2);
    row++;

    // Output Filename
    converterLayout->addWidget(new QLabel(tr("Output Filename:"), m_tab), row, 0);
    m_outputFileInput = new QLineEdit(m_tab);
    m_outputFileInput->setPlaceholderText(tr("output.mp4"));
    converterLayout->addWidget(m_outputFileInput, row, 1);
    m_outputFileBrowseButton = new QPushButton(tr("Browse..."), m_tab);
    m_outputFileBrowseButton->setFixedWidth(80);
    connect(m_outputFileBrowseButton, &QPushButton::clicked, this, &ToolsModule::browseOutputFile);
    converterLayout->addWidget(m_outputFileBrowseButton, row, 2);
    row++;

    // Framerate - Editable ComboBox with context menu
    converterLayout->addWidget(new QLabel(tr("Framerate (fps):"), m_tab), row, 0);
    m_framerateSelector = new QComboBox(m_tab);
    m_framerateSelector->setEditable(true);
    m_framerateSelector->setFixedWidth(200);
    m_framerateSelector->setToolTip(tr("Frames per second for output video\nRight-click to add or delete values"));

    QSettings framerateSettings(AppPaths::globalConf(), QSettings::IniFormat);
    framerateSettings.beginGroup(m_tabGroup);
    for (int i = 0; i < 10; ++i) {
        QString val = framerateSettings.value(QString("Tools/CustomFramerate%1").arg(i + 1)).toString();
        if (!val.isEmpty()) {
            m_framerateSelector->addItem(val);
        }
    }

    m_framerateSelector->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_framerateSelector, &QComboBox::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Framerate Options"), m_tab);
        QAction *addAction = contextMenu.addAction(tr("Add new framerate..."));
        connect(addAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newFps = QInputDialog::getText(m_tab, tr("Add Framerate"),
                                                   tr("Enter framerate (fps):"),
                                                   QLineEdit::Normal, "", &ok);
            if (ok && !newFps.isEmpty()) {
                bool isNumber;
                newFps.toDouble(&isNumber);
                if (isNumber) {
                    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
                    settings.beginGroup(m_tabGroup);
                    QStringList customFramerates;
                    for (int i = 0; i < 10; ++i) {
                        QString val = settings.value(QString("Tools/CustomFramerate%1").arg(i + 1)).toString();
                        if (!val.isEmpty()) customFramerates.append(val);
                    }
                    if (!customFramerates.contains(newFps)) {
                        customFramerates.append(newFps);
                        for (int i = 0; i < 10; ++i) {
                            settings.remove(QString("Tools/CustomFramerate%1").arg(i + 1));
                        }
                        for (int i = 0; i < customFramerates.size() && i < 10; ++i) {
                            settings.setValue(QString("Tools/CustomFramerate%1").arg(i + 1), customFramerates[i]);
                        }
                        m_framerateSelector->addItem(newFps);
                        m_framerateSelector->setCurrentText(newFps);
                    }
                }
            }
        });
        contextMenu.addSeparator();
        QString currentValue = m_framerateSelector->currentText();
        QAction *deleteAction = contextMenu.addAction(tr("Delete '%1'").arg(currentValue));
        deleteAction->setEnabled(!currentValue.isEmpty());
        connect(deleteAction, &QAction::triggered, this, [this, currentValue]() {
            QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
            settings.beginGroup(m_tabGroup);
            QStringList customFramerates;
            for (int i = 0; i < 10; ++i) {
                QString val = settings.value(QString("Tools/CustomFramerate%1").arg(i + 1)).toString();
                if (!val.isEmpty()) customFramerates.append(val);
            }
            customFramerates.removeAll(currentValue);
            for (int i = 0; i < 10; ++i) {
                settings.remove(QString("Tools/CustomFramerate%1").arg(i + 1));
            }
            for (int i = 0; i < customFramerates.size() && i < 10; ++i) {
                settings.setValue(QString("Tools/CustomFramerate%1").arg(i + 1), customFramerates[i]);
            }
            int idx = m_framerateSelector->findText(currentValue);
            if (idx >= 0) m_framerateSelector->removeItem(idx);
        });
        contextMenu.exec(m_framerateSelector->mapToGlobal(pos));
    });
    m_framerateSelector->setFixedWidth(180);

    // Codec Selection
    m_codecComboBox = new QComboBox(m_tab);
    m_codecComboBox->setFixedWidth(180);
    m_codecComboBox->addItem(tr("H.264 (libx264)"), "libx264");
    m_codecComboBox->addItem(tr("H.265/HEVC (libx265)"), "libx265");
    m_codecComboBox->addItem("MJPEG", "mjpeg");
    m_codecComboBox->setToolTip(tr("Video codec for encoding"));

    auto *framerateCodecLayout = new QHBoxLayout;
    framerateCodecLayout->addWidget(m_framerateSelector);
    framerateCodecLayout->addStretch();
    framerateCodecLayout->addWidget(new QLabel(tr("Codec:"), m_tab));
    framerateCodecLayout->addSpacing(10);
    framerateCodecLayout->addWidget(m_codecComboBox);
    converterLayout->addLayout(framerateCodecLayout, row, 1, 1, 2);
    row++;

    // Resize/Scale - Editable with context menu
    converterLayout->addWidget(new QLabel(tr("Resize:"), m_tab), row, 0);
    m_resizeSelector = new QComboBox(m_tab);
    m_resizeSelector->setEditable(true);
    m_resizeSelector->setFixedWidth(180);
    m_resizeSelector->setToolTip(tr("Resize video to specific resolution\\nFormat: WIDTHxHEIGHT (e.g., 1920x1080)\\nRight-click to add or delete values"));

    QSettings resizeSettings(AppPaths::globalConf(), QSettings::IniFormat);
    resizeSettings.beginGroup(m_tabGroup);
    QStringList customResolutions;
    for (int i = 0; i < 10; ++i) {
        QString val = resizeSettings.value(QString("Tools/CustomResolution%1").arg(i + 1)).toString();
        if (!val.isEmpty()) customResolutions.append(val);
    }
    m_resizeSelector->addItem(tr("Original Size"), "original");
    for (const QString &res : customResolutions) {
        if (!res.isEmpty()) m_resizeSelector->addItem(res);
    }
    m_resizeSelector->setCurrentIndex(0);

    m_resizeSelector->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_resizeSelector, &QComboBox::customContextMenuRequested, this, [this](const QPoint &pos) {
        QMenu contextMenu(tr("Resolution Options"), m_tab);
        QAction *addAction = contextMenu.addAction(tr("Add new resolution..."));
        connect(addAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newRes = QInputDialog::getText(m_tab, tr("Add Resolution"),
                                                   tr("Enter resolution (e.g., 1920x1080):"),
                                                   QLineEdit::Normal, "", &ok);
            if (ok && !newRes.isEmpty()) {
                QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
                settings.beginGroup(m_tabGroup);
                QStringList customResolutions;
                for (int i = 0; i < 10; ++i) {
                    QString val = settings.value(QString("Tools/CustomResolution%1").arg(i + 1)).toString();
                    if (!val.isEmpty()) customResolutions.append(val);
                }
                if (!customResolutions.contains(newRes)) {
                    customResolutions.append(newRes);
                    for (int i = 0; i < 10; ++i) {
                        settings.remove(QString("Tools/CustomResolution%1").arg(i + 1));
                    }
                    for (int i = 0; i < customResolutions.size() && i < 10; ++i) {
                        settings.setValue(QString("Tools/CustomResolution%1").arg(i + 1), customResolutions[i]);
                    }
                    m_resizeSelector->addItem(newRes);
                    m_resizeSelector->setCurrentText(newRes);
                }
            }
        });
        contextMenu.addSeparator();
        QString currentValue = m_resizeSelector->currentText();
        QAction *deleteAction = contextMenu.addAction(tr("Delete '%1'").arg(currentValue));
        deleteAction->setEnabled(!currentValue.isEmpty());
        connect(deleteAction, &QAction::triggered, this, [this, currentValue]() {
            QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
            settings.beginGroup(m_tabGroup);
            QStringList customResolutions;
            for (int i = 0; i < 10; ++i) {
                QString val = settings.value(QString("Tools/CustomResolution%1").arg(i + 1)).toString();
                if (!val.isEmpty()) customResolutions.append(val);
            }
            customResolutions.removeAll(currentValue);
            for (int i = 0; i < 10; ++i) {
                settings.remove(QString("Tools/CustomResolution%1").arg(i + 1));
            }
            for (int i = 0; i < customResolutions.size() && i < 10; ++i) {
                settings.setValue(QString("Tools/CustomResolution%1").arg(i + 1), customResolutions[i]);
            }
            int idx = m_resizeSelector->findText(currentValue);
            if (idx >= 0) m_resizeSelector->removeItem(idx);
            m_resizeSelector->setCurrentIndex(0);
        });
        contextMenu.exec(m_resizeSelector->mapToGlobal(pos));
    });

    // Encoding Preset
    m_presetComboBox = new QComboBox(m_tab);
    m_presetComboBox->setFixedWidth(180);
    for (const QString &p : {"ultrafast", "superfast", "veryfast", "faster", "fast",
                              "medium", "slow", "slower", "veryslow"}) {
        m_presetComboBox->addItem(p);
    }
    m_presetComboBox->setCurrentText("medium");
    m_presetComboBox->setToolTip(tr("Encoding speed vs compression efficiency tradeoff"));

    auto *resizeEncodingLayout = new QHBoxLayout;
    resizeEncodingLayout->addWidget(m_resizeSelector);
    resizeEncodingLayout->addStretch();
    resizeEncodingLayout->addWidget(new QLabel(tr("Encoding:"), m_tab));
    resizeEncodingLayout->addSpacing(10);
    resizeEncodingLayout->addWidget(m_presetComboBox);
    converterLayout->addLayout(resizeEncodingLayout, row, 1, 1, 2);
    row++;

    // Quality (CRF)
    converterLayout->addWidget(new QLabel(tr("Quality (CRF):"), m_tab), row, 0);
    m_qualitySlider = new QSlider(Qt::Horizontal, m_tab);
    m_qualitySlider->setRange(18, 28);
    m_qualitySlider->setValue(23);
    m_qualitySlider->setToolTip(tr("Constant Rate Factor: 18=high quality/large file, 28=lower quality/small file"));
    m_qualitySlider->setStyleSheet("");
    m_qualityLabel = new QLabel("23", m_tab);
    connect(m_qualitySlider, &QSlider::valueChanged, m_qualityLabel,
            static_cast<void (QLabel::*)(int)>(&QLabel::setNum));
    auto *qualityLayout = new QHBoxLayout;
    qualityLayout->addWidget(m_qualitySlider);
    qualityLayout->addWidget(m_qualityLabel);
    converterLayout->addLayout(qualityLayout, row, 1, 1, 2);
    row++;

    // Progress Bar
    converterLayout->addWidget(new QLabel(tr("Progress:"), m_tab), row, 0);
    m_progressBar = new QProgressBar(m_tab);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setAlignment(Qt::AlignCenter);
    m_progressBar->setStyleSheet("QProgressBar::chunk { background-color: #27ae60; }");
    m_progressBar->setFormat("%p%");
    converterLayout->addWidget(m_progressBar, row, 1, 1, 2);
    row++;

    // Start/Stop Buttons
    auto *buttonLayout = new QHBoxLayout;
    m_startButton = new QPushButton(tr("Start Conversion"), m_tab);
    m_stopButton = new QPushButton(tr("Stop"), m_tab);
    m_stopButton->setEnabled(false);
    connect(m_startButton, &QPushButton::clicked, this, &ToolsModule::startVideoConversion);
    connect(m_stopButton, &QPushButton::clicked, this, &ToolsModule::stopVideoConversion);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    converterLayout->addLayout(buttonLayout, row, 1, 1, 2);
    row++;

    // Log Output
    converterLayout->addWidget(new QLabel(tr("Log Output:"), m_tab), row, 0, Qt::AlignTop);
    m_logOutput = new QTextEdit(m_tab);
    m_logOutput->setReadOnly(true);
    m_logOutput->setMaximumHeight(150);
    m_logOutput->setPlaceholderText(tr("Conversion log will appear here..."));
    converterLayout->addWidget(m_logOutput, row, 1, 1, 2);

    // Make group collapsible
    helpers.append(CollapsibleHelper::makeCollapsible(
        videoConverterGroup,
        "UI/Tools/VideoConverterGroup",
        [adjustWindowCallback]() { if (adjustWindowCallback) adjustWindowCallback(); }
    ));

    mainLayout->addWidget(videoConverterGroup);
    mainLayout->addStretch();
}

void ToolsModule::browseInputDir()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    QString startDir = m_inputDirInput->text();
    if (startDir.isEmpty() || !QFileInfo(startDir).exists()) {
        startDir = AppPaths::sanitizeIfSystemPath(settings.value("Paths/GuiToolsInputPath", AppPaths::contentOutput()).toString(), AppPaths::contentOutput());
    }

    QString folder = QFileDialog::getExistingDirectory(m_tab, tr("Select Image Sequence Folder"),
                                                       startDir, QFileDialog::ShowDirsOnly);
    if (!folder.isEmpty()) {
        m_inputDirInput->setText(folder);
    }
}

void ToolsModule::browseOutputFile()
{
    QSettings settings(AppPaths::globalConf(), QSettings::IniFormat);
    settings.beginGroup(m_tabGroup);
    QString startDir = QFileInfo(m_outputFileInput->text()).absolutePath();
    if (startDir.isEmpty() || !QFileInfo(startDir).exists()) {
        startDir = settings.value("Paths/GuiToolsOutputPath", AppPaths::contentOutput()).toString();
    }

    QString fileName = QFileDialog::getSaveFileName(m_tab, tr("Save Video As"),
                                                    startDir + "/output.mp4",
                                                    "Video Files (*.mp4 *.mkv *.avi *.mov);;All Files (*)");
    if (!fileName.isEmpty()) {
        m_outputFileInput->setText(fileName);
    }
}

void ToolsModule::startVideoConversion()
{
    QString inputDir = m_inputDirInput->text();
    QString pattern = m_patternSelector->currentText();
    QString outputFile = m_outputFileInput->text();

    if (inputDir.isEmpty()) {
        m_logOutput->append("Error: Please select an input directory");
        return;
    }
    if (pattern.isEmpty()) {
        m_logOutput->append("Error: Please specify an image pattern");
        return;
    }
    if (outputFile.isEmpty()) {
        m_logOutput->append("Error: Please specify an output filename");
        return;
    }

    // Count images
    QDir dir(inputDir);
    QStringList filters;
    if (pattern.contains("%")) {
        QString ext = pattern.mid(pattern.lastIndexOf('.'));
        filters << "*" + ext;
    } else {
        filters << pattern;
    }
    QStringList imageFiles = dir.entryList(filters, QDir::Files, QDir::Name);
    m_totalFrames = imageFiles.count();

    if (m_totalFrames == 0) {
        m_logOutput->append("Error: No images found matching pattern '" + pattern + "'");
        QMessageBox::warning(m_tab, tr("No Images Found"),
            tr("No images found matching pattern '") + pattern + tr("' in directory.\n\n") +
            tr("Please check the pattern and directory."));
        return;
    }

    m_logOutput->append("Found " + QString::number(m_totalFrames) + " images");

    QString framerate = m_framerateSelector->currentText();
    QString codec = m_codecComboBox->currentData().toString();
    QString quality = m_qualityLabel->text();
    QString preset = m_presetComboBox->currentText();
    QString resizeData = m_resizeSelector->currentData().toString();
    QString resizeText = m_resizeSelector->currentText();

    QStringList args;
    args << "-framerate" << framerate;
    args << "-i" << (inputDir + "/" + pattern);
    args << "-c:v" << codec;
    if (codec == "libx264" || codec == "libx265") {
        args << "-crf" << quality;
        args << "-preset" << preset;
    }
    args << "-pix_fmt" << "yuv420p";

    QString resolution;
    if (!resizeData.isEmpty() && resizeData != "original") {
        resolution = resizeData;
    } else if (!resizeText.isEmpty() && !resizeText.startsWith("Original", Qt::CaseInsensitive)) {
        resolution = resizeText.split(" ").first();
    }
    if (!resolution.isEmpty() && resolution.contains("x")) {
        args << "-vf" << ("scale=" + resolution + ":force_original_aspect_ratio=decrease:force_divisible_by=2");
    }

    args << "-y" << outputFile;

    if (m_conversionProcess) {
        delete m_conversionProcess;
    }
    m_conversionProcess = new QProcess(this);

    connect(m_conversionProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        m_logOutput->append(m_conversionProcess->readAllStandardOutput());
    });

    connect(m_conversionProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString output = m_conversionProcess->readAllStandardError();
        m_logOutput->append(output);
        if (output.contains("frame=") && m_totalFrames > 0) {
            QRegularExpression re("frame=\\s*(\\d+)");
            QRegularExpressionMatch match = re.match(output);
            if (match.hasMatch()) {
                int currentFrame = match.captured(1).toInt();
                int percentage = qMin((currentFrame * 100) / m_totalFrames, 100);
                m_progressBar->setValue(percentage);
                if (percentage > 0 && percentage < 100) {
                    int elapsed = m_conversionStartTime.secsTo(QTime::currentTime());
                    if (elapsed > 0) {
                        int remaining = (elapsed * 100 / percentage) - elapsed;
                        m_progressBar->setFormat(QString("%p% - %1:%2:%3")
                            .arg(remaining / 3600, 2, 10, QChar('0'))
                            .arg((remaining % 3600) / 60, 2, 10, QChar('0'))
                            .arg(remaining % 60, 2, 10, QChar('0')));
                    }
                } else if (percentage >= 100) {
                    m_progressBar->setFormat("%p% - Complete");
                }
            }
        }
    });

    connect(m_conversionProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_logOutput->append("\n=== Conversion completed successfully ===");
            m_progressBar->setValue(100);
            m_progressBar->setFormat("%p% - Complete");
            QMessageBox msgBox(m_tab);
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setWindowTitle(tr("Conversion Complete"));
            msgBox.setText(tr("Video conversion completed successfully!"));
            msgBox.setInformativeText(tr("Processed ") + QString::number(m_totalFrames) +
                                       tr(" frames.\n\nOutput: ") + m_outputFileInput->text());
            QProcess::startDetached("paplay", QStringList() << "/usr/share/sounds/freedesktop/stereo/complete.oga");
            msgBox.exec();
        } else {
            m_logOutput->append("\n=== Conversion failed or was stopped ===");
            m_progressBar->setValue(0);
            m_progressBar->setFormat("%p%");
            if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                QMessageBox::warning(m_tab, tr("Conversion Failed"),
                    tr("Video conversion failed or was interrupted.\n\nCheck the log output for details."));
            }
        }
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
    });

    m_logOutput->clear();
    m_progressBar->setValue(0);
    m_progressBar->setRange(0, 100);
    m_progressBar->setFormat("%p% - Calculating...");
    m_conversionStartTime = QTime::currentTime();
    m_logOutput->append("Starting conversion...");
    m_logOutput->append("Command: ffmpeg " + args.join(" "));
    m_logOutput->append("");

    m_conversionProcess->start("ffmpeg", args);
    if (!m_conversionProcess->waitForStarted(3000)) {
        m_logOutput->append("Error: Failed to start ffmpeg. Is it installed?");
        QMessageBox::critical(m_tab, tr("FFmpeg Error"),
            tr("Failed to start ffmpeg. Please ensure it is installed.\n\nInstall with: sudo apt install ffmpeg"));
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        return;
    }
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
}

void ToolsModule::stopVideoConversion()
{
    if (m_conversionProcess && m_conversionProcess->state() == QProcess::Running) {
        m_logOutput->append("\nStopping conversion...");
        m_conversionProcess->terminate();
        QTimer::singleShot(3000, this, [this]() {
            if (m_conversionProcess && m_conversionProcess->state() == QProcess::Running) {
                m_conversionProcess->kill();
            }
        });
    }
}
