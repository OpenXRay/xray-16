namespace XRay.SdkControls
{
    partial class TreeView
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Код, автоматически созданный конструктором компонентов

        private void InitializeComponent()
        {
            this.SuspendLayout();

            this.multiSelect = true;
            this.selectableGroups = true;
            this.PathSeparator = "/";

            this.components = new System.ComponentModel.Container();

            this.filterPanel = new TreeViewFilterPanel()
            {
                Visible = false,
                Dock = System.Windows.Forms.DockStyle.Bottom,
                ParentTreeView = this
            };
            this.searchPanel = new TreeViewSearchPanel()
            {
                Dock = System.Windows.Forms.DockStyle.Bottom,
                Visible = false,
                ParentTreeView = this
            };

            this.TreeViewContainer = new System.Windows.Forms.Panel();
            this.TreeViewContainer.Controls.Add(this.filterPanel);
            this.TreeViewContainer.Controls.Add(this.searchPanel);

            this.addFolderMenuItem = new System.Windows.Forms.ToolStripMenuItem()
            {
                Name = "addFolderMenuItem",
                Size = new System.Drawing.Size(152, 22),
                Text = "Add folder"
            };
            this.addFolderMenuItem.Click += CreateGroup;

            this.addFileMenuItem = new System.Windows.Forms.ToolStripMenuItem()
            {
                Name = "addFileMenuItem",
                Size = new System.Drawing.Size(152, 22),
                Text = "Add item"
            };
            this.addFileMenuItem.Click += this.CreateItem;

            this.removeMenuItem = new System.Windows.Forms.ToolStripMenuItem()
            {
                Name = "removeMenuItem",
                Size = new System.Drawing.Size(152, 22),
                Text = "Remove item"
            };
            this.removeMenuItem.Click += this.RemoveItem;

            this.contextMenu = new System.Windows.Forms.ContextMenuStrip()
            {
                Name = "contextMenu",
                Size = new System.Drawing.Size(153, 92)

            };

            this.contextMenu.SuspendLayout();
            this.contextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[]
            {
                this.addFolderMenuItem,
                this.addFileMenuItem,
                this.removeMenuItem
            });
            this.contextMenu.ResumeLayout(false);

            this.ResumeLayout(true);
        }

        #endregion
    }
}
