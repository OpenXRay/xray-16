using System;
using System.ComponentModel;
using System.Collections;
using System.Windows.Forms;
using System.Data;
using System.Drawing;

namespace xrPostprocessEditor
{
    partial class KeyFrameBox
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.lbKeyFrames = new System.Windows.Forms.ListBox();
            this.lHeader = new System.Windows.Forms.Label();
            this.tlpPointList = new System.Windows.Forms.TableLayoutPanel();
            this.flpPointListControls = new System.Windows.Forms.FlowLayoutPanel();
            this.btnAdd = new System.Windows.Forms.Button();
            this.btnRemove = new System.Windows.Forms.Button();
            this.btnClear = new System.Windows.Forms.Button();
            this.btnCopyFrom = new XRay.SdkControls.MenuButton();
            this.pnBottom = new System.Windows.Forms.Panel();
            this.numKeyFrameTime = new System.Windows.Forms.NumericUpDown();
            this.lKeyFrameTime = new System.Windows.Forms.Label();
            this.tlpPointList.SuspendLayout();
            this.flpPointListControls.SuspendLayout();
            this.pnBottom.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numKeyFrameTime)).BeginInit();
            this.SuspendLayout();
            // 
            // lbKeyFrames
            // 
            this.lbKeyFrames.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lbKeyFrames.IntegralHeight = false;
            this.lbKeyFrames.Location = new System.Drawing.Point(3, 19);
            this.lbKeyFrames.Name = "lbKeyFrames";
            this.lbKeyFrames.Size = new System.Drawing.Size(177, 27);
            this.lbKeyFrames.TabIndex = 0;
            this.lbKeyFrames.SelectedIndexChanged += new System.EventHandler(this.lbKeyFrames_SelectedIndexChanged);
            // 
            // lHeader
            // 
            this.lHeader.AutoSize = true;
            this.lHeader.Location = new System.Drawing.Point(3, 0);
            this.lHeader.Name = "lHeader";
            this.lHeader.Size = new System.Drawing.Size(59, 13);
            this.lHeader.TabIndex = 1;
            this.lHeader.Text = "Key frames";
            // 
            // tlpPointList
            // 
            this.tlpPointList.ColumnCount = 1;
            this.tlpPointList.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpPointList.Controls.Add(this.flpPointListControls, 0, 3);
            this.tlpPointList.Controls.Add(this.lHeader, 0, 0);
            this.tlpPointList.Controls.Add(this.lbKeyFrames, 0, 1);
            this.tlpPointList.Controls.Add(this.pnBottom, 0, 2);
            this.tlpPointList.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpPointList.Location = new System.Drawing.Point(0, 0);
            this.tlpPointList.Name = "tlpPointList";
            this.tlpPointList.RowCount = 4;
            this.tlpPointList.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 16F));
            this.tlpPointList.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpPointList.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 21F));
            this.tlpPointList.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 30F));
            this.tlpPointList.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tlpPointList.Size = new System.Drawing.Size(183, 100);
            this.tlpPointList.TabIndex = 2;
            // 
            // flpPointListControls
            // 
            this.flpPointListControls.Controls.Add(this.btnAdd);
            this.flpPointListControls.Controls.Add(this.btnRemove);
            this.flpPointListControls.Controls.Add(this.btnClear);
            this.flpPointListControls.Controls.Add(this.btnCopyFrom);
            this.flpPointListControls.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flpPointListControls.Location = new System.Drawing.Point(0, 70);
            this.flpPointListControls.Margin = new System.Windows.Forms.Padding(0);
            this.flpPointListControls.Name = "flpPointListControls";
            this.flpPointListControls.Size = new System.Drawing.Size(183, 30);
            this.flpPointListControls.TabIndex = 3;
            // 
            // btnAdd
            // 
            this.btnAdd.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.btnAdd.Location = new System.Drawing.Point(3, 3);
            this.btnAdd.Name = "btnAdd";
            this.btnAdd.Size = new System.Drawing.Size(24, 24);
            this.btnAdd.TabIndex = 2;
            this.btnAdd.Text = "+";
            this.btnAdd.UseVisualStyleBackColor = true;
            this.btnAdd.Click += new System.EventHandler(this.btnAdd_Click);
            // 
            // btnRemove
            // 
            this.btnRemove.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.btnRemove.Location = new System.Drawing.Point(30, 3);
            this.btnRemove.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.btnRemove.Name = "btnRemove";
            this.btnRemove.Size = new System.Drawing.Size(24, 24);
            this.btnRemove.TabIndex = 4;
            this.btnRemove.Text = "-";
            this.btnRemove.UseVisualStyleBackColor = true;
            this.btnRemove.Click += new System.EventHandler(this.btnRemove_Click);
            // 
            // btnClear
            // 
            this.btnClear.Location = new System.Drawing.Point(57, 3);
            this.btnClear.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.btnClear.Name = "btnClear";
            this.btnClear.Size = new System.Drawing.Size(48, 24);
            this.btnClear.TabIndex = 3;
            this.btnClear.Text = "Clear";
            this.btnClear.UseVisualStyleBackColor = true;
            this.btnClear.Click += new System.EventHandler(this.btnClear_Click);
            // 
            // btnCopyFrom
            // 
            this.btnCopyFrom.Location = new System.Drawing.Point(108, 3);
            this.btnCopyFrom.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.btnCopyFrom.Name = "btnCopyFrom";
            this.btnCopyFrom.Size = new System.Drawing.Size(72, 24);
            this.btnCopyFrom.TabIndex = 4;
            this.btnCopyFrom.Text = "Copy from";
            this.btnCopyFrom.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.btnCopyFrom.UseVisualStyleBackColor = true;
            // 
            // pnBottom
            // 
            this.pnBottom.Controls.Add(this.numKeyFrameTime);
            this.pnBottom.Controls.Add(this.lKeyFrameTime);
            this.pnBottom.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnBottom.Location = new System.Drawing.Point(0, 49);
            this.pnBottom.Margin = new System.Windows.Forms.Padding(0);
            this.pnBottom.Name = "pnBottom";
            this.pnBottom.Size = new System.Drawing.Size(183, 21);
            this.pnBottom.TabIndex = 4;
            // 
            // numKeyFrameTime
            // 
            this.numKeyFrameTime.DecimalPlaces = 2;
            this.numKeyFrameTime.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.numKeyFrameTime.Location = new System.Drawing.Point(100, 0);
            this.numKeyFrameTime.Maximum = new decimal(new int[] {
            9000,
            0,
            0,
            0});
            this.numKeyFrameTime.Name = "numKeyFrameTime";
            this.numKeyFrameTime.Size = new System.Drawing.Size(80, 21);
            this.numKeyFrameTime.TabIndex = 4;
            this.numKeyFrameTime.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.numKeyFrameTime.ValueChanged += new System.EventHandler(this.numKeyFrameTime_ValueChanged);
            // 
            // lKeyFrameTime
            // 
            this.lKeyFrameTime.AutoSize = true;
            this.lKeyFrameTime.Location = new System.Drawing.Point(3, 2);
            this.lKeyFrameTime.Name = "lKeyFrameTime";
            this.lKeyFrameTime.Size = new System.Drawing.Size(90, 13);
            this.lKeyFrameTime.TabIndex = 3;
            this.lKeyFrameTime.Text = "Key frame time (s)";
            // 
            // KeyFrameBox
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.tlpPointList);
            this.MinimumSize = new System.Drawing.Size(183, 100);
            this.Name = "KeyFrameBox";
            this.Size = new System.Drawing.Size(183, 100);
            this.tlpPointList.ResumeLayout(false);
            this.tlpPointList.PerformLayout();
            this.flpPointListControls.ResumeLayout(false);
            this.pnBottom.ResumeLayout(false);
            this.pnBottom.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.numKeyFrameTime)).EndInit();
            this.ResumeLayout(false);

        }

        private System.Windows.Forms.ListBox lbKeyFrames;
        private System.Windows.Forms.Label lHeader;
        private System.Windows.Forms.TableLayoutPanel tlpPointList;
        private System.Windows.Forms.FlowLayoutPanel flpPointListControls;
        private System.Windows.Forms.Button btnAdd;
        private System.Windows.Forms.Button btnRemove;
        private System.Windows.Forms.Button btnClear;
        private XRay.SdkControls.MenuButton btnCopyFrom;

        #endregion
        private Panel pnBottom;
        private Label lKeyFrameTime;
        private NumericUpDown numKeyFrameTime;

    }
}
