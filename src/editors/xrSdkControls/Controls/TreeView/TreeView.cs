using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public partial class TreeView : System.Windows.Forms.TreeView
    {
        public TreeView()
        {
            InitializeComponent();

            selectedNodes = new List<System.Windows.Forms.TreeNode>();

            GetType().GetProperty("DoubleBuffered",
                    System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance |
                    System.Reflection.BindingFlags.Public)
                ?.SetValue(this, true, null);

            Root = GetType().GetField("Root",
                    System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic)
                ?.GetValue(this) as TreeNode;
        }

        // Events
        public event EventHandler ItemCreate;
        public event EventHandler GroupCreate;
        public event EventHandler ItemRemove;
        public event EventHandler ItemsLoaded;
        public event EventHandler SelectedItemsChanged;

        // Fields
        private ITreeViewSource source;
        private ContextMenuStrip contextMenu;
        private ToolStripMenuItem addFolderMenuItem;
        private ToolStripMenuItem addFileMenuItem;
        private ToolStripMenuItem removeMenuItem;

        private bool isContainerCreated;
        private bool isRightClicked;

        private List<System.Windows.Forms.TreeNode> selectedNodes;
        private System.Windows.Forms.TreeNode lastSelectedNode;
        private System.Windows.Forms.TreeNode justSelectedNode;

        private TreeViewFilterPanel filterPanel;
        private TreeViewSearchPanel searchPanel;

        public Panel TreeViewContainer;

        // Properties
        public ContextMenuStrip NodesContextMenu
        {
            get => contextMenu;
            set => contextMenu = value;
        }

        [Browsable(false)]
        public ITreeViewSource Source
        {
            get => source;
            set
            {
                source = value;
                if (value != null)
                    value.Parent = this;
            }
        }

        [Browsable(false)]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public System.Windows.Forms.TreeNode Root;

        public bool AutoExpandOnFilter
        {
            get => filterPanel.AutoExpandOnFilter;
            set => filterPanel.AutoExpandOnFilter = value;
        }

        public bool FilterVisible
        {
            get => filterPanel.Visible;
            set => filterPanel.Visible = value;
        }

        private bool multiSelect;

        public bool MultiSelect
        {
            get => multiSelect;
            set => multiSelect = value;
        }

        private bool selectableGroups;

        public bool SelectableGroups
        {
            get => selectableGroups;
            set => selectableGroups = value;
        }

        [Browsable(false)]
        public ReadOnlyCollection<System.Windows.Forms.TreeNode> SelectedNodes => selectedNodes.AsReadOnly();

        public new TreeNodeCollection Nodes => base.Nodes;

        // Methods
        internal System.Windows.Forms.TreeNode ProcessItemPath(string[] segments, bool createPath = true, int imageIndexCollapsed = -1,
            int imageIndexExpanded = -1)
        {
            System.Windows.Forms.TreeNode node = null;

            var firstSegment = segments[0];
            if (Nodes.ContainsKey(firstSegment))
                node = Nodes[firstSegment];
            else
            {
                node = new TreeNode(firstSegment)
                {
                    Text = firstSegment,
                    NodeType = TreeNodeType.GroupItem
                };

                Nodes.Add(node);
            }
            segments = segments.Skip(1).ToArray();

            foreach (string segment in segments)
            {
                if (segment == string.Empty)
                    continue;

                if (node.Nodes.ContainsKey(segment))
                {
                    node = node.Nodes[segment] as TreeNode;
                    continue;
                }

                if (!createPath)
                    return null;

                TreeNode newNode = new TreeNode(segment)
                {
                    ImageIndexCollapsed = imageIndexCollapsed,
                    ImageIndexExpanded = imageIndexExpanded,
                    Name = segment,
                    NodeType = TreeNodeType.GroupItem,
                    ContextMenuStrip = contextMenu
                };

                if (imageIndexCollapsed == -1)
                {
                    newNode.ImageIndex = 0;
                    newNode.SelectedImageIndex = 0;
                }
                else
                {
                    newNode.ImageIndex = imageIndexCollapsed;
                    newNode.SelectedImageIndex = imageIndexCollapsed;
                }

                node.Nodes.Add(newNode);
                node = newNode;
            }

            return node;
        }

        internal System.Windows.Forms.TreeNode ProcessItemPath(string segments, string separator)
        {
            return ProcessItemPath(segments.Split(new string[] { separator }, StringSplitOptions.RemoveEmptyEntries));
        }

        internal System.Windows.Forms.TreeNode ProcessItemPath(string segments)
        {
            return ProcessItemPath(segments, PathSeparator);
        }

        public void TrackActiveNode(string fullPath)
        {
            System.Windows.Forms.TreeNode node = GetNode(fullPath);
            if (node != null)
                SelectedNode = node;
        }

        public TreeNode GetNode(string fullPath)
        {
            string[] segments = fullPath.Split('/');
            System.Windows.Forms.TreeNode node = Root;
            foreach (string segment in segments)
            {
                if (segment == string.Empty)
                    continue;

                if (node.Nodes.ContainsKey(segment))
                {
                    node = node.Nodes[segment];
                    continue;
                }
            }
            if (node == Root)
                return null;
            return node as TreeNode;
        }

        public new void Refresh()
        {
            Nodes.Clear();
            source?.Refresh();
            base.Refresh();
        }

        public void AddItem(IEnumerable<string> items)
        {
            foreach (string item in items)
                AddItem(item);
        }

        public TreeNode AddItem(string filePath) => AddItem(filePath, -1);

        public TreeNode AddItem(string filePath, int imageIndex)
        {
            string[] segments = filePath.Split('/');
            string fileName = segments[segments.Length - 1];
            segments[segments.Length - 1] = "";

            System.Windows.Forms.TreeNode nod = null;

            if (segments.Length > 1)
            {
                if (imageIndex == -1)
                    nod = ProcessItemPath(segments);
                else
                    nod = ProcessItemPath(segments, false);
            }

            TreeNode node = new TreeNode(fileName)
            {
                Tag = null,
                NodeType = TreeNodeType.SingleItem,
                ContextMenuStrip = contextMenu
            };

            if (imageIndex == -1)
            {
                node.ImageIndex = 2;
                node.SelectedImageIndex = 2;
            }
            else
            {
                node.ImageIndex = imageIndex;
                node.SelectedImageIndex = imageIndex;
                node.ImageIndexExpanded = imageIndex;
                node.ImageIndexCollapsed = imageIndex;
            }

            if (nod != null)
                nod.Nodes.Add(node);
            else
                Nodes.Add(node);

            System.Windows.Forms.TreeNode selected = SelectedNode;
            Sort();
            SelectedNode = selected;
            return node;
        }

        public TreeNode AddGroup(string folderPath)
        {
            return ProcessItemPath(folderPath.Split('/')) as TreeNode;
        }

        public TreeNode AddGroup(string folderPath, int imageIndexCollapsed, int imageIndexExpanded)
        {
            return ProcessItemPath(folderPath.Split('/'), true, imageIndexCollapsed, imageIndexExpanded) as TreeNode;
        }

        public void RemoveItem(string folderPath)
        {
            System.Windows.Forms.TreeNode node = ProcessItemPath(folderPath.Split('/'), false);
            node?.Remove();
        }

        public void RemoveGroup(string groupPath)
        {
            RemoveItem(groupPath);
        }

        public void ChangeItemContext(string filePath, object context)
        {
            string[] segments = filePath.Split('/');
            string fileName = segments[segments.Length - 1];

            segments[segments.Length - 1] = "";

            System.Windows.Forms.TreeNode dir = ProcessItemPath(segments, false);
            if (dir != null && dir.Nodes.ContainsKey(fileName))
                dir.Nodes[fileName].Tag = context;
        }

        public void CreateItem(object sender, EventArgs e)
        {
            ItemCreate?.Invoke(this, new EventArgs());
        }

        public void CreateGroup(object sender, EventArgs e)
        {
            GroupCreate?.Invoke(this, new EventArgs());
        }

        public void RemoveItem(object sender, EventArgs e)
        {
            ItemRemove?.Invoke(this, new EventArgs());
        }

        public void OnItemsLoaded()
        {
            ItemsLoaded?.Invoke(this, EventArgs.Empty);
        }
    }
}
