using System;
using System.IO;
using System.Runtime.InteropServices;
using CommandLine;
using QmlBuildTasks.Introspection;

namespace QmlBuildTasks
{
    public class Options
    {
        [Option("baseDir", Required = true, HelpText = "Base directory for .qml files")]
        public string BaseDirectory { get; set; }

        [Option("output", Required = true, HelpText = "Where to write the generated code to")]
        public string OutputPath { get; set; }

        [Option("qmlClassSuffix", Required = false,
            HelpText = "Suffix to append to class names generated for .qml files")]
        public string QmlClassSuffix { get; set; }

        [Option("pascalCase", Required = false, HelpText = "Convert class members to pascal case (C# case)")]
        public bool PascalCase { get; set; }

        [Option("nativeLib", Required = false, HelpText = "Path to Native DLL to preload.")]
        public string NativeLibraryPath { get; set; }
    }

    public static class BuildToolsCli
    {
        [STAThread]
        public static void Main(string[] args)
        {
            Parser.Default.ParseArguments<Options>(args)
                .WithParsed(o =>
                {
                    if (o.NativeLibraryPath != null)
                    {
                        NativeLibrary.SetDllImportResolver(typeof(OpenTemple.Interop.OpenTempleLib).Assembly,
                            (name, assembly, path) =>
                            {
                                if (name == "OpenTemple.Native")
                                {
                                    return NativeLibrary.Load(o.NativeLibraryPath);
                                }

                                return NativeLibrary.Load(name);
                            });

                        NativeLibrary.SetDllImportResolver(typeof(BuildToolsCli).Assembly,
                            (name, assembly, path) =>
                            {
                                if (name == "OpenTemple.Native")
                                {
                                    return NativeLibrary.Load(o.NativeLibraryPath);
                                }

                                return NativeLibrary.Load(name);
                            });
                    }

                    using var qmlInfo = new QmlInfo(o);

                    qmlInfo.AddNativeModule("OpenTemple", 1);

                    foreach (var file in Directory.EnumerateFiles(o.BaseDirectory, "*.qml", SearchOption.AllDirectories)
                    )
                    {
                        qmlInfo.LoadFile(file.Replace("\\", "/"));
                    }

                    var generatedCode = qmlInfo.Process();
                    File.WriteAllText(o.OutputPath, generatedCode);
                });
        }
    }
}