namespace XRay.SdkControls
{
    partial class ColorPicker
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ColorPicker));
            this.tbrRed = new System.Windows.Forms.TrackBar();
            this.tbrGreen = new System.Windows.Forms.TrackBar();
            this.tbrAlpha = new System.Windows.Forms.TrackBar();
            this.tbrBlue = new System.Windows.Forms.TrackBar();
            this.lRed = new System.Windows.Forms.Label();
            this.lGreen = new System.Windows.Forms.Label();
            this.lBlue = new System.Windows.Forms.Label();
            this.lAlpha = new System.Windows.Forms.Label();
            this.chkHexademical = new System.Windows.Forms.CheckBox();
            this.tbHexColor = new System.Windows.Forms.TextBox();
            this.lHexColor = new System.Windows.Forms.Label();
            this.pbColor = new XRay.SdkControls.ColorSampleBox();
            this.numAlpha = new XRay.SdkControls.IntegerUpDown();
            this.numBlue = new XRay.SdkControls.IntegerUpDown();
            this.numGreen = new XRay.SdkControls.IntegerUpDown();
            this.numRed = new XRay.SdkControls.IntegerUpDown();
            ((System.ComponentModel.ISupportInitialize)(this.tbrRed)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tbrGreen)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tbrAlpha)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.tbrBlue)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numAlpha)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numBlue)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numGreen)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numRed)).BeginInit();
            this.SuspendLayout();
            // 
            // tbrRed
            // 
            this.tbrRed.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbrRed.AutoSize = false;
            this.tbrRed.BackColor = System.Drawing.SystemColors.Window;
            this.tbrRed.Location = new System.Drawing.Point(17, 8);
            this.tbrRed.Margin = new System.Windows.Forms.Padding(3, 3, 0, 3);
            this.tbrRed.Maximum = 255;
            this.tbrRed.Name = "tbrRed";
            this.tbrRed.Size = new System.Drawing.Size(81, 23);
            this.tbrRed.SmallChange = 16;
            this.tbrRed.TabIndex = 4;
            this.tbrRed.TabStop = false;
            this.tbrRed.TickFrequency = 64;
            this.tbrRed.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // tbrGreen
            // 
            this.tbrGreen.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbrGreen.AutoSize = false;
            this.tbrGreen.BackColor = System.Drawing.SystemColors.Window;
            this.tbrGreen.Location = new System.Drawing.Point(17, 34);
            this.tbrGreen.Margin = new System.Windows.Forms.Padding(3, 3, 0, 3);
            this.tbrGreen.Maximum = 255;
            this.tbrGreen.Name = "tbrGreen";
            this.tbrGreen.Size = new System.Drawing.Size(81, 23);
            this.tbrGreen.SmallChange = 16;
            this.tbrGreen.TabIndex = 5;
            this.tbrGreen.TabStop = false;
            this.tbrGreen.TickFrequency = 64;
            this.tbrGreen.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // tbrAlpha
            // 
            this.tbrAlpha.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbrAlpha.AutoSize = false;
            this.tbrAlpha.BackColor = System.Drawing.SystemColors.Window;
            this.tbrAlpha.Location = new System.Drawing.Point(17, 86);
            this.tbrAlpha.Margin = new System.Windows.Forms.Padding(3, 3, 0, 3);
            this.tbrAlpha.Maximum = 255;
            this.tbrAlpha.Name = "tbrAlpha";
            this.tbrAlpha.Size = new System.Drawing.Size(81, 23);
            this.tbrAlpha.SmallChange = 16;
            this.tbrAlpha.TabIndex = 7;
            this.tbrAlpha.TabStop = false;
            this.tbrAlpha.TickFrequency = 64;
            this.tbrAlpha.TickStyle = System.Windows.Forms.TickStyle.None;
            this.tbrAlpha.Value = 255;
            // 
            // tbrBlue
            // 
            this.tbrBlue.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbrBlue.AutoSize = false;
            this.tbrBlue.BackColor = System.Drawing.SystemColors.Window;
            this.tbrBlue.Location = new System.Drawing.Point(17, 60);
            this.tbrBlue.Margin = new System.Windows.Forms.Padding(3, 3, 0, 3);
            this.tbrBlue.Maximum = 255;
            this.tbrBlue.Name = "tbrBlue";
            this.tbrBlue.Size = new System.Drawing.Size(81, 23);
            this.tbrBlue.SmallChange = 16;
            this.tbrBlue.TabIndex = 6;
            this.tbrBlue.TabStop = false;
            this.tbrBlue.TickFrequency = 64;
            this.tbrBlue.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // lRed
            // 
            this.lRed.AutoSize = true;
            this.lRed.Location = new System.Drawing.Point(3, 11);
            this.lRed.Name = "lRed";
            this.lRed.Size = new System.Drawing.Size(15, 13);
            this.lRed.TabIndex = 9;
            this.lRed.Text = "R";
            // 
            // lGreen
            // 
            this.lGreen.AutoSize = true;
            this.lGreen.Location = new System.Drawing.Point(3, 37);
            this.lGreen.Name = "lGreen";
            this.lGreen.Size = new System.Drawing.Size(15, 13);
            this.lGreen.TabIndex = 10;
            this.lGreen.Text = "G";
            // 
            // lBlue
            // 
            this.lBlue.AutoSize = true;
            this.lBlue.Location = new System.Drawing.Point(3, 63);
            this.lBlue.Name = "lBlue";
            this.lBlue.Size = new System.Drawing.Size(14, 13);
            this.lBlue.TabIndex = 11;
            this.lBlue.Text = "B";
            // 
            // lAlpha
            // 
            this.lAlpha.AutoSize = true;
            this.lAlpha.Location = new System.Drawing.Point(3, 89);
            this.lAlpha.Name = "lAlpha";
            this.lAlpha.Size = new System.Drawing.Size(14, 13);
            this.lAlpha.TabIndex = 12;
            this.lAlpha.Text = "A";
            // 
            // chkHexademical
            // 
            this.chkHexademical.AutoSize = true;
            this.chkHexademical.Location = new System.Drawing.Point(3, 115);
            this.chkHexademical.Name = "chkHexademical";
            this.chkHexademical.Size = new System.Drawing.Size(87, 17);
            this.chkHexademical.TabIndex = 13;
            this.chkHexademical.Text = "Hexademical";
            this.chkHexademical.UseVisualStyleBackColor = true;
            // 
            // tbHexColor
            // 
            this.tbHexColor.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper;
            this.tbHexColor.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.tbHexColor.Location = new System.Drawing.Point(167, 113);
            this.tbHexColor.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.tbHexColor.MaxLength = 8;
            this.tbHexColor.Name = "tbHexColor";
            this.tbHexColor.Size = new System.Drawing.Size(80, 21);
            this.tbHexColor.TabIndex = 14;
            this.tbHexColor.Text = "AAAAAAAA";
            this.tbHexColor.Visible = false;
            // 
            // lHexColor
            // 
            this.lHexColor.AutoSize = true;
            this.lHexColor.Location = new System.Drawing.Point(150, 116);
            this.lHexColor.Name = "lHexColor";
            this.lHexColor.Size = new System.Drawing.Size(14, 13);
            this.lHexColor.TabIndex = 15;
            this.lHexColor.Text = "#";
            this.lHexColor.Visible = false;
            // 
            // pbColor
            // 
            this.pbColor.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.pbColor.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("pbColor.BackgroundImage")));
            this.pbColor.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.pbColor.ColorSample = System.Drawing.Color.Empty;
            this.pbColor.Location = new System.Drawing.Point(149, 9);
            this.pbColor.Name = "pbColor";
            this.pbColor.Size = new System.Drawing.Size(98, 98);
            this.pbColor.TabIndex = 8;
            this.pbColor.TabStop = false;
            // 
            // numAlpha
            // 
            this.numAlpha.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.numAlpha.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.numAlpha.Hexadecimal = false;
            this.numAlpha.Increment = 1;
            this.numAlpha.Location = new System.Drawing.Point(101, 87);
            this.numAlpha.Maximum = 255;
            this.numAlpha.Minimum = 0;
            this.numAlpha.Name = "numAlpha";
            this.numAlpha.Size = new System.Drawing.Size(44, 21);
            this.numAlpha.TabIndex = 3;
            this.numAlpha.Value = 255;
            // 
            // numBlue
            // 
            this.numBlue.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.numBlue.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.numBlue.Hexadecimal = false;
            this.numBlue.Increment = 1;
            this.numBlue.Location = new System.Drawing.Point(101, 61);
            this.numBlue.Maximum = 255;
            this.numBlue.Minimum = 0;
            this.numBlue.Name = "numBlue";
            this.numBlue.Size = new System.Drawing.Size(44, 21);
            this.numBlue.TabIndex = 2;
            this.numBlue.Value = 0;
            // 
            // numGreen
            // 
            this.numGreen.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.numGreen.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.numGreen.Hexadecimal = false;
            this.numGreen.Increment = 1;
            this.numGreen.Location = new System.Drawing.Point(101, 35);
            this.numGreen.Maximum = 255;
            this.numGreen.Minimum = 0;
            this.numGreen.Name = "numGreen";
            this.numGreen.Size = new System.Drawing.Size(44, 21);
            this.numGreen.TabIndex = 1;
            this.numGreen.Value = 0;
            // 
            // numRed
            // 
            this.numRed.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.numRed.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.numRed.Hexadecimal = false;
            this.numRed.Increment = 1;
            this.numRed.Location = new System.Drawing.Point(101, 9);
            this.numRed.Maximum = 255;
            this.numRed.Minimum = 0;
            this.numRed.Name = "numRed";
            this.numRed.Size = new System.Drawing.Size(44, 21);
            this.numRed.TabIndex = 0;
            this.numRed.Value = 0;
            // 
            // ColorPicker
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Window;
            this.Controls.Add(this.lHexColor);
            this.Controls.Add(this.tbHexColor);
            this.Controls.Add(this.chkHexademical);
            this.Controls.Add(this.lAlpha);
            this.Controls.Add(this.lBlue);
            this.Controls.Add(this.lGreen);
            this.Controls.Add(this.lRed);
            this.Controls.Add(this.pbColor);
            this.Controls.Add(this.numAlpha);
            this.Controls.Add(this.tbrAlpha);
            this.Controls.Add(this.numBlue);
            this.Controls.Add(this.tbrBlue);
            this.Controls.Add(this.numGreen);
            this.Controls.Add(this.tbrGreen);
            this.Controls.Add(this.numRed);
            this.Controls.Add(this.tbrRed);
            this.MaximumSize = new System.Drawing.Size(9000, 144);
            this.MinimumSize = new System.Drawing.Size(256, 144);
            this.Name = "ColorPicker";
            this.Size = new System.Drawing.Size(256, 144);
            this.Load += new System.EventHandler(this.ColorPicker_Load);
            ((System.ComponentModel.ISupportInitialize)(this.tbrRed)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tbrGreen)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tbrAlpha)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.tbrBlue)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numAlpha)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numBlue)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numGreen)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numRed)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private XRay.SdkControls.IntegerUpDown numRed;
        private XRay.SdkControls.IntegerUpDown numGreen;
        private XRay.SdkControls.IntegerUpDown numBlue;
        private XRay.SdkControls.IntegerUpDown numAlpha;
        private System.Windows.Forms.TrackBar tbrRed;
        private System.Windows.Forms.TrackBar tbrGreen;
        private System.Windows.Forms.TrackBar tbrBlue;
        private System.Windows.Forms.TrackBar tbrAlpha;
        private System.Windows.Forms.Label lRed;
        private System.Windows.Forms.Label lGreen;
        private System.Windows.Forms.Label lBlue;
        private System.Windows.Forms.Label lAlpha;
        private XRay.SdkControls.ColorSampleBox pbColor;
        private System.Windows.Forms.CheckBox chkHexademical;
        private System.Windows.Forms.Label lHexColor;
        private System.Windows.Forms.TextBox tbHexColor;
    }
}
