//  Copyright (c) 2006, Gustavo Franco
//  Email:  gustavo_franco@hotmail.com
//  All rights reserved.

//  Redistribution and use in source and binary forms, with or without modification, 
//  are permitted provided that the following conditions are met:

//  Redistributions of source code must retain the above copyright notice, 
//  this list of conditions and the following disclaimer. 
//  Redistributions in binary form must reproduce the above copyright notice, 
//  this list of conditions and the following disclaimer in the documentation 
//  and/or other materials provided with the distribution. 

//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE. IT CAN BE DISTRIBUTED FREE OF CHARGE AS LONG AS THIS HEADER 
//  REMAINS UNCHANGED.

using System;
using System.IO;
using System.Data;
using System.Text;
using System.Drawing;
using System.Windows.Forms;
using System.ComponentModel;
using System.Drawing.Imaging;
using System.Collections.Generic;

using CustomControls.Controls;

namespace CustomControls
{
    [Author("Franco, Gustavo")]
    public partial class FormOpenFileDialog : OpenFileDialogEx
    {
        #region Constructors
        public FormOpenFileDialog()
        {
            InitializeComponent();
        }
        #endregion

        #region Overrides
        public override void OnFileNameChanged(string filePath)
        {
            if (filePath.ToLower().EndsWith(".bmp") ||
                filePath.ToLower().EndsWith(".jpg") ||
                filePath.ToLower().EndsWith(".png") ||
                filePath.ToLower().EndsWith(".tif") ||
                filePath.ToLower().EndsWith(".gif"))
            {
                if (pbxPreview.Image != null)
                    pbxPreview.Image.Dispose();

                try
                {
                    FileInfo fi = new FileInfo(filePath);
                    pbxPreview.Image    = Bitmap.FromFile(filePath);
                    lblSizeValue.Text   = (fi.Length / 1024).ToString() + "KB";
                    lblColorsValue.Text = GetColorsCountFromImage(pbxPreview.Image);
                    lblFormatValue.Text = GetFormatFromImage(pbxPreview.Image);
                }
                catch(Exception){}
            }
            else
            {
                if (pbxPreview.Image != null)
                    pbxPreview.Image.Dispose();
                pbxPreview.Image = null;
            }
        }

        public override void OnFolderNameChanged(string folderName)
        {
            if (pbxPreview.Image != null)
                pbxPreview.Image.Dispose();
            pbxPreview.Image = null;
            lblSizeValue.Text   = string.Empty;
            lblColorsValue.Text = string.Empty;
            lblFormatValue.Text = string.Empty;
        }

        public override void OnClosingDialog()
        {
            if (pbxPreview.Image != null)
                pbxPreview.Image.Dispose();
        }
        #endregion

        #region Private Methods
        private string GetColorsCountFromImage(Image image)
        {
            switch(image.PixelFormat)
            {
                case PixelFormat.Format16bppArgb1555:
                case PixelFormat.Format16bppGrayScale:
                case PixelFormat.Format16bppRgb555:
                case PixelFormat.Format16bppRgb565:
                    return "16 bits (65536 colors)";
                case PixelFormat.Format1bppIndexed:
                    return "1 bit (Black & White)";
                case PixelFormat.Format24bppRgb:
                    return "24 bits (True Colors)";
                case PixelFormat.Format32bppArgb:
                case PixelFormat.Format32bppPArgb:
                case PixelFormat.Format32bppRgb:
                    return "32 bits (Alpha Channel)";
                case PixelFormat.Format4bppIndexed:
                    return "4 bits (16 colors)";
                case PixelFormat.Format8bppIndexed:
                    return "8 bits (256 colors)";
            }
            return string.Empty;
        }

        private string GetFormatFromImage(Image image)
        {
            if (image.RawFormat.Equals(ImageFormat.Bmp))
                return "BMP";
            else if (image.RawFormat.Equals(ImageFormat.Gif))
                return "GIF";
            else if (image.RawFormat.Equals(ImageFormat.Jpeg))
                return "JPG";
            else if (image.RawFormat.Equals(ImageFormat.Png))
                return "PNG";
            else if (image.RawFormat.Equals(ImageFormat.Tiff))
                return "TIFF";
            return string.Empty;
        }
        #endregion
    }
}