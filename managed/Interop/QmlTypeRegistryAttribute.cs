using System;

namespace OpenTemple.Interop
{
    [AttributeUsage(AttributeTargets.Assembly)]
    public class QmlTypeRegistryAttribute : Attribute
    {
        public Type RegistryType { get; }

        public QmlTypeRegistryAttribute(Type registryType)
        {
            RegistryType = registryType;
        }
    }
}