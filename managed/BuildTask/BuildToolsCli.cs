using System;
using System.Collections.Generic;
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

        [Option('I', Required = false, HelpText = "Extra import paths.")]
        public IEnumerable<string> ImportPaths { get; set; }

        [Option('x', Separator = ':', Required = false, HelpText = "Exclude path pattern from processed QML files.")]
        public IEnumerable<string> ExcludePatterns { get; set; }

        [Option("addQmlModule", Required = false, HelpText = "Adds additional QML modules (Name:Version)")]
        public IEnumerable<string> AdditionalQmlModules { get; set; }

        [Option("addMetaClasses", Required = false, Separator = ';', HelpText = "Adds additional metaclasses")]
        public IEnumerable<string> AdditionalMetaClasses { get; set; }
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
                    foreach (var qmlModuleSpec in o.AdditionalQmlModules)
                    {
                        var parts = qmlModuleSpec.Split(':');
                        if (parts.Length != 2 || !int.TryParse(parts[1], out var majorVersion))
                        {
                            throw new ArgumentException("Invalid QML module spec: " + qmlModuleSpec);
                        }

                        qmlInfo.AddNativeModule(parts[0], majorVersion);
                    }

                    foreach (var metaClassName in o.AdditionalMetaClasses)
                    {
                        qmlInfo.AddMetaClass(metaClassName);
                    }

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