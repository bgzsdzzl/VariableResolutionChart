// Source/VariableResolutionChart/Private/Tests/VRCGridTests.cpp
#include "VRCGrid.h"
#include "VRCTypes.h"

#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "Serialization/MemoryWriter.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    constexpr auto Flags =
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

// ===========================================================================
// Construction
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Create_Empty,
    "VariableResolutionChart.Grid.Create.Empty",
    Flags)

    bool FVRCGrid_Create_Empty::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create());
    TestNotNull(TEXT("Grid created"), Grid.Get());
    TestEqual(TEXT("Width"), Grid->GetWidth(), 0);
    TestEqual(TEXT("Height"), Grid->GetHeight(), 0);
    TestEqual(TEXT("Cell count"), Grid->GetRawCells().Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Create_WithSize,
    "VariableResolutionChart.Grid.Create.WithSize",
    Flags)

    bool FVRCGrid_Create_WithSize::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(4, 4, EVRCCellType::Float));

    TestEqual(TEXT("Width"), Grid->GetWidth(), 4);
    TestEqual(TEXT("Height"), Grid->GetHeight(), 4);
    TestTrue(TEXT("(0,0) type"), Grid->GetCellType(0, 0) == EVRCCellType::Float);
    TestTrue(TEXT("(3,3) type"), Grid->GetCellType(3, 3) == EVRCCellType::Float);
    TestFalse(TEXT("(4,0) out of bounds"), Grid->IsValidCoordinate(4, 0));
    TestFalse(TEXT("(0,4) out of bounds"), Grid->IsValidCoordinate(0, 4));
    return true;
}

// ===========================================================================
// Resize
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Resize_GrowPreservesOverlap,
    "VariableResolutionChart.Grid.Resize.GrowPreservesOverlap",
    Flags)

    bool FVRCGrid_Resize_GrowPreservesOverlap::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(4, 4, EVRCCellType::Float));
    Grid->SetFloat(0, 0, 1.0f);
    Grid->SetFloat(3, 3, 9.0f);

    Grid->Resize(6, 6, EVRCCellType::Int32);

    TestEqual(TEXT("Width"), Grid->GetWidth(), 6);
    TestEqual(TEXT("Height"), Grid->GetHeight(), 6);

    // Overlap preserved by coordinate.
    TestEqual(TEXT("(0,0) value kept"), Grid->GetFloat(0, 0), 1.0f);
    TestEqual(TEXT("(3,3) value kept"), Grid->GetFloat(3, 3), 9.0f);
    TestTrue(TEXT("(0,0) type kept"), Grid->GetCellType(0, 0) == EVRCCellType::Float);

    // New cells take the default type.
    TestTrue(TEXT("(4,4) new cell default type"), Grid->GetCellType(4, 4) == EVRCCellType::Int32);
    TestTrue(TEXT("(5,5) new cell default type"), Grid->GetCellType(5, 5) == EVRCCellType::Int32);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Resize_SameSizeIsNoOp,
    "VariableResolutionChart.Grid.Resize.SameSizeIsNoOp",
    Flags)

    bool FVRCGrid_Resize_SameSizeIsNoOp::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(3, 3, EVRCCellType::Float));
    Grid->SetString(1, 1, TEXT("hello"));

    UVRCGrid* Returned = Grid->Resize(3, 3, EVRCCellType::Int32);

    TestEqual(TEXT("Resize returns this"), Returned, Grid.Get());
    TestEqual(TEXT("Width"), Grid->GetWidth(), 3);
    TestEqual(TEXT("Height"), Grid->GetHeight(), 3);
    TestEqual(TEXT("Value kept"), Grid->GetString(1, 1), FString(TEXT("hello")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Resize_ToZeroClears,
    "VariableResolutionChart.Grid.Resize.ToZeroClears",
    Flags)

    bool FVRCGrid_Resize_ToZeroClears::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(4, 4, EVRCCellType::Float));
    Grid->SetString(0, 0, TEXT("x"));

    Grid->Resize(0, 4, EVRCCellType::Float);

    TestEqual(TEXT("Width"), Grid->GetWidth(), 0);
    TestEqual(TEXT("Height"), Grid->GetHeight(), 0);
    TestEqual(TEXT("Cell count"), Grid->GetRawCells().Num(), 0);
    TestEqual(TEXT("Payload count"), Grid->GetExternalPayloadForTesting().Num(), 0);
    return true;
}

// ===========================================================================
// Payload reindexing across Resize
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Resize_PayloadReindexed,
    "VariableResolutionChart.Grid.Resize.PayloadReindexed",
    Flags)

    bool FVRCGrid_Resize_PayloadReindexed::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(4, 4, EVRCCellType::Float));
    Grid->SetString(1, 1, TEXT("a")); // old index 1*4+1 = 5
    Grid->SetString(2, 3, TEXT("b")); // old index 3*4+2 = 14

    Grid->Resize(6, 6, EVRCCellType::Float);

    // New indices: 1*6+1 = 7, 3*6+2 = 20
    const TMap<int32, FVRCPayload>& Payload = Grid->GetExternalPayloadForTesting();
    TestEqual(TEXT("Payload count"), Payload.Num(), 2);
    TestNotNull(TEXT("Index 7 present"), Payload.Find(7));
    TestNotNull(TEXT("Index 20 present"), Payload.Find(20));
    TestNull(TEXT("Old index 5 absent"), Payload.Find(5));
    TestNull(TEXT("Old index 14 absent"), Payload.Find(14));

    // Public API still reads back correctly.
    TestEqual(TEXT("String at (1,1)"), Grid->GetString(1, 1), FString(TEXT("a")));
    TestEqual(TEXT("String at (2,3)"), Grid->GetString(2, 3), FString(TEXT("b")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Resize_ShrinkDropsOutOfBoundsPayload,
    "VariableResolutionChart.Grid.Resize.ShrinkDropsOutOfBoundsPayload",
    Flags)

    bool FVRCGrid_Resize_ShrinkDropsOutOfBoundsPayload::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(4, 4, EVRCCellType::Float));
    Grid->SetString(1, 1, TEXT("keep"));
    Grid->SetString(3, 3, TEXT("drop"));

    TestEqual(TEXT("Payload before"), Grid->GetExternalPayloadForTesting().Num(), 2);

    Grid->Resize(2, 2, EVRCCellType::Float);

    // New index for (1,1): 1*2+1 = 3
    const TMap<int32, FVRCPayload>& Payload = Grid->GetExternalPayloadForTesting();
    TestEqual(TEXT("Payload after"), Payload.Num(), 1);
    TestNotNull(TEXT("In-bounds payload at index 3"), Payload.Find(3));
    TestEqual(TEXT("Value kept"), Grid->GetString(1, 1), FString(TEXT("keep")));
    return true;
}

// ===========================================================================
// SetCellType / SetRegionType
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_SetCellType_ClearsValueAndPayload,
    "VariableResolutionChart.Grid.SetCellType.ClearsValueAndPayload",
    Flags)

    bool FVRCGrid_SetCellType_ClearsValueAndPayload::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(2, 2, EVRCCellType::Float));
    Grid->SetFloat(0, 0, 5.0f);
    Grid->SetString(0, 0, TEXT("world"));

    TestTrue(TEXT("Type is String"), Grid->GetCellType(0, 0) == EVRCCellType::String);
    TestEqual(TEXT("String readable"), Grid->GetString(0, 0), FString(TEXT("world")));

    Grid->SetCellType(0, 0, EVRCCellType::Float);

    TestTrue(TEXT("Type is Float"), Grid->GetCellType(0, 0) == EVRCCellType::Float);
    TestEqual(TEXT("Float reset"), Grid->GetFloat(0, 0), 0.0f);
    TestTrue(TEXT("String payload cleared"), Grid->GetString(0, 0).IsEmpty());
    TestNull(TEXT("No orphan payload"), Grid->GetExternalPayloadForTesting().Find(0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_SetCellType_SameTypePreservesValue,
    "VariableResolutionChart.Grid.SetCellType.SameTypePreservesValue",
    Flags)

    bool FVRCGrid_SetCellType_SameTypePreservesValue::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(2, 2, EVRCCellType::Float));
    Grid->SetFloat(0, 0, 5.0f);

    Grid->SetCellType(0, 0, EVRCCellType::Float);

    // SetCellType to the same type must be a no-op, not a reset.
    TestEqual(TEXT("Value preserved"), Grid->GetFloat(0, 0), 5.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_SetRegionType_ClampsOutOfBounds,
    "VariableResolutionChart.Grid.SetRegionType.ClampsOutOfBounds",
    Flags)

    bool FVRCGrid_SetRegionType_ClampsOutOfBounds::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(4, 4, EVRCCellType::Float));

    Grid->SetRegionType(FIntRect(-10, -10, 100, 100), EVRCCellType::Bool);

    TestTrue(TEXT("(0,0)"), Grid->GetCellType(0, 0) == EVRCCellType::Bool);
    TestTrue(TEXT("(3,3)"), Grid->GetCellType(3, 3) == EVRCCellType::Bool);
    TestEqual(TEXT("Width unchanged"), Grid->GetWidth(), 4);
    TestEqual(TEXT("Height unchanged"), Grid->GetHeight(), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_SetRegionType_ClearsPayloadInRegionOnly,
    "VariableResolutionChart.Grid.SetRegionType.ClearsPayloadInRegionOnly",
    Flags)

    bool FVRCGrid_SetRegionType_ClearsPayloadInRegionOnly::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(4, 4, EVRCCellType::Float));
    Grid->SetString(1, 1, TEXT("gone"));
    Grid->SetString(2, 2, TEXT("kept"));

    Grid->SetRegionType(FIntRect(0, 0, 2, 2), EVRCCellType::Float);

    TestTrue(TEXT("(1,1) cleared"), Grid->GetString(1, 1).IsEmpty());
    TestEqual(TEXT("(2,2) kept"), Grid->GetString(2, 2), FString(TEXT("kept")));
    return true;
}

// ===========================================================================
// Out-of-bounds and wrong-type behavior
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Setters_OutOfBoundsReturnFalse,
    "VariableResolutionChart.Grid.Setters.OutOfBoundsReturnFalse",
    Flags)

    bool FVRCGrid_Setters_OutOfBoundsReturnFalse::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(2, 2, EVRCCellType::Float));

    TestFalse(TEXT("SetFloat(-1,0)"), Grid->SetFloat(-1, 0, 1.0f));
    TestFalse(TEXT("SetFloat(2,0)"), Grid->SetFloat(2, 0, 1.0f));
    TestFalse(TEXT("SetInt(0,-1)"), Grid->SetInt(0, -1, 1));
    TestFalse(TEXT("SetBool(2,2)"), Grid->SetBool(2, 2, true));
    TestFalse(TEXT("SetString(2,2)"), Grid->SetString(2, 2, TEXT("x")));
    TestFalse(TEXT("SetBytes(-1,-1)"), Grid->SetBytes(-1, -1, TArray<uint8>()));
    TestFalse(TEXT("SetCellType(2,2)"), Grid->SetCellType(2, 2, EVRCCellType::Bool));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Getters_WrongTypeReturnsDefault,
    "VariableResolutionChart.Grid.Getters.WrongTypeReturnsDefault",
    Flags)

    bool FVRCGrid_Getters_WrongTypeReturnsDefault::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(2, 2, EVRCCellType::Float));
    Grid->SetInt(0, 0, 42);

    TestEqual(TEXT("GetFloat on Int cell"), Grid->GetFloat(0, 0), 0.0f);
    TestEqual(TEXT("GetBool on Int cell"), Grid->GetBool(0, 0), false);
    TestTrue(TEXT("GetString on Int cell is empty"), Grid->GetString(0, 0).IsEmpty());
    TestEqual(TEXT("GetBytes on Int cell is empty"), Grid->GetBytes(0, 0).Num(), 0);

    // Out-of-bounds also returns defaults.
    TestEqual(TEXT("GetFloat OOB"), Grid->GetFloat(-1, -1), 0.0f);
    TestEqual(TEXT("GetInt OOB"), Grid->GetInt(-1, -1), 0);
    TestTrue(TEXT("GetCellType OOB is None"),
        Grid->GetCellType(-1, -1) == EVRCCellType::None);
    return true;
}

// ===========================================================================
// Serialization
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Serialize_RoundTrip,
    "VariableResolutionChart.Grid.Serialize.RoundTrip",
    Flags)

    bool FVRCGrid_Serialize_RoundTrip::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Src(UVRCGrid::Create(4, 4, EVRCCellType::Float));
    Src->SetFloat(0, 0, 3.14f);
    Src->SetInt(1, 0, 42);
    Src->SetBool(2, 0, true);
    Src->SetVector2(3, 0, FVector2D(1.0, 2.0));
    Src->SetVector3(0, 1, FVector(1.0, 2.0, 3.0));
    Src->SetColor(1, 1, FLinearColor(0.25f, 0.5f, 0.75f, 1.0f));
    Src->SetString(2, 1, TEXT("round-trip"));
    Src->SetBytes(3, 1, TArray<uint8>{10, 20, 30});

    const TArray<uint8> Bytes = Src->SerializeToBytes();
    TestTrue(TEXT("Serialized non-empty"), Bytes.Num() > 0);

    TStrongObjectPtr<UVRCGrid> Dst(NewObject<UVRCGrid>());
    TestTrue(TEXT("Deserialize succeeded"), Dst->DeserializeFromBytes(Bytes));

    TestEqual(TEXT("Width"), Dst->GetWidth(), 4);
    TestEqual(TEXT("Height"), Dst->GetHeight(), 4);

    TestEqual(TEXT("Float"), Dst->GetFloat(0, 0), 3.14f);
    TestEqual(TEXT("Int"), Dst->GetInt(1, 0), 42);
    TestEqual(TEXT("Bool"), Dst->GetBool(2, 0), true);
    TestEqual(TEXT("Vector2"), Dst->GetVector2(3, 0), FVector2D(1.0, 2.0));
    TestEqual(TEXT("Vector3"), Dst->GetVector3(0, 1), FVector(1.0, 2.0, 3.0));
    TestEqual(TEXT("Color"), Dst->GetColor(1, 1), FLinearColor(0.25f, 0.5f, 0.75f, 1.0f));
    TestEqual(TEXT("String"), Dst->GetString(2, 1), FString(TEXT("round-trip")));

    const TArray<uint8> DstBytes = Dst->GetBytes(3, 1);
    TestEqual(TEXT("Bytes length"), DstBytes.Num(), 3);
    TestEqual(TEXT("Bytes[0]"), (int32)DstBytes[0], 10);
    TestEqual(TEXT("Bytes[1]"), (int32)DstBytes[1], 20);
    TestEqual(TEXT("Bytes[2]"), (int32)DstBytes[2], 30);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Deserialize_RejectsInvalid,
    "VariableResolutionChart.Grid.Deserialize.RejectsInvalid",
    Flags)

    bool FVRCGrid_Deserialize_RejectsInvalid::RunTest(const FString&)
{
    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(2, 2, EVRCCellType::Float));
    Grid->SetFloat(0, 0, 1.0f);

    TestFalse(TEXT("Empty input rejected"), Grid->DeserializeFromBytes(TArray<uint8>()));

    // Grid untouched after failed load.
    TestEqual(TEXT("Width still 2"), Grid->GetWidth(), 2);
    TestEqual(TEXT("Value still 1.0"), Grid->GetFloat(0, 0), 1.0f);

    // Garbage input.
    TArray<uint8> Garbage;
    Garbage.Init(0xAB, 16);
    TestFalse(TEXT("Garbage rejected"), Grid->DeserializeFromBytes(Garbage));
    TestEqual(TEXT("Width still 2 after garbage"), Grid->GetWidth(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVRCGrid_Deserialize_RejectsFutureVersion,
    "VariableResolutionChart.Grid.Deserialize.RejectsFutureVersion",
    Flags)

    bool FVRCGrid_Deserialize_RejectsFutureVersion::RunTest(const FString&)
{
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes, /*bIsPersistent=*/ true);
    int32 FutureVersion = 999;
    Writer << FutureVersion;

    TStrongObjectPtr<UVRCGrid> Grid(UVRCGrid::Create(1, 1, EVRCCellType::Float));
    TestFalse(TEXT("Future version rejected"), Grid->DeserializeFromBytes(Bytes));
    TestEqual(TEXT("Width untouched"), Grid->GetWidth(), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS