using System.Drawing;
using System.Windows.Forms;

namespace xrPostprocessEditor
{
    partial class MainDialog
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
            this.tabEffects = new System.Windows.Forms.TabControl();
            this.tpAC = new System.Windows.Forms.TabPage();
            this.tlpAC = new System.Windows.Forms.TableLayoutPanel();
            this.kfbAC = new xrPostprocessEditor.KeyFrameBox();
            this.pnAC = new System.Windows.Forms.Panel();
            this.cpAC = new XRay.SdkControls.ColorPicker();
            this.tpBC = new System.Windows.Forms.TabPage();
            this.tlpBC = new System.Windows.Forms.TableLayoutPanel();
            this.pnBC = new System.Windows.Forms.Panel();
            this.cpBC = new XRay.SdkControls.ColorPicker();
            this.kfbBC = new xrPostprocessEditor.KeyFrameBox();
            this.tpGC = new System.Windows.Forms.TabPage();
            this.tlpGC = new System.Windows.Forms.TableLayoutPanel();
            this.pnGC = new System.Windows.Forms.Panel();
            this.cpGC = new XRay.SdkControls.ColorPicker();
            this.kfbGC = new xrPostprocessEditor.KeyFrameBox();
            this.tpDuality = new System.Windows.Forms.TabPage();
            this.tlpDuality = new System.Windows.Forms.TableLayoutPanel();
            this.pnDuality = new System.Windows.Forms.Panel();
            this.nslDualityX = new XRay.SdkControls.NumericSlider();
            this.lDualityY = new System.Windows.Forms.Label();
            this.nslDualityY = new XRay.SdkControls.NumericSlider();
            this.lDualityX = new System.Windows.Forms.Label();
            this.kfbDuality = new xrPostprocessEditor.KeyFrameBox();
            this.tpNoise = new System.Windows.Forms.TabPage();
            this.tlpNoise = new System.Windows.Forms.TableLayoutPanel();
            this.pnNoise = new System.Windows.Forms.Panel();
            this.lNoiseFPS = new System.Windows.Forms.Label();
            this.nslNoiseFPS = new XRay.SdkControls.NumericSlider();
            this.nslNoiseIntensity = new XRay.SdkControls.NumericSlider();
            this.lNoiseGrain = new System.Windows.Forms.Label();
            this.nslNoiseGrain = new XRay.SdkControls.NumericSlider();
            this.lNoiseIntensity = new System.Windows.Forms.Label();
            this.kfbNoise = new xrPostprocessEditor.KeyFrameBox();
            this.tpBlur = new System.Windows.Forms.TabPage();
            this.tlpBlur = new System.Windows.Forms.TableLayoutPanel();
            this.pnBlur = new System.Windows.Forms.Panel();
            this.kfbBlur = new xrPostprocessEditor.KeyFrameBox();
            this.tpColorMapping = new System.Windows.Forms.TabPage();
            this.tlpColorMapping = new System.Windows.Forms.TableLayoutPanel();
            this.pnColorMapping = new System.Windows.Forms.Panel();
            this.nslColorMappingInfluence = new XRay.SdkControls.NumericSlider();
            this.lColorMappingInfluence = new System.Windows.Forms.Label();
            this.tbColorMappingTexture = new System.Windows.Forms.TextBox();
            this.lColorMappingTexture = new System.Windows.Forms.Label();
            this.kfbColorMapping = new xrPostprocessEditor.KeyFrameBox();
            this.sbMainBar = new System.Windows.Forms.StatusBar();
            this.sbpEffectTimeLabel = new System.Windows.Forms.StatusBarPanel();
            this.sbpEffectTime = new System.Windows.Forms.StatusBarPanel();
            this.msMainMenu = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.miNew = new System.Windows.Forms.ToolStripMenuItem();
            this.miLoad = new System.Windows.Forms.ToolStripMenuItem();
            this.miSave = new System.Windows.Forms.ToolStripMenuItem();
            this.pbDrawingSurface = new System.Windows.Forms.PictureBox();
            this.nslBlur = new XRay.SdkControls.NumericSlider();
            this.lBlur = new System.Windows.Forms.Label();
            this.tabEffects.SuspendLayout();
            this.tpAC.SuspendLayout();
            this.tlpAC.SuspendLayout();
            this.pnAC.SuspendLayout();
            this.tpBC.SuspendLayout();
            this.tlpBC.SuspendLayout();
            this.pnBC.SuspendLayout();
            this.tpGC.SuspendLayout();
            this.tlpGC.SuspendLayout();
            this.pnGC.SuspendLayout();
            this.tpDuality.SuspendLayout();
            this.tlpDuality.SuspendLayout();
            this.pnDuality.SuspendLayout();
            this.tpNoise.SuspendLayout();
            this.tlpNoise.SuspendLayout();
            this.pnNoise.SuspendLayout();
            this.tpBlur.SuspendLayout();
            this.tlpBlur.SuspendLayout();
            this.pnBlur.SuspendLayout();
            this.tpColorMapping.SuspendLayout();
            this.tlpColorMapping.SuspendLayout();
            this.pnColorMapping.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sbpEffectTimeLabel)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.sbpEffectTime)).BeginInit();
            this.msMainMenu.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbDrawingSurface)).BeginInit();
            this.SuspendLayout();
            // 
            // tabEffects
            // 
            this.tabEffects.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tabEffects.Controls.Add(this.tpAC);
            this.tabEffects.Controls.Add(this.tpBC);
            this.tabEffects.Controls.Add(this.tpGC);
            this.tabEffects.Controls.Add(this.tpDuality);
            this.tabEffects.Controls.Add(this.tpNoise);
            this.tabEffects.Controls.Add(this.tpBlur);
            this.tabEffects.Controls.Add(this.tpColorMapping);
            this.tabEffects.Location = new System.Drawing.Point(5, 203);
            this.tabEffects.Name = "tabEffects";
            this.tabEffects.Padding = new System.Drawing.Point(0, 0);
            this.tabEffects.SelectedIndex = 0;
            this.tabEffects.Size = new System.Drawing.Size(616, 213);
            this.tabEffects.TabIndex = 0;
            // 
            // tpAC
            // 
            this.tpAC.Controls.Add(this.tlpAC);
            this.tpAC.Location = new System.Drawing.Point(4, 22);
            this.tpAC.Name = "tpAC";
            this.tpAC.Padding = new System.Windows.Forms.Padding(3);
            this.tpAC.Size = new System.Drawing.Size(608, 187);
            this.tpAC.TabIndex = 0;
            this.tpAC.Text = "Add color";
            this.tpAC.UseVisualStyleBackColor = true;
            // 
            // tlpAC
            // 
            this.tlpAC.ColumnCount = 2;
            this.tlpAC.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpAC.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tlpAC.Controls.Add(this.kfbAC, 0, 0);
            this.tlpAC.Controls.Add(this.pnAC, 1, 0);
            this.tlpAC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpAC.Location = new System.Drawing.Point(3, 3);
            this.tlpAC.Name = "tlpAC";
            this.tlpAC.RowCount = 1;
            this.tlpAC.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpAC.Size = new System.Drawing.Size(602, 181);
            this.tlpAC.TabIndex = 2;
            // 
            // kfbAC
            // 
            this.kfbAC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kfbAC.Location = new System.Drawing.Point(0, 0);
            this.kfbAC.Margin = new System.Windows.Forms.Padding(0);
            this.kfbAC.MinimumSize = new System.Drawing.Size(183, 100);
            this.kfbAC.Name = "kfbAC";
            this.kfbAC.SelectedIndex = -1;
            this.kfbAC.Size = new System.Drawing.Size(183, 181);
            this.kfbAC.TabIndex = 0;
            // 
            // pnAC
            // 
            this.pnAC.Controls.Add(this.cpAC);
            this.pnAC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnAC.Location = new System.Drawing.Point(183, 0);
            this.pnAC.Margin = new System.Windows.Forms.Padding(0);
            this.pnAC.Name = "pnAC";
            this.pnAC.Size = new System.Drawing.Size(419, 181);
            this.pnAC.TabIndex = 1;
            // 
            // cpAC
            // 
            this.cpAC.AlphaEnabled = false;
            this.cpAC.BackColor = System.Drawing.SystemColors.Window;
            this.cpAC.Hexadecimal = false;
            this.cpAC.Location = new System.Drawing.Point(0, 0);
            this.cpAC.Margin = new System.Windows.Forms.Padding(0);
            this.cpAC.MaximumSize = new System.Drawing.Size(9000, 144);
            this.cpAC.MinimumSize = new System.Drawing.Size(256, 144);
            this.cpAC.Name = "cpAC";
            this.cpAC.Size = new System.Drawing.Size(264, 144);
            this.cpAC.TabIndex = 0;
            this.cpAC.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.cpAC.Value = System.Drawing.Color.FromArgb(((int)(((byte)(127)))), ((int)(((byte)(127)))), ((int)(((byte)(127)))));
            // 
            // tpBC
            // 
            this.tpBC.Controls.Add(this.tlpBC);
            this.tpBC.Location = new System.Drawing.Point(4, 22);
            this.tpBC.Name = "tpBC";
            this.tpBC.Padding = new System.Windows.Forms.Padding(3);
            this.tpBC.Size = new System.Drawing.Size(608, 187);
            this.tpBC.TabIndex = 1;
            this.tpBC.Text = "Base color";
            this.tpBC.UseVisualStyleBackColor = true;
            // 
            // tlpBC
            // 
            this.tlpBC.ColumnCount = 2;
            this.tlpBC.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpBC.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tlpBC.Controls.Add(this.pnBC, 0, 0);
            this.tlpBC.Controls.Add(this.kfbBC, 0, 0);
            this.tlpBC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpBC.Location = new System.Drawing.Point(3, 3);
            this.tlpBC.Name = "tlpBC";
            this.tlpBC.RowCount = 1;
            this.tlpBC.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpBC.Size = new System.Drawing.Size(602, 181);
            this.tlpBC.TabIndex = 3;
            // 
            // pnBC
            // 
            this.pnBC.Controls.Add(this.cpBC);
            this.pnBC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnBC.Location = new System.Drawing.Point(183, 0);
            this.pnBC.Margin = new System.Windows.Forms.Padding(0);
            this.pnBC.Name = "pnBC";
            this.pnBC.Size = new System.Drawing.Size(419, 181);
            this.pnBC.TabIndex = 2;
            // 
            // cpBC
            // 
            this.cpBC.AlphaEnabled = false;
            this.cpBC.BackColor = System.Drawing.SystemColors.Window;
            this.cpBC.Hexadecimal = false;
            this.cpBC.Location = new System.Drawing.Point(0, 0);
            this.cpBC.Margin = new System.Windows.Forms.Padding(0);
            this.cpBC.MaximumSize = new System.Drawing.Size(9000, 144);
            this.cpBC.MinimumSize = new System.Drawing.Size(256, 144);
            this.cpBC.Name = "cpBC";
            this.cpBC.Size = new System.Drawing.Size(264, 144);
            this.cpBC.TabIndex = 0;
            this.cpBC.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.cpBC.Value = System.Drawing.Color.FromArgb(((int)(((byte)(127)))), ((int)(((byte)(127)))), ((int)(((byte)(127)))));
            // 
            // kfbBC
            // 
            this.kfbBC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kfbBC.Location = new System.Drawing.Point(0, 0);
            this.kfbBC.Margin = new System.Windows.Forms.Padding(0);
            this.kfbBC.MinimumSize = new System.Drawing.Size(183, 100);
            this.kfbBC.Name = "kfbBC";
            this.kfbBC.SelectedIndex = -1;
            this.kfbBC.Size = new System.Drawing.Size(183, 181);
            this.kfbBC.TabIndex = 0;
            // 
            // tpGC
            // 
            this.tpGC.Controls.Add(this.tlpGC);
            this.tpGC.Location = new System.Drawing.Point(4, 22);
            this.tpGC.Name = "tpGC";
            this.tpGC.Padding = new System.Windows.Forms.Padding(3);
            this.tpGC.Size = new System.Drawing.Size(608, 187);
            this.tpGC.TabIndex = 2;
            this.tpGC.Text = "Gray color";
            this.tpGC.UseVisualStyleBackColor = true;
            // 
            // tlpGC
            // 
            this.tlpGC.ColumnCount = 2;
            this.tlpGC.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpGC.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tlpGC.Controls.Add(this.pnGC, 0, 0);
            this.tlpGC.Controls.Add(this.kfbGC, 0, 0);
            this.tlpGC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpGC.Location = new System.Drawing.Point(3, 3);
            this.tlpGC.Name = "tlpGC";
            this.tlpGC.RowCount = 1;
            this.tlpGC.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpGC.Size = new System.Drawing.Size(602, 181);
            this.tlpGC.TabIndex = 4;
            // 
            // pnGC
            // 
            this.pnGC.Controls.Add(this.cpGC);
            this.pnGC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnGC.Location = new System.Drawing.Point(183, 0);
            this.pnGC.Margin = new System.Windows.Forms.Padding(0);
            this.pnGC.Name = "pnGC";
            this.pnGC.Size = new System.Drawing.Size(419, 181);
            this.pnGC.TabIndex = 2;
            // 
            // cpGC
            // 
            this.cpGC.AlphaEnabled = true;
            this.cpGC.BackColor = System.Drawing.SystemColors.Window;
            this.cpGC.Hexadecimal = false;
            this.cpGC.Location = new System.Drawing.Point(0, 0);
            this.cpGC.Margin = new System.Windows.Forms.Padding(0);
            this.cpGC.MaximumSize = new System.Drawing.Size(9000, 144);
            this.cpGC.MinimumSize = new System.Drawing.Size(256, 144);
            this.cpGC.Name = "cpGC";
            this.cpGC.Size = new System.Drawing.Size(264, 144);
            this.cpGC.TabIndex = 0;
            this.cpGC.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.cpGC.Value = System.Drawing.Color.FromArgb(((int)(((byte)(127)))), ((int)(((byte)(127)))), ((int)(((byte)(127)))));
            // 
            // kfbGC
            // 
            this.kfbGC.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kfbGC.Location = new System.Drawing.Point(0, 0);
            this.kfbGC.Margin = new System.Windows.Forms.Padding(0);
            this.kfbGC.MinimumSize = new System.Drawing.Size(183, 100);
            this.kfbGC.Name = "kfbGC";
            this.kfbGC.SelectedIndex = -1;
            this.kfbGC.Size = new System.Drawing.Size(183, 181);
            this.kfbGC.TabIndex = 0;
            // 
            // tpDuality
            // 
            this.tpDuality.Controls.Add(this.tlpDuality);
            this.tpDuality.Location = new System.Drawing.Point(4, 22);
            this.tpDuality.Name = "tpDuality";
            this.tpDuality.Padding = new System.Windows.Forms.Padding(3);
            this.tpDuality.Size = new System.Drawing.Size(608, 187);
            this.tpDuality.TabIndex = 3;
            this.tpDuality.Text = "Duality";
            this.tpDuality.UseVisualStyleBackColor = true;
            // 
            // tlpDuality
            // 
            this.tlpDuality.ColumnCount = 2;
            this.tlpDuality.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpDuality.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tlpDuality.Controls.Add(this.pnDuality, 0, 0);
            this.tlpDuality.Controls.Add(this.kfbDuality, 0, 0);
            this.tlpDuality.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpDuality.Location = new System.Drawing.Point(3, 3);
            this.tlpDuality.Name = "tlpDuality";
            this.tlpDuality.RowCount = 1;
            this.tlpDuality.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpDuality.Size = new System.Drawing.Size(602, 181);
            this.tlpDuality.TabIndex = 5;
            // 
            // pnDuality
            // 
            this.pnDuality.Controls.Add(this.nslDualityX);
            this.pnDuality.Controls.Add(this.lDualityY);
            this.pnDuality.Controls.Add(this.nslDualityY);
            this.pnDuality.Controls.Add(this.lDualityX);
            this.pnDuality.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnDuality.Location = new System.Drawing.Point(183, 0);
            this.pnDuality.Margin = new System.Windows.Forms.Padding(0);
            this.pnDuality.Name = "pnDuality";
            this.pnDuality.Size = new System.Drawing.Size(419, 181);
            this.pnDuality.TabIndex = 2;
            // 
            // nslDualityX
            // 
            this.nslDualityX.BackColor = System.Drawing.SystemColors.Window;
            this.nslDualityX.DecimalPlaces = 2;
            this.nslDualityX.Hexadecimal = false;
            this.nslDualityX.Location = new System.Drawing.Point(55, 9);
            this.nslDualityX.Maximum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nslDualityX.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslDualityX.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.nslDualityX.MinimumSize = new System.Drawing.Size(64, 0);
            this.nslDualityX.Name = "nslDualityX";
            this.nslDualityX.Size = new System.Drawing.Size(150, 21);
            this.nslDualityX.SliderPrecision = 100;
            this.nslDualityX.SpinnerWidth = 54;
            this.nslDualityX.TabIndex = 9;
            this.nslDualityX.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nslDualityX.TickFrequency = 25;
            this.nslDualityX.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslDualityX.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // lDualityY
            // 
            this.lDualityY.AutoSize = true;
            this.lDualityY.Location = new System.Drawing.Point(3, 38);
            this.lDualityY.Name = "lDualityY";
            this.lDualityY.Size = new System.Drawing.Size(43, 13);
            this.lDualityY.TabIndex = 8;
            this.lDualityY.Text = "Y offset";
            // 
            // nslDualityY
            // 
            this.nslDualityY.BackColor = System.Drawing.SystemColors.Window;
            this.nslDualityY.DecimalPlaces = 2;
            this.nslDualityY.Hexadecimal = false;
            this.nslDualityY.Location = new System.Drawing.Point(55, 36);
            this.nslDualityY.Maximum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nslDualityY.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslDualityY.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            -2147483648});
            this.nslDualityY.MinimumSize = new System.Drawing.Size(64, 0);
            this.nslDualityY.Name = "nslDualityY";
            this.nslDualityY.Size = new System.Drawing.Size(150, 21);
            this.nslDualityY.SliderPrecision = 100;
            this.nslDualityY.SpinnerWidth = 54;
            this.nslDualityY.TabIndex = 7;
            this.nslDualityY.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nslDualityY.TickFrequency = 25;
            this.nslDualityY.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslDualityY.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // lDualityX
            // 
            this.lDualityX.AutoSize = true;
            this.lDualityX.Location = new System.Drawing.Point(3, 11);
            this.lDualityX.Name = "lDualityX";
            this.lDualityX.Size = new System.Drawing.Size(43, 13);
            this.lDualityX.TabIndex = 6;
            this.lDualityX.Text = "X offset";
            // 
            // kfbDuality
            // 
            this.kfbDuality.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kfbDuality.Location = new System.Drawing.Point(0, 0);
            this.kfbDuality.Margin = new System.Windows.Forms.Padding(0);
            this.kfbDuality.MinimumSize = new System.Drawing.Size(183, 100);
            this.kfbDuality.Name = "kfbDuality";
            this.kfbDuality.SelectedIndex = -1;
            this.kfbDuality.Size = new System.Drawing.Size(183, 181);
            this.kfbDuality.TabIndex = 0;
            // 
            // tpNoise
            // 
            this.tpNoise.Controls.Add(this.tlpNoise);
            this.tpNoise.Location = new System.Drawing.Point(4, 22);
            this.tpNoise.Name = "tpNoise";
            this.tpNoise.Padding = new System.Windows.Forms.Padding(3);
            this.tpNoise.Size = new System.Drawing.Size(608, 187);
            this.tpNoise.TabIndex = 4;
            this.tpNoise.Text = "Noise";
            this.tpNoise.UseVisualStyleBackColor = true;
            // 
            // tlpNoise
            // 
            this.tlpNoise.ColumnCount = 2;
            this.tlpNoise.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpNoise.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tlpNoise.Controls.Add(this.pnNoise, 0, 0);
            this.tlpNoise.Controls.Add(this.kfbNoise, 0, 0);
            this.tlpNoise.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpNoise.Location = new System.Drawing.Point(3, 3);
            this.tlpNoise.Name = "tlpNoise";
            this.tlpNoise.RowCount = 1;
            this.tlpNoise.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpNoise.Size = new System.Drawing.Size(602, 181);
            this.tlpNoise.TabIndex = 6;
            // 
            // pnNoise
            // 
            this.pnNoise.Controls.Add(this.lNoiseFPS);
            this.pnNoise.Controls.Add(this.nslNoiseFPS);
            this.pnNoise.Controls.Add(this.nslNoiseIntensity);
            this.pnNoise.Controls.Add(this.lNoiseGrain);
            this.pnNoise.Controls.Add(this.nslNoiseGrain);
            this.pnNoise.Controls.Add(this.lNoiseIntensity);
            this.pnNoise.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnNoise.Location = new System.Drawing.Point(183, 0);
            this.pnNoise.Margin = new System.Windows.Forms.Padding(0);
            this.pnNoise.Name = "pnNoise";
            this.pnNoise.Size = new System.Drawing.Size(419, 181);
            this.pnNoise.TabIndex = 2;
            // 
            // lNoiseFPS
            // 
            this.lNoiseFPS.AutoSize = true;
            this.lNoiseFPS.Location = new System.Drawing.Point(3, 65);
            this.lNoiseFPS.Name = "lNoiseFPS";
            this.lNoiseFPS.Size = new System.Drawing.Size(27, 13);
            this.lNoiseFPS.TabIndex = 15;
            this.lNoiseFPS.Text = "FPS";
            // 
            // nslNoiseFPS
            // 
            this.nslNoiseFPS.BackColor = System.Drawing.SystemColors.Window;
            this.nslNoiseFPS.DecimalPlaces = 2;
            this.nslNoiseFPS.Hexadecimal = false;
            this.nslNoiseFPS.Location = new System.Drawing.Point(55, 63);
            this.nslNoiseFPS.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
            this.nslNoiseFPS.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslNoiseFPS.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nslNoiseFPS.MinimumSize = new System.Drawing.Size(64, 0);
            this.nslNoiseFPS.Name = "nslNoiseFPS";
            this.nslNoiseFPS.Size = new System.Drawing.Size(150, 21);
            this.nslNoiseFPS.SliderPrecision = 100;
            this.nslNoiseFPS.SpinnerWidth = 54;
            this.nslNoiseFPS.TabIndex = 14;
            this.nslNoiseFPS.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nslNoiseFPS.TickFrequency = 25;
            this.nslNoiseFPS.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslNoiseFPS.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            // 
            // nslNoiseIntensity
            // 
            this.nslNoiseIntensity.BackColor = System.Drawing.SystemColors.Window;
            this.nslNoiseIntensity.DecimalPlaces = 2;
            this.nslNoiseIntensity.Hexadecimal = false;
            this.nslNoiseIntensity.Location = new System.Drawing.Point(55, 9);
            this.nslNoiseIntensity.Maximum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nslNoiseIntensity.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslNoiseIntensity.Minimum = new decimal(new int[] {
            0,
            0,
            0,
            0});
            this.nslNoiseIntensity.MinimumSize = new System.Drawing.Size(64, 0);
            this.nslNoiseIntensity.Name = "nslNoiseIntensity";
            this.nslNoiseIntensity.Size = new System.Drawing.Size(150, 21);
            this.nslNoiseIntensity.SliderPrecision = 100;
            this.nslNoiseIntensity.SpinnerWidth = 54;
            this.nslNoiseIntensity.TabIndex = 13;
            this.nslNoiseIntensity.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nslNoiseIntensity.TickFrequency = 25;
            this.nslNoiseIntensity.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslNoiseIntensity.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // lNoiseGrain
            // 
            this.lNoiseGrain.AutoSize = true;
            this.lNoiseGrain.Location = new System.Drawing.Point(3, 38);
            this.lNoiseGrain.Name = "lNoiseGrain";
            this.lNoiseGrain.Size = new System.Drawing.Size(32, 13);
            this.lNoiseGrain.TabIndex = 12;
            this.lNoiseGrain.Text = "Grain";
            // 
            // nslNoiseGrain
            // 
            this.nslNoiseGrain.BackColor = System.Drawing.SystemColors.Window;
            this.nslNoiseGrain.DecimalPlaces = 2;
            this.nslNoiseGrain.Hexadecimal = false;
            this.nslNoiseGrain.Location = new System.Drawing.Point(55, 36);
            this.nslNoiseGrain.Maximum = new decimal(new int[] {
            1000,
            0,
            0,
            0});
            this.nslNoiseGrain.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslNoiseGrain.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            131072});
            this.nslNoiseGrain.MinimumSize = new System.Drawing.Size(64, 0);
            this.nslNoiseGrain.Name = "nslNoiseGrain";
            this.nslNoiseGrain.Size = new System.Drawing.Size(150, 21);
            this.nslNoiseGrain.SliderPrecision = 100;
            this.nslNoiseGrain.SpinnerWidth = 54;
            this.nslNoiseGrain.TabIndex = 11;
            this.nslNoiseGrain.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nslNoiseGrain.TickFrequency = 25;
            this.nslNoiseGrain.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslNoiseGrain.Value = new decimal(new int[] {
            1,
            0,
            0,
            131072});
            // 
            // lNoiseIntensity
            // 
            this.lNoiseIntensity.AutoSize = true;
            this.lNoiseIntensity.Location = new System.Drawing.Point(3, 11);
            this.lNoiseIntensity.Name = "lNoiseIntensity";
            this.lNoiseIntensity.Size = new System.Drawing.Size(46, 13);
            this.lNoiseIntensity.TabIndex = 10;
            this.lNoiseIntensity.Text = "Intensity";
            // 
            // kfbNoise
            // 
            this.kfbNoise.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kfbNoise.Location = new System.Drawing.Point(0, 0);
            this.kfbNoise.Margin = new System.Windows.Forms.Padding(0);
            this.kfbNoise.MinimumSize = new System.Drawing.Size(183, 100);
            this.kfbNoise.Name = "kfbNoise";
            this.kfbNoise.SelectedIndex = -1;
            this.kfbNoise.Size = new System.Drawing.Size(183, 181);
            this.kfbNoise.TabIndex = 0;
            // 
            // tpBlur
            // 
            this.tpBlur.Controls.Add(this.tlpBlur);
            this.tpBlur.Location = new System.Drawing.Point(4, 22);
            this.tpBlur.Name = "tpBlur";
            this.tpBlur.Padding = new System.Windows.Forms.Padding(3);
            this.tpBlur.Size = new System.Drawing.Size(608, 187);
            this.tpBlur.TabIndex = 5;
            this.tpBlur.Text = "Blur";
            this.tpBlur.UseVisualStyleBackColor = true;
            // 
            // tlpBlur
            // 
            this.tlpBlur.ColumnCount = 2;
            this.tlpBlur.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpBlur.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tlpBlur.Controls.Add(this.pnBlur, 0, 0);
            this.tlpBlur.Controls.Add(this.kfbBlur, 0, 0);
            this.tlpBlur.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpBlur.Location = new System.Drawing.Point(3, 3);
            this.tlpBlur.Name = "tlpBlur";
            this.tlpBlur.RowCount = 1;
            this.tlpBlur.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpBlur.Size = new System.Drawing.Size(602, 181);
            this.tlpBlur.TabIndex = 6;
            // 
            // pnBlur
            // 
            this.pnBlur.Controls.Add(this.nslBlur);
            this.pnBlur.Controls.Add(this.lBlur);
            this.pnBlur.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnBlur.Location = new System.Drawing.Point(183, 0);
            this.pnBlur.Margin = new System.Windows.Forms.Padding(0);
            this.pnBlur.Name = "pnBlur";
            this.pnBlur.Size = new System.Drawing.Size(419, 181);
            this.pnBlur.TabIndex = 2;
            // 
            // kfbBlur
            // 
            this.kfbBlur.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kfbBlur.Location = new System.Drawing.Point(0, 0);
            this.kfbBlur.Margin = new System.Windows.Forms.Padding(0);
            this.kfbBlur.MinimumSize = new System.Drawing.Size(183, 100);
            this.kfbBlur.Name = "kfbBlur";
            this.kfbBlur.SelectedIndex = -1;
            this.kfbBlur.Size = new System.Drawing.Size(183, 181);
            this.kfbBlur.TabIndex = 0;
            // 
            // tpColorMapping
            // 
            this.tpColorMapping.Controls.Add(this.tlpColorMapping);
            this.tpColorMapping.Location = new System.Drawing.Point(4, 22);
            this.tpColorMapping.Name = "tpColorMapping";
            this.tpColorMapping.Padding = new System.Windows.Forms.Padding(3);
            this.tpColorMapping.Size = new System.Drawing.Size(608, 187);
            this.tpColorMapping.TabIndex = 6;
            this.tpColorMapping.Text = "Color mapping";
            this.tpColorMapping.UseVisualStyleBackColor = true;
            // 
            // tlpColorMapping
            // 
            this.tlpColorMapping.ColumnCount = 2;
            this.tlpColorMapping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 183F));
            this.tlpColorMapping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tlpColorMapping.Controls.Add(this.pnColorMapping, 0, 0);
            this.tlpColorMapping.Controls.Add(this.kfbColorMapping, 0, 0);
            this.tlpColorMapping.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tlpColorMapping.Location = new System.Drawing.Point(3, 3);
            this.tlpColorMapping.Name = "tlpColorMapping";
            this.tlpColorMapping.RowCount = 1;
            this.tlpColorMapping.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tlpColorMapping.Size = new System.Drawing.Size(602, 181);
            this.tlpColorMapping.TabIndex = 6;
            // 
            // pnColorMapping
            // 
            this.pnColorMapping.Controls.Add(this.nslColorMappingInfluence);
            this.pnColorMapping.Controls.Add(this.lColorMappingInfluence);
            this.pnColorMapping.Controls.Add(this.tbColorMappingTexture);
            this.pnColorMapping.Controls.Add(this.lColorMappingTexture);
            this.pnColorMapping.Dock = System.Windows.Forms.DockStyle.Fill;
            this.pnColorMapping.Location = new System.Drawing.Point(183, 0);
            this.pnColorMapping.Margin = new System.Windows.Forms.Padding(0);
            this.pnColorMapping.Name = "pnColorMapping";
            this.pnColorMapping.Size = new System.Drawing.Size(419, 181);
            this.pnColorMapping.TabIndex = 2;
            // 
            // nslColorMappingInfluence
            // 
            this.nslColorMappingInfluence.BackColor = System.Drawing.SystemColors.Window;
            this.nslColorMappingInfluence.DecimalPlaces = 2;
            this.nslColorMappingInfluence.Hexadecimal = false;
            this.nslColorMappingInfluence.Location = new System.Drawing.Point(55, 9);
            this.nslColorMappingInfluence.Maximum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nslColorMappingInfluence.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslColorMappingInfluence.Minimum = new decimal(new int[] {
            0,
            0,
            0,
            0});
            this.nslColorMappingInfluence.MinimumSize = new System.Drawing.Size(64, 0);
            this.nslColorMappingInfluence.Name = "nslColorMappingInfluence";
            this.nslColorMappingInfluence.Size = new System.Drawing.Size(150, 21);
            this.nslColorMappingInfluence.SliderPrecision = 100;
            this.nslColorMappingInfluence.SpinnerWidth = 54;
            this.nslColorMappingInfluence.TabIndex = 16;
            this.nslColorMappingInfluence.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nslColorMappingInfluence.TickFrequency = 25;
            this.nslColorMappingInfluence.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslColorMappingInfluence.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // lColorMappingInfluence
            // 
            this.lColorMappingInfluence.AutoSize = true;
            this.lColorMappingInfluence.Location = new System.Drawing.Point(3, 11);
            this.lColorMappingInfluence.Name = "lColorMappingInfluence";
            this.lColorMappingInfluence.Size = new System.Drawing.Size(51, 13);
            this.lColorMappingInfluence.TabIndex = 14;
            this.lColorMappingInfluence.Text = "Influence";
            // 
            // tbColorMappingTexture
            // 
            this.tbColorMappingTexture.Location = new System.Drawing.Point(6, 55);
            this.tbColorMappingTexture.MaxLength = 260;
            this.tbColorMappingTexture.Name = "tbColorMappingTexture";
            this.tbColorMappingTexture.Size = new System.Drawing.Size(199, 20);
            this.tbColorMappingTexture.TabIndex = 1;
            // 
            // lColorMappingTexture
            // 
            this.lColorMappingTexture.AutoSize = true;
            this.lColorMappingTexture.Location = new System.Drawing.Point(3, 39);
            this.lColorMappingTexture.Name = "lColorMappingTexture";
            this.lColorMappingTexture.Size = new System.Drawing.Size(43, 13);
            this.lColorMappingTexture.TabIndex = 0;
            this.lColorMappingTexture.Text = "Texture";
            // 
            // kfbColorMapping
            // 
            this.kfbColorMapping.Dock = System.Windows.Forms.DockStyle.Fill;
            this.kfbColorMapping.Location = new System.Drawing.Point(0, 0);
            this.kfbColorMapping.Margin = new System.Windows.Forms.Padding(0);
            this.kfbColorMapping.MinimumSize = new System.Drawing.Size(183, 100);
            this.kfbColorMapping.Name = "kfbColorMapping";
            this.kfbColorMapping.SelectedIndex = -1;
            this.kfbColorMapping.Size = new System.Drawing.Size(183, 181);
            this.kfbColorMapping.TabIndex = 0;
            // 
            // sbMainBar
            // 
            this.sbMainBar.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(204)));
            this.sbMainBar.Location = new System.Drawing.Point(0, 420);
            this.sbMainBar.Name = "sbMainBar";
            this.sbMainBar.Panels.AddRange(new System.Windows.Forms.StatusBarPanel[] {
            this.sbpEffectTimeLabel,
            this.sbpEffectTime});
            this.sbMainBar.ShowPanels = true;
            this.sbMainBar.Size = new System.Drawing.Size(624, 22);
            this.sbMainBar.TabIndex = 1;
            // 
            // sbpEffectTimeLabel
            // 
            this.sbpEffectTimeLabel.Alignment = System.Windows.Forms.HorizontalAlignment.Right;
            this.sbpEffectTimeLabel.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Contents;
            this.sbpEffectTimeLabel.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.sbpEffectTimeLabel.MinWidth = 8;
            this.sbpEffectTimeLabel.Name = "sbpEffectTimeLabel";
            this.sbpEffectTimeLabel.Text = "Effect time:";
            this.sbpEffectTimeLabel.Width = 72;
            // 
            // sbpEffectTime
            // 
            this.sbpEffectTime.AutoSize = System.Windows.Forms.StatusBarPanelAutoSize.Contents;
            this.sbpEffectTime.BorderStyle = System.Windows.Forms.StatusBarPanelBorderStyle.None;
            this.sbpEffectTime.Name = "sbpEffectTime";
            this.sbpEffectTime.Text = "0.00";
            this.sbpEffectTime.Width = 36;
            // 
            // msMainMenu
            // 
            this.msMainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem});
            this.msMainMenu.Location = new System.Drawing.Point(0, 0);
            this.msMainMenu.Name = "msMainMenu";
            this.msMainMenu.Size = new System.Drawing.Size(624, 24);
            this.msMainMenu.TabIndex = 2;
            this.msMainMenu.TextDirection = System.Windows.Forms.ToolStripTextDirection.Vertical90;
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.miNew,
            this.miLoad,
            this.miSave});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.fileToolStripMenuItem.Text = "File";
            this.fileToolStripMenuItem.TextDirection = System.Windows.Forms.ToolStripTextDirection.Horizontal;
            // 
            // miNew
            // 
            this.miNew.Name = "miNew";
            this.miNew.ShortcutKeyDisplayString = "Ctrl+N";
            this.miNew.Size = new System.Drawing.Size(143, 22);
            this.miNew.Text = "New";
            this.miNew.Click += new System.EventHandler(this.CreateEffect);
            // 
            // miLoad
            // 
            this.miLoad.Name = "miLoad";
            this.miLoad.ShortcutKeyDisplayString = "Ctrl+O";
            this.miLoad.Size = new System.Drawing.Size(143, 22);
            this.miLoad.Text = "Load";
            this.miLoad.Click += new System.EventHandler(this.LoadEffect);
            // 
            // miSave
            // 
            this.miSave.Name = "miSave";
            this.miSave.ShortcutKeyDisplayString = "Ctrl+S";
            this.miSave.Size = new System.Drawing.Size(143, 22);
            this.miSave.Text = "Save";
            this.miSave.Click += new System.EventHandler(this.SaveEffect);
            // 
            // pbDrawingSurface
            // 
            this.pbDrawingSurface.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.pbDrawingSurface.BackColor = System.Drawing.Color.White;
            this.pbDrawingSurface.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.pbDrawingSurface.Location = new System.Drawing.Point(5, 27);
            this.pbDrawingSurface.Name = "pbDrawingSurface";
            this.pbDrawingSurface.Size = new System.Drawing.Size(614, 170);
            this.pbDrawingSurface.TabIndex = 3;
            this.pbDrawingSurface.TabStop = false;
            // 
            // nslBlur
            // 
            this.nslBlur.BackColor = System.Drawing.SystemColors.Window;
            this.nslBlur.DecimalPlaces = 2;
            this.nslBlur.Hexadecimal = false;
            this.nslBlur.Location = new System.Drawing.Point(55, 9);
            this.nslBlur.Maximum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.nslBlur.MaximumSize = new System.Drawing.Size(9000, 21);
            this.nslBlur.Minimum = new decimal(new int[] {
            0,
            0,
            0,
            0});
            this.nslBlur.MinimumSize = new System.Drawing.Size(64, 0);
            this.nslBlur.Name = "nslBlur";
            this.nslBlur.Size = new System.Drawing.Size(150, 21);
            this.nslBlur.SliderPrecision = 100;
            this.nslBlur.SpinnerWidth = 54;
            this.nslBlur.TabIndex = 18;
            this.nslBlur.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.nslBlur.TickFrequency = 25;
            this.nslBlur.TickStyle = System.Windows.Forms.TickStyle.None;
            this.nslBlur.Value = new decimal(new int[] {
            0,
            0,
            0,
            0});
            // 
            // lBlur
            // 
            this.lBlur.AutoSize = true;
            this.lBlur.Location = new System.Drawing.Point(3, 11);
            this.lBlur.Name = "lBlur";
            this.lBlur.Size = new System.Drawing.Size(40, 13);
            this.lBlur.TabIndex = 17;
            this.lBlur.Text = "Radius";
            // 
            // MainDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(624, 442);
            this.Controls.Add(this.pbDrawingSurface);
            this.Controls.Add(this.tabEffects);
            this.Controls.Add(this.sbMainBar);
            this.Controls.Add(this.msMainMenu);
            this.MainMenuStrip = this.msMainMenu;
            this.MinimumSize = new System.Drawing.Size(640, 480);
            this.Name = "MainDialog";
            this.Text = "X-Ray Postprocess Effect Editor";
            this.tabEffects.ResumeLayout(false);
            this.tpAC.ResumeLayout(false);
            this.tlpAC.ResumeLayout(false);
            this.pnAC.ResumeLayout(false);
            this.tpBC.ResumeLayout(false);
            this.tlpBC.ResumeLayout(false);
            this.pnBC.ResumeLayout(false);
            this.tpGC.ResumeLayout(false);
            this.tlpGC.ResumeLayout(false);
            this.pnGC.ResumeLayout(false);
            this.tpDuality.ResumeLayout(false);
            this.tlpDuality.ResumeLayout(false);
            this.pnDuality.ResumeLayout(false);
            this.pnDuality.PerformLayout();
            this.tpNoise.ResumeLayout(false);
            this.tlpNoise.ResumeLayout(false);
            this.pnNoise.ResumeLayout(false);
            this.pnNoise.PerformLayout();
            this.tpBlur.ResumeLayout(false);
            this.tlpBlur.ResumeLayout(false);
            this.pnBlur.ResumeLayout(false);
            this.pnBlur.PerformLayout();
            this.tpColorMapping.ResumeLayout(false);
            this.tlpColorMapping.ResumeLayout(false);
            this.pnColorMapping.ResumeLayout(false);
            this.pnColorMapping.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.sbpEffectTimeLabel)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.sbpEffectTime)).EndInit();
            this.msMainMenu.ResumeLayout(false);
            this.msMainMenu.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbDrawingSurface)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TabControl tabEffects;
        private System.Windows.Forms.TabPage tpAC;
        private System.Windows.Forms.StatusBar sbMainBar;
        private MenuStrip msMainMenu;
        private ToolStripMenuItem fileToolStripMenuItem;
        private ToolStripMenuItem miNew;
        private ToolStripMenuItem miLoad;
        private ToolStripMenuItem miSave;
        private PictureBox pbDrawingSurface;
        private StatusBarPanel sbpEffectTimeLabel;
        private StatusBarPanel sbpEffectTime;
        private TabPage tpBC;
        private TableLayoutPanel tlpAC;
        private KeyFrameBox kfbAC;
        private XRay.SdkControls.ColorPicker cpAC;
        private TableLayoutPanel tlpBC;
        private KeyFrameBox kfbBC;
        private TabPage tpGC;
        private TabPage tpDuality;
        private TabPage tpNoise;
        private TabPage tpBlur;
        private TabPage tpColorMapping;
        private TableLayoutPanel tlpGC;
        private KeyFrameBox kfbGC;
        private TableLayoutPanel tlpDuality;
        private KeyFrameBox kfbDuality;
        private Panel pnAC;
        private Panel pnBC;
        private XRay.SdkControls.ColorPicker cpBC;
        private Panel pnGC;
        private XRay.SdkControls.ColorPicker cpGC;
        private Panel pnDuality;
        private TableLayoutPanel tlpNoise;
        private Panel pnNoise;
        private KeyFrameBox kfbNoise;
        private TableLayoutPanel tlpBlur;
        private Panel pnBlur;
        private KeyFrameBox kfbBlur;
        private TableLayoutPanel tlpColorMapping;
        private Panel pnColorMapping;
        private KeyFrameBox kfbColorMapping;
        private Label lDualityX;
        private XRay.SdkControls.NumericSlider nslDualityX;
        private Label lDualityY;
        private XRay.SdkControls.NumericSlider nslDualityY;
        private Label lNoiseFPS;
        private XRay.SdkControls.NumericSlider nslNoiseFPS;
        private XRay.SdkControls.NumericSlider nslNoiseIntensity;
        private Label lNoiseGrain;
        private XRay.SdkControls.NumericSlider nslNoiseGrain;
        private Label lNoiseIntensity;
        private Label lColorMappingInfluence;
        private TextBox tbColorMappingTexture;
        private Label lColorMappingTexture;
        private XRay.SdkControls.NumericSlider nslColorMappingInfluence;
        private XRay.SdkControls.NumericSlider nslBlur;
        private Label lBlur;
    }
}

