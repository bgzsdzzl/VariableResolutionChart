\# API Reference



All types below live in the `VariableResolutionChart` module.

Include the umbrella header to pull in everything:



```cpp

\#include "VRCGrid.h"

\#include "VRCTypes.h"

```



\## Quick Reference



| Category | Functions |

| --- | --- |

| Construction | `Create` |

| Size | `GetWidth`, `GetHeight`, `IsValidCoordinate`, `Resize`, `Clear` |

| Bulk type | `SetAllType`, `SetRegionType` |

| Typed accessors | `SetFloat`/`GetFloat`, `SetInt`/`GetInt`, `SetBool`/`GetBool`, `SetVector2`/`GetVector2`, `SetVector3`/`GetVector3`, `SetColor`/`GetColor`, `SetString`/`GetString`, `SetBytes`/`GetBytes` |

| Cell type | `SetCellType`, `GetCellType` |

| Low-level | `GetRawCells` |



\---



\## `EVRCCellType`



Enum tag stored per cell. Determines which payload field is meaningful.



| Value | Meaning |

| --- | --- |

| `None` | Uninitialized. Getters treat this as "no value". |

| `Float` | `FVRCCell::FloatValue` is valid. |

| `Int32` | `FVRCCell::IntValue` is valid. |

| `Bool` | `FVRCCell::BoolValue` is valid. |

| `Vector2` | `FVRCCell::Vector2Value` is valid. |

| `Vector3` | `FVRCCell::Vector3Value` is valid. |

| `LinearColor` | `FVRCCell::ColorValue` is valid. |

| `String` | Value lives in `FVRCPayload::StringValue` (out-of-line). |

| `Bytes` | Value lives in `FVRCPayload::ByteValue` (out-of-line). |



\## `FVRCCell`



Inline cell storage. POD-only; \~72 bytes. Only the field matching `Type` is meaningful.



```cpp

EVRCCellType Type;

float        FloatValue;

int32        IntValue;

bool         BoolValue;

FVector2D    Vector2Value;

FVector      Vector3Value;

FLinearColor ColorValue;

```



\## `FVRCPayload`



Out-of-line storage for `String` and `Bytes` cells. Only allocated when a cell

actually carries one of those types.



```cpp

FString       StringValue;

TArray<uint8> ByteValue;

```



\---



\## Construction



\### `static UVRCGrid\* UVRCGrid::Create(int32 InitialWidth = 0, int32 InitialHeight = 0, EVRCCellType InitialType = EVRCCellType::Float)`



Creates a new grid. Returns `this` in the form of the grid pointer, so

configuration can be chained:



```cpp

UVRCGrid\* Grid = UVRCGrid::Create(64, 64, EVRCCellType::Float)

&#x20;   ->SetRegionType(FIntRect(0,  0,  32, 32), EVRCCellType::LinearColor)

&#x20;   ->SetRegionType(FIntRect(32, 32, 64, 64), EVRCCellType::Vector3);

```



Pass `0` for either dimension to create an empty grid; call `Resize` afterwards.



\*\*Ownership\*\*: The returned object is a `UObject` created with `NewObject<UVRCGrid>()`

and has no outer. Keep a reference on a member variable or pass it to a UObject

that outlives the grid, otherwise it may be garbage collected.



\---



\## Size



\### `int32 GetWidth() const`

\### `int32 GetHeight() const`



Return the current dimensions. Both are `0` for an empty grid.



\### `bool IsValidCoordinate(int32 X, int32 Y) const`



Returns `true` if `(X, Y)` is inside the grid bounds (`0 <= X < Width`,

`0 <= Y < Height`). All accessors use this internally, so out-of-bounds

calls never assert.



\### `UVRCGrid\* Resize(int32 NewWidth, int32 NewHeight, EVRCCellType DefaultType = EVRCCellType::Float)`



Resizes the grid. The overlapping region is preserved \*\*by coordinate\*\*:



\- A cell at `(X, Y)` in the old grid keeps its value if `X < NewWidth` and

&#x20; `Y < NewHeight`.

\- Any cell introduced by the resize is initialized with `DefaultType`

&#x20; and a zeroed payload.

\- `String` / `Bytes` payloads attached to surviving cells are re-keyed to

&#x20; their new linear indices. Payloads attached to cells that fall outside

&#x20; the new extent are dropped.

\- Setting either dimension to `0` is equivalent to `Clear()`.



Returns `this` for chaining. Complexity is `O(NewWidth \* NewHeight)`.



```cpp

Grid->Resize(128, 128, EVRCCellType::Float); // growing

Grid->Resize(32, 32, EVRCCellType::Float);   // shrinking

```



\### `UVRCGrid\* Clear()`



Empties the grid. Both dimensions become `0`, all cells are destroyed,

and all out-of-line payloads are released. Returns `this`.



\---



\## Bulk type configuration



\### `UVRCGrid\* SetAllType(EVRCCellType Type)`



Sets every cell to `Type`. All existing values are reset to the default

of the new type, and any out-of-line payload is dropped.



Returns `this`. Complexity is `O(Width \* Height)`.



\### `UVRCGrid\* SetRegionType(const FIntRect\& Region, EVRCCellType Type)`



Sets every cell in the half-open rectangle `\[Region.Min, Region.Max)` to

`Type`. The rectangle is clamped to the grid bounds; out-of-range values

in `Region` are ignored.



```cpp

// Set a 32x32 block in the top-left corner to LinearColor.

Grid->SetRegionType(FIntRect(0, 0, 32, 32), EVRCCellType::LinearColor);

```



\*\*Note\*\*: `FIntRect` is half-open. `FIntRect(0, 0, 32, 32)` covers

`X ∈ \[0, 32)` and `Y ∈ \[0, 32)` — 32 columns by 32 rows.



Returns `this`.



\---



\## Typed accessors



Each setter stamps the cell's `EVRCCellType` automatically, so callers never

touch `FVRCCell` directly. Each getter returns the default of its value type

if the coordinate is out of bounds \*\*or\*\* the stored type does not match.



| Value type | Setter | Getter |

| --- | --- | --- |

| Float | `SetFloat(X, Y, float)` | `GetFloat(X, Y)` |

| Int32 | `SetInt(X, Y, int32)` | `GetInt(X, Y)` |

| Bool | `SetBool(X, Y, bool)` | `GetBool(X, Y)` |

| Vector2 | `SetVector2(X, Y, FVector2D)` | `GetVector2(X, Y)` |

| Vector3 | `SetVector3(X, Y, FVector)` | `GetVector3(X, Y)` |

| LinearColor | `SetColor(X, Y, FLinearColor)` | `GetColor(X, Y)` |

| String | `SetString(X, Y, FString)` | `GetString(X, Y)` |

| Bytes | `SetBytes(X, Y, TArray<uint8>)` | `GetBytes(X, Y)` |



\*\*Setter return value\*\*: `true` on success, `false` on out-of-bounds

coordinates. Setting a cell whose type differs from the payload type

overwrites both the type tag and the value.



\*\*Getter defaults\*\* when the type does not match:



| Type | Default |

| --- | --- |

| Float | `0.0f` |

| Int32 | `0` |

| Bool | `false` |

| Vector2 | `FVector2D::ZeroVector` |

| Vector3 | `FVector::ZeroVector` |

| LinearColor | `FLinearColor::Black` |

| String | empty `FString` |

| Bytes | empty `TArray<uint8>` |



```cpp

Grid->SetFloat(10, 10, 3.14f);

float V = Grid->GetFloat(10, 10);       // 3.14

EVRCCellType T = Grid->GetCellType(10, 10); // Float



Grid->SetColor(10, 10, FLinearColor::Red);

float V2 = Grid->GetFloat(10, 10);      // 0.0 (type is now LinearColor)

```



\---



\## Cell type



\### `bool SetCellType(int32 X, int32 Y, EVRCCellType NewType)`



Changes the cell's type without writing a value. The cell is reset to a

zeroed value of `NewType`; any existing `String` or `Bytes` payload is

dropped.



Returns `false` if the coordinate is out of bounds.



\### `EVRCCellType GetCellType(int32 X, int32 Y) const`



Returns the cell's type tag, or `EVRCCellType::None` if the coordinate is

out of bounds.



\---



\## Low-level access



\### `const TArray<FVRCCell>\& GetRawCells() const`



Returns the underlying cell array by const reference for zero-copy iteration

in C++. The array is stored row-major: `Index = Y \* Width + X`.



\*\*Important\*\*: This view does \*\*not\*\* include `String` or `Bytes` payloads;

those are kept in a separate sparse map. Use `GetString` / `GetBytes` for

those types.

