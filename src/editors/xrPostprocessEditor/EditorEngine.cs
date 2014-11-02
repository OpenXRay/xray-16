using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using XRay.ManagedApi.Core;

namespace xrPostprocessEditor
{
    public struct NoiseParams
    {
        public float Intensity;
        public float Grain;
        public float FPS;
    }

    public struct ColorMappingParams
    {
        public float Influence;
        public string Texture;
    }

    public class EditorEngine : IDisposable
    {
        private BasicPostProcessAnimator animator;

        public EditorEngine()
        {
            animator = new BasicPostProcessAnimator(0, false);
        }

        public void Dispose()
        {
            if (animator == null)
                return;
            animator.Dispose();
            animator = null;
        }

        public PostProcessParamBase GetParam(PostProcessParamType paramType)
        {
            return animator.GetParam(paramType);
        }
        
        public void CreateKey(PostProcessParamType paramType, float time)
        {
            PostProcessParamBase param = animator.GetParam(paramType);
            try
            {
                switch (paramType)
                {
                    case PostProcessParamType.AddColor:
                    case PostProcessParamType.BaseColor:
                        // 3 components
                        param.AddValue(time, 0.0f, 0);
                        param.AddValue(time, 0.0f, 1);
                        param.AddValue(time, 0.0f, 2);
                        break;
                    case PostProcessParamType.GrayColor:
                        // 3+1 components
                        param.AddValue(time, 0.0f, 0);
                        param.AddValue(time, 0.0f, 1);
                        param.AddValue(time, 0.0f, 2);
                        param.Dispose();
                        param = animator.GetParam(PostProcessParamType.GrayValue);
                        param.AddValue(time, 0.0f, 0);
                        break;
                    case PostProcessParamType.DualityH:
                        param.AddValue(time, 0.44f, 0);
                        param.Dispose();
                        param = animator.GetParam(PostProcessParamType.DualityV);
                        param.AddValue(time, 0.44f, 0);
                        // 2 components
                        break;
                    case PostProcessParamType.NoiseIntensity:
                        param.AddValue(time, 0.33f, 0);
                        param.Dispose();
                        param = animator.GetParam(PostProcessParamType.NoiseGrain);
                        param.AddValue(time, 0.11f, 0);
                        param.Dispose();
                        param = animator.GetParam(PostProcessParamType.NoiseFps);
                        param.AddValue(time, 1.0f, 0);
                        break;
                    case PostProcessParamType.Blur:
                    case PostProcessParamType.ColorMappingInfluence:
                        // 1 component
                        param.AddValue(time, 0.33f, 0);
                        break;
                    default:
                        throw new ArgumentException("Invalid param type.");
                }
            }
            finally
            {
                param.Dispose();
            }
        }

        public ColorF GetAddColor(int keyIndex)
        {
            ColorF result;
            float time;
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.AddColor))
            {
                time = param.GetKeyTime(keyIndex);
                result.a = 0;
                param.GetValue(time, out result.r, 0);
                param.GetValue(time, out result.g, 1);
                param.GetValue(time, out result.b, 2);
            }
            return result;
        }

        public ColorF GetBaseColor(int keyIndex)
        {
            ColorF result;
            float time;
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.BaseColor))
            {
                time = param.GetKeyTime(keyIndex);
                result.a = 0;
                param.GetValue(time, out result.r, 0);
                param.GetValue(time, out result.g, 1);
                param.GetValue(time, out result.b, 2);
            }
            return result;
        }

        public ColorF GetGrayColor(int keyIndex)
        {
            ColorF result;
            float time;
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.GrayColor))
            {
                time = param.GetKeyTime(keyIndex);                
                param.GetValue(time, out result.r, 0);
                param.GetValue(time, out result.g, 1);
                param.GetValue(time, out result.b, 2);
            }
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.GrayValue))
            {
                param.GetValue(time, out result.a, 0);
            }
            return result;
        }

        public Vector2F GetDuality(int keyIndex)
        {
            Vector2F result;
            float time;
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.DualityH))
            {
                time = param.GetKeyTime(keyIndex);
                param.GetValue(time, out result.x, 0);
            }
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.DualityV))
            {
                param.GetValue(time, out result.y, 0);
            }
            return result;
        }

        public NoiseParams GetNoise(int keyIndex)
        {
            NoiseParams result;
            float time;
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.NoiseIntensity))
            {
                time = param.GetKeyTime(keyIndex);
                param.GetValue(time, out result.Intensity, 0);
            }
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.NoiseGrain))
            {
                param.GetValue(time, out result.Grain, 0);
            }
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.NoiseFps))
            {
                param.GetValue(time, out result.FPS, 0);
            }
            return result;
        }

        public float GetBlur(int keyIndex)
        {
            float result;
            float time;
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.Blur))
            {
                time = param.GetKeyTime(keyIndex);
                param.GetValue(time, out result, 0);
            }
            return result;
        }

        public ColorMappingParams GetColorMapping(int keyIndex)
        {
            ColorMappingParams result;
            float time;
            using (PostProcessParamBase param = animator.GetParam(PostProcessParamType.ColorMappingInfluence))
            {
                time = param.GetKeyTime(keyIndex);
                param.GetValue(time, out result.Influence, 0);
            }
            result.Texture = animator.PPInfo.ColorMappingGradient1;
            return result;
        }

        public void Reset() { animator.Create(); }

        public void LoadEffect(string fileName) { animator.Load(fileName, false); }

        public void SaveEffect(string fileName) { animator.Save(fileName); }

        public float EffectDuration { get { return animator.Length; } }
    }
}
