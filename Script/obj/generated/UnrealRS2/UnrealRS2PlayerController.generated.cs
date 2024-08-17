using UnrealSharp.Engine;
using UnrealSharp.Niagara;
using UnrealSharp.EnhancedInput;
using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.UnrealRS2;

[UClass, GeneratedType("UnrealRS2PlayerController", "UnrealSharp.UnrealRS2.UnrealRS2PlayerController")]
public partial class AUnrealRS2PlayerController : UnrealSharp.Engine.APlayerController
{
    static AUnrealRS2PlayerController()
    {
        IntPtr NativeClassPtr = UCoreUObjectExporter.CallGetNativeClassFromName("UnrealRS2PlayerController");
        ShortPressThreshold_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "ShortPressThreshold");
        FXCursor_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "FXCursor");
        DefaultMappingContext_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "DefaultMappingContext");
        SetDestinationClickAction_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "SetDestinationClickAction");
        SetDestinationTouchAction_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "SetDestinationTouchAction");
    }
    
    static int ShortPressThreshold_Offset;
    
    /// <summary>
    /// Time Threshold to know if it was a short press
    /// </summary>
    public float ShortPressThreshold
    {
        get
        {
            return BlittableMarshaller<float>.FromNative(IntPtr.Add(NativeObject, ShortPressThreshold_Offset), 0);
        }
    }
    
    
    static int FXCursor_Offset;
    
    /// <summary>
    /// FX Class that we will spawn when clicking
    /// </summary>
    public UnrealSharp.Niagara.UNiagaraSystem FXCursor
    {
        get
        {
            return ObjectMarshaller<UnrealSharp.Niagara.UNiagaraSystem>.FromNative(IntPtr.Add(NativeObject, FXCursor_Offset), 0);
        }
    }
    
    
    static int DefaultMappingContext_Offset;
    
    /// <summary>
    /// MappingContext
    /// </summary>
    public UnrealSharp.EnhancedInput.UInputMappingContext DefaultMappingContext
    {
        get
        {
            return ObjectMarshaller<UnrealSharp.EnhancedInput.UInputMappingContext>.FromNative(IntPtr.Add(NativeObject, DefaultMappingContext_Offset), 0);
        }
    }
    
    
    static int SetDestinationClickAction_Offset;
    
    /// <summary>
    /// Jump Input Action
    /// </summary>
    public UnrealSharp.EnhancedInput.UInputAction SetDestinationClickAction
    {
        get
        {
            return ObjectMarshaller<UnrealSharp.EnhancedInput.UInputAction>.FromNative(IntPtr.Add(NativeObject, SetDestinationClickAction_Offset), 0);
        }
    }
    
    
    static int SetDestinationTouchAction_Offset;
    
    /// <summary>
    /// Jump Input Action
    /// </summary>
    public UnrealSharp.EnhancedInput.UInputAction SetDestinationTouchAction
    {
        get
        {
            return ObjectMarshaller<UnrealSharp.EnhancedInput.UInputAction>.FromNative(IntPtr.Add(NativeObject, SetDestinationTouchAction_Offset), 0);
        }
    }
    
    
}