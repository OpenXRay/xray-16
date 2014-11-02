using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace xrPostprocessEditor
{
    public partial class KeyFrameBox : UserControl
    {
        public event EventHandler SelectedIndexChanged;
        public event EventHandler AddButtonClick;
        public event EventHandler RemoveButtonClick;
        public event EventHandler ClearButtonClick;
        public event EventHandler KeyFrameTimeChanged;

        public ContextMenu CopyMenu { get { return btnCopyFrom.Menu; } }

        public ListBox.ObjectCollection Items { get { return lbKeyFrames.Items; } }

        public int SelectedIndex
        {
            get { return lbKeyFrames.SelectedIndex; }
            set { lbKeyFrames.SelectedIndex = value; }
        }

        public KeyFrameBox()
        {
            InitializeComponent();
        }
        
        private void lbKeyFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (SelectedIndexChanged != null)
                SelectedIndexChanged(this, e);
        }

        private void btnAdd_Click(object sender, EventArgs e)
        {
            if (AddButtonClick != null)
                AddButtonClick(this, e);
        }

        private void btnRemove_Click(object sender, EventArgs e)
        {
            if (RemoveButtonClick != null)
                RemoveButtonClick(this, e);
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            if (ClearButtonClick != null)
                ClearButtonClick(this, e);
        }

        private void numKeyFrameTime_ValueChanged(object sender, EventArgs e)
        {
            if (KeyFrameTimeChanged != null)
                KeyFrameTimeChanged(this, e);
        }
    }
}
