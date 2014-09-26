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
using System.Text;
using System.Drawing;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace CustomControls.OS
{
	#region WINDOWINFO
    [Author("Franco, Gustavo")]
    [StructLayout(LayoutKind.Sequential)]
    public struct WINDOWINFO
    {
       public UInt32 cbSize;
       public RECT rcWindow;
       public RECT rcClient;
       public UInt32 dwStyle;
       public UInt32 dwExStyle;
       public UInt32 dwWindowStatus;
       public UInt32 cxWindowBorders;
       public UInt32 cyWindowBorders;
       public UInt16 atomWindowType;
       public UInt16 wCreatorVersion;
   }
    #endregion

    #region POINT
    [Author("Franco, Gustavo")]
	[StructLayout(LayoutKind.Sequential)]
	public struct POINT
	{
		public int x;
		public int y;

        #region Constructors
        public POINT(int x, int y)
		{
			this.x = x;
			this.y = y;
		}

        public POINT(Point point)
        {
            x = point.X;
            y = point.Y;
        }
        #endregion
    }
	#endregion

    #region RECT
    [Author("Franco, Gustavo")]
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT 
    { 
        public uint left; 
        public uint top; 
        public uint right; 
        public uint bottom;

        #region Properties
        public POINT Location
        {
            get {return new POINT((int) left, (int) top);}
            set
            {
                right   -= (left - (uint) value.x);
                bottom  -= (bottom - (uint) value.y);
                left    = (uint) value.x;
                top     = (uint) value.y;
            }
        }

        public uint Width
        {
            get {return right - left;}
            set {right = left + value;}
        }

        public uint Height
        {
            get {return bottom - top;}
            set {bottom = top + value;}
        }
        #endregion

        #region Overrides
        public override string ToString()
        {
            return left + ":" + top + ":" + right + ":" + bottom;
        }
        #endregion
    }
    #endregion

    #region WINDOWPOS
    [Author("Franco, Gustavo")]
    [StructLayout(LayoutKind.Sequential)]
	public struct WINDOWPOS
	{
		public IntPtr hwnd;
		public IntPtr hwndAfter;
		public int x;
		public int y;
		public int cx;
		public int cy;
		public uint flags;

        #region Overrides
        public override string ToString()
        {
            return x + ":" + y + ":" + cx + ":" + cy + ":" + ((SWP_Flags) flags).ToString();
        }
        #endregion
    }
    #endregion

    #region NCCALCSIZE_PARAMS
    public struct NCCALCSIZE_PARAMS
    {
        public RECT rgrc1;
        public RECT rgrc2;
        public RECT rgrc3;
        public IntPtr lppos;
    }
    #endregion

    #region
    public struct NMHDR 
    {
        public IntPtr  hwndFrom;
        public uint    idFrom;
        public uint    code;
    } 
    #endregion

    #region OFNOTIFY
    [Author("Franco, Gustavo")]
    [StructLayout(LayoutKind.Sequential)]
    public struct OFNOTIFY 
    {
        public NMHDR hdr;
        public IntPtr OPENFILENAME;
        public IntPtr fileNameShareViolation;
    }
    #endregion
}
