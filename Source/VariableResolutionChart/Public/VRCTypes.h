#pragma once

#include "CoreMinimal.h"
#include "VRCTypes.generated.h"

UENUM(BlueprintType)
enum class EVRCCellType : uint8
{
    None        UMETA(DisplayName = "None"),
    Float       UMETA(DisplayName = "Float"),
    Int32       UMETA(DisplayName = "Int32"),
    Bool        UMETA(DisplayName = "Bool"),
    Vector2     UMETA(DisplayName = "Vector2"),
    Vector3     UMETA(DisplayName = "Vector3"),
    LinearColor UMETA(DisplayName = "Linear Color"),
    String      UMETA(DisplayName = "String"),
    Bytes       UMETA(DisplayName = "Raw Bytes")
};

/**
 * Inline cell payload. Deliberately POD-only: a grid of N cells is one flat
 * array with trivial construction / destruction / copy.
 *
 * String and Bytes payloads do NOT live here; they are stored out-of-line
 * in UVRCGrid::ExternalPayload and only allocated when actually used.
 */
USTRUCT(BlueprintType)
struct VARIABLERESOLUTIONCHART_API FVRCCell
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "VRC") EVRCCellType Type = EVRCCellType::None;
    UPROPERTY(BlueprintReadWrite, Category = "VRC") float        FloatValue = 0.0f;
    UPROPERTY(BlueprintReadWrite, Category = "VRC") int32        IntValue = 0;
    UPROPERTY(BlueprintReadWrite, Category = "VRC") bool         BoolValue = false;
    UPROPERTY(BlueprintReadWrite, Category = "VRC") FVector2D    Vector2Value = FVector2D::ZeroVector;
    UPROPERTY(BlueprintReadWrite, Category = "VRC") FVector      Vector3Value = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite, Category = "VRC") FLinearColor ColorValue = FLinearColor::Black;
};

FORCEINLINE FArchive& operator<<(FArchive& Ar, FVRCCell& Cell)
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

/** Out-of-line storage for String / Bytes cells. */
USTRUCT()
struct VARIABLERESOLUTIONCHART_API FVRCPayload
{
    GENERATED_BODY()

    UPROPERTY() FString        StringValue;
    UPROPERTY() TArray<uint8>  ByteValue;
};

FORCEINLINE FArchive& operator<<(FArchive& Ar, FVRCPayload& Payload)
{
    Ar << Payload.StringValue;
    Ar << Payload.ByteValue;
    return Ar;
}