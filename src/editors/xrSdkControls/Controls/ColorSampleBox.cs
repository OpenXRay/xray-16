using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public class ColorSampleBox : UserControl
    {
        private Color color;

        public ColorSampleBox()
        {
            base.BackgroundImage = Properties.Resources.Background;
            base.BackgroundImageLayout = ImageLayout.Tile;
        }

        public Color ColorSample
        {
            get { return color; }
            set
            {
                if (color == value)
                    return;
                color = value;
                Invalidate();
            }
        }

        protected override CreateParams CreateParams
        {
            get
            {
                var cp = base.CreateParams;
                cp.ExStyle |= 0x02000000; // set WS_EX_COMPOSITED
                return cp;
            }
        }

        public override Image BackgroundImage
        {
            get { return base.BackgroundImage; }
            set { }
        }

        public override ImageLayout BackgroundImageLayout
        {
            get { return base.BackgroundImageLayout; }
            set { }
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            using (var backBrush = new SolidBrush(color))
            {
                e.Graphics.CompositingMode = CompositingMode.SourceOver;
                e.Graphics.FillRectangle(backBrush, ClientRectangle);
            }
            base.OnPaint(e);
        }
    }
}
