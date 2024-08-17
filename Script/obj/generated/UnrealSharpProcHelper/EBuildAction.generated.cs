using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.UnrealSharpProcHelper;

[UEnum, GeneratedType("EBuildAction", "UnrealSharp.UnrealSharpProcHelper.EBuildAction")]
public enum EBuildAction
{
    Build = 0,
    Clean = 1,
    GenerateProject = 2,
    Rebuild = 3,
    Weave = 4,
}