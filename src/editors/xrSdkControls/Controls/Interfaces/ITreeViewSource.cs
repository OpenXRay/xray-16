namespace XRay.SdkControls
{
    public interface ITreeViewSource
    {
        TreeView Parent
        {
            get;
            set;
        }

        void Refresh();
    }
}
