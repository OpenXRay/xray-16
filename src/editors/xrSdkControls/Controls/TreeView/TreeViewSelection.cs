using System;
using System.Drawing;
using System.Collections.Generic;

namespace XRay.SdkControls
{
    partial class TreeView
    {
        private void SelectNode(System.Windows.Forms.TreeNode node)
        {
            if (!(node is TreeNode nod) || nod.IsSelected)
                return;

            nod.IsSelected = true;
            nod.BackColor = SystemColors.Highlight;
            nod.ForeColor = SystemColors.HighlightText;
            selectedNodes.Add(nod);
            Invalidate(nod.Bounds);
        }

        private void SelectSubNodes(System.Windows.Forms.TreeNode node)
        {
            foreach (TreeNode subNode in node.Nodes)
            {
                SelectNode(subNode);
                SelectSubNodes(subNode);
            }
        }

        private void SelectAll()
        {
            if (Nodes.Count == 0)
                return;

            for (var node = Nodes[0]; node != null; node = node.NextVisibleNode)
            {
                if (!(!SelectableGroups && ((TreeNode)node).NodeType == TreeNodeType.GroupItem))
                    SelectNode(node);
            }
        }
        private void DeselectNode(System.Windows.Forms.TreeNode node)
        {
            if (!(node is TreeNode nod))
                return;

            nod.IsSelected = false;
            nod.BackColor = Color.Empty;
            nod.ForeColor = Color.Empty;

            selectedNodes.Remove(nod);
            Invalidate(nod.Bounds);
        }

        private void DeselectNodes(List<System.Windows.Forms.TreeNode> nodes)
        {
            nodes.ForEach(DeselectNode);
            //foreach (var node in nodes)
            //    DeselectNode(node);
        }
        private void DeselectAllNodes()
        {
            selectedNodes.ForEach(DeselectNode);
        }
    }
}
