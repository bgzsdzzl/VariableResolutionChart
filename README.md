# VariableResolutionChart

A runtime-resizable 2D grid for Unreal Engine with per-cell type tagging.
Pure CPU — no Virtual Texture, no Texture2DArray, no RHI dependency.

## Features

- Runtime resize with overlap preserved by coordinate, `O(W·H)`
- Per-cell type: `Float`, `Int32`, `Bool`, `Vector2`, `Vector3`, `LinearColor`, `String`, `Bytes`
- POD cell storage; `String` / `Bytes` payloads stored out-of-line in a sparse map
- Static factory + chainable bulk configuration
- Blueprint and C++ friendly

## Quick Start

### C++

```cpp
UVRCGrid* Grid = UVRCGrid::Create(64, 64, EVRCCellType::Float)
    ->SetRegionType(FIntRect(0,  0,  32, 32), EVRCCellType::LinearColor)
    ->SetRegionType(FIntRect(32, 32, 64, 64), EVRCCellType::Vector3);

Grid->SetFloat(10, 10, 3.14f);
Grid->SetColor(5, 5, FLinearColor::Red);
Blueprint
text
Create VRC Grid
    -> Set Region Type (FIntRect(0, 0, 32, 32), Linear Color)
    -> Set Region Type (FIntRect(32, 32, 64, 64), Vector3)
    -> Set Float (10, 10, 3.14)
API
Size
Function	Description
Create(W, H, Type)	Static factory. Returns a grid; chainable configuration.
Resize(W, H, Type)	Resize, preserving overlap by coordinate. New cells take Type.
Clear()	Empty the grid; width and height reset to 0.
GetWidth() / GetHeight()	Current dimensions.
IsValidCoordinate(X, Y)	Bounds check.
Bulk type configuration
Function	Description
SetAllType(Type)	Set every cell to Type, resetting values.
SetRegionType(FIntRect, Type)	Set cells in the half-open rect [Min, Max). Clamped to bounds.
Typed accessors
Each setter stamps the cell's Type automatically. Getters return the default of their value type when the stored type does not match.

Value type	Setter	Getter
Float	SetFloat(X, Y, float)	GetFloat(X, Y)
Int32	SetInt(X, Y, int32)	GetInt(X, Y)
Bool	SetBool(X, Y, bool)	GetBool(X, Y)
Vector2	SetVector2(X, Y, FVector2D)	GetVector2(X, Y)
Vector3	SetVector3(X, Y, FVector)	GetVector3(X, Y)
LinearColor	SetColor(X, Y, FLinearColor)	GetColor(X, Y)
String	SetString(X, Y, FString)	GetString(X, Y)
Bytes	SetBytes(X, Y, TArray<uint8>)	GetBytes(X, Y)
Cell type
Function	Description
SetCellType(X, Y, Type)	Change the cell's type, resetting its value.
GetCellType(X, Y)	Read the cell's type tag.
Design Notes
FVRCCell is a POD struct (~72 bytes). Copy, move, and zero are memcpy-level; grids of numeric cells involve no per-cell heap allocation.

String and Bytes values live in UVRCGrid::ExternalPayload, a TMap<int32, FVRCPayload> keyed by linear index. Grids that only use numeric types pay nothing for it.

SetCellType, SetAllType, and SetRegionType reset the affected cells to the default of the new type and drop any external payload. This is intentional: reading a Float from a cell whose type was just changed to Int32 should not silently return stale data.

Resize preserves the overlapping region by coordinate. New cells take the caller-supplied default type.

GetRawCells() returns the underlying TArray<FVRCCell> by const reference for zero-copy iteration in C++. It does not include String / Bytes payloads; those are in ExternalPayload.

Installation
Copy VariableResolutionChart/ into your project's Plugins/ folder.

Regenerate project files and build.

Enable the plugin in the editor under Edit -> Plugins -> Data Structures.

No engine modifications required.

Compatibility
Unreal Engine 5.5 - 5.8 (tested on 5.8.1)

Build dependencies: Core, CoreUObject, Engine

License
Apache License 2.0. See LICENSE and NOTICE.

Attribution is required. Endorsement and trademark use are not granted.
VRC_README_EOF