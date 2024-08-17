using UnrealSharp.DeveloperSettings;
using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.CSharpForUE;

[UClass, GeneratedType("CSDeveloperSettings", "UnrealSharp.CSharpForUE.CSDeveloperSettings")]
public partial class UCSDeveloperSettings : UnrealSharp.DeveloperSettings.UDeveloperSettings
{
    static UCSDeveloperSettings()
    {
        IntPtr NativeClassPtr = UCoreUObjectExporter.CallGetNativeClassFromName("CSDeveloperSettings");
        bCrashOnException_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "bCrashOnException");
        bRequireFocusForHotReload_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "bRequireFocusForHotReload");
    }
    
    static int bCrashOnException_Offset;
    
    /// <summary>
    /// Should we exit PIE when an exception is thrown in C#?
    /// </summary>
    public bool CrashOnException
    {
        get
        {
            return BoolMarshaller.FromNative(IntPtr.Add(NativeObject, bCrashOnException_Offset), 0);
        }
        set
        {
            BoolMarshaller.ToNative(IntPtr.Add(NativeObject, bCrashOnException_Offset), 0, value);
        }
    }
    
    
    static int bRequireFocusForHotReload_Offset;
    
    /// <summary>
    /// Whether Hot Reload should wait for the Editor to gain focus
    /// </summary>
    public bool RequireFocusForHotReload
    {
        get
        {
            return BoolMarshaller.FromNative(IntPtr.Add(NativeObject, bRequireFocusForHotReload_Offset), 0);
        }
        set
        {
            BoolMarshaller.ToNative(IntPtr.Add(NativeObject, bRequireFocusForHotReload_Offset), 0, value);
        }
    }
    
    
}