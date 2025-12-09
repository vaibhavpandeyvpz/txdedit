#include "TexturePropertiesWidget.h"
#include "libtxd/txd_converter.h"
#include <QFormLayout>
#include <QLabel>
#include <QIntValidator>
#include <QFontMetrics>
#include <cstring>

TexturePropertiesWidget::TexturePropertiesWidget(QWidget *parent)
    : QWidget(parent), currentTexture(nullptr) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Scroll area for properties
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    contentWidget = new QWidget;
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    contentLayout->setSpacing(8);
    contentLayout->setAlignment(Qt::AlignTop);
    
    // Basic properties
    basicGroup = new QGroupBox("Basic properties", contentWidget);
    basicGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QFormLayout* basicLayout = new QFormLayout(basicGroup);
    basicLayout->setSpacing(8);
    basicLayout->setLabelAlignment(Qt::AlignRight);
    basicLayout->setContentsMargins(10, 15, 10, 10);
    basicLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    nameEdit = new QLineEdit(contentWidget);
    connect(nameEdit, &QLineEdit::textChanged, this, &TexturePropertiesWidget::onNameChanged);
    basicLayout->addRow("Diffuse name:", nameEdit);
    
    alphaNameEdit = new QLineEdit(contentWidget);
    connect(alphaNameEdit, &QLineEdit::textChanged, this, &TexturePropertiesWidget::onAlphaNameChanged);
    basicLayout->addRow("Alpha name:", alphaNameEdit);
    
    // Width input field with number validation
    widthEdit = new QLineEdit(contentWidget);
    widthEdit->setValidator(new QIntValidator(1, 4096, widthEdit));
    widthEdit->setText("256");
    widthEdit->setReadOnly(true); // Read-only: dimensions come from texture data
    basicLayout->addRow("Width:", widthEdit);
    
    // Height input field with number validation
    heightEdit = new QLineEdit(contentWidget);
    heightEdit->setValidator(new QIntValidator(1, 4096, heightEdit));
    heightEdit->setText("256");
    heightEdit->setReadOnly(true); // Read-only: dimensions come from texture data
    basicLayout->addRow("Height:", heightEdit);
    
    // Mipmap input field with number validation
    mipmapEdit = new QLineEdit(contentWidget);
    mipmapEdit->setValidator(new QIntValidator(1, 16, mipmapEdit));
    mipmapEdit->setText("1");
    connect(mipmapEdit, &QLineEdit::editingFinished, 
            this, &TexturePropertiesWidget::onMipmapCountChanged);
    basicLayout->addRow("Mipmaps:", mipmapEdit);
    
    alphaCheck = new CheckBox("", contentWidget);
    connect(alphaCheck, &QCheckBox::toggled, this, &TexturePropertiesWidget::onAlphaChannelToggled);
    basicLayout->addRow("Has alpha:", alphaCheck);
    
    contentLayout->addWidget(basicGroup);
    
    // Format properties
    formatGroup = new QGroupBox("Format", contentWidget);
    formatGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QFormLayout* formatLayout = new QFormLayout(formatGroup);
    formatLayout->setSpacing(8);
    formatLayout->setLabelAlignment(Qt::AlignRight);
    formatLayout->setContentsMargins(10, 15, 10, 10);
    formatLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    formatCombo = new QComboBox(contentWidget);
    QListView* formatView = new QListView();
    formatView->setSpacing(0);
    formatView->setUniformItemSizes(true);
    formatCombo->setView(formatView);
    formatCombo->setEditable(false);
    // Note: libtxd uses B8G8R8A8, not R8G8B8A8
    formatCombo->addItem("B8G8R8A8", static_cast<uint32_t>(LibTXD::RasterFormat::B8G8R8A8));
    formatCombo->addItem("B8G8R8", static_cast<uint32_t>(LibTXD::RasterFormat::B8G8R8));
    formatCombo->addItem("R5G6B5", static_cast<uint32_t>(LibTXD::RasterFormat::R5G6B5));
    formatCombo->addItem("A1R5G5B5", static_cast<uint32_t>(LibTXD::RasterFormat::A1R5G5B5));
    formatCombo->addItem("R4G4B4A4", static_cast<uint32_t>(LibTXD::RasterFormat::R4G4B4A4));
    formatCombo->addItem("LUM8", static_cast<uint32_t>(LibTXD::RasterFormat::LUM8));
    // Calculate width based on longest item text
    QFontMetrics fm(formatCombo->font());
    int maxWidth = 0;
    for (int i = 0; i < formatCombo->count(); i++) {
        int width = fm.horizontalAdvance(formatCombo->itemText(i));
        if (width > maxWidth) maxWidth = width;
    }
    formatCombo->view()->setMinimumWidth(maxWidth + 40); // Add padding for borders and scrollbar
    formatLayout->addRow("Raster format:", formatCombo);
    
    compressionCombo = new QComboBox(contentWidget);
    QListView* compressionView = new QListView();
    compressionCombo->setView(compressionView);
    compressionCombo->setEditable(false);
    compressionCombo->addItem("None", static_cast<int>(LibTXD::Compression::NONE));
    compressionCombo->addItem("DXT1", static_cast<int>(LibTXD::Compression::DXT1));
    compressionCombo->addItem("DXT3", static_cast<int>(LibTXD::Compression::DXT3));
    // Calculate width based on longest item text
    QFontMetrics fmComp(compressionCombo->font());
    int maxWidthComp = 0;
    for (int i = 0; i < compressionCombo->count(); i++) {
        int width = fmComp.horizontalAdvance(compressionCombo->itemText(i));
        if (width > maxWidthComp) maxWidthComp = width;
    }
    compressionCombo->view()->setMinimumWidth(maxWidthComp + 40);
    formatLayout->addRow("Compression:", compressionCombo);
    
    // Connect format/compression changes
    connect(formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (currentTexture) {
            LibTXD::RasterFormat format = static_cast<LibTXD::RasterFormat>(formatCombo->currentData().toUInt());
            currentTexture->setRasterFormat(format);
            // Don't emit propertyChanged for format changes
            // Raster format is metadata for saving, not for display
        }
    });
    
    connect(compressionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (currentTexture) {
            LibTXD::Compression comp = static_cast<LibTXD::Compression>(compressionCombo->currentData().toInt());
            currentTexture->setCompression(comp);
            // Don't emit propertyChanged for compression-only changes
            // Compression is metadata for saving, not for display
        }
    });
    
    contentLayout->addWidget(formatGroup);
    
    // Flags
    flagsGroup = new QGroupBox("Flags", contentWidget);
    flagsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QFormLayout* flagsLayout = new QFormLayout(flagsGroup);
    flagsLayout->setSpacing(8);
    flagsLayout->setLabelAlignment(Qt::AlignRight);
    flagsLayout->setContentsMargins(10, 15, 10, 10);
    flagsLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    
    filterCombo = new QComboBox(contentWidget);
    QListView* filterView = new QListView();
    filterView->setSpacing(0);
    filterView->setUniformItemSizes(true);
    filterCombo->setView(filterView);
    filterCombo->setEditable(false);
    // Note: libtxd doesn't have separate filter flags enum, using raw values
    filterCombo->addItem("None", 0);
    filterCombo->addItem("Nearest", 1);
    filterCombo->addItem("Linear", 2);
    filterCombo->addItem("Mip Nearest", 3);
    filterCombo->addItem("Mip Linear", 4);
    filterCombo->addItem("Linear Mip Nearest", 5);
    filterCombo->addItem("Linear Mip Linear", 6);
    // Calculate width based on longest item text
    QFontMetrics fmFilter(filterCombo->font());
    int maxWidthFilter = 0;
    for (int i = 0; i < filterCombo->count(); i++) {
        int width = fmFilter.horizontalAdvance(filterCombo->itemText(i));
        if (width > maxWidthFilter) maxWidthFilter = width;
    }
    filterCombo->view()->setMinimumWidth(maxWidthFilter + 40);
    flagsLayout->addRow("Filter:", filterCombo);
    
    uWrapCombo = new QComboBox(contentWidget);
    QListView* uWrapView = new QListView();
    uWrapView->setSpacing(0);
    uWrapView->setUniformItemSizes(true);
    uWrapCombo->setView(uWrapView);
    uWrapCombo->setEditable(false);
    // Note: libtxd doesn't have separate wrap flags enum, using raw values
    uWrapCombo->addItem("None", 0);
    uWrapCombo->addItem("Wrap", 1);
    uWrapCombo->addItem("Mirror", 2);
    uWrapCombo->addItem("Clamp", 3);
    // Calculate width based on longest item text
    QFontMetrics fmUWrap(uWrapCombo->font());
    int maxWidthUWrap = 0;
    for (int i = 0; i < uWrapCombo->count(); i++) {
        int width = fmUWrap.horizontalAdvance(uWrapCombo->itemText(i));
        if (width > maxWidthUWrap) maxWidthUWrap = width;
    }
    uWrapCombo->view()->setMinimumWidth(maxWidthUWrap + 40);
    flagsLayout->addRow("U wrap:", uWrapCombo);
    
    vWrapCombo = new QComboBox(contentWidget);
    QListView* vWrapView = new QListView();
    vWrapView->setSpacing(0);
    vWrapView->setUniformItemSizes(true);
    vWrapCombo->setView(vWrapView);
    vWrapCombo->setEditable(false);
    // Note: libtxd doesn't have separate wrap flags enum, using raw values
    vWrapCombo->addItem("None", 0);
    vWrapCombo->addItem("Wrap", 1);
    vWrapCombo->addItem("Mirror", 2);
    vWrapCombo->addItem("Clamp", 3);
    // Calculate width based on longest item text
    QFontMetrics fmVWrap(vWrapCombo->font());
    int maxWidthVWrap = 0;
    for (int i = 0; i < vWrapCombo->count(); i++) {
        int width = fmVWrap.horizontalAdvance(vWrapCombo->itemText(i));
        if (width > maxWidthVWrap) maxWidthVWrap = width;
    }
    vWrapCombo->view()->setMinimumWidth(maxWidthVWrap + 40);
    flagsLayout->addRow("V wrap:", vWrapCombo);
    
    // Connect flag changes
    // Note: libtxd only has filterFlags as a single uint32_t, not separate wrap flags
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        if (currentTexture) {
            currentTexture->setFilterFlags(filterCombo->currentData().toUInt());
            // Don't emit propertyChanged for filter changes
            // Filter is metadata for rendering, not for display
        }
    });
    
    // Note: libtxd doesn't support separate U/V wrap flags yet
    // These combos are kept for UI consistency but won't modify the texture
    connect(uWrapCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        // Not implemented in libtxd yet
    });
    
    connect(vWrapCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        // Not implemented in libtxd yet
    });
    
    contentLayout->addWidget(flagsGroup);
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
    
    clear();
}

void TexturePropertiesWidget::setTexture(LibTXD::Texture* texture) {
    currentTexture = texture;
    if (texture) {
        // Show all groups when texture is set
        basicGroup->show();
        formatGroup->show();
        flagsGroup->show();
    }
    updateUI();
}

void TexturePropertiesWidget::clear() {
    currentTexture = nullptr;
    blockSignals(true);
    
    nameEdit->clear();
    alphaNameEdit->clear();
    widthEdit->setText("256");
    heightEdit->setText("256");
    mipmapEdit->setText("1");
    alphaCheck->setChecked(false);
    formatCombo->setCurrentIndex(0);
    compressionCombo->setCurrentIndex(0);
    filterCombo->setCurrentIndex(0);
    uWrapCombo->setCurrentIndex(0);
    vWrapCombo->setCurrentIndex(0);
    
    basicGroup->setEnabled(false);
    formatGroup->setEnabled(false);
    flagsGroup->setEnabled(false);
    
    // Hide all groups when no texture is selected
    basicGroup->hide();
    formatGroup->hide();
    flagsGroup->hide();
    
    blockSignals(false);
}

void TexturePropertiesWidget::updateUI() {
    if (!currentTexture) {
        clear();
        return;
    }
    
    blockSignals(true);
    
    // Ensure groups are visible when updating
    basicGroup->show();
    formatGroup->show();
    flagsGroup->show();
    
    basicGroup->setEnabled(true);
    formatGroup->setEnabled(true);
    flagsGroup->setEnabled(true);
    
    nameEdit->setText(QString::fromStdString(currentTexture->getName()));
    alphaNameEdit->setText(QString::fromStdString(currentTexture->getMaskName()));
    
    // Get dimensions from first mipmap
    if (currentTexture->getMipmapCount() > 0) {
        const auto& mipmap = currentTexture->getMipmap(0);
        widthEdit->setText(QString::number(mipmap.width));
        heightEdit->setText(QString::number(mipmap.height));
    } else {
        widthEdit->setText("0");
        heightEdit->setText("0");
    }
    
    mipmapEdit->setText(QString::number(currentTexture->getMipmapCount()));
    alphaCheck->setChecked(currentTexture->hasAlpha());
    
    // Set format combo
    LibTXD::RasterFormat format = currentTexture->getRasterFormat();
    for (int i = 0; i < formatCombo->count(); i++) {
        if (formatCombo->itemData(i).toUInt() == static_cast<uint32_t>(format)) {
            formatCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // Set compression combo
    LibTXD::Compression comp = currentTexture->getCompression();
    for (int i = 0; i < compressionCombo->count(); i++) {
        if (compressionCombo->itemData(i).toInt() == static_cast<int>(comp)) {
            compressionCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // Set filter
    uint32_t filter = currentTexture->getFilterFlags();
    for (int i = 0; i < filterCombo->count(); i++) {
        if (filterCombo->itemData(i).toUInt() == filter) {
            filterCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // Note: libtxd doesn't support separate U/V wrap flags yet
    // Keep combos at default values
    uWrapCombo->setCurrentIndex(0);
    vWrapCombo->setCurrentIndex(0);
    
    blockSignals(false);
}

void TexturePropertiesWidget::blockSignals(bool block) {
    nameEdit->blockSignals(block);
    alphaNameEdit->blockSignals(block);
    widthEdit->blockSignals(block);
    heightEdit->blockSignals(block);
    mipmapEdit->blockSignals(block);
    alphaCheck->blockSignals(block);
    formatCombo->blockSignals(block);
    compressionCombo->blockSignals(block);
    filterCombo->blockSignals(block);
    uWrapCombo->blockSignals(block);
    vWrapCombo->blockSignals(block);
}

void TexturePropertiesWidget::onNameChanged() {
    if (currentTexture) {
        currentTexture->setName(nameEdit->text().toStdString());
        emit propertyChanged();
    }
}

void TexturePropertiesWidget::onAlphaNameChanged() {
    if (currentTexture) {
        currentTexture->setMaskName(alphaNameEdit->text().toStdString());
        emit propertyChanged();
    }
}


void TexturePropertiesWidget::onMipmapCountChanged() {
    if (currentTexture) {
        bool ok;
        int value = mipmapEdit->text().toInt(&ok);
        if (ok && value >= 1 && value <= 16) {
            // Note: libtxd doesn't have setMipmapCount - mipmaps are managed individually
            // This is read-only for now - user can't change mipmap count
            mipmapEdit->setText(QString::number(currentTexture->getMipmapCount()));
        } else {
            // Revert to current value
            mipmapEdit->setText(QString::number(currentTexture->getMipmapCount()));
        }
    }
}

void TexturePropertiesWidget::onAlphaChannelToggled(bool enabled) {
    if (!currentTexture || currentTexture->getMipmapCount() == 0) {
        return;
    }
    
    if (enabled) {
        bool needsReset = false;
        bool hadAlphaBefore = currentTexture->hasAlpha();
        
        if (!hadAlphaBefore) {
            // Case 1: Texture never had alpha before
            needsReset = true;
        } else {
            // Case 2: Check if existing alpha resolution matches diffuse resolution
            // In libtxd, alpha and diffuse are in the same mipmap, so they should always match
            // But we check to be safe - if conversion fails, reset to white
            const auto& mipmap = currentTexture->getMipmap(0);
            auto existingRGBA = LibTXD::TextureConverter::convertToRGBA8(*currentTexture, 0);
            if (!existingRGBA) {
                // Can't verify alpha, reset to white
                needsReset = true;
            }
            // If alpha exists and conversion succeeds, alpha should match diffuse (same mipmap)
            // So we don't need to reset in this case
        }
        
        if (needsReset) {
            // Reset alpha to white (#ffffff)
            const auto& mipmap = currentTexture->getMipmap(0);
            auto rgbaData = LibTXD::TextureConverter::convertToRGBA8(*currentTexture, 0);
            
            if (rgbaData) {
                size_t dataSize = mipmap.width * mipmap.height * 4;
                std::vector<uint8_t> newTextureData(dataSize);
                std::memcpy(newTextureData.data(), rgbaData.get(), dataSize);
                
                // Set all alpha channels to white (255)
                for (size_t i = 3; i < dataSize; i += 4) {
                    newTextureData[i] = 255;
                }
                
                // Update texture data
                LibTXD::Compression compression = currentTexture->getCompression();
                if (compression != LibTXD::Compression::NONE) {
                    auto compressedData = LibTXD::TextureConverter::compressToDXT(newTextureData.data(), mipmap.width, mipmap.height, compression, 1.0f);
                    if (compressedData) {
                        size_t compressedSize = LibTXD::TextureConverter::getCompressedDataSize(mipmap.width, mipmap.height, compression);
                        auto& mip = currentTexture->getMipmap(0);
                        mip.data.assign(compressedData.get(), compressedData.get() + compressedSize);
                        mip.dataSize = compressedSize;
                    } else {
                        // Fallback to uncompressed
                        currentTexture->setCompression(LibTXD::Compression::NONE);
                        auto& mip = currentTexture->getMipmap(0);
                        mip.data = newTextureData;
                        mip.dataSize = dataSize;
                    }
                } else {
                    // Uncompressed - update mipmap data directly
                    auto& mip = currentTexture->getMipmap(0);
                    mip.data = newTextureData;
                    mip.dataSize = dataSize;
                }
            }
        }
    }
    
    currentTexture->setHasAlpha(enabled);
    emit propertyChanged();
}

