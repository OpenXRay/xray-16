using System;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    partial class TreeView
    {
        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (e.Control)
            {
                switch (e.KeyCode)
                {
                case Keys.F:
                    searchPanel.Visible = true;
                    searchPanel.Enabled = true;
                    searchPanel.searchBox.Text = "";
                    searchPanel.searchBox.Focus();
                    searchPanel.searchNext.Enabled = false;
                    searchPanel.searchNext.Enabled = false;
                    e.Handled = true;
                    break;
                case Keys.A:
                    if (MultiSelect)
                        SelectAll();
                    break;
                case Keys.D:
                    DeselectAllNodes();
                    break;
                }
            }

            base.OnKeyDown(e);
        }

        protected override void OnKeyUp(KeyEventArgs e)
        {
            base.OnKeyUp(e);
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                if (selectedNodes.Contains(GetNodeAt(e.Location)))
                    isRightClicked = true;
            }

            base.OnMouseDown(e);
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            isRightClicked = false;
            base.OnMouseUp(e);
        }

        protected override void OnNodeMouseClick(TreeNodeMouseClickEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
                SelectedNode = e.Node;

            base.OnNodeMouseClick(e);
        }

        protected override void OnAfterCollapse(TreeViewEventArgs e)
        {
            if (!(e.Node is TreeNode node))
                return;

            if (node.ImageIndexCollapsed == -1)
            {
                e.Node.ImageIndex = 0;
                e.Node.SelectedImageIndex = 0;
            }
            else
            {
                e.Node.ImageIndex = node.ImageIndexCollapsed;
                e.Node.SelectedImageIndex = node.ImageIndexCollapsed;
            }

            base.OnAfterCollapse(e);
        }

        protected override void OnAfterExpand(TreeViewEventArgs e)
        {
            if (!(e.Node is TreeNode node))
                return;

            if (node.ImageIndexCollapsed == -1)
            {
                e.Node.ImageIndex = 1;
                e.Node.SelectedImageIndex = 1;
            }
            else
            {
                e.Node.ImageIndex = node.ImageIndexExpanded;
                e.Node.SelectedImageIndex = node.ImageIndexExpanded;
            }

            base.OnAfterExpand(e);
        }

        protected override void OnBeforeSelect(TreeViewCancelEventArgs e)
        {
            if (isRightClicked || !SelectableGroups && (e.Node as TreeNode).NodeType == TreeNodeType.GroupItem)
            {
                e.Cancel = true;
                base.OnBeforeSelect(e);
                return;
            }

            if (MultiSelect)
            {
                if (ModifierKeys == Keys.Control && selectedNodes.Contains(e.Node))
                {
                    if (justSelectedNode == e.Node)
                    {
                        justSelectedNode = null;
                        e.Cancel = true;
                        return;
                    }
                    DeselectNode(e.Node);
                    SelectedNode = null;
                    e.Cancel = true;
                    SelectedItemsChanged?.Invoke(this, EventArgs.Empty);
                }
            }

            base.OnBeforeSelect(e);
        }

        protected override void OnAfterSelect(TreeViewEventArgs e)
        {
            if (!MultiSelect)
            {
                base.OnAfterSelect(e);
                selectedNodes.Clear();
                selectedNodes.Add(e.Node);
                SelectedItemsChanged?.Invoke(this, EventArgs.Empty);
                return;
            }

            justSelectedNode = SelectedNode;
            SelectedNode = null;
            if (ModifierKeys == Keys.Control && !selectedNodes.Contains(e.Node))
            {
                SelectNode(e.Node);
                lastSelectedNode = e.Node;
                base.OnAfterSelect(e);
                SelectedItemsChanged?.Invoke(this, EventArgs.Empty);
                justSelectedNode = null;
                return;
            }
            else if (!(ModifierKeys == Keys.Shift) || (ModifierKeys == Keys.Control && lastSelectedNode == null))
            {
                DeselectNodes(selectedNodes);
                SelectNode(e.Node);
                lastSelectedNode = e.Node;
                base.OnAfterSelect(e);
                SelectedItemsChanged?.Invoke(this, EventArgs.Empty);
                justSelectedNode = null;
                return;
            }

            if (lastSelectedNode != null)
            {
                //Shift pressed
                TreeNode lowerNode;
                TreeNode upperNode;
                TreeNode prevLowerNode; // why it's not used?

                //Set upper - lower nodes
                if (lastSelectedNode.Bounds.Y < e.Node.Bounds.Y)
                {
                    upperNode = lastSelectedNode as TreeNode;
                    lowerNode = e.Node as TreeNode;
                }
                else
                {
                    upperNode = e.Node as TreeNode;
                    lowerNode = lastSelectedNode as TreeNode;
                }
                prevLowerNode = lowerNode;

                while (lowerNode != null && lowerNode != upperNode)
                {
                    if (!(!SelectableGroups && lowerNode.NodeType == TreeNodeType.GroupItem))
                        SelectNode(lowerNode);
                    prevLowerNode = lowerNode;
                    lowerNode = lowerNode.PrevVisibleNode as TreeNode;
                }
                if (lowerNode != null)
                    SelectNode(lowerNode);

                lastSelectedNode = e.Node as TreeNode;
            }
            lastSelectedNode = e.Node as TreeNode;
            base.OnAfterSelect(e);
            SelectedItemsChanged?.Invoke(this, EventArgs.Empty);
            justSelectedNode = null;
        }

        protected override void OnParentChanged(EventArgs e)
        {
            if (/*DesignMode ||*/ isContainerCreated)
            {
                base.OnParentChanged(e);
                return;
            }

            isContainerCreated = true;
            TreeViewContainer.Location = Location;
            TreeViewContainer.Size = Size;
            if (Dock == DockStyle.None)
                TreeViewContainer.Anchor = Anchor;
            else
                TreeViewContainer.Dock = Dock;

            var parent = Parent;
            parent.SuspendLayout();

            var childIndex = parent.Controls.GetChildIndex(this);
            parent.Controls.Remove(this);

            var filterVisible = filterPanel.Visible;
            parent.Controls.Add(TreeViewContainer);
            parent.Controls.SetChildIndex(TreeViewContainer, childIndex);
            filterPanel.Visible = filterVisible;

            base.OnParentChanged(e);

            Dock = DockStyle.Fill;
            TreeViewContainer.Controls.Add(this);
            TreeViewContainer.Controls.SetChildIndex(this, 0);

            parent.ResumeLayout();
        }
    }
}
