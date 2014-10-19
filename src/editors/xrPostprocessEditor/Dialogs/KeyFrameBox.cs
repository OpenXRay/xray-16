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
        public event EventHandler Add;
        public event EventHandler Remove;
        public event EventHandler Clear;

        public KeyFrameBox()
        {
            InitializeComponent();
        }
        
        private void lbKeyFrames_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void btnAdd_Click(object sender, EventArgs e)
        {

        }

        private void btnRemove_Click(object sender, EventArgs e)
        {

        }

        private void btnClear_Click(object sender, EventArgs e)
        {

        }

        //public string SelectedItem { get {return lbPointList.Se} set; }
    }
}
