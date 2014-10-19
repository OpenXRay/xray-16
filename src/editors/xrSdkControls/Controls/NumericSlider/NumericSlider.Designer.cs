namespace XRay.SdkControls
{
    partial class NumericSlider
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
            this.trackBar = new System.Windows.Forms.TrackBar();
            this.numSpinner = new System.Windows.Forms.NumericUpDown();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSpinner)).BeginInit();
            this.SuspendLayout();
            // 
            // trackBar
            // 
            this.trackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar.AutoSize = false;
            this.trackBar.BackColor = System.Drawing.SystemColors.Window;
            this.trackBar.Location = new System.Drawing.Point(0, -1);
            this.trackBar.Margin = new System.Windows.Forms.Padding(3, 3, 0, 3);
            this.trackBar.Minimum = 0;
            this.trackBar.Maximum = 100;
            this.trackBar.MinimumSize = new System.Drawing.Size(32, 0);
            this.trackBar.Name = "trackBar";
            this.trackBar.Size = new System.Drawing.Size(212, 23);
            this.trackBar.SmallChange = 16;
            this.trackBar.TabIndex = 4;
            this.trackBar.TabStop = false;
            this.trackBar.TickFrequency = 64;
            this.trackBar.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // numSpinner
            // 
            this.numSpinner.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.numSpinner.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.numSpinner.Hexadecimal = false;
            this.numSpinner.Increment = 1;
            this.numSpinner.Location = new System.Drawing.Point(212, 0);
            this.numSpinner.Maximum = 10;
            this.numSpinner.Minimum = 0;
            this.numSpinner.MinimumSize = new System.Drawing.Size(32, 0);
            this.numSpinner.Name = "numSpinner";
            this.numSpinner.Size = new System.Drawing.Size(44, 21);
            this.numSpinner.TabIndex = 0;
            this.numSpinner.Value = 0;
            // 
            // NumericSlider
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Window;
            this.Controls.Add(this.numSpinner);
            this.Controls.Add(this.trackBar);
            this.MaximumSize = new System.Drawing.Size(9000, 21);
            this.MinimumSize = new System.Drawing.Size(64, 21);
            this.Name = "NumericSlider";
            this.Size = new System.Drawing.Size(256, 21);
            ((System.ComponentModel.ISupportInitialize)(this.trackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numSpinner)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.NumericUpDown numSpinner;
        private System.Windows.Forms.TrackBar trackBar;
    }
}
