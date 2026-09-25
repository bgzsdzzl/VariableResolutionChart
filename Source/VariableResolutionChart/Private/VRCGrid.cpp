#include "VRCGrid.h"
#include "VRCTypes.h"
#include "Serialization/Archive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

UVRCGrid* UVRCGrid::Create(int32 InitialWidth, int32 InitialHeight, EVRCCellType InitialType)
{
    UVRCGrid* Grid = NewObject<UVRCGrid>();
    if (InitialWidth > 0 && InitialHeight > 0)
    {
        Grid->Resize(InitialWidth, InitialHeight, InitialType);
    }
    return Grid;
}

bool UVRCGrid::IsValidCoordinate(int32 X, int32 Y) const
{
    return X >= 0 && X < Width && Y >= 0 && Y < Height;
}

// ---------------------------------------------------------------------------
// Size
// ---------------------------------------------------------------------------

UVRCGrid* UVRCGrid::Clear()
{
    Width = 0;
    Height = 0;
    Cells.Reset();
    ExternalPayload.Reset();
    return this;
}

UVRCGrid* UVRCGrid::Resize(int32 NewWidth, int32 NewHeight, EVRCCellType DefaultType)
{
    NewWidth = FMath::Max(0, NewWidth);
    NewHeight = FMath::Max(0, NewHeight);

    if (NewWidth == Width && NewHeight == Height)
    {
        return this;
    }
    if (NewWidth == 0 || NewHeight == 0)
    {
        return Clear();
    }

    TArray<FVRCCell> NewCells;
    NewCells.SetNumZeroed(NewWidth * NewHeight);

    const int32 CopyW = FMath::Min(Width, NewWidth);
    const int32 CopyH = FMath::Min(Height, NewHeight);

    FVRCCell DefaultCell;
    DefaultCell.Type = DefaultType;

    for (int32 Y = 0; Y < NewHeight; ++Y)
    {
        const int32 DstRow = Y * NewWidth;
        const int32 SrcRow = Y * Width;

        for (int32 X = 0; X < NewWidth; ++X)
        {
            const bool bOverlap = (X < CopyW) && (Y < CopyH);
            NewCells[DstRow + X] = bOverlap ? Cells[SrcRow + X] : DefaultCell;
        }
    }

    // Re-key external payloads that survived the resize.
    TMap<int32, FVRCPayload> NewPayload;
    for (auto& Pair : ExternalPayload)
    {
        const int32 OldIndex = Pair.Key;
        const int32 OldY = OldIndex / Width;
        const int32 OldX = OldIndex % Width;

        if (OldX < NewWidth && OldY < NewHeight)
        {
            const int32 NewIndex = OldY * NewWidth + OldX;
            NewPayload.Add(NewIndex, MoveTemp(Pair.Value));
        }
    }
    ExternalPayload = MoveTemp(NewPayload);

    Cells = MoveTemp(NewCells);
    Width = NewWidth;
    Height = NewHeight;
    return this;
}

// ---------------------------------------------------------------------------
// Bulk type configuration
// ---------------------------------------------------------------------------

UVRCGrid* UVRCGrid::SetAllType(EVRCCellType Type)
{
    ExternalPayload.Reset();

    FVRCCell DefaultCell;
    DefaultCell.Type = Type;

    for (FVRCCell& Cell : Cells)
    {
        Cell = DefaultCell;
    }
    return this;
}

UVRCGrid* UVRCGrid::SetRegionType(const FIntRect& Region, EVRCCellType Type)
{
    const int32 X0 = FMath::Clamp(Region.Min.X, 0, Width);
    const int32 Y0 = FMath::Clamp(Region.Min.Y, 0, Height);
    const int32 X1 = FMath::Clamp(Region.Max.X, 0, Width);
    const int32 Y1 = FMath::Clamp(Region.Max.Y, 0, Height);

    FVRCCell DefaultCell;
    DefaultCell.Type = Type;

    for (int32 Y = Y0; Y < Y1; ++Y)
    {
        const int32 Row = Y * Width;
        for (int32 X = X0; X < X1; ++X)
        {
            const int32 Index = Row + X;
            Cells[Index] = DefaultCell;
            ExternalPayload.Remove(Index);
        }
    }
    return this;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void UVRCGrid::ResetCellAt(int32 Index, EVRCCellType Type)
{
    Cells[Index] = FVRCCell{};
    Cells[Index].Type = Type;
    ExternalPayload.Remove(Index);
}

FVector2D UVRCGrid::ReadVector2(int32 Index) const
{
    const FVRCCell& C = Cells[Index];
    return C.Type == EVRCCellType::Vector2 ? C.Vector2Value : FVector2D::ZeroVector;
}

FVector UVRCGrid::ReadVector3(int32 Index) const
{
    const FVRCCell& C = Cells[Index];
    return C.Type == EVRCCellType::Vector3 ? C.Vector3Value : FVector::ZeroVector;
}

FLinearColor UVRCGrid::ReadColor(int32 Index) const
{
    const FVRCCell& C = Cells[Index];
    return C.Type == EVRCCellType::LinearColor ? C.ColorValue : FLinearColor::Black;
}

// ---------------------------------------------------------------------------
// Typed accessors
// ---------------------------------------------------------------------------

#define VRC_INLINE_SETTER(Name, EnumName, Field, Type)                          \
bool UVRCGrid::Set##Name(int32 X, int32 Y, Type Value)                          \
{                                                                                \
    if (!IsValidCoordinate(X, Y)) return false;                                  \
    const int32 Index = ToIndex(X, Y);                                           \
    ResetCellAt(Index, EVRCCellType::EnumName);                                  \
    Cells[Index].Field = Value;                                                  \
    return true;                                                                 \
}

VRC_INLINE_SETTER(Float, Float, FloatValue, float)
VRC_INLINE_SETTER(Int, Int32, IntValue, int32)
VRC_INLINE_SETTER(Bool, Bool, BoolValue, bool)
VRC_INLINE_SETTER(Vector2, Vector2, Vector2Value, FVector2D)
VRC_INLINE_SETTER(Vector3, Vector3, Vector3Value, FVector)
VRC_INLINE_SETTER(Color, LinearColor, ColorValue, FLinearColor)

#undef VRC_INLINE_SETTER

float UVRCGrid::GetFloat(int32 X, int32 Y) const
{
    if (!IsValidCoordinate(X, Y)) return 0.0f;
    const FVRCCell& C = Cells[ToIndex(X, Y)];
    return C.Type == EVRCCellType::Float ? C.FloatValue : 0.0f;
}

int32 UVRCGrid::GetInt(int32 X, int32 Y) const
{
    if (!IsValidCoordinate(X, Y)) return 0;
    const FVRCCell& C = Cells[ToIndex(X, Y)];
    return C.Type == EVRCCellType::Int32 ? C.IntValue : 0;
}

bool UVRCGrid::GetBool(int32 X, int32 Y) const
{
    if (!IsValidCoordinate(X, Y)) return false;
    const FVRCCell& C = Cells[ToIndex(X, Y)];
    return C.Type == EVRCCellType::Bool ? C.BoolValue : false;
}

FVector2D UVRCGrid::GetVector2(int32 X, int32 Y) const
{
    return IsValidCoordinate(X, Y) ? ReadVector2(ToIndex(X, Y)) : FVector2D::ZeroVector;
}

FVector UVRCGrid::GetVector3(int32 X, int32 Y) const
{
    return IsValidCoordinate(X, Y) ? ReadVector3(ToIndex(X, Y)) : FVector::ZeroVector;
}

FLinearColor UVRCGrid::GetColor(int32 X, int32 Y) const
{
    return IsValidCoordinate(X, Y) ? ReadColor(ToIndex(X, Y)) : FLinearColor::Black;
}

// --- String / Bytes: out-of-line payload -----------------------------------

bool UVRCGrid::SetString(int32 X, int32 Y, const FString& Value)
{
    if (!IsValidCoordinate(X, Y)) return false;
    const int32 Index = ToIndex(X, Y);
    ResetCellAt(Index, EVRCCellType::String);
    ExternalPayload.FindOrAdd(Index).StringValue = Value;
    return true;
}

FString UVRCGrid::GetString(int32 X, int32 Y) const
{
    if (!IsValidCoordinate(X, Y)) return FString();
    const int32 Index = ToIndex(X, Y);
    if (Cells[Index].Type != EVRCCellType::String) return FString();
    const FVRCPayload* P = ExternalPayload.Find(Index);
    return P ? P->StringValue : FString();
}

bool UVRCGrid::SetBytes(int32 X, int32 Y, const TArray<uint8>& Value)
{
    if (!IsValidCoordinate(X, Y)) return false;
    const int32 Index = ToIndex(X, Y);
    ResetCellAt(Index, EVRCCellType::Bytes);
    ExternalPayload.FindOrAdd(Index).ByteValue = Value;
    return true;
}

TArray<uint8> UVRCGrid::GetBytes(int32 X, int32 Y) const
{
    if (!IsValidCoordinate(X, Y)) return {};
    const int32 Index = ToIndex(X, Y);
    if (Cells[Index].Type != EVRCCellType::Bytes) return {};
    const FVRCPayload* P = ExternalPayload.Find(Index);
    return P ? P->ByteValue : TArray<uint8>();
}

// ---------------------------------------------------------------------------
// Type manipulation
// ---------------------------------------------------------------------------

bool UVRCGrid::SetCellType(int32 X, int32 Y, EVRCCellType NewType)
{
    if (!IsValidCoordinate(X, Y)) return false;
    const int32 Index = ToIndex(X, Y);
    if (Cells[Index].Type != NewType)
    {
        ResetCellAt(Index, NewType);
    }
    return true;
}

EVRCCellType UVRCGrid::GetCellType(int32 X, int32 Y) const
{
    return IsValidCoordinate(X, Y) ? Cells[ToIndex(X, Y)].Type : EVRCCellType::None;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

namespace
{
    // Bump this whenever the serialized layout changes.
    constexpr int32 GVRCGridSerializationVersion = 1;

    FORCEINLINE FArchive& SerializeCell(FArchive& Ar, FVRCCell& Cell)
    {
        // Serialize the enum as uint8 to keep the wire format stable across
        // platforms and independent of UENUM reflection.
        uint8 TypeByte = static_cast<uint8>(Cell.Type);
        Ar << TypeByte;
        if (Ar.IsLoading())
        {
            Cell.Type = static_cast<EVRCCellType>(TypeByte);
        }

        Ar << Cell.FloatValue;
        Ar << Cell.IntValue;
        Ar << Cell.BoolValue;
        Ar << Cell.Vector2Value;
        Ar << Cell.Vector3Value;
        Ar << Cell.ColorValue;

        return Ar;
    }

    FORCEINLINE FArchive& SerializePayload(FArchive& Ar, FVRCPayload& Payload)
    {
        Ar << Payload.StringValue;
        Ar << Payload.ByteValue;
        return Ar;
    }

    void SerializeGridData(
        FArchive& Ar,
        int32& InOutWidth,
        int32& InOutHeight,
        TArray<FVRCCell>& InOutCells,
        TMap<int32, FVRCPayload>& InOutPayload)
    {
        int32 Version = GVRCGridSerializationVersion;
        Ar << Version;

        if (Ar.IsLoading() && Version > GVRCGridSerializationVersion)
        {
            // Data was written by a newer build; refuse rather than guess.
            UE_LOG(LogTemp, Warning,
                TEXT("VRCGrid: stream version %d is newer than supported version %d"),
                Version, GVRCGridSerializationVersion);
            Ar.SetError();
            return;
        }

        Ar << InOutWidth;
        Ar << InOutHeight;

        // --- Inline cells ---
        int32 CellCount = Ar.IsLoading() ? 0 : InOutCells.Num();
        Ar << CellCount;

        if (Ar.IsLoading())
        {
            // Reject inconsistent sizes so a corrupted stream cannot blow up memory.
            if (CellCount < 0
                || InOutWidth < 0 || InOutHeight < 0
                || CellCount != InOutWidth * InOutHeight)
            {
                Ar.SetError();
                return;
            }
            InOutCells.SetNum(CellCount);
        }

        for (FVRCCell& Cell : InOutCells)
        {
            SerializeCell(Ar, Cell);
        }

        // --- External payload ---
        int32 PayloadCount = Ar.IsLoading() ? 0 : InOutPayload.Num();
        Ar << PayloadCount;

        if (Ar.IsLoading())
        {
            if (PayloadCount < 0 || PayloadCount > InOutCells.Num())
            {
                Ar.SetError();
                return;
            }
            InOutPayload.Empty(PayloadCount);

            for (int32 Index = 0; Index < PayloadCount; ++Index)
            {
                int32 Key = 0;
                Ar << Key;

                FVRCPayload Entry;
                SerializePayload(Ar, Entry);
                InOutPayload.Add(Key, MoveTemp(Entry));
            }
        }
        else
        {
            for (TPair<int32, FVRCPayload>& Pair : InOutPayload)
            {
                Ar << Pair.Key;
                SerializePayload(Ar, Pair.Value);
            }
        }
    }
} // namespace

void UVRCGrid::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);
    SerializeGridData(Ar, Width, Height, Cells, ExternalPayload);
}

TArray<uint8> UVRCGrid::SerializeToBytes() const
{
    TArray<uint8> OutBytes;
    FMemoryWriter Writer(OutBytes, /*bIsPersistent=*/ true);

    // Serialize is non-const, but the save path never mutates state.
    const_cast<UVRCGrid*>(this)->Serialize(Writer);

    return OutBytes;
}

bool UVRCGrid::DeserializeFromBytes(const TArray<uint8>& InBytes)
{
    if (InBytes.Num() == 0)
    {
        return false;
    }

    // Read into temporaries first; only commit on success.
    int32 TmpWidth = 0;
    int32 TmpHeight = 0;
    TArray<FVRCCell> TmpCells;
    TMap<int32, FVRCPayload> TmpPayload;

    FMemoryReader Reader(InBytes, /*bIsPersistent=*/ true);
    SerializeGridData(Reader, TmpWidth, TmpHeight, TmpCells, TmpPayload);

    if (Reader.IsError())
    {
        return false;
    }

    Width = TmpWidth;
    Height = TmpHeight;
    Cells = MoveTemp(TmpCells);
    ExternalPayload = MoveTemp(TmpPayload);

    return true;
}