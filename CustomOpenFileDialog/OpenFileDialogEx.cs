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
using System.Text;
using System.Data;
using System.Drawing;
using System.Threading;
using System.Windows.Forms;
using System.ComponentModel;
using System.Collections.Generic;
using System.Runtime.InteropServices;

using CustomControls.OS;

namespace CustomControls.Controls
{
    [Author("Franco, Gustavo")]
    public partial class OpenFileDialogEx : UserControl
    {
        #region delegates
        public delegate void FileNameChangedHandler(OpenFileDialogEx sender, string filePath);
        #endregion

        #region Events
        public event FileNameChangedHandler FileNameChanged;
        public event FileNameChangedHandler FolderNameChanged;
        public event EventHandler           ClosingDialog;
        #endregion

        #region Constants Declaration
		private SetWindowPosFlags UFLAGSHIDE = 
            SetWindowPosFlags.SWP_NOACTIVATE |
			SetWindowPosFlags.SWP_NOOWNERZORDER |
			SetWindowPosFlags.SWP_NOMOVE |
            SetWindowPosFlags.SWP_NOSIZE | 
            SetWindowPosFlags.SWP_HIDEWINDOW;
        #endregion

        #region Variables Declaration
        private AddonWindowLocation mStartLocation  = AddonWindowLocation.Right;
        private FolderViewMode      mDefaultViewMode= FolderViewMode.Default;
        #endregion

        #region Constructors
        public OpenFileDialogEx()
        {
            InitializeComponent();
            //SetStyle(ControlStyles.SupportsTransparentBackColor, true);
        }
        #endregion

        #region Properties
        public OpenFileDialog OpenDialog
        {
            get {return dlgOpen;}
        }

        [DefaultValue(AddonWindowLocation.Right)]
        public AddonWindowLocation StartLocation
        {
            get {return mStartLocation;}
            set {mStartLocation = value;}
        }

        [DefaultValue(FolderViewMode.Default)]
        public FolderViewMode DefaultViewMode
        {
            get {return mDefaultViewMode;}
            set {mDefaultViewMode = value;}
        }
        #endregion

        #region Virtuals
        public virtual void OnFileNameChanged(string fileName)
        {
            if (FileNameChanged != null)
                FileNameChanged(this, fileName);
        }

        public virtual void OnFolderNameChanged(string folderName)
        {
            if (FolderNameChanged != null)
                FolderNameChanged(this, folderName);
        }

        public virtual void OnClosingDialog()
        {
            if (ClosingDialog != null)
                ClosingDialog(this, new EventArgs());
        }
        #endregion

        #region Methods
        public void ShowDialog()
        {
            ShowDialog(null);
        }

        public void ShowDialog(IWin32Window owner)
        {
            DummyForm form = new DummyForm(this);
            form.Show(owner);
            Win32.SetWindowPos(form.Handle, IntPtr.Zero, 0, 0, 0, 0, UFLAGSHIDE);
            form.WatchForActivate = true;
            try
            {
                dlgOpen.ShowDialog(form);
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
            form.Dispose();
            form.Close();
        }
        #endregion

        #region Helper Classes
        [Author("Franco, Gustavo")]
        private class OpenDialogNative : NativeWindow, IDisposable
        {
            #region Constants Declaration
		    private SetWindowPosFlags UFLAGSSIZE = 
                SetWindowPosFlags.SWP_NOACTIVATE |
			    SetWindowPosFlags.SWP_NOOWNERZORDER |
			    SetWindowPosFlags.SWP_NOMOVE;
		    private SetWindowPosFlags UFLAGSHIDE = 
                SetWindowPosFlags.SWP_NOACTIVATE |
			    SetWindowPosFlags.SWP_NOOWNERZORDER |
			    SetWindowPosFlags.SWP_NOMOVE |
                SetWindowPosFlags.SWP_NOSIZE | 
                SetWindowPosFlags.SWP_HIDEWINDOW;
		    private SetWindowPosFlags UFLAGSZORDER = 
                SetWindowPosFlags.SWP_NOACTIVATE |
			    SetWindowPosFlags.SWP_NOMOVE |
                SetWindowPosFlags.SWP_NOSIZE; 
            #endregion

            #region Variables Declaration
            private Size                mOriginalSize;
            private IntPtr              mOpenDialogHandle;
            private IntPtr              mListViewPtr;
            private WINDOWINFO          mListViewInfo;
            private BaseDialogNative    mBaseDialogNative;
            private IntPtr              mComboFolders;
            private WINDOWINFO          mComboFoldersInfo;
            private IntPtr              mGroupButtons;
            private WINDOWINFO          mGroupButtonsInfo;
            private IntPtr              mComboFileName;
            private WINDOWINFO          mComboFileNameInfo;
            private IntPtr              mComboExtensions;
            private WINDOWINFO          mComboExtensionsInfo;
            private IntPtr              mOpenButton;
            private WINDOWINFO          mOpenButtonInfo;
            private IntPtr              mCancelButton;
            private WINDOWINFO          mCancelButtonInfo;
            private IntPtr              mHelpButton;
            private WINDOWINFO          mHelpButtonInfo;
            private OpenFileDialogEx    mSourceControl;
            private IntPtr              mToolBarFolders;
            private WINDOWINFO          mToolBarFoldersInfo;
            private IntPtr              mLabelFileName;
            private WINDOWINFO          mLabelFileNameInfo;
            private IntPtr              mLabelFileType;
            private WINDOWINFO          mLabelFileTypeInfo;
            private IntPtr              mChkReadOnly;
            private WINDOWINFO          mChkReadOnlyInfo;
            private bool                mIsClosing          = false;
            private bool                mInitializated      = false;
            private RECT                mOpenDialogWindowRect = new RECT();
            private RECT                mOpenDialogClientRect = new RECT();
            #endregion

            #region Constructors
            public OpenDialogNative(IntPtr handle, OpenFileDialogEx sourceControl)
            {
                mOpenDialogHandle   = handle;
                mSourceControl      = sourceControl;
                AssignHandle(mOpenDialogHandle);
            }
            #endregion

            #region Events
            private void BaseDialogNative_FileNameChanged(BaseDialogNative sender, string filePath)
            {
                if (mSourceControl != null)
                    mSourceControl.OnFileNameChanged(filePath);
            }

            private void BaseDialogNative_FolderNameChanged(BaseDialogNative sender, string folderName)
            {
                if (mSourceControl != null)
                    mSourceControl.OnFolderNameChanged(folderName);
            }
            #endregion

            #region Properties
            public bool IsClosing
            {
                get {return mIsClosing;}
                set {mIsClosing = value;}
            }
            #endregion

            #region Methods
            public void Dispose()
            {
                ReleaseHandle();
                if (mBaseDialogNative != null)
                {
                    mBaseDialogNative.FileNameChanged -= new BaseDialogNative.FileNameChangedHandler(BaseDialogNative_FileNameChanged);
                    mBaseDialogNative.FolderNameChanged -= new BaseDialogNative.FileNameChangedHandler(BaseDialogNative_FolderNameChanged);
                    mBaseDialogNative.Dispose();
                }
            }
            #endregion

            #region Private Methods
            private void PopulateWindowsHandlers()
            {
                Win32.EnumChildWindows(mOpenDialogHandle, new Win32.EnumWindowsCallBack(OpenFileDialogEnumWindowCallBack), 0);
            }

            private bool OpenFileDialogEnumWindowCallBack(IntPtr hwnd, int lParam) 
            {
                StringBuilder className = new StringBuilder(256);
                Win32.GetClassName(hwnd, className, className.Capacity);
                int controlID = Win32.GetDlgCtrlID(hwnd);
                WINDOWINFO windowInfo;
                Win32.GetWindowInfo(hwnd, out windowInfo);

                // Dialog Window
                if (className.ToString().StartsWith("#32770"))
                {
                    mBaseDialogNative = new BaseDialogNative(hwnd);
                    mBaseDialogNative.FileNameChanged   += new BaseDialogNative.FileNameChangedHandler(BaseDialogNative_FileNameChanged);
                    mBaseDialogNative.FolderNameChanged += new BaseDialogNative.FileNameChangedHandler(BaseDialogNative_FolderNameChanged);
                    return true;
                }

                switch((ControlsID) controlID)
                {
                    case ControlsID.DefaultView:
                        mListViewPtr = hwnd;
                        Win32.GetWindowInfo(hwnd, out mListViewInfo);
                        if (mSourceControl.DefaultViewMode != FolderViewMode.Default)
                            Win32.SendMessage(mListViewPtr, (int) Msg.WM_COMMAND, (int) mSourceControl.DefaultViewMode, 0);
                        break;
                    case ControlsID.ComboFolder:
                        mComboFolders       = hwnd;
                        mComboFoldersInfo   = windowInfo;
                        break;
                    case ControlsID.ComboFileType:
                        mComboExtensions       = hwnd;
                        mComboExtensionsInfo   = windowInfo;
                        break;
                    case ControlsID.ComboFileName:
                        if (className.ToString().ToLower() == "comboboxex32")
                        {
                            mComboFileName          = hwnd;
                            mComboFileNameInfo      = windowInfo;
                        }
                        break;
                    case ControlsID.GroupFolder:
                        mGroupButtons       = hwnd;
                        mGroupButtonsInfo   = windowInfo;
                        break;
                    case ControlsID.LeftToolBar:
                        mToolBarFolders     = hwnd;
                        mToolBarFoldersInfo = windowInfo;
                        break;
                    case ControlsID.ButtonOpen:
                        mOpenButton         = hwnd;
                        mOpenButtonInfo     = windowInfo;
                        break;
                    case ControlsID.ButtonCancel:
                        mCancelButton       = hwnd;
                        mCancelButtonInfo   = windowInfo;
                        break;
                    case ControlsID.ButtonHelp:
                        mHelpButton         = hwnd;
                        mHelpButtonInfo     = windowInfo;
                        break;
                    case ControlsID.CheckBoxReadOnly:
                        mChkReadOnly        = hwnd;
                        mChkReadOnlyInfo    = windowInfo;
                        break;
                    case ControlsID.LabelFileName:
                        mLabelFileName      = hwnd;
                        mLabelFileNameInfo  = windowInfo;
                        break;
                    case ControlsID.LabelFileType:
                        mLabelFileType      = hwnd;
                        mLabelFileTypeInfo  = windowInfo;
                        break;
                }

                return true;
            }

            private void InitControls()
            {
                mInitializated = true;

                // Lets get information about the current open dialog
                Win32.GetClientRect(mOpenDialogHandle, ref mOpenDialogClientRect);
                Win32.GetWindowRect(mOpenDialogHandle, ref mOpenDialogWindowRect);

                // Lets borrow the Handles from the open dialog control
                PopulateWindowsHandlers();

                // Resize OpenDialog to make fit our extra form
                switch(mSourceControl.StartLocation)
                {
                    case AddonWindowLocation.Right:
                        // Now we transfer the control to the open dialog
                        mSourceControl.Location = new Point((int) (mOpenDialogClientRect.Width - mSourceControl.Width), 0);

                        // Everything is ready, now lets change the parent
                        Win32.SetParent(mSourceControl.Handle, mOpenDialogHandle);

                        // Send the control to the back
                        Win32.SetWindowPos(mSourceControl.Handle, (IntPtr) ZOrderPos.HWND_BOTTOM, 0, 0, 0, 0, UFLAGSZORDER);
                        break;
                    case AddonWindowLocation.Bottom:
                        // Now we transfer the control to the open dialog
                        mSourceControl.Location = new Point(0, (int) (mOpenDialogClientRect.Height - mSourceControl.Height));

                        // Everything is ready, now lets change the parent
                        Win32.SetParent(mSourceControl.Handle, mOpenDialogHandle);

                        // Send the control to the back
                        Win32.SetWindowPos(mSourceControl.Handle, (IntPtr) ZOrderPos.HWND_BOTTOM, 0, 0, 0, 0, UFLAGSZORDER);
                        break;
                    case AddonWindowLocation.None:
                        // We don't have to do too much in this case, but set parent must be the first call
                        // because else ZOrder won't worl
                        Win32.SetParent(mSourceControl.Handle, mOpenDialogHandle);

                        // Send the control to the back
                        Win32.SetWindowPos(mSourceControl.Handle, (IntPtr) ZOrderPos.HWND_BOTTOM, 0, 0, 0, 0, UFLAGSZORDER);
                        break;
                }
            }
            #endregion

            #region Overrides
            protected override void WndProc(ref Message m)
            {
                switch(m.Msg)
                {
                    case (int) Msg.WM_SHOWWINDOW:
                        mInitializated = true;
                        InitControls();
                        break;
                    case (int) Msg.WM_WINDOWPOSCHANGING:
                        if (!mIsClosing)
                        {
                            if (!mInitializated)
                            {
                                WINDOWPOS pos = (WINDOWPOS) Marshal.PtrToStructure(m.LParam, typeof(WINDOWPOS));
                                if (mSourceControl.StartLocation == AddonWindowLocation.Right)
                                {
                                    if (pos.flags != 0 && ((pos.flags & (int) SWP_Flags.SWP_NOSIZE) != (int) SWP_Flags.SWP_NOSIZE))
                                    {
                                        mOriginalSize = new Size(pos.cx, pos.cy);

                                        pos.cx += mSourceControl.Width;
                                        Marshal.StructureToPtr(pos, m.LParam, true);
                                    }
                                }

                                if (mSourceControl.StartLocation == AddonWindowLocation.Bottom)
                                {
                                    if (pos.flags != 0 && ((pos.flags & (int) SWP_Flags.SWP_NOSIZE) != (int) SWP_Flags.SWP_NOSIZE))
                                    {
                                        mOriginalSize = new Size(pos.cx, pos.cy);

                                        pos.cy += mSourceControl.Height;
                                        Marshal.StructureToPtr(pos, m.LParam, true);
                                    }
                                }
                            }

                            RECT currentSize;
                            switch(mSourceControl.StartLocation)
                            {
                                case AddonWindowLocation.Right:
                                    currentSize = new RECT();
                                    Win32.GetClientRect(mOpenDialogHandle, ref currentSize);
                                    mSourceControl.Height = (int) currentSize.Height; 
                                    break;
                                case AddonWindowLocation.Bottom:
                                    currentSize = new RECT();
                                    Win32.GetClientRect(mOpenDialogHandle, ref currentSize);
                                    mSourceControl.Width = (int) currentSize.Width; 
                                    break;
                                case AddonWindowLocation.None:
                                    currentSize = new RECT();
                                    Win32.GetClientRect(mOpenDialogHandle, ref currentSize);
                                    mSourceControl.Width = (int) currentSize.Width; 
                                    mSourceControl.Height = (int) currentSize.Height; 
                                    break;
                            }
                        }
                        break;
                    case (int) Msg.WM_IME_NOTIFY:
                        if (m.WParam == (IntPtr) ImeNotify.IMN_CLOSESTATUSWINDOW)
                        {
                            mIsClosing = true;
                            mSourceControl.OnClosingDialog();

                            Win32.SetWindowPos(mOpenDialogHandle, IntPtr.Zero, 0, 0, 0, 0, UFLAGSHIDE);
                            Win32.GetWindowRect(mOpenDialogHandle, ref mOpenDialogWindowRect);
                            Win32.SetWindowPos(mOpenDialogHandle, IntPtr.Zero, 
                                (int) (mOpenDialogWindowRect.left), 
                                (int) (mOpenDialogWindowRect.top), 
                                (int) (mOriginalSize.Width), 
                                (int) (mOriginalSize.Height), 
                                UFLAGSSIZE);
                        }
                        break;
                }
                base.WndProc(ref m);
            }
            #endregion
        }

        [Author("Franco, Gustavo")]
        private class BaseDialogNative : NativeWindow, IDisposable
        {
            #region delegates
            public delegate void FileNameChangedHandler(BaseDialogNative sender, string filePath);
            #endregion

            #region Events
            public event FileNameChangedHandler FileNameChanged;
            public event FileNameChangedHandler FolderNameChanged;
            #endregion

            #region Variables Declaration
            private IntPtr mhandle;
            #endregion

            #region Constructors
            public BaseDialogNative(IntPtr handle)
            {
                mhandle = handle;
                AssignHandle(handle);
            }
            #endregion

            #region Methods
            public void Dispose()
            {
                ReleaseHandle();
            }
            #endregion

            #region Overrides
            protected override void WndProc(ref Message m)
            {
                switch (m.Msg)
                {
                    case (int) Msg.WM_NOTIFY:
                        OFNOTIFY ofNotify = (OFNOTIFY) Marshal.PtrToStructure(m.LParam, typeof(OFNOTIFY));
                        if (ofNotify.hdr.code == (uint) DialogChangeStatus.CDN_SELCHANGE)
                        {
                            StringBuilder filePath = new StringBuilder(256);
                            Win32.SendMessage(Win32.GetParent(mhandle), (int) DialogChangeProperties.CDM_GETFILEPATH, (int) 256, filePath);
                            if (FileNameChanged != null)
                                FileNameChanged(this, filePath.ToString());
                        }
                        else if (ofNotify.hdr.code == (uint) DialogChangeStatus.CDN_FOLDERCHANGE)
                        {
                            StringBuilder folderPath = new StringBuilder(256);
                            Win32.SendMessage(Win32.GetParent(mhandle), (int) DialogChangeProperties.CDM_GETFOLDERPATH, (int) 256, folderPath);
                            if (FolderNameChanged != null)
                                FolderNameChanged(this, folderPath.ToString());
                        }
                        break;
                }
                base.WndProc(ref m);
            }
            #endregion
        }

        [Author("Franco, Gustavo")]
        private class DummyForm : Form
        {
            #region Variables Declaration
            private OpenDialogNative    mNativeDialog       = null;
            private OpenFileDialogEx    mFileDialogEx       = null;
            private bool                mWatchForActivate   = false;
            private IntPtr              mOpenDialogHandle   = IntPtr.Zero;
            #endregion

            #region Constructors
            public DummyForm(OpenFileDialogEx fileDialogEx)
            {
                mFileDialogEx = fileDialogEx;
                this.Text           = "";
                this.StartPosition  = FormStartPosition.Manual;
                this.Location       = new Point(-32000, -32000);
                this.ShowInTaskbar  = false;
            }
            #endregion

            #region Properties
            public bool WatchForActivate
            {
                get {return mWatchForActivate;}
                set {mWatchForActivate = value;}
            }
            #endregion

            #region Overrides
            protected override void OnClosing(CancelEventArgs e)
            {
                if (mNativeDialog != null)
                    mNativeDialog.Dispose();
                base.OnClosing(e);
            }

            protected override void WndProc(ref Message m)
            {
                if (mWatchForActivate && m.Msg == (int) Msg.WM_ACTIVATE)
                {
                    mWatchForActivate   = false;
                    mOpenDialogHandle   = m.LParam;
                    mNativeDialog       = new OpenDialogNative(m.LParam, mFileDialogEx);
                }
                base.WndProc(ref m);
            }
            #endregion
        }
        #endregion
    }

    #region Enums
    public enum AddonWindowLocation
    {
        None    = 0,
        Right   = 1,
        Bottom  = 2
    }

    public enum ControlsID
    {
        ButtonOpen	    = 0x1,
        ButtonCancel	= 0x2,
        ButtonHelp	    = 0x40E,
        GroupFolder     = 0x440,
        LabelFileType   = 0x441,
        LabelFileName   = 0x442,
        LabelLookIn     = 0x443,
        DefaultView     = 0x461,
        LeftToolBar     = 0x4A0,
        ComboFileName   = 0x47c,
        ComboFileType   = 0x470,
        ComboFolder     = 0x471,
        CheckBoxReadOnly= 0x410
    }
    #endregion
}