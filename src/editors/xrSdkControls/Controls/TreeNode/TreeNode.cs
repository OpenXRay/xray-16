namespace XRay.SdkControls
{
    public enum TreeNodeType
    {
        SingleItem,
        GroupItem
    }

    public class TreeNode : System.Windows.Forms.TreeNode
    {
        public TreeNode() {}

        public TreeNode(string name) : base(name) => Name = name;

        public TreeNode(string name, int imageIndex, int selectedImageIndex)
            : base(name, imageIndex, selectedImageIndex) => Name = name;

        public TreeNode(string name, int imageIndex, int selectedImageIndex, System.Windows.Forms.TreeNode[] children)
            : base(name, imageIndex, selectedImageIndex, children) => Name = name;

        public TreeNode(string name, System.Windows.Forms.TreeNode[] children)
            : base(name, children) => Name = name;

        public TreeNodeType NodeType;

        public new bool IsSelected;
        public int ImageIndexExpanded;
        public int ImageIndexCollapsed;

        public new TreeNode Parent => base.Parent as TreeNode;

        public TreeNode AddNodeSingle(string name, int imageIndex = -1)
        {
            TreeNode node = new TreeNode(name)
            {
                NodeType = TreeNodeType.SingleItem,
                ContextMenuStrip = ContextMenuStrip
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
            }

            Nodes.Add(node);
            return node;
        }

        public TreeNode AddNodeGroup(string name, int imageIndexExpanded = -1, int imageIndexCollapsed = -1)
        {
            TreeNode node = new TreeNode(name)
            {
                ImageIndexCollapsed = imageIndexCollapsed,
                ImageIndexExpanded = imageIndexExpanded,
                Name = name,
                NodeType = TreeNodeType.SingleItem,
                ContextMenuStrip = ContextMenuStrip
            };

            if (imageIndexCollapsed == -1)
            {
                node.ImageIndex = 0;
                node.SelectedImageIndex = 0;
            }
            else
            {
                node.ImageIndex = imageIndexCollapsed;
                node.SelectedImageIndex = imageIndexCollapsed;
            }

            Nodes.Add(node);
            return node;
        }
    }
}
