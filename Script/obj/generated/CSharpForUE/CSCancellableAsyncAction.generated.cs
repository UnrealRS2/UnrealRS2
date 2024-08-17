using UnrealSharp.Engine;
using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.CSharpForUE;

[UClass(ClassFlags.Abstract), GeneratedType("CSCancellableAsyncAction", "UnrealSharp.CSharpForUE.CSCancellableAsyncAction")]
public partial class UCSCancellableAsyncAction : UnrealSharp.Engine.UCancellableAsyncAction
{
    static UCSCancellableAsyncAction()
    {
        IntPtr NativeClassPtr = UCoreUObjectExporter.CallGetNativeClassFromName("CSCancellableAsyncAction");
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
    
    // ReceiveCancel
    IntPtr ReceiveCancel_NativeFunction;
    
    [UFunction(FunctionFlags.BlueprintEvent), GeneratedType("ReceiveCancel")]
    protected virtual void Cancel()
    {
        unsafe
        {
            if (ReceiveCancel_NativeFunction == IntPtr.Zero)
            {
                ReceiveCancel_NativeFunction = UClassExporter.CallGetNativeFunctionFromInstanceAndName(NativeObject, "ReceiveCancel");
            }
            UObjectExporter.CallInvokeNativeFunction(NativeObject, ReceiveCancel_NativeFunction, IntPtr.Zero);
        }
    }
    
    // Hide implementation function from Intellisense
    [System.ComponentModel.EditorBrowsable(System.ComponentModel.EditorBrowsableState.Never)]
    protected virtual void Cancel_Implementation()
    {
    }
    void Invoke_ReceiveCancel(IntPtr buffer, IntPtr returnBuffer)
    {
        unsafe
        {
            Cancel_Implementation();
        }
    }
    
    
}