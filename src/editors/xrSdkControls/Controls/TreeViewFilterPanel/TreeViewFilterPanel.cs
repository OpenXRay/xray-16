using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public partial class TreeViewFilterPanel : UserControl
    {
        public TreeViewFilterPanel()
        {
            InitializeComponent();
            invisibleNodes = new Dictionary<string, TreeNode>();
            invisibleFolders = new Dictionary<string, string>();
        }

        private Dictionary<string, TreeNode> invisibleNodes;
        private Dictionary<string, string> invisibleFolders;

        public TreeView ParentTreeView;
        public bool AutoExpandOnFilter;

        private static void ExpandParentLine(TreeNode node)
        {
            while (node.Parent != null)
            {
                node = node.Parent;
                node.Expand();
            }
        }

        private void OnFilterTextChanged(object sender, EventArgs e)
        {
            string filter = (sender as TextBox)?.Text.ToLower();

            if (filter != string.Empty)
                FilterNodes(ParentTreeView.Nodes, filter);
            FilterInvisibleNodes(filter);

            ParentTreeView.Sort();
        }

        private void FilterInvisibleNodes(string filter)
        {
            string[] keys = new string[invisibleNodes.Count];

            invisibleNodes.Keys.CopyTo(keys, 0);

            foreach (string key in keys)
            {
                var segments = key.Split('/');
                if (invisibleNodes[key].Text.ToLower().Contains(filter))
                {
                    segments[segments.Length - 1] = "";
                    ParentTreeView.ProcessItemPath(segments).Nodes.Add(invisibleNodes[key]);
                    if (AutoExpandOnFilter && filter != string.Empty)
                        ExpandParentLine(invisibleNodes[key]);
                    invisibleNodes.Remove(key);
                }
            }

            keys = new string[invisibleFolders.Count];
            invisibleFolders.Keys.CopyTo(keys, 0);

            foreach(string key in keys)
            {
                if (invisibleFolders[key].ToLower().Contains(filter))
                {
                    TreeNode newNode = ParentTreeView.ProcessItemPath(key.Split('/')) as TreeNode;
                    if (AutoExpandOnFilter && filter != string.Empty)
                        ExpandParentLine(newNode);
                    invisibleFolders.Remove(key);
                }
            }
        }
        private void FilterNodes(TreeNodeCollection nodes, string filter)
        {
            for (int i = 0; i < nodes.Count; ++i)
            {
                var node = nodes[i] as TreeNode;
                if (node == null)
                    continue;

                if (node.NodeType == TreeNodeType.SingleItem)
                {
                    if (!node.Text.ToLower().Contains(filter))
                    {
                        invisibleNodes.Add(node.FullPath, node);
                        nodes.RemoveAt(i--);
                    }
                    else
                    if (AutoExpandOnFilter)
                        ExpandParentLine(node);
                }
                else
                {
                    FilterNodes(node.Nodes, filter);
                    if (node.Nodes.Count != 0 && node.Text.ToLower().Contains(filter))
                        continue;

                    invisibleFolders.Add(node.FullPath, node.Text);
                    nodes.RemoveAt(i--);
                    node.Collapse();
                }
            }
        }
    }
}
