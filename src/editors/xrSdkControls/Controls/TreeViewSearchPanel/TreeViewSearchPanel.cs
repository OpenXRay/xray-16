using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace XRay.SdkControls
{
    public partial class TreeViewSearchPanel : UserControl
    {
        public TreeViewSearchPanel()
        {
            InitializeComponent();
            findedNodes = new List<TreeNode>();
            searchNext.Enabled = false;
            searchPrev.Enabled = false;
        }

        private List<TreeNode> findedNodes;
        private int searchResultIndex;
        private Color selectedNodePrevColor;

        public TreeView ParentTreeView;

        private static void ExpandParentLine(TreeNode node)
        {
            while (node.Parent != null)
            {
                node = node.Parent;
                node.Expand();
            }
        }

        private static void ColorizeNodes(TreeNodeCollection nodes, Color foreColor, Color backColor)
        {
            foreach(TreeNode node in nodes)
            {
                node.ForeColor = foreColor;
                node.BackColor = backColor;
                ColorizeNodes(node.Nodes, foreColor, backColor);
            }
        }

        private void SearchNodes(TreeNodeCollection nodes, string searchStr)
        {
            foreach(TreeNode node in nodes)

            {
                node.BackColor = Color.White;
                node.Collapse();

                if (node.Text.ToLower().Contains(searchStr))
                {
                    node.BackColor = Color.FromArgb(220, 255, 220);
                    findedNodes.Add(node);
                    ExpandParentLine(node);
                }
                SearchNodes(node.Nodes, searchStr);
            }
        }

        private void searchBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                findedNodes.Clear();
                string searchStr = (sender as TextBox)?.Text.ToLower();
                if (searchStr != string.Empty)
                    SearchNodes(ParentTreeView.Nodes, searchStr);
                ParentTreeView.SelectedNode = null;

                if (findedNodes.Count == 0)
                    MessageBox.Show("There is no matches!");
                else if (findedNodes.Count == 1)
                    resultLabel.Text = "Result: 1 / 1";
                else
                {
                    resultLabel.Enabled = true;
                    searchNext.Enabled = true;
                    searchResultIndex = 0;

                    resultLabel.Text = "Result: 1 / " + findedNodes.Count;

                    findedNodes[0].EnsureVisible();
                    selectedNodePrevColor = findedNodes[0].BackColor;
                    findedNodes[0].BackColor = Color.Coral;
                }
            }
            if (Visible && e.KeyCode == Keys.Escape)
            {
                Close();
                e.Handled = true;
            }
        }

        private void searchPrev_Click(object sender, EventArgs e)
        {
            searchNext.Enabled = true;
            if (findedNodes[searchResultIndex].BackColor == Color.Coral)
                findedNodes[searchResultIndex].BackColor = selectedNodePrevColor;
            selectedNodePrevColor = findedNodes[--searchResultIndex].BackColor;
            findedNodes[searchResultIndex].BackColor = Color.Coral;
            findedNodes[searchResultIndex].EnsureVisible();
            ParentTreeView.SelectedNode = null;
            resultLabel.Text = "Result: " + (searchResultIndex + 1) + " / " + findedNodes.Count;
            if (searchResultIndex == 0)
                searchPrev.Enabled = false;
        }

        private void searchNext_Click(object sender, EventArgs e)
        {
            searchPrev.Enabled = true;
            if (findedNodes[searchResultIndex].BackColor == Color.Coral)
                findedNodes[searchResultIndex].BackColor = selectedNodePrevColor;
            selectedNodePrevColor = findedNodes[++searchResultIndex].BackColor;
            findedNodes[searchResultIndex].BackColor = Color.Coral;
            findedNodes[searchResultIndex].EnsureVisible();
            ParentTreeView.SelectedNode = null;
            resultLabel.Text = "Result: " + (searchResultIndex + 1) + " / " + findedNodes.Count;

            if (searchResultIndex == findedNodes.Count - 1)
                searchNext.Enabled = false;
        }

        private void searchClose_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void searchButton_Click(object sender, EventArgs e)
        {
            searchBox_KeyDown(searchBox, new KeyEventArgs(Keys.Enter));
        }

        private void Close()
        {
            if (Visible)
            {
                Visible = false;
                ColorizeNodes(ParentTreeView.Nodes, ParentTreeView.ForeColor, ParentTreeView.BackColor);
            }

            resultLabel.Enabled = false;
            searchNext.Enabled = false;
            searchPrev.Enabled = false;
            findedNodes.Clear();
            searchResultIndex = -1;
            resultLabel.Text = "Results: - / -";
        }
    }
}
