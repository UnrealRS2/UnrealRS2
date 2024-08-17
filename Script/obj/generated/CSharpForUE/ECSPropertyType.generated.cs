using UnrealSharp.Attributes;
using UnrealSharp.Interop;

namespace UnrealSharp.CSharpForUE;

[UEnum, GeneratedType("ECSPropertyType", "UnrealSharp.CSharpForUE.ECSPropertyType")]
public enum ECSPropertyType : byte
{
    Unknown = 0,
    Bool = 1,
    Int8 = 2,
    Int16 = 3,
    Int = 4,
    Int64 = 5,
    Byte = 6,
    UInt16 = 7,
    UInt32 = 8,
    UInt64 = 9,
    Double = 10,
    Float = 11,
    Enum = 12,
    Interface = 13,
    Struct = 14,
    Class = 15,
    Object = 16,
    ObjectPtr = 17,
    DefaultComponent = 18,
    LazyObject = 19,
    WeakObject = 20,
    SoftClass = 21,
    SoftObject = 22,
    Delegate = 23,
    MulticastInlineDelegate = 24,
    MulticastSparseDelegate = 25,
    Array = 26,
    Map = 27,
    Set = 28,
    String = 29,
    Name = 30,
    Text = 31,
    GameplayTag = 32,
    GameplayTagContainer = 33,
    InternalNativeFixedSizeArray = 34,
    InternalManagedFixedSizeArray = 35,
}