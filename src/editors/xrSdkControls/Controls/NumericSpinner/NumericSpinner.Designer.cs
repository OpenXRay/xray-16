namespace XRay.SdkControls
{
    partial class NumericSpinner
    {
        /// <summary> 
        /// Обязательная переменная конструктора.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Освободить все используемые ресурсы.
        /// </summary>
        /// <param name="disposing">истинно, если управляемый ресурс должен быть удален; иначе ложно.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Код, автоматически созданный конструктором компонентов

        /// <summary> 
        /// Требуемый метод для поддержки конструктора — не изменяйте 
        /// содержимое этого метода с помощью редактора кода.
        /// </summary>
        private void InitializeComponent()
        {
            this.numSpinner = new System.Windows.Forms.NumericUpDown();
            this.btnHSpin = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.numSpinner)).BeginInit();
            this.SuspendLayout();
            // 
            // numSpinner
            // 
            this.numSpinner.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.numSpinner.DecimalPlaces = 3;
            this.numSpinner.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
            this.numSpinner.Location = new System.Drawing.Point(0, 0);
            this.numSpinner.Name = "numSpinner";
            this.numSpinner.Size = new System.Drawing.Size(93, 20);
            this.numSpinner.TabIndex = 0;
            // 
            // btnHSpin
            // 
            this.btnHSpin.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnHSpin.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.btnHSpin.Location = new System.Drawing.Point(93, -1);
            this.btnHSpin.Name = "btnHSpin";
            this.btnHSpin.Size = new System.Drawing.Size(22, 22);
            this.btnHSpin.TabIndex = 1;
            this.btnHSpin.Text = "<>";
            this.btnHSpin.UseVisualStyleBackColor = true;
            this.btnHSpin.MouseDown += new System.Windows.Forms.MouseEventHandler(this.btnHSpin_MouseDown);
            this.btnHSpin.MouseMove += new System.Windows.Forms.MouseEventHandler(this.btnHSpin_MouseMove);
            this.btnHSpin.MouseUp += new System.Windows.Forms.MouseEventHandler(this.btnHSpin_MouseUp);
            // 
            // NumericSpinner
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.btnHSpin);
            this.Controls.Add(this.numSpinner);
            this.MinimumSize = new System.Drawing.Size(70, 22);
            this.Name = "NumericSpinner";
            this.Size = new System.Drawing.Size(115, 22);
            ((System.ComponentModel.ISupportInitialize)(this.numSpinner)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.NumericUpDown numSpinner;
        private System.Windows.Forms.Button btnHSpin;
    }
}
