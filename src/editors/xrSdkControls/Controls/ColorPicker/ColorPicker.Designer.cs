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
            this.chkHexadecimal = new System.Windows.Forms.CheckBox();
            this.tbHexColor = new System.Windows.Forms.TextBox();
            this.lHexColor = new System.Windows.Forms.Label();
            this.pbColor = new XRay.SdkControls.ColorSampleBox();
            this.nslRed = new XRay.SdkControls.NumericSlider();
            this.nslGreen = new XRay.SdkControls.NumericSlider();
            this.nslBlue = new XRay.SdkControls.NumericSlider();
            this.nslAlpha = new XRay.SdkControls.NumericSlider();
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
            // chkHexadecimal
            // 
            this.chkHexadecimal.AutoSize = true;
            this.chkHexadecimal.Location = new System.Drawing.Point(3, 115);
            this.chkHexadecimal.Name = "chkHexadecimal";
            this.chkHexadecimal.Size = new System.Drawing.Size(87, 17);
            this.chkHexadecimal.TabIndex = 13;
            this.chkHexadecimal.Text = "Hexadecimal";
            this.chkHexadecimal.UseVisualStyleBackColor = true;
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
            // nslRed
            // 
            this.nslRed.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.nslRed.BackColor = System.Drawing.SystemColors.Window;
            this.nslRed.DecimalPlaces = 0;
            this.nslRed.Hexadecimal = false;
            this.nslRed.Location = new System.Drawing.Point(17, 9);
            this.nslRed.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.nslRed.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslRed.Minimum = new decimal(new int[] {
            0,
            0,
            0,
            0});
            this.nslRed.MinimumSize = new System.Drawing.Size(100, 21);
            this.nslRed.Name = "nslRed";
            this.nslRed.Size = new System.Drawing.Size(128, 21);
            this.nslRed.SliderPrecision = 100;
            this.nslRed.SpinnerWidth = 44;
            this.nslRed.TabIndex = 16;
            this.nslRed.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
            this.nslRed.TickFrequency = 64;
            this.nslRed.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslRed.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // nslGreen
            // 
            this.nslGreen.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.nslGreen.BackColor = System.Drawing.SystemColors.Window;
            this.nslGreen.DecimalPlaces = 0;
            this.nslGreen.Hexadecimal = false;
            this.nslGreen.Location = new System.Drawing.Point(17, 35);
            this.nslGreen.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.nslGreen.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslGreen.Minimum = new decimal(new int[] {
            0,
            0,
            0,
            0});
            this.nslGreen.MinimumSize = new System.Drawing.Size(100, 21);
            this.nslGreen.Name = "nslGreen";
            this.nslGreen.Size = new System.Drawing.Size(128, 21);
            this.nslGreen.SliderPrecision = 100;
            this.nslGreen.SpinnerWidth = 44;
            this.nslGreen.TabIndex = 17;
            this.nslGreen.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
            this.nslGreen.TickFrequency = 64;
            this.nslGreen.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslGreen.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // nslBlue
            // 
            this.nslBlue.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.nslBlue.BackColor = System.Drawing.SystemColors.Window;
            this.nslBlue.DecimalPlaces = 0;
            this.nslBlue.Hexadecimal = false;
            this.nslBlue.Location = new System.Drawing.Point(17, 61);
            this.nslBlue.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.nslBlue.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslBlue.Minimum = new decimal(new int[] {
            0,
            0,
            0,
            0});
            this.nslBlue.MinimumSize = new System.Drawing.Size(100, 21);
            this.nslBlue.Name = "nslBlue";
            this.nslBlue.Size = new System.Drawing.Size(128, 21);
            this.nslBlue.SliderPrecision = 100;
            this.nslBlue.SpinnerWidth = 44;
            this.nslBlue.TabIndex = 18;
            this.nslBlue.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
            this.nslBlue.TickFrequency = 64;
            this.nslBlue.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslBlue.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // nslAlpha
            // 
            this.nslAlpha.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.nslAlpha.BackColor = System.Drawing.SystemColors.Window;
            this.nslAlpha.DecimalPlaces = 0;
            this.nslAlpha.Hexadecimal = false;
            this.nslAlpha.Location = new System.Drawing.Point(17, 87);
            this.nslAlpha.Maximum = new decimal(new int[] {
            255,
            0,
            0,
            0});
            this.nslAlpha.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslAlpha.Minimum = new decimal(new int[] {
            0,
            0,
            0,
            0});
            this.nslAlpha.MinimumSize = new System.Drawing.Size(100, 21);
            this.nslAlpha.Name = "nslAlpha";
            this.nslAlpha.Size = new System.Drawing.Size(128, 21);
            this.nslAlpha.SliderPrecision = 100;
            this.nslAlpha.SpinnerWidth = 44;
            this.nslAlpha.TabIndex = 19;
            this.nslAlpha.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
            this.nslAlpha.TickFrequency = 64;
            this.nslAlpha.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslAlpha.Value = new decimal(new int[] {
            255,
            0,
            0,
            0});
            // 
            // ColorPicker
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Window;
            this.Controls.Add(this.lHexColor);
            this.Controls.Add(this.tbHexColor);
            this.Controls.Add(this.chkHexadecimal);
            this.Controls.Add(this.lAlpha);
            this.Controls.Add(this.lBlue);
            this.Controls.Add(this.lGreen);
            this.Controls.Add(this.lRed);
            this.Controls.Add(this.pbColor);
            this.Controls.Add(this.nslRed);
            this.Controls.Add(this.nslGreen);
            this.Controls.Add(this.nslBlue);
            this.Controls.Add(this.nslAlpha);
            this.MaximumSize = new System.Drawing.Size(9000, 144);
            this.MinimumSize = new System.Drawing.Size(256, 144);
            this.Name = "ColorPicker";
            this.Size = new System.Drawing.Size(256, 144);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lRed;
        private System.Windows.Forms.Label lGreen;
        private System.Windows.Forms.Label lBlue;
        private System.Windows.Forms.Label lAlpha;
        private XRay.SdkControls.ColorSampleBox pbColor;
        private System.Windows.Forms.CheckBox chkHexadecimal;
        private System.Windows.Forms.Label lHexColor;
        private System.Windows.Forms.TextBox tbHexColor;
        private NumericSlider nslRed;
        private NumericSlider nslGreen;
        private NumericSlider nslBlue;
        private NumericSlider nslAlpha;
    }
}
