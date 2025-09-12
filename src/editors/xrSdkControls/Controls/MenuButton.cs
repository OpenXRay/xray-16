using System;
using System.Drawing;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public class MenuButton : Button
    {
        private ContextMenu menu = new ContextMenu();

        protected override void OnClick(EventArgs e)
        {
            // for ContextMenuStrip
            //var screenPoint = PointToScreen(new Point(Left, Bottom));
            //var currentScreen = Screen.FromPoint(screenPoint);
            //var flip = screenPoint.Y + menu.Size.Height > currentScreen.WorkingArea.Height;
            //menu.Show(this, new Point(0, flip ? -menu.Height : Height));
            menu.Show(this, new Point(0, Height));
            base.OnClick(e);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            int arrowX = ClientRectangle.Width-14;
            int arrowY = ClientRectangle.Height/2-1;
            var brush = Enabled ? SystemBrushes.ControlText : SystemBrushes.ButtonShadow;
            var arrowPoints = new []
            {
                new Point(arrowX, arrowY),
                new Point(arrowX+7, arrowY),
                new Point(arrowX+3, arrowY+4)
            };
            e.Graphics.FillPolygon(brush, arrowPoints);
        }

        public ContextMenu Menu
        {
            get { return menu; }
        }
    }
}
