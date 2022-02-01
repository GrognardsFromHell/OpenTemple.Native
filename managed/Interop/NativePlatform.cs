using System;
using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Runtime.InteropServices;
using System.Security;

namespace OpenTemple.Interop;

public static class NativePlatform
{
    static NativePlatform()
    {
        // Determine where the user data folder is
        var folder = GameFolders_GetUserDataFolder();

        UserDataFolder = folder != null ? Path.GetFullPath(folder) : null;
    }

    /// <summary>
    /// A path to the folder where user facing documents may be stored.
    /// On Windows this will be the Documents library.
    /// </summary>
    public static string UserDataFolder { get; }

    /// <summary>
    /// Opens the user's preferred file browsing application at the given local folder.
    /// </summary>
    public static void OpenFolder(string path)
    {
        Shell_OpenFolder(path);
    }

    /// <summary>
    /// Opens the user's preferred browser for the given URL.
    /// </summary>
    public static void OpenUrl(string url)
    {
        Shell_OpenUrl(url);
    }

    /// <summary>
    /// Allows the user to select a folder on their computer.
    /// </summary>
    /// <param name="title">Shown as the title of the folder picker</param>
    /// <param name="startingPath">Optionally provide the starting point of the folder picker process.</param>
    /// <param name="pickedFolder">Receives the path to the picked folder.</param>
    /// <returns>True if the user picked a folder, false if they canceled.</returns>
    /// <exception cref="InvalidOperationException"></exception>
    public static bool PickFolder(string title, string startingPath, out string pickedFolder)
    {
        var result = Shell_PickFolder(title, startingPath, out pickedFolder);

        if (result == 0)
        {
            return true;
        }
        else if (result == 1)
        {
            pickedFolder = null;
            return false;
        }
        else
        {
            throw new InvalidOperationException($"Failed to show folder picker. Code: {result}");
        }
    }

    /// <summary>
    /// Shows a prompt to the user with detailed info that they can confirm or close.
    /// </summary>
    /// <param name="errorIcon">Should the prompt display an error icon?</param>
    /// <param name="title">The window title.</param>
    /// <param name="emphasizedText">Emphasized text at the beginning of the prompt.</param>
    /// <param name="detailedText">More detailed in-depth text.</param>
    /// <returns>True if the user confirmed.</returns>
    public static bool ShowPrompt(bool errorIcon,
        string title,
        string emphasizedText,
        string detailedText)
    {
        return Shell_ShowPrompt(errorIcon, title, emphasizedText, detailedText);
    }

    /// <summary>
    /// Shows a message to the user with detailed info.
    /// </summary>
    /// <param name="errorIcon">Should the prompt display an error icon?</param>
    /// <param name="title">The window title.</param>
    /// <param name="emphasizedText">Emphasized text at the beginning of the prompt.</param>
    /// <param name="detailedText">More detailed in-depth text.</param>
    public static void ShowMessage(bool errorIcon,
        string title,
        string emphasizedText,
        string detailedText)
    {
        Shell_ShowMessage(errorIcon, title, emphasizedText, detailedText);
    }

    /// <summary>
    /// Copies text to the system clipboard.
    /// </summary>
    public static void SetClipboardText(IntPtr nativeWindowHandle, string text)
    {
        var errorCode = Shell_SetClipboardText(nativeWindowHandle, text);
        if (errorCode != 0)
        {
            Debug.Print("Failed to copy text '{0}' to clipboard: {1}", text, errorCode);
        }
    }

    /// <summary>
    /// Gets text from the system clipboard.
    /// </summary>
    public static bool TryGetClipboardText(IntPtr nativeWindowHandle, [NotNullWhen(true)] out string text)
    {
        return Shell_GetClipboardText(nativeWindowHandle, out text) == 0 && text != null;
    }

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    private static extern string GameFolders_GetUserDataFolder();

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    private static extern void Shell_OpenFolder(string path);

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    private static extern void Shell_OpenUrl(string url);

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    private static extern int Shell_PickFolder(
        string title,
        string startingDirectory,
        out string pickedFolder
    );

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    [return: MarshalAs(UnmanagedType.I1)]
    private static extern bool Shell_ShowPrompt(
        bool errorIcon,
        string promptTitle,
        string promptEmphasized,
        string promptDetailed);

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    private static extern void Shell_ShowMessage(
        bool errorIcon,
        string promptTitle,
        string promptEmphasized,
        string promptDetailed);

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    private static extern int Shell_SetClipboardText(
        IntPtr nativeWindowHandle,
        string text
    );

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    private static extern int Shell_GetClipboardText(
        IntPtr nativeWindowHandle,
        [MarshalAs(UnmanagedType.LPWStr)]
        out string text
    );

    [DllImport(OpenTempleLib.Path, CharSet = CharSet.Unicode)]
    public static extern string FindInstallDirectory();
}