# XAD Tape Format Analysis

## Key Data Structures

### Tape Class (src/XAD/Tape.hpp)

```cpp
template <class Real, std::size_t N = 1>
class Tape {
private:
    // Main tape storage
    typename TapeContainerTraits<Real, slot_type>::operations_type operations_;
    typename TapeContainerTraits<Real, slot_type>::statements_type statement_;
    std::vector<derivative_type> derivatives_;
};
```

### Type Definitions

```cpp
// From TapeContainer.hpp
statements_type = ChunkContainer<std::pair<slot_type, slot_type>>;
operations_type = OperationsContainerPaired<Real, slot_type>;
```

**Where**:
- `slot_type` = `unsigned int` (variable ID on tape)
- `Real` = `double` (or float)

---

## How Operations Are Stored

### statements_
Stores pairs: `(operation_index, lhs_slot)`

Each statement represents one assignment:
```cpp
AD z = x + y;  // Creates one statement
```

### operations_
Stores the actual operation data as `(multiplier, slot)` pairs.

The `operation_index` in a statement points to a range in `operations_`.

---

## Example: `z = x * 2.0 + y`

Let's trace what happens:

### Step 1: Register inputs
```cpp
tape.registerInput(x);  // x gets slot 0
tape.registerInput(y);  // y gets slot 1
tape.newRecording();
```

### Step 2: Expression evaluation
```cpp
AD z = x * 2.0 + y;  // z gets slot 2
```

This creates TWO intermediate operations in XAD's expression templates:
1. `temp1 = x * 2.0`
2. `z = temp1 + y`

### Step 3: What's recorded

**statements_**:
```
statement_[0] = (op_index=0, lhs_slot=2)   // z's statement
```

**operations_**:
```
operations_[0] = (multiplier=2.0, slot=0)   // x contribution
operations_[1] = (multiplier=1.0, slot=1)   // y contribution
```

### Interpretation
For slot 2 (z):
- Look up statement_[0] → operation starts at index 0
- operations_[0] = 2.0 * value[slot_0]  (2.0 * x)
- operations_[1] = 1.0 * value[slot_1]  (1.0 * y)
- Result: z = 2.0*x + 1.0*y

---

## Key Methods

### registerInput(active_type& inp)
```cpp
void registerInput(active_type& inp) {
    inp.slot_ = registerVariable();  // Allocate slot
    pushLhs(inp.slot_);              // Record statement
}
```

### registerOutput(active_type& outp)
```cpp
void registerOutput(active_type& outp) {
    outp.slot_ = registerVariable();
    pushLhs(outp.slot_);
}
```

### pushLhs(slot_type slot)
```cpp
void pushLhs(slot_type slot) {
    statement_.emplace_back(operations_.size(), slot);
}
```

Creates a new statement pointing to current operation index.

### pushAll(multipliers, slots, n)
```cpp
template <class MulIt, class SlotIt>
void pushAll(MulIt multipliers, SlotIt slots, unsigned n) {
    operations_.append_n(multipliers, slots, n);
}
```

Adds operation data (multiplier, slot) pairs.

---

## Challenge for Forge Integration

### Problem: No Explicit OpCode

XAD stores operations as `(multiplier, slot)` pairs, but **doesn't store the operation type**.

**Example ambiguity**:
```
operations_ = [(1.0, slot_x), (1.0, slot_y)]
```

Could mean:
- `result = x + y` (Add operation)
- `result = x; result += y` (two separate operations)

### Where Operation Type IS Known

The operation type is available during expression template evaluation in:
- `BinaryExpr.hpp` - knows it's Add, Mul, etc.
- `UnaryExpr.hpp` - knows it's Neg, Sin, etc.

But by the time it's recorded to tape, only multipliers/slots are stored.

---

## Our Solution Strategy

### Option 1: Pattern Matching (Implement First)
Infer operation from multiplier patterns:
- `[(1.0, a), (1.0, b)]` → Add
- `[(1.0, a), (-1.0, b)]` → Sub
- `[(-1.0, a)]` → Neg
- etc.

### Option 2: Extend Tape Format (Future)
Modify XAD to also store OpCode alongside multipliers.

---

## What We Need to Access

To convert XAD tape → Forge graph, we need:

1. **statements_** - to iterate through all operations
2. **operations_** - to get (multiplier, slot) data
3. **Input slots** - which slots are inputs
4. **Output slots** - which slots are outputs

Currently these are **private** - we need to add accessor methods!
