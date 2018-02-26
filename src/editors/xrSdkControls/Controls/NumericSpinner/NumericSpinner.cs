using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public sealed partial class NumericSpinner : UserControl
    {
        private bool ignoreOnChanged;

        public NumericSpinner()
        {
            InitializeComponent();
        }

        public event EventHandler ValueChanged;

        public decimal Value
        {
            get => numSpinner.Value;
            set => numSpinner.Value = value;
        }

        public decimal Minimum
        {
            get => numSpinner.Minimum;
            set => numSpinner.Minimum = value;
        }
        public decimal Maximum
        {
            get => numSpinner.Maximum;
            set => numSpinner.Maximum = value;
        }

        public bool Hexadecimal
        {
            get => numSpinner.Hexadecimal;
            set => numSpinner.Hexadecimal = value;
        }

        public HorizontalAlignment TextAlign
        {
            get => numSpinner.TextAlign;
            set => numSpinner.TextAlign = value;
        }

        public int DecimalPlaces
        {
            get => numSpinner.DecimalPlaces;
            set => numSpinner.DecimalPlaces = value;
        }

        public decimal Precision
        {
            get;
            set;
            /*get { return trackBar.Maximum; }
            set { trackBar.Maximum = value; }*/
        }

        private void OnValueChanged()
        {
            ValueChanged?.Invoke(this, null);
        }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);
            numSpinner.ValueChanged += (obj, args) =>
            {
                if (ignoreOnChanged)
                    return;
                OnValueChanged();
            };
        }

        private bool isDragging;
        private Point mousePos;
        private int mouseX;
        private int accumulation;

        private void btnHSpin_MouseDown(object sender, MouseEventArgs e)
        {
            isDragging = true;
            mouseX = e.X;
            mousePos = Cursor.Position;
            Cursor.Hide();
        }

        private void btnHSpin_MouseUp(object sender, MouseEventArgs e)
        {
            isDragging = false;
            mouseX = btnHSpin.Location.X;
            Cursor.Show();
        }

        private void btnHSpin_MouseMove(object sender, MouseEventArgs e)
        {
            if (!isDragging || mouseX == e.X)
                return;

            bool increasing;
            var newValue = numSpinner.Value;
            if (mouseX > e.X)
            {
                increasing = false;
                newValue -= (mouseX - e.X) * Precision;
            }
            else
            {
                increasing = true;
                newValue += (e.X - mouseX) * Precision;
            }

            if (newValue > numSpinner.Maximum)
                numSpinner.Value = numSpinner.Maximum;
            else if (newValue < numSpinner.Minimum)
                numSpinner.Value = numSpinner.Minimum;
            else
                numSpinner.Value = newValue;

            if (accumulation > 1 || accumulation < -1)
            {
                Cursor.Position = mousePos;
                accumulation = 0;
            }
            if (increasing)
                ++accumulation;
            else
                --accumulation;
        }
    }
}
