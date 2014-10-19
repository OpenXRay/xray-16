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
                color = value;
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
                islRed.Hexadecimal = value;
                islGreen.Hexadecimal = value;
                islBlue.Hexadecimal = value;
                islAlpha.Hexadecimal = value;
            }
        }

        private void ColorPicker_Load(object sender, EventArgs e)
        {
            islRed.ValueChanged += (obj, args) => UpdateColor();
            islGreen.ValueChanged += (obj, args) => UpdateColor();
            islBlue.ValueChanged += (obj, args) => UpdateColor();
            islAlpha.ValueChanged += (obj, args) => UpdateColor();
            chkHexadecimal.CheckedChanged += (obj, args) => Hexadecimal = chkHexadecimal.Checked;
            UpdateColor();
        }
        
        private void UpdateColor()
        {
            var newColor = Color.FromArgb(islAlpha.Value, islRed.Value, islGreen.Value, islBlue.Value);
            if (pbColor.ColorSample == newColor)
                return;
            pbColor.ColorSample = newColor;
            if (ColorChanged != null)
                ColorChanged(this, Value);
        }
    }
}
