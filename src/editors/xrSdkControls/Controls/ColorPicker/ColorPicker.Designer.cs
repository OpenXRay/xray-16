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
            this.lRed = new System.Windows.Forms.Label();
            this.lGreen = new System.Windows.Forms.Label();
            this.lBlue = new System.Windows.Forms.Label();
            this.lAlpha = new System.Windows.Forms.Label();
            this.chkHexademical = new System.Windows.Forms.CheckBox();
            this.tbHexColor = new System.Windows.Forms.TextBox();
            this.lHexColor = new System.Windows.Forms.Label();
            this.pbColor = new XRay.SdkControls.ColorSampleBox();
            this.islRed = new XRay.SdkControls.IntegerSlider();
            this.islGreen = new XRay.SdkControls.IntegerSlider();
            this.islBlue = new XRay.SdkControls.IntegerSlider();
            this.islAlpha = new XRay.SdkControls.IntegerSlider();
            this.SuspendLayout();
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
            // islRed
            // 
            this.islRed.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.islRed.BackColor = System.Drawing.SystemColors.Window;
            this.islRed.Hexademical = false;
            this.islRed.LargeChange = 5;
            this.islRed.Location = new System.Drawing.Point(17, 9);
            this.islRed.Maximum = 255;
            this.islRed.MaximumSize = new System.Drawing.Size(9000, 21);
            this.islRed.Minimum = 0;
            this.islRed.MinimumSize = new System.Drawing.Size(100, 21);
            this.islRed.Name = "islRed";
            this.islRed.Size = new System.Drawing.Size(128, 21);
            this.islRed.SmallChange = 16;
            this.islRed.TabIndex = 16;
            this.islRed.TickFrequency = 64;
            this.islRed.TickStyle = System.Windows.Forms.TickStyle.None;
            this.islRed.Value = 0;
            // 
            // islGreen
            // 
            this.islGreen.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.islGreen.BackColor = System.Drawing.SystemColors.Window;
            this.islGreen.Hexademical = false;
            this.islGreen.LargeChange = 5;
            this.islGreen.Location = new System.Drawing.Point(17, 35);
            this.islGreen.Maximum = 255;
            this.islGreen.MaximumSize = new System.Drawing.Size(9000, 21);
            this.islGreen.Minimum = 0;
            this.islGreen.MinimumSize = new System.Drawing.Size(100, 21);
            this.islGreen.Name = "islGreen";
            this.islGreen.Size = new System.Drawing.Size(128, 21);
            this.islGreen.SmallChange = 16;
            this.islGreen.TabIndex = 17;
            this.islGreen.TickFrequency = 64;
            this.islGreen.TickStyle = System.Windows.Forms.TickStyle.None;
            this.islGreen.Value = 0;
            // 
            // islBlue
            // 
            this.islBlue.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.islBlue.BackColor = System.Drawing.SystemColors.Window;
            this.islBlue.Hexademical = false;
            this.islBlue.LargeChange = 5;
            this.islBlue.Location = new System.Drawing.Point(17, 61);
            this.islBlue.Maximum = 255;
            this.islBlue.MaximumSize = new System.Drawing.Size(9000, 21);
            this.islBlue.Minimum = 0;
            this.islBlue.MinimumSize = new System.Drawing.Size(100, 21);
            this.islBlue.Name = "islBlue";
            this.islBlue.Size = new System.Drawing.Size(128, 21);
            this.islBlue.SmallChange = 16;
            this.islBlue.TabIndex = 18;
            this.islBlue.TickFrequency = 64;
            this.islBlue.TickStyle = System.Windows.Forms.TickStyle.None;
            this.islBlue.Value = 0;
            // 
            // islAlpha
            // 
            this.islAlpha.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.islAlpha.BackColor = System.Drawing.SystemColors.Window;
            this.islAlpha.Hexademical = false;
            this.islAlpha.LargeChange = 5;
            this.islAlpha.Location = new System.Drawing.Point(17, 87);
            this.islAlpha.Maximum = 255;
            this.islAlpha.MaximumSize = new System.Drawing.Size(9000, 21);
            this.islAlpha.Minimum = 0;
            this.islAlpha.MinimumSize = new System.Drawing.Size(100, 21);
            this.islAlpha.Name = "islAlpha";
            this.islAlpha.Size = new System.Drawing.Size(128, 21);
            this.islAlpha.SmallChange = 16;
            this.islAlpha.TabIndex = 19;
            this.islAlpha.TickFrequency = 64;
            this.islAlpha.TickStyle = System.Windows.Forms.TickStyle.None;
            this.islAlpha.Value = 255;
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
            this.Controls.Add(this.islRed);
            this.Controls.Add(this.islGreen);
            this.Controls.Add(this.islBlue);
            this.Controls.Add(this.islAlpha);
            this.MaximumSize = new System.Drawing.Size(9000, 144);
            this.MinimumSize = new System.Drawing.Size(256, 144);
            this.Name = "ColorPicker";
            this.Size = new System.Drawing.Size(256, 144);
            this.Load += new System.EventHandler(this.ColorPicker_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lRed;
        private System.Windows.Forms.Label lGreen;
        private System.Windows.Forms.Label lBlue;
        private System.Windows.Forms.Label lAlpha;
        private XRay.SdkControls.ColorSampleBox pbColor;
        private System.Windows.Forms.CheckBox chkHexademical;
        private System.Windows.Forms.Label lHexColor;
        private System.Windows.Forms.TextBox tbHexColor;
        private IntegerSlider islRed;
        private IntegerSlider islGreen;
        private IntegerSlider islBlue;
        private IntegerSlider islAlpha;
    }
}
