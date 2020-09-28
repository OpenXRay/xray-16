using System;
using System.Drawing;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    // XXX nitrocaster: implement hex color TextBox (NumericBox)
    public sealed partial class ColorPicker : UserControl
    {
        public delegate void ColorChangedEventHandler(object sender, Color color);

        private bool hexadecimal;
        private bool ignoreOnChanged = false;
        private bool alphaEnabled = true;
        private HorizontalAlignment textAlignment = HorizontalAlignment.Left;
        
        public ColorPicker()
        {
            InitializeComponent();
        }

        public event ColorChangedEventHandler ColorChanged;

        public Color Value
        {
            get { return pbColor.ColorSample; }
            set
            {
                if (pbColor.ColorSample == value)
                    return;
                ignoreOnChanged = true;
                if (alphaEnabled)
                    nslAlpha.Value = value.A;
                nslRed.Value = value.R;
                nslGreen.Value = value.G;
                nslBlue.Value = value.B;
                pbColor.ColorSample = value;
                ignoreOnChanged = false;
                UpdateColor();
            }
        }

        public byte Red { get; private set; }
        public byte Green { get; private set; }
        public byte Blue { get; private set; }
        public byte Alpha { get; private set; }

        public bool AlphaEnabled
        {
            get { return alphaEnabled; }
            set
            {
                if (alphaEnabled == value)
                    return;
                alphaEnabled = value;
                ignoreOnChanged = true;
                nslAlpha.Value = nslAlpha.Maximum;
                ignoreOnChanged = false;
                UpdateColor();
                lAlpha.Visible = alphaEnabled;
                nslAlpha.Visible = alphaEnabled;
                int delta = (alphaEnabled ? 1 : -1)*27;
                Point loc = chkHexadecimal.Location;
                loc.Y += delta;
                chkHexadecimal.Location = loc;
            }
        }

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

        public HorizontalAlignment TextAlign
        {
            get { return textAlignment; }
            set
            {
                if (textAlignment == value)
                    return;
                textAlignment = value;
                nslAlpha.TextAlign = value;
                nslRed.TextAlign = value;
                nslGreen.TextAlign = value;
                nslBlue.TextAlign = value;
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
            ColorChanged?.Invoke(this, Value);
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
