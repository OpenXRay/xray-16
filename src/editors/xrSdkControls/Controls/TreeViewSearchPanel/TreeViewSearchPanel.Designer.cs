namespace XRay.SdkControls
{
    partial class TreeViewSearchPanel
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
            this.searchLabel = new System.Windows.Forms.Label();
            this.searchBox = new System.Windows.Forms.TextBox();
            this.searchButton = new System.Windows.Forms.Button();
            this.resultLabel = new System.Windows.Forms.Label();
            this.searchPrev = new System.Windows.Forms.Button();
            this.searchNext = new System.Windows.Forms.Button();
            this.searchClose = new System.Windows.Forms.Button();
            this.SuspendLayout();
            //
            // searchLabel
            //
            this.searchLabel.AutoSize = true;
            this.searchLabel.Location = new System.Drawing.Point(3, 6);
            this.searchLabel.Name = "searchLabel";
            this.searchLabel.Size = new System.Drawing.Size(44, 13);
            this.searchLabel.TabIndex = 0;
            this.searchLabel.Text = "Search:";
            //
            // searchBox
            //
            this.searchBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
            | System.Windows.Forms.AnchorStyles.Right)));
            this.searchBox.Location = new System.Drawing.Point(46, 3);
            this.searchBox.Name = "searchBox";
            this.searchBox.Size = new System.Drawing.Size(128, 20);
            this.searchBox.TabIndex = 1;
            this.searchBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.searchBox_KeyDown);
            //
            // searchButton
            //
            this.searchButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.searchButton.Location = new System.Drawing.Point(174, 2);
            this.searchButton.Name = "searchButton";
            this.searchButton.Size = new System.Drawing.Size(53, 22);
            this.searchButton.TabIndex = 2;
            this.searchButton.Text = "Search";
            this.searchButton.UseVisualStyleBackColor = true;
            this.searchButton.Click += new System.EventHandler(this.searchButton_Click);
            //
            // resultLabel
            //
            this.resultLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.resultLabel.AutoSize = true;
            this.resultLabel.Location = new System.Drawing.Point(3, 31);
            this.resultLabel.Name = "resultLabel";
            this.resultLabel.Size = new System.Drawing.Size(65, 13);
            this.resultLabel.TabIndex = 3;
            this.resultLabel.Text = "Results: - / -";
            //
            // searchPrev
            //
            this.searchPrev.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.searchPrev.Location = new System.Drawing.Point(98, 26);
            this.searchPrev.Name = "searchPrev";
            this.searchPrev.Size = new System.Drawing.Size(39, 22);
            this.searchPrev.TabIndex = 4;
            this.searchPrev.Text = "Prev";
            this.searchPrev.UseVisualStyleBackColor = true;
            this.searchPrev.Click += new System.EventHandler(this.searchPrev_Click);
            //
            // searchNext
            //
            this.searchNext.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.searchNext.Location = new System.Drawing.Point(136, 26);
            this.searchNext.Name = "searchNext";
            this.searchNext.Size = new System.Drawing.Size(39, 22);
            this.searchNext.TabIndex = 5;
            this.searchNext.Text = "Next";
            this.searchNext.UseVisualStyleBackColor = true;
            this.searchNext.Click += new System.EventHandler(this.searchNext_Click);
            //
            // searchClose
            //
            this.searchClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.searchClose.Location = new System.Drawing.Point(174, 26);
            this.searchClose.Name = "searchClose";
            this.searchClose.Size = new System.Drawing.Size(53, 22);
            this.searchClose.TabIndex = 6;
            this.searchClose.Text = "Close";
            this.searchClose.UseVisualStyleBackColor = true;
            this.searchClose.Click += new System.EventHandler(this.searchClose_Click);
            //
            // TreeViewSearchPanel
            //
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.searchClose);
            this.Controls.Add(this.searchNext);
            this.Controls.Add(this.searchPrev);
            this.Controls.Add(this.resultLabel);
            this.Controls.Add(this.searchButton);
            this.Controls.Add(this.searchBox);
            this.Controls.Add(this.searchLabel);
            this.Name = "TreeViewSearchPanel";
            this.Size = new System.Drawing.Size(236, 53);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label searchLabel;
        public System.Windows.Forms.TextBox searchBox;
        private System.Windows.Forms.Button searchButton;
        private System.Windows.Forms.Label resultLabel;
        private System.Windows.Forms.Button searchPrev;
        public System.Windows.Forms.Button searchNext;
        private System.Windows.Forms.Button searchClose;
    }
}
