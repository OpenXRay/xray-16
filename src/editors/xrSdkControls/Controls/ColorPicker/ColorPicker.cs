using System;
using System.Drawing;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    // XXX nitrocaster: implement hex color TextBox (NumericBox)
    public sealed partial class ColorPicker : UserControl
    {
        public delegate void ColorChangedEventHandler(object sender, Color color);

        private Color color;
        private bool hexadecimal;
        private bool ignoreOnChanged = false;
        
        public ColorPicker()
        {
            InitializeComponent();
        }

        public event ColorChangedEventHandler ColorChanged;

        public Color Value
        {
            get { return color; }
            set
            {
                if (color == value)
                    return;
                color = value;
                ignoreOnChanged = true;
                nslAlpha.Value = color.A;
                nslRed.Value = color.R;
                nslGreen.Value = color.G;
                nslBlue.Value = color.B;
                ignoreOnChanged = false;
                UpdateColor();
            }
        }

        public byte Red { get; private set; }
        public byte Green { get; private set; }
        public byte Blue { get; private set; }
        public byte Alpha { get; private set; }

        public bool Hexadecimal
        {
            get { return hexadecimal; }
            set
            {
                if (hexadecimal == value)
                    return;
                hexadecimal = value;
                chkHexadecimal.Checked = value;
                nslRed.Hexadecimal = value;
                nslGreen.Hexadecimal = value;
                nslBlue.Hexadecimal = value;
                nslAlpha.Hexadecimal = value;
            }
        }

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);
            nslRed.ValueChanged += (obj, args) => UpdateColor();
            nslGreen.ValueChanged += (obj, args) => UpdateColor();
            nslBlue.ValueChanged += (obj, args) => UpdateColor();
            nslAlpha.ValueChanged += (obj, args) => UpdateColor();
            chkHexadecimal.CheckedChanged += (obj, args) => Hexadecimal = chkHexadecimal.Checked;
            UpdateColor();
        }

        private void OnColorChanged()
        {
            if (!ignoreOnChanged && ColorChanged != null)
                ColorChanged(this, Value);
        }

        private void UpdateColor()
        {
            if (ignoreOnChanged)
                return;
            var newColor = Color.FromArgb(
                Convert.ToInt32(nslAlpha.Value),
                Convert.ToInt32(nslRed.Value),
                Convert.ToInt32(nslGreen.Value),
                Convert.ToInt32(nslBlue.Value));
            if (pbColor.ColorSample == newColor)
                return;
            pbColor.ColorSample = newColor;
            OnColorChanged();
        }
    }
}
