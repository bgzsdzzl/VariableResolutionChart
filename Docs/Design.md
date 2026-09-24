\# Design Notes



This document explains the reasoning behind a handful of decisions in

`VariableResolutionChart`. It is not a tutorial; read `API.md` first.



\## Goals



\- Runtime-resizable 2D grid.

\- Per-cell type tag so a single grid can mix different value types.

\- No dependency on Virtual Texture, Texture2DArray, or any rendering resource.

\- Cheap for the common case: grids of numeric cells.



\## Storage layout



Cells are stored in a single `TArray<FVRCCell>`, row-major, indexed by

`Y \* Width + X`. The grid is not sparse; every coordinate in `\[0, Width) x

\[0, Height)` has a cell.



```

Index = Y \* Width + X



&#x20;       X=0  X=1  X=2  X=3

Y=0  \[   0    1    2    3 ]

Y=1  \[   4    5    6    7 ]

Y=2  \[   8    9   10   11 ]

```



\## Why `FVRCCell` is POD-only



`FVRCCell` contains only inline value types:



```cpp

EVRCCellType Type;

float        FloatValue;

int32        IntValue;

bool         BoolValue;

FVector2D    Vector2Value;

FVector      Vector3Value;

FLinearColor ColorValue;

```



That is roughly 72 bytes, no pointers, no heap.



The alternative would be to add `FString` and `TArray<uint8>` directly to

`FVRCCell`. That would:



\- Force a constructor and destructor on every cell (killing `memcpy`-level

&#x20; copy / move / zero).

\- Add a `FString` (≈ 16 bytes) and a `TArray` header (≈ 16 bytes) to every

&#x20; cell, even if the grid never uses them.

\- Make a 1000x1000 numeric grid pay for 2 million objects it will never use.



Since numeric grids are the dominant use case, this trade-off is not worth

it.



\## Why `String` / `Bytes` live out-of-line



`String` and `Bytes` payloads are stored in a separate sparse map:



```cpp

TMap<int32, FVRCPayload> ExternalPayload;

```



`FVRCPayload` contains the actual `FString` and `TArray<uint8>`. A cell of

type `String` or `Bytes` has a corresponding entry at its linear index; a

numeric cell has no entry.



Consequences:



\- A grid of 1000x1000 `Float` cells allocates \*\*zero\*\* `FVRCPayload`

&#x20; entries.

\- A grid that uses `String` in 50 cells allocates exactly 50 entries, plus

&#x20; the `FString` buffers themselves.

\- The map is keyed by linear index, so on `Resize` each surviving entry

&#x20; must be re-keyed to its new index. This is the only place the sparse

&#x20; layout leaks into the resize code, and it is handled explicitly.



`GetRawCells()` returns the flat `TArray<FVRCCell>` only. Callers that need

`String` or `Bytes` values must go through `GetString` / `GetBytes`.



\## Type stamping on write



Every setter writes the cell's `Type` field along with the value:



```cpp

bool UVRCGrid::SetFloat(int32 X, int32 Y, float Value)

{

&#x20;   // ... bounds check ...

&#x20;   ResetCellAt(Index, EVRCCellType::Float);

&#x20;   Cells\[Index].FloatValue = Value;

&#x20;   return true;

}

```



`ResetCellAt` zeroes the cell and removes any external payload. The result

is that a cell's type is always consistent with the value it holds — the

user cannot accidentally have `Type == Int32` while the meaningful data

sits in `FloatValue`.



\## Why `SetCellType` clears the value



`SetCellType(X, Y, NewType)` does \*\*not\*\* preserve the previous value:



```cpp

Grid->SetFloat(0, 0, 3.14f);

Grid->SetCellType(0, 0, EVRCCellType::Int32);

float V = Grid->GetFloat(0, 0); // 0.0, not 3.14

```



The alternative would be to convert between types (float → int32, int32 →

float, etc.). That introduces a large matrix of conversions, each with its

own truncation / rounding / clamping rules, and every new type adds more

rows and columns to it.



Clearing is the conservative choice: it makes the operation always safe,

and the caller who wants a conversion can read the value, convert it

explicitly, and write it back.



```cpp

// Explicit conversion.

int32 I = static\_cast<int32>(Grid->GetFloat(0, 0));

Grid->SetInt(0, 0, I);

```



\## Why `Resize` preserves overlap by coordinate



The alternative is to preserve by \*\*linear index\*\*: cell `N` in the old

array becomes cell `N` in the new array. This is faster (one `memcpy`), but

it produces layouts that are hard to reason about:



```

Old: 4x4,  New: 8x4



Preserve by index:       Preserve by coordinate:

\[ a b c d ]              \[ a b c d  . . . . ]

\[ e f g h ]       →      \[ e f g h  . . . . ]

\[ i j k l ]              \[ i j k l  . . . . ]

\[ m n o p ]              \[ m n o p  . . . . ]

```



Index-based would place `e` at `(4, 0)`, `f` at `(5, 0)`, etc. — the data

would "wrap" into the new columns. Coordinate-based keeps the visual layout

stable, which is what users expect when they imagine a grid.



The cost is that resize is `O(NewWidth \* NewHeight)` instead of `O(N)`, and

that external payload indices must be re-keyed. For grids in the hundreds

to low thousands of cells, this is negligible.



\## Why chainable methods



Configuration methods (`Resize`, `Clear`, `SetAllType`, `SetRegionType`)

return `this` rather than `void`. This makes setup read as a single

expression:



```cpp

UVRCGrid\* Grid = UVRCGrid::Create(64, 64, EVRCCellType::Float)

&#x20;   ->SetRegionType(FIntRect(0,  0,  32, 32), EVRCCellType::LinearColor)

&#x20;   ->SetRegionType(FIntRect(32, 32, 64, 64), EVRCCellType::Vector3);

```



In Blueprint the return value is ignored; each node chains to the next by

default. No behavior depends on the chaining.



\## Thread safety



`UVRCGrid` is \*\*not\*\* thread-safe. All accessors read and write the

underlying `TArray` and `TMap` without synchronization. Callers that need

to use a grid from a worker thread must provide their own external lock.



For the common pattern (single-threaded producer, single-threaded consumer

on the game thread), this is sufficient. If you need concurrent access,

consider a copy-on-write scheme at the call site rather than adding locks

here.



\## Known limitations



\- \*\*No bounds relaxation.\*\* Reading or writing outside the grid returns

&#x20; `false` / a default value; it does not grow the grid. Call `Resize`

&#x20; explicitly.

\- \*\*No type conversion on `SetCellType`.\*\* See above.

\- \*\*No serialization helpers.\*\* `GetRawCells` exposes the inline array, but

&#x20; `String` / `Bytes` payloads are not part of that view. If you need to

&#x20; serialize a full grid, iterate it manually.

\- \*\*No iterators / views.\*\* C++ callers can iterate `GetRawCells()` directly;

&#x20; Blueprint callers must use per-cell getters.

\- \*\*Payload map is not compacted on shrink.\*\* `Resize` re-keys surviving

&#x20; entries but does not periodically compact. If a workload repeatedly adds

&#x20; and removes `String` / `Bytes` cells, the map may accumulate tombstones

&#x20; until the next `TMap` rehash. In practice this is rare.

