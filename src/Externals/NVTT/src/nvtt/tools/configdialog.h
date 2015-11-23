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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QtGui/QDialog>

#include "ui_configdialog.h"

#include <nvtt/nvtt.h>


class ConfigDialog : public QDialog
{
	Q_OBJECT
public:
	ConfigDialog(QWidget *parent = 0);
	ConfigDialog(const char * fileName, QWidget *parent = 0);
	
protected slots:
	
	void openClicked();
	void generateMipmapsChanged(int state);
	void mipmapFilterChanged(QString name);
	
	void colorWeightChanged();
	void uniformWeightToggled(bool checked);
	void luminanceWeightToggled(bool checked);
	
	void normalMapModeChanged(bool checked);
	
	bool open(QString fileName);
	
private:
	
	void init();
	
private:
	Ui::ConfigDialog ui;
	
	nvtt::InputOptions inputOptions;
	nvtt::CompressionOptions compressionOptions;
	nvtt::OutputOptions outputOptions;
	
};


#endif // CONFIGDIALOG_H
