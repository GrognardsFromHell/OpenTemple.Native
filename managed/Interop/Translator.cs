using System;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTemple.Interop
{
    [return: MarshalAs(UnmanagedType.LPWStr)]
    public delegate string TranslatorDelegate(
        [MarshalAs(UnmanagedType.LPUTF8Str)]
        string context,
        [MarshalAs(UnmanagedType.LPUTF8Str)]
        string sourceText,
        [MarshalAs(UnmanagedType.LPUTF8Str)]
        string disambiguation,
        int n
    );

    [SuppressUnmanagedCodeSecurity]
    public static class Translator
    {
        public static void Install(TranslatorDelegate translator)
        {
            translator_install(NativeDelegate.Create(translator));
        }

        public static string qsTrId(string id)
        {
            return id;
        }

        public static string qsTr(string text)
        {
            return text;
        }

        [DllImport(OpenTempleLib.Path)]
        private static extern void translator_install(NativeDelegate dlgt);
    }
}