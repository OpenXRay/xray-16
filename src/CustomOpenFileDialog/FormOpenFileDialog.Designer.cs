namespace CustomControls
{
    partial class FormOpenFileDialog
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.pbxPreview = new System.Windows.Forms.PictureBox();
            this.lblColors = new System.Windows.Forms.Label();
            this.lblFormat = new System.Windows.Forms.Label();
            this.lblSize = new System.Windows.Forms.Label();
            this.lblSizeValue = new System.Windows.Forms.Label();
            this.lblFormatValue = new System.Windows.Forms.Label();
            this.lblColorsValue = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbxPreview)).BeginInit();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.pbxPreview);
            this.groupBox1.Location = new System.Drawing.Point(5, 30);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(226, 220);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Preview";
            // 
            // pbxPreview
            // 
            this.pbxPreview.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pbxPreview.Location = new System.Drawing.Point(3, 16);
            this.pbxPreview.Name = "pbxPreview";
            this.pbxPreview.Size = new System.Drawing.Size(220, 201);
            this.pbxPreview.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.pbxPreview.TabIndex = 4;
            this.pbxPreview.TabStop = false;
            // 
            // lblColors
            // 
            this.lblColors.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lblColors.Location = new System.Drawing.Point(2, 296);
            this.lblColors.Name = "lblColors";
            this.lblColors.Size = new System.Drawing.Size(42, 13);
            this.lblColors.TabIndex = 3;
            this.lblColors.Text = "Colors:";
            // 
            // lblFormat
            // 
            this.lblFormat.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lblFormat.Location = new System.Drawing.Point(2, 260);
            this.lblFormat.Name = "lblFormat";
            this.lblFormat.Size = new System.Drawing.Size(42, 13);
            this.lblFormat.TabIndex = 4;
            this.lblFormat.Text = "Format:";
            // 
            // lblSize
            // 
            this.lblSize.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lblSize.Location = new System.Drawing.Point(2, 278);
            this.lblSize.Name = "lblSize";
            this.lblSize.Size = new System.Drawing.Size(42, 13);
            this.lblSize.TabIndex = 5;
            this.lblSize.Text = "Size:";
            // 
            // lblSizeValue
            // 
            this.lblSizeValue.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lblSizeValue.Location = new System.Drawing.Point(50, 278);
            this.lblSizeValue.Name = "lblSizeValue";
            this.lblSizeValue.Size = new System.Drawing.Size(178, 13);
            this.lblSizeValue.TabIndex = 8;
            // 
            // lblFormatValue
            // 
            this.lblFormatValue.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lblFormatValue.Location = new System.Drawing.Point(50, 260);
            this.lblFormatValue.Name = "lblFormatValue";
            this.lblFormatValue.Size = new System.Drawing.Size(178, 13);
            this.lblFormatValue.TabIndex = 7;
            // 
            // lblColorsValue
            // 
            this.lblColorsValue.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.lblColorsValue.Location = new System.Drawing.Point(50, 296);
            this.lblColorsValue.Name = "lblColorsValue";
            this.lblColorsValue.Size = new System.Drawing.Size(178, 13);
            this.lblColorsValue.TabIndex = 6;
            // 
            // FormOpenFileDialog
            // 
            this.Controls.Add(this.lblSizeValue);
            this.Controls.Add(this.lblFormatValue);
            this.Controls.Add(this.lblColorsValue);
            this.Controls.Add(this.lblSize);
            this.Controls.Add(this.lblFormat);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.lblColors);
            this.Name = "FormOpenFileDialog";
            this.Size = new System.Drawing.Size(237, 318);
            this.groupBox1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.pbxPreview)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lblColors;
        private System.Windows.Forms.PictureBox pbxPreview;
        private System.Windows.Forms.Label lblFormat;
        private System.Windows.Forms.Label lblSize;
        private System.Windows.Forms.Label lblSizeValue;
        private System.Windows.Forms.Label lblFormatValue;
        private System.Windows.Forms.Label lblColorsValue;
    }
}