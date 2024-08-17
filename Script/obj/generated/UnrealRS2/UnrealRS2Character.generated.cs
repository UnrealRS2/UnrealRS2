using UnrealSharp.Engine;
using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.UnrealRS2;

[UClass, GeneratedType("UnrealRS2Character", "UnrealSharp.UnrealRS2.UnrealRS2Character")]
public partial class AUnrealRS2Character : UnrealSharp.Engine.ACharacter
{
    static AUnrealRS2Character()
    {
        IntPtr NativeClassPtr = UCoreUObjectExporter.CallGetNativeClassFromName("UnrealRS2Character");
        TopDownCameraComponent_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "TopDownCameraComponent");
        CameraBoom_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "CameraBoom");
    }
    
    static int TopDownCameraComponent_Offset;
    
    /// <summary>
    /// Top down camera
    /// </summary>
    public UnrealSharp.Engine.UCameraComponent TopDownCameraComponent
    {
        get
        {
            return ObjectMarshaller<UnrealSharp.Engine.UCameraComponent>.FromNative(IntPtr.Add(NativeObject, TopDownCameraComponent_Offset), 0);
        }
    }
    
    
    static int CameraBoom_Offset;
    
    /// <summary>
    /// Camera boom positioning the camera above the character
    /// </summary>
    public UnrealSharp.Engine.USpringArmComponent CameraBoom
    {
        get
        {
            return ObjectMarshaller<UnrealSharp.Engine.USpringArmComponent>.FromNative(IntPtr.Add(NativeObject, CameraBoom_Offset), 0);
        }
    }
    
    
}