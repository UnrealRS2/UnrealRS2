using UnrealSharp.Engine;
using UnrealSharp.CSharpForUE;
using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.CSharpForUE;

[UClass, GeneratedType("CSTestActor", "UnrealSharp.CSharpForUE.CSTestActor")]
public partial class ACSTestActor : UnrealSharp.Engine.AActor
{
    static ACSTestActor()
    {
        IntPtr NativeClassPtr = UCoreUObjectExporter.CallGetNativeClassFromName("CSTestActor");
        MyTestInteger_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "MyTestInteger");
        MyTestString_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "MyTestString");
        MyTestString_NativeProperty = FPropertyExporter.CallGetNativePropertyFromName(NativeClassPtr, "MyTestString");
        MyTestMap_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "MyTestMap");
        MyTestMap_NativeProperty = FPropertyExporter.CallGetNativePropertyFromName(NativeClassPtr, "MyTestMap");
        MyTestArray_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "MyTestArray");
        MyTestArray_NativeProperty = FPropertyExporter.CallGetNativePropertyFromName(NativeClassPtr, "MyTestArray");
        MyReadOnlyTestMap_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "MyReadOnlyTestMap");
        MyReadOnlyTestMap_NativeProperty = FPropertyExporter.CallGetNativePropertyFromName(NativeClassPtr, "MyReadOnlyTestMap");
        MyReadOnlyTestArray_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(NativeClassPtr, "MyReadOnlyTestArray");
        MyReadOnlyTestArray_NativeProperty = FPropertyExporter.CallGetNativePropertyFromName(NativeClassPtr, "MyReadOnlyTestArray");
        MyScriptMethod_NativeFunction = UClassExporter.CallGetNativeFunctionFromClassAndName(NativeClassPtr, "MyScriptMethod");
        MyScriptMethod_ParamsSize = UFunctionExporter.CallGetNativeFunctionParamsSize(MyScriptMethod_NativeFunction);
        MyScriptMethod_MyInteger_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(MyScriptMethod_NativeFunction, "MyInteger");
        MyScriptMethod_ReturnValue_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(MyScriptMethod_NativeFunction, "ReturnValue");
        MyNonScriptMethod_NativeFunction = UClassExporter.CallGetNativeFunctionFromClassAndName(NativeClassPtr, "MyNonScriptMethod");
        MyNonScriptMethod_ParamsSize = UFunctionExporter.CallGetNativeFunctionParamsSize(MyNonScriptMethod_NativeFunction);
        MyNonScriptMethod_MyInteger_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(MyNonScriptMethod_NativeFunction, "MyInteger");
        MyNonScriptMethod_ReturnValue_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(MyNonScriptMethod_NativeFunction, "ReturnValue");
        MyTestFunction_NativeFunction = UClassExporter.CallGetNativeFunctionFromClassAndName(NativeClassPtr, "MyTestFunction");
        MyTestFunction_ParamsSize = UFunctionExporter.CallGetNativeFunctionParamsSize(MyTestFunction_NativeFunction);
        MyTestFunction_TestMap_Offset = FPropertyExporter.CallGetPropertyOffsetFromName(MyTestFunction_NativeFunction, "TestMap");
        MyTestFunction_TestMap_NativeProperty = FPropertyExporter.CallGetNativePropertyFromName(MyTestFunction_NativeFunction, "TestMap");
    }
    
    static int MyTestInteger_Offset;
    
    /// <summary>
    /// MyTestInteger is a test integer
    /// </summary>
    public int MyTestInteger
    {
        get
        {
            return BlittableMarshaller<int>.FromNative(IntPtr.Add(NativeObject, MyTestInteger_Offset), 0);
        }
    }
    
    
    static int MyTestString_Offset;
    static readonly IntPtr MyTestString_NativeProperty;
    
    /// <summary>
    /// MyTestString is a test string
    /// </summary>
    public string MyTestString
    {
        get
        {
            IntPtr MyTestString_NativePtr = IntPtr.Add(NativeObject,MyTestString_Offset);
            return StringMarshaller.FromNative(MyTestString_NativePtr,0);
        }
    }
    
    
    static int MyTestMap_Offset;
    static IntPtr MyTestMap_NativeProperty;
    MapMarshaller<FName, int> MyTestMap_Marshaller = null;
    
    /// <summary>
    /// MyTestMap is a test map
    /// </summary>
    public System.Collections.Generic.IDictionary<FName, int> MyTestMap
    {
        get
        {
            MyTestMap_Marshaller ??= new MapMarshaller<FName, int>(1, MyTestMap_NativeProperty, BlittableMarshaller<FName>.ToNative, BlittableMarshaller<FName>.FromNative, BlittableMarshaller<int>.ToNative, BlittableMarshaller<int>.FromNative);
            return MyTestMap_Marshaller.FromNative(IntPtr.Add(NativeObject, MyTestMap_Offset), 0);
        }
    }
    
    
    static int MyTestArray_Offset;
    static IntPtr MyTestArray_NativeProperty;
    ArrayMarshaller<int> MyTestArray_Marshaller = null;
    
    /// <summary>
    /// MyTestArray is a test array
    /// </summary>
    public System.Collections.Generic.IList<int> MyTestArray
    {
        get
        {
            MyTestArray_Marshaller ??= new ArrayMarshaller<int>(1, MyTestArray_NativeProperty, BlittableMarshaller<int>.ToNative, BlittableMarshaller<int>.FromNative);
            return MyTestArray_Marshaller.FromNative(IntPtr.Add(NativeObject, MyTestArray_Offset), 0);
        }
    }
    
    
    static int MyReadOnlyTestMap_Offset;
    static IntPtr MyReadOnlyTestMap_NativeProperty;
    MapReadOnlyMarshaller<FName, int> MyReadOnlyTestMap_Marshaller = null;
    
    /// <summary>
    /// MyReadOnlyTestMap is a read-only test map
    /// </summary>
    public System.Collections.Generic.IReadOnlyDictionary<FName, int> MyReadOnlyTestMap
    {
        get
        {
            MyReadOnlyTestMap_Marshaller ??= new MapReadOnlyMarshaller<FName, int>(1, MyReadOnlyTestMap_NativeProperty, BlittableMarshaller<FName>.ToNative, BlittableMarshaller<FName>.FromNative, BlittableMarshaller<int>.ToNative, BlittableMarshaller<int>.FromNative);
            return MyReadOnlyTestMap_Marshaller.FromNative(IntPtr.Add(NativeObject, MyReadOnlyTestMap_Offset), 0);
        }
    }
    
    
    static int MyReadOnlyTestArray_Offset;
    static IntPtr MyReadOnlyTestArray_NativeProperty;
    ArrayReadOnlyMarshaller<int> MyReadOnlyTestArray_Marshaller = null;
    
    /// <summary>
    /// MyReadOnlyTestArray is a read-only test array
    /// </summary>
    public System.Collections.Generic.IReadOnlyList<int> MyReadOnlyTestArray
    {
        get
        {
            MyReadOnlyTestArray_Marshaller ??= new ArrayReadOnlyMarshaller<int>(1, MyReadOnlyTestArray_NativeProperty, BlittableMarshaller<int>.ToNative, BlittableMarshaller<int>.FromNative);
            return MyReadOnlyTestArray_Marshaller.FromNative(IntPtr.Add(NativeObject, MyReadOnlyTestArray_Offset), 0);
        }
    }
    
    // MyScriptMethod
    static IntPtr MyScriptMethod_NativeFunction;
    static int MyScriptMethod_ParamsSize;
    static int MyScriptMethod_MyInteger_Offset;
    static int MyScriptMethod_ReturnValue_Offset;
    
    [UFunction, GeneratedType("MyScriptMethod")]
    public bool MyScriptMethod(int myInteger)
    {
        unsafe
        {
            byte* ParamsBufferAllocation = stackalloc byte[MyScriptMethod_ParamsSize];
            nint ParamsBuffer = (nint) ParamsBufferAllocation;
            UStructExporter.CallInitializeStruct(MyScriptMethod_NativeFunction, ParamsBuffer);
            BlittableMarshaller<int>.ToNative(IntPtr.Add(ParamsBuffer, MyScriptMethod_MyInteger_Offset), 0, myInteger);
            
            UObjectExporter.CallInvokeNativeFunction(NativeObject, MyScriptMethod_NativeFunction, ParamsBuffer);
            
            bool returnValue;
            returnValue = BoolMarshaller.FromNative(IntPtr.Add(ParamsBuffer, MyScriptMethod_ReturnValue_Offset), 0);
            
            return returnValue;
        }
    }
    
    // MyNonScriptMethod
    static IntPtr MyNonScriptMethod_NativeFunction;
    static int MyNonScriptMethod_ParamsSize;
    static int MyNonScriptMethod_MyInteger_Offset;
    static int MyNonScriptMethod_ReturnValue_Offset;
    
    [UFunction, GeneratedType("MyNonScriptMethod")]
    public bool MyNonScriptMethod(int myInteger)
    {
        unsafe
        {
            byte* ParamsBufferAllocation = stackalloc byte[MyNonScriptMethod_ParamsSize];
            nint ParamsBuffer = (nint) ParamsBufferAllocation;
            UStructExporter.CallInitializeStruct(MyNonScriptMethod_NativeFunction, ParamsBuffer);
            BlittableMarshaller<int>.ToNative(IntPtr.Add(ParamsBuffer, MyNonScriptMethod_MyInteger_Offset), 0, myInteger);
            
            UObjectExporter.CallInvokeNativeFunction(NativeObject, MyNonScriptMethod_NativeFunction, ParamsBuffer);
            
            bool returnValue;
            returnValue = BoolMarshaller.FromNative(IntPtr.Add(ParamsBuffer, MyNonScriptMethod_ReturnValue_Offset), 0);
            
            return returnValue;
        }
    }
    
    // MyTestFunction
    static IntPtr MyTestFunction_NativeFunction;
    static int MyTestFunction_ParamsSize;
    static int MyTestFunction_TestMap_Offset;
    static IntPtr MyTestFunction_TestMap_NativeProperty;
    MapCopyMarshaller<FName, int> MyTestFunction_TestMap_Marshaller = null;
    
    [UFunction, GeneratedType("MyTestFunction")]
    public void MyTestFunction(System.Collections.Generic.IDictionary<FName, int> testMap)
    {
        unsafe
        {
            byte* ParamsBufferAllocation = stackalloc byte[MyTestFunction_ParamsSize];
            nint ParamsBuffer = (nint) ParamsBufferAllocation;
            UStructExporter.CallInitializeStruct(MyTestFunction_NativeFunction, ParamsBuffer);
            MyTestFunction_TestMap_Marshaller ??= new MapCopyMarshaller<FName, int>(MyTestFunction_TestMap_NativeProperty, BlittableMarshaller<FName>.ToNative, BlittableMarshaller<FName>.FromNative, BlittableMarshaller<int>.ToNative, BlittableMarshaller<int>.FromNative);
            IntPtr MyTestFunction_TestMap_NativeProperty_NativeBuffer = IntPtr.Add(ParamsBuffer, MyTestFunction_TestMap_Offset);
            MyTestFunction_TestMap_Marshaller.ToNative(MyTestFunction_TestMap_NativeProperty_NativeBuffer, 0, testMap);
            
            UObjectExporter.CallInvokeNativeFunction(NativeObject, MyTestFunction_NativeFunction, ParamsBuffer);
            
        }
    }
    
    
}