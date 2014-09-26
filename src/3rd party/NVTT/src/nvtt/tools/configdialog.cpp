// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "configdialog.h"

#include <QtCore/QDebug>

#include <QtGui/QImage>


ConfigDialog::ConfigDialog(QWidget *parent/*=0*/) : QDialog(parent)
{
	init();
}

ConfigDialog::ConfigDialog(const char * fileName, QWidget *parent/*=0*/) : QDialog(parent)
{
	init();
	
	open(fileName);
}

void ConfigDialog::init()
{
	ui.setupUi(this);
	
	connect(ui.openButton, SIGNAL(clicked()), this, SLOT(openClicked()));
	connect(ui.generateMipmapsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(generateMipmapsChanged(int)));
	connect(ui.mipmapFilterComboBox, SIGNAL(activated(QString)), this, SLOT(mipmapFilterChanged(QString)));
	//connect(ui.mipmapFilterSettings, SIGNAL(clicked()), this, SLOT(mipmapFilterSettingsShow()));
	
	connect(ui.redSpinBox, SIGNAL(valueChanged(double)), this, SLOT(colorWeightChanged()));
	connect(ui.greenSpinBox, SIGNAL(valueChanged(double)), this, SLOT(colorWeightChanged()));
	connect(ui.blueSpinBox, SIGNAL(valueChanged(double)), this, SLOT(colorWeightChanged()));
	connect(ui.uniformButton, SIGNAL(toggled(bool)), this, SLOT(uniformWeightToggled(bool)));
	connect(ui.luminanceButton, SIGNAL(toggled(bool)), this, SLOT(luminanceWeightToggled(bool)));
	
	//connect(ui.rgbMapRadioButton, SIGNAL(toggled(bool)), this, SLOT(colorModeChanged()));
	connect(ui.normalMapRadioButton, SIGNAL(toggled(bool)), this, SLOT(normalMapModeChanged(bool)));
}


void ConfigDialog::openClicked()
{
	// @@ Open file dialog.
	
	QString fileName;
	
	open(fileName);
}

void ConfigDialog::generateMipmapsChanged(int state)
{
	Q_UNUSED(state);
	
	bool generateMipmapEnabled = ui.generateMipmapsCheckBox->isChecked();
	
	ui.mipmapFilterLabel->setEnabled(generateMipmapEnabled);
	ui.mipmapFilterComboBox->setEnabled(generateMipmapEnabled);
	ui.limitMipmapsCheckBox->setEnabled(generateMipmapEnabled);
	
	bool enableFilterSettings = (ui.mipmapFilterComboBox->currentText() == "Kaiser");
	ui.mipmapFilterSettings->setEnabled(generateMipmapEnabled && enableFilterSettings);
	
	bool enableMaxLevel = ui.limitMipmapsCheckBox->isChecked();
	ui.maxLevelLabel->setEnabled(generateMipmapEnabled && enableMaxLevel);
	ui.maxLevelSpinBox->setEnabled(generateMipmapEnabled && enableMaxLevel);
}

void ConfigDialog::mipmapFilterChanged(QString name)
{
	bool enableFilterSettings = (name == "Kaiser");
	ui.mipmapFilterSettings->setEnabled(enableFilterSettings);
}


void ConfigDialog::colorWeightChanged()
{
	double r = ui.redSpinBox->value();
	double g = ui.greenSpinBox->value();
	double b = ui.blueSpinBox->value();
	
	bool uniform = (r == 1.0 && g == 1.0 && b == 1.0);
	bool luminance = (r == 0.3 && g == 0.59 && b == 0.11);
	
	ui.uniformButton->setChecked(uniform);
	ui.luminanceButton->setChecked(luminance);
}

void ConfigDialog::uniformWeightToggled(bool checked)
{
	if (checked)
	{
		ui.redSpinBox->setValue(1.0);
		ui.greenSpinBox->setValue(1.0);
		ui.blueSpinBox->setValue(1.0);
	}
}

void ConfigDialog::luminanceWeightToggled(bool checked)
{
	if (checked)
	{
		ui.redSpinBox->setValue(0.3);
		ui.greenSpinBox->setValue(0.59);
		ui.blueSpinBox->setValue(0.11);
	}
}

void ConfigDialog::normalMapModeChanged(bool checked)
{
	ui.alphaModeGroupBox->setEnabled(!checked);
	ui.inputGammaSpinBox->setEnabled(!checked);
	ui.inputGammaLabel->setEnabled(!checked);
	ui.outputGammaSpinBox->setEnabled(!checked);
	ui.outputGammaLabel->setEnabled(!checked);
}


bool ConfigDialog::open(QString fileName)
{
	// @@ Load image.
	QImage image;
	
	// @@ If success.
	{
		ui.imagePathLineEdit->setText(fileName);
		
		// @@ Set image in graphics view.
		
		// @@ Set image description.
		
		// @@ Provide image to nvtt.
		
		int w = image.width();
		int h = image.height();
		void * data = NULL;
		
		inputOptions.setTextureLayout(nvtt::TextureType_2D, w, h);
		inputOptions.setMipmapData(data, w, h);
		
		return true;
	}

	return false;
}




