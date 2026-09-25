#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VRCTypes.h"
#include "VRCGrid.generated.h"

/**
 * Runtime-resizable 2D grid of typed cells.
 *
 * Storage is row-major: Index = Y * Width + X.
 * Resizing preserves the overlapping region by coordinate; new cells take
 * the caller-supplied default type.
 *
 * Typical construction:
 *
 *     UVRCGrid* Grid = UVRCGrid::Create(64, 64, EVRCCellType::Float)
 *         ->SetRegionType(FIntRect(0,  0, 32, 32), EVRCCellType::LinearColor)
 *         ->SetRegionType(FIntRect(32, 32, 64, 64), EVRCCellType::Vector3);
 */
UCLASS(BlueprintType)
class VARIABLERESOLUTIONCHART_API UVRCGrid : public UObject
{
    GENERATED_BODY()

public:
    // =====================================================================
    // Construction
    // =====================================================================

    /**
     * Create a grid. Pass 0x0 to start empty and chain Resize() afterwards.
     * All configuration methods return `this` so setup reads as one expression.
     */
    UFUNCTION(BlueprintCallable, Category = "VRC|Grid", meta = (DisplayName = "Create VRC Grid"))
    static UVRCGrid* Create(int32 InitialWidth = 0,
        int32 InitialHeight = 0,
        EVRCCellType InitialType = EVRCCellType::Float);

    // =====================================================================
    // Size
    // =====================================================================

    UFUNCTION(BlueprintPure, Category = "VRC|Grid") int32 GetWidth()  const { return Width; }
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") int32 GetHeight() const { return Height; }

    UFUNCTION(BlueprintPure, Category = "VRC|Grid")
    bool IsValidCoordinate(int32 X, int32 Y) const;

    /** Resize, preserving overlap by coordinate. New cells take DefaultType. Chainable. */
    UFUNCTION(BlueprintCallable, Category = "VRC|Grid")
    UVRCGrid* Resize(int32 NewWidth, int32 NewHeight,
        EVRCCellType DefaultType = EVRCCellType::Float);

    /** Empty the grid. Chainable. */
    UFUNCTION(BlueprintCallable, Category = "VRC|Grid")
    UVRCGrid* Clear();

    // =====================================================================
    // Bulk type configuration (chainable)
    // =====================================================================

    /** Set every cell to Type. Chainable. */
    UFUNCTION(BlueprintCallable, Category = "VRC|Grid")
    UVRCGrid* SetAllType(EVRCCellType Type);

    /**
     * Set every cell in the half-open rectangle [Region.Min, Region.Max)
     * to Type. Region is clamped to the grid bounds. Chainable.
     */
    UFUNCTION(BlueprintCallable, Category = "VRC|Grid")
    UVRCGrid* SetRegionType(const FIntRect& Region, EVRCCellType Type);

    // =====================================================================
    // Typed accessors - each setter stamps the cell's Type automatically
    // =====================================================================

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool  SetFloat(int32 X, int32 Y, float Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") float GetFloat(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool  SetInt(int32 X, int32 Y, int32 Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") int32 GetInt(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool  SetBool(int32 X, int32 Y, bool Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") bool  GetBool(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool      SetVector2(int32 X, int32 Y, FVector2D Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") FVector2D GetVector2(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool    SetVector3(int32 X, int32 Y, FVector Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") FVector GetVector3(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool         SetColor(int32 X, int32 Y, FLinearColor Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") FLinearColor GetColor(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool    SetString(int32 X, int32 Y, const FString& Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") FString GetString(int32 X, int32 Y) const;

    UFUNCTION(BlueprintCallable, Category = "VRC|Grid") bool          SetBytes(int32 X, int32 Y, const TArray<uint8>& Value);
    UFUNCTION(BlueprintPure, Category = "VRC|Grid") TArray<uint8> GetBytes(int32 X, int32 Y) const;

    // =====================================================================
    // Type manipulation without writing a value
    // =====================================================================

    /** Change the cell's type, clearing whatever value it held. */
    UFUNCTION(BlueprintCallable, Category = "VRC|Grid")
    bool SetCellType(int32 X, int32 Y, EVRCCellType NewType);

    UFUNCTION(BlueprintPure, Category = "VRC|Grid")
    EVRCCellType GetCellType(int32 X, int32 Y) const;

    // =====================================================================
    // Low-level access (C++ only)
    // =====================================================================

    const TArray<FVRCCell>& GetRawCells() const { return Cells; }

    // =====================================================================
    // Serialization
    // =====================================================================

    /** Uses the UE archive system; SaveGame and disk persistence pick this up automatically. */
    virtual void Serialize(FArchive& Ar) override;

    /** Packs the full grid into a byte array for custom storage or network transport. */
    UFUNCTION(BlueprintCallable, Category = "VRC|Serialization")
    TArray<uint8> SerializeToBytes() const;

    /** Restores the grid from a byte array. On failure the current grid is left untouched. */
    UFUNCTION(BlueprintCallable, Category = "VRC|Serialization")
    bool DeserializeFromBytes(const TArray<uint8>& InBytes);

private:
    UPROPERTY() int32 Width = 0;
    UPROPERTY() int32 Height = 0;
    UPROPERTY() TArray<FVRCCell> Cells;

    /** Out-of-line storage for String / Bytes cells, keyed by linear index. */
    UPROPERTY() TMap<int32, FVRCPayload> ExternalPayload;

    #if WITH_DEV_AUTOMATION_TESTS
    /** Test-only accessor. Lets automation tests assert no orphan payload entries. */
    const TMap<int32, FVRCPayload>& GetExternalPayloadForTesting() const { return ExternalPayload; }
    #endif

    FORCEINLINE int32 ToIndex(int32 X, int32 Y) const { return Y * Width + X; }

    /** Overwrite a cell with a fresh default of the given type; drop payload. */
    void ResetCellAt(int32 Index, EVRCCellType Type);

    FVector2D    ReadVector2(int32 Index) const;
    FVector      ReadVector3(int32 Index) const;
    FLinearColor ReadColor(int32 Index) const;
};