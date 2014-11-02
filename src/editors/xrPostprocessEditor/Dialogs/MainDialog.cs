using System;
using System.Drawing;
using System.Windows.Forms;
using XRay.ManagedApi.Core;

namespace xrPostprocessEditor
{
    public partial class MainDialog : Form
    {
        private class ChannelDesc
        {
            public delegate void UpdateHandler(int keyIndex);
            public TabPage Page;
            public KeyFrameBox List;
            public PostProcessParamType Type;
            public string Name;
            public UpdateHandler Update;

            public ChannelDesc(TabPage page, KeyFrameBox kfb, PostProcessParamType type, string name,
                UpdateHandler updater)
            {
                Page = page;
                List = kfb;
                Type = type;
                Name = name;
                Update = updater;
            }
        }

        private EditorEngine engine;
        private readonly string defaultEffectName = "untitled";
        private string effectName;
        private ChannelDesc[] chInfo;

        public MainDialog()
        {
            InitializeComponent();
            chInfo = new ChannelDesc[]
            {
                new ChannelDesc(tpAC, kfbAC, PostProcessParamType.AddColor, "Add color", UpdateAC),
                new ChannelDesc(tpBC, kfbBC, PostProcessParamType.BaseColor, "Base color", UpdateBC),
                new ChannelDesc(tpGC, kfbGC, PostProcessParamType.GrayColor, "Gray color", UpdateGC),
                new ChannelDesc(tpDuality, kfbDuality, PostProcessParamType.DualityH, "Duality", UpdateDuality),
                new ChannelDesc(tpNoise, kfbNoise, PostProcessParamType.NoiseIntensity, "Noise", UpdateNoise),
                new ChannelDesc(tpBlur, kfbBlur, PostProcessParamType.Blur, "Blur", UpdateBlur),
                new ChannelDesc(tpColorMapping, kfbColorMapping, PostProcessParamType.ColorMappingInfluence,
                    "Color mapping", UpdateColorMapping)
            };
            for (int kfbIndex = 0; kfbIndex < chInfo.Length; kfbIndex++)
            {
                var dstChannel = chInfo[kfbIndex];
                for (int tabIndex = 0; tabIndex < chInfo.Length; tabIndex++)
                {
                    var srcChannel = chInfo[tabIndex];
                    var item = new MenuItem(srcChannel.Page.Text);
                    if (kfbIndex != tabIndex)
                        item.Click += (s, e) => CopyKeyFrames(dstChannel, srcChannel);
                    else
                        item.Enabled = false;
                    dstChannel.List.CopyMenu.MenuItems.Add(item);
                }
            }
            SetCurrentEffectName(defaultEffectName);
            foreach (var ch in chInfo)
                ch.List.SelectedIndexChanged += (s, e) => ch.Update((s as KeyFrameBox).SelectedIndex);
        }

        Color ConvertColor(ColorF value)
        {
            Color result = Color.FromArgb(
                (byte)(255*value.a),
                (byte)(255*value.r),
                (byte)(255*value.g),
                (byte)(255*value.b));
            return result;
        }

        private void UpdateAC(int keyIndex)
        {
            ColorF value = engine.GetAddColor(keyIndex);
            cpAC.Value = ConvertColor(value);
        }

        private void UpdateBC(int keyIndex)
        {
            ColorF value = engine.GetBaseColor(keyIndex);
            cpBC.Value = ConvertColor(value);
        }

        private void UpdateGC(int keyIndex)
        {
            ColorF value = engine.GetGrayColor(keyIndex);
            cpGC.Value = ConvertColor(value);
        }

        private void UpdateDuality(int keyIndex)
        {
            Vector2F value = engine.GetDuality(keyIndex);
            nslDualityX.Value = (decimal)value.x;
            nslDualityY.Value = (decimal)value.y;
        }

        private void UpdateNoise(int keyIndex)
        {
            NoiseParams value = engine.GetNoise(keyIndex);
            nslNoiseIntensity.Value = (decimal)value.Intensity;
            nslNoiseGrain.Value = (decimal)value.Grain;
            nslNoiseFPS.Value = (decimal)value.FPS;
        }

        private void UpdateBlur(int keyIndex)
        {
            float value = engine.GetBlur(keyIndex);
            nslBlur.Value = (decimal)value;
        }

        private void UpdateColorMapping(int keyIndex)
        {
            ColorMappingParams value = engine.GetColorMapping(keyIndex);
            nslColorMappingInfluence.Value = (decimal)value.Influence;
            tbColorMappingTexture.Text = value.Texture;
        }
        
        public void Initialize(EditorEngine engine) { this.engine = engine; }

        private void CopyKeyFrames(ChannelDesc dst, ChannelDesc src)
        {
            using (var dstParam = engine.GetParam(dst.Type))
            using (var srcParam = engine.GetParam(src.Type))
            {
                // 1. engine: remove old keyframes
                dst.List.Items.Clear();
                dstParam.Reset();
                // 2. engine: create new ones
                for (int i = 0; i < srcParam.KeyCount; i++)
                    engine.CreateKey(dst.Type, srcParam.GetKeyTime(i));
            }
            LoadChannel(dst);
        }

        private void SetCurrentEffectName(string name)
        {
            effectName = name;
            Text = String.Format("{0} - {1}", effectName, Application.ProductName);
        }

        private void LoadChannel(ChannelDesc ch)
        {
            using (var param = engine.GetParam(ch.Type))
            {
                ch.List.Items.Clear();
                for (int i = 0; i < param.KeyCount; i++)
                    ch.List.Items.Add(param.GetKeyTime(i));
            }
        }
        
        private void LoadAllChannels()
        {
            foreach (var ch in chInfo)
                LoadChannel(ch);
        }

        private void CreateEffect(object sender, EventArgs e)
        {
            // XXX: show confirmation dialog if there are unsaved changes
            engine.Reset();
            SetCurrentEffectName(defaultEffectName);
            LoadAllChannels();
        }

        private void LoadEffect(object sender, EventArgs e)
        {
            string fileName;
            using (var dlg = new OpenFileDialog())
            {
                dlg.RestoreDirectory = true;
                dlg.Multiselect = false;
                dlg.Filter = "Post-process effects (.ppe)|*.ppe|All Files (*.*)|*.*";
                if (dlg.ShowDialog() != DialogResult.OK)
                    return;
                fileName = dlg.FileName;
            }
            engine.LoadEffect(fileName);
            SetCurrentEffectName(fileName);
            LoadAllChannels();
        }

        private void SaveEffect(object sender, EventArgs e)
        {
            if (engine.EffectDuration == 0.0f)
            {
                MessageBox.Show("Can't save zero length effect.", Application.ProductName);
                return;
            }
            using (var dlg = new SaveFileDialog())
            {
                dlg.RestoreDirectory = true;
                dlg.Filter = "Post-process effects (.ppe)|*.ppe|All Files (*.*)|*.*";
                if (dlg.ShowDialog() == DialogResult.OK)
                    engine.SaveEffect(dlg.FileName);
            }
        }
    }
}
