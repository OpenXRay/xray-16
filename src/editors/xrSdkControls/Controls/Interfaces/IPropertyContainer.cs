using Flobbster.Windows.Forms;

namespace XRay.SdkControls
{
    public interface IPropertyContainer
    {
        IProperty GetProperty(PropertySpec description);
    }
}
