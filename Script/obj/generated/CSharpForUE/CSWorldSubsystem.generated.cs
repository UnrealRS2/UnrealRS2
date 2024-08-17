using UnrealSharp.Engine;
using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.CSharpForUE;

[UClass(ClassFlags.Abstract), GeneratedType("CSWorldSubsystem", "UnrealSharp.CSharpForUE.CSWorldSubsystem")]
public partial class UCSWorldSubsystem : UnrealSharp.Engine.UTickableWorldSubsystem
{
    static UCSWorldSubsystem()
    {
        IntPtr NativeClassPtr = UCoreUObjectExporter.CallGetNativeClassFromName("CSWorldSubsystem");
        IntPtr K2_Tick_NativeFunction = UClassExporter.CallGetNativeFunctionFromClassAndName(NativeClassPtr, "K2_Tick");
        K2_Tick_ParamsSize = UFunctionExporter.CallGetNativeFunctionParamsSize(K2_Tick_NativeFunction);
        K2_Tick_DeltaTime_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(K2_Tick_NativeFunction, "DeltaTime");
        IntPtr K2_ShouldCreateSubsystem_NativeFunction = UClassExporter.CallGetNativeFunctionFromClassAndName(NativeClassPtr, "K2_ShouldCreateSubsystem");
        K2_ShouldCreateSubsystem_ParamsSize = UFunctionExporter.CallGetNativeFunctionParamsSize(K2_ShouldCreateSubsystem_NativeFunction);
        K2_ShouldCreateSubsystem_ReturnValue_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(K2_ShouldCreateSubsystem_NativeFunction, "ReturnValue");
    }
    // K2_PostInitialize
    IntPtr K2_PostInitialize_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_PostInitialize")]
    protected virtual void PostInitialize()
    {
        unsafe
        {
            if (K2_PostInitialize_NativeFunction == IntPtr.Zero)
            {
                K2_PostInitialize_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_PostInitialize");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_PostInitialize_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void PostInitialize_Implementation()
    {
    }
    void Invoke_K2_PostInitialize(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            PostInitialize_Implementation();
        }
    }
    
    // K2_Tick
    IntPtr K2_Tick_NativeFunction;
    static int K2_Tick_ParamsSize;
    static int K2_Tick_DeltaTime_Offset;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_Tick")]
    protected virtual void Tick(float deltaTime)
    {
        unsafe
        {
            if (K2_Tick_NativeFunction == IntPtr.Zero)
            {
                K2_Tick_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_Tick");
            }
            byte* ParamsBufferAllocation = stackalloc byte[K2_Tick_ParamsSize];
            nint ParamsBuffer = (nint) ParamsBufferAllocation;
            UStructExporter.CallInitializeStruct(K2_Tick_NativeFunction, ParamsBuffer);
            BlittableMarshaller<float>.ToNative(IntPtr.Add(ParamsBuffer, K2_Tick_DeltaTime_Offset), 0, deltaTime);
            
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_Tick_NativeFunction, ParamsBuffer);
            
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void Tick_Implementation(float DeltaTime)
    {
    }
    void Invoke_K2_Tick(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            float DeltaTime =  BlittableMarshaller<float>.FromNative(IntPtr.Add(buffer, K2_Tick_DeltaTime_Offset), 0);
            Tick_Implementation(DeltaTime);
        }
    }
    
    // K2_OnWorldBeginPlay
    IntPtr K2_OnWorldBeginPlay_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_OnWorldBeginPlay")]
    protected virtual void OnWorldBeginPlay()
    {
        unsafe
        {
            if (K2_OnWorldBeginPlay_NativeFunction == IntPtr.Zero)
            {
                K2_OnWorldBeginPlay_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_OnWorldBeginPlay");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_OnWorldBeginPlay_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void OnWorldBeginPlay_Implementation()
    {
    }
    void Invoke_K2_OnWorldBeginPlay(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            OnWorldBeginPlay_Implementation();
        }
    }
    
    // K2_OnWorldComponentsUpdated
    IntPtr K2_OnWorldComponentsUpdated_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_OnWorldComponentsUpdated")]
    protected virtual void OnWorldComponentsUpdated()
    {
        unsafe
        {
            if (K2_OnWorldComponentsUpdated_NativeFunction == IntPtr.Zero)
            {
                K2_OnWorldComponentsUpdated_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_OnWorldComponentsUpdated");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_OnWorldComponentsUpdated_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void OnWorldComponentsUpdated_Implementation()
    {
    }
    void Invoke_K2_OnWorldComponentsUpdated(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            OnWorldComponentsUpdated_Implementation();
        }
    }
    
    // K2_UpdateStreamingState
    IntPtr K2_UpdateStreamingState_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_UpdateStreamingState")]
    protected virtual void UpdateStreamingState()
    {
        unsafe
        {
            if (K2_UpdateStreamingState_NativeFunction == IntPtr.Zero)
            {
                K2_UpdateStreamingState_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_UpdateStreamingState");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_UpdateStreamingState_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void UpdateStreamingState_Implementation()
    {
    }
    void Invoke_K2_UpdateStreamingState(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            UpdateStreamingState_Implementation();
        }
    }
    
    // K2_ShouldCreateSubsystem
    IntPtr K2_ShouldCreateSubsystem_NativeFunction;
    static int K2_ShouldCreateSubsystem_ParamsSize;
    static int K2_ShouldCreateSubsystem_ReturnValue_Offset;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_ShouldCreateSubsystem")]
    protected virtual bool ShouldCreateSubsystem()
    {
        unsafe
        {
            if (K2_ShouldCreateSubsystem_NativeFunction == IntPtr.Zero)
            {
                K2_ShouldCreateSubsystem_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_ShouldCreateSubsystem");
            }
            byte* ParamsBufferAllocation = stackalloc byte[K2_ShouldCreateSubsystem_ParamsSize];
            nint ParamsBuffer = (nint) ParamsBufferAllocation;
            UStructExporter.CallInitializeStruct(K2_ShouldCreateSubsystem_NativeFunction, ParamsBuffer);
            
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_ShouldCreateSubsystem_NativeFunction, ParamsBuffer);
            
            bool returnValue;
            returnValue = BoolMarshaller.FromNative(IntPtr.Add(ParamsBuffer, K2_ShouldCreateSubsystem_ReturnValue_Offset), 0);
            
            return returnValue;
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual bool ShouldCreateSubsystem_Implementation()
    {
        return default(bool);
    }
    void Invoke_K2_ShouldCreateSubsystem(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            bool returnValue = ShouldCreateSubsystem_Implementation();
            BoolMarshaller.ToNative(IntPtr.Add(returnBuffer, 0), 0, returnValue);
        }
    }
    
    // K2_Initialize
    IntPtr K2_Initialize_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_Initialize")]
    protected virtual void Initialize()
    {
        unsafe
        {
            if (K2_Initialize_NativeFunction == IntPtr.Zero)
            {
                K2_Initialize_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_Initialize");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_Initialize_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void Initialize_Implementation()
    {
    }
    void Invoke_K2_Initialize(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            Initialize_Implementation();
        }
    }
    
    // K2_Deinitialize
    IntPtr K2_Deinitialize_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("K2_Deinitialize")]
    protected virtual void Deinitialize()
    {
        unsafe
        {
            if (K2_Deinitialize_NativeFunction == IntPtr.Zero)
            {
                K2_Deinitialize_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "K2_Deinitialize");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, K2_Deinitialize_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void Deinitialize_Implementation()
    {
    }
    void Invoke_K2_Deinitialize(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            Deinitialize_Implementation();
        }
    }
    
    
}