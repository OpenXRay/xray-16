using System;
using System.Collections.Generic;
using System.Text;

namespace CustomControls
{
    [AttributeUsage(AttributeTargets.Class | 
                    AttributeTargets.Enum |    
                    AttributeTargets.Interface |
                    AttributeTargets.Struct,
                    AllowMultiple = true)]
    [Author("Franco, Gustavo")]
    internal class AuthorAttribute : Attribute
    {
        #region Constructors
        public AuthorAttribute(string authorName)
        {
        }
        #endregion
    }
}
