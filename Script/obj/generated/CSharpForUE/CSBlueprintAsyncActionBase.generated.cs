using UnrealSharp.Engine;
using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.CSharpForUE;

[UClass(ClassFlags.Abstract), GeneratedType("CSBlueprintAsyncActionBase", "UnrealSharp.CSharpForUE.CSBlueprintAsyncActionBase")]
public partial class UCSBlueprintAsyncActionBase : UnrealSharp.Engine.UBlueprintAsyncActionBase
{
    static UCSBlueprintAsyncActionBase()
    {
        IntPtr NativeClassPtr = UCoreUObjectExporter.CallGetNativeClassFromName("CSBlueprintAsyncActionBase");
    }
    // ReceiveActivate
    IntPtr ReceiveActivate_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("ReceiveActivate")]
    protected virtual void Activate()
    {
        unsafe
        {
            if (ReceiveActivate_NativeFunction == IntPtr.Zero)
            {
                ReceiveActivate_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "ReceiveActivate");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, ReceiveActivate_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void Activate_Implementation()
    {
    }
    void Invoke_ReceiveActivate(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            Activate_Implementation();
        }
    }
    
    
}