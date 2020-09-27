using Flobbster.Windows.Forms;
using Microsoft.Win32;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public class PropertyGrid : System.Windows.Forms.PropertyGrid
    {
        private Control view;
        private Point prevLocation;

        public PropertyGrid()
        {
            Initialize();
            SetupEventHandlers();
        }

        private int GetSplitterWidth()
        {
            Type gridType = view.GetType();
            FieldInfo field = gridType.GetField("labelWidth", BindingFlags.NonPublic | BindingFlags.Instance);
            Object value = field.GetValue(view);
            return value is int ? (int)value : 0;
        }

        public void Initialize()
        {
            foreach (Control control in Controls)
            {
                if (control.GetType().Name.ToUpper() == "PROPERTYGRIDVIEW")
                {
                    view = control;
                    break;
                }
            }
        }

        public void OnChildControlMouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (SelectedObject == null || SelectedGridItem == null)
                return;
            var descriptor_raw = SelectedGridItem.PropertyDescriptor;
            Debug.Assert(descriptor_raw != null);
            var descriptor = descriptor_raw as PropertyBag.PropertySpecDescriptor;
            var container = descriptor?.bag as IPropertyContainer;
            IProperty property = container?.GetProperty(descriptor?.item);
            var mouseEvents = property as IMouseListener;
            if (mouseEvents == null)
                return;
            mouseEvents.OnDoubleClick(this);
        }

        public void OnChildControlMouseMove(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Middle)
            {
                prevLocation = e.Location;
                return;
            }
            if (e.Location.X == prevLocation.X)
                return;
            if (SelectedObject == null)
                return;
            if (SelectedGridItem == null)
                return;
            PropertyDescriptor rawDescriptor = SelectedGridItem.PropertyDescriptor;
            if (rawDescriptor == null)
                return;
            var descriptor = rawDescriptor as PropertyBag.PropertySpecDescriptor;
            var container = descriptor?.bag as IPropertyContainer;
            IProperty rawProperty = container?.GetProperty(descriptor?.item);
            Debug.Assert(rawProperty != null);
            var incrementable = rawProperty as IIncrementable;
            if (incrementable == null)
                return;
            incrementable.Increment(e.Location.X - prevLocation.X);
            Refresh();
            prevLocation = e.Location;
        }

        public void OnChildControlMouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Middle)
                return;
            prevLocation = e.Location;
        }

        public void SetupEventHandlers()
        {
            foreach (Control control in Controls)
            {
                control.MouseDoubleClick += OnChildControlMouseDoubleClick;
                control.MouseMove += OnChildControlMouseMove;
                control.MouseDown += OnChildControlMouseDown;
            }
        }

        public void save(RegistryKey root, string key)
        {
            RegistryKey grid = root.CreateSubKey(key);
            grid.SetValue("Splitter", GetSplitterWidth());
            grid.Close();
        }

        public void load(RegistryKey root, string key)
        {
            RegistryKey grid = root.OpenSubKey(key);
            if (grid == null)
                return;
            int position = GetRegValue(grid, "Splitter", GetSplitterWidth());
            grid.Close();
            Type grid_type = view.GetType();
            FieldInfo field = grid_type.GetField("labelWidth", BindingFlags.NonPublic | BindingFlags.Instance);
            field.SetValue(view, position);
        }

        private static T GetRegValue<T>(RegistryKey key, string name, T defaultValue)
        {
            object value = key.GetValue(name);
            if (value == null)
                return defaultValue;
            return (T)value;
        }
    }
}
