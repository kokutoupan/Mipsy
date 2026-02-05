# Mipsy

Mipsy is a simple C-like language compiler targeting MIPS architecture.
C言語ライクな独自言語をMIPSアセンブリにコンパイルする学習用コンパイラです。

## Features (特徴)

* **C-like Syntax**: 変数宣言、関数定義、if/whileなどの制御構文をサポート
* **Pointers & Arrays**: ポインタ演算、多次元配列へのアクセス
* **Bitwise Operations**: シフト、AND/OR/XORなどのビット演算に対応
* **Optimizations**:
  * **Register Allocation**:
    * Sethi-Ullman algorithm (Weighted register allocation)
  * **Control Flow**:
    * Branch chaining & Jump threading
    * Branch inversion
    * Unreachable code elimination
    * Unused label deletion
  * **Peephole & Data Flow**:
    * Delay slot filling
    * NOP removal
    * Constant folding & propagation (ADDIU chain)
    * Zero / Move propagation
    * Address calculation folding (ADDIU+LW -> LW offset)
    * Dead code elimination
  * **Function Call**:
    * Frame pointer omission (FPO)
    * Leaf function optimization (Return address omission)
    * Iterative optimization (applying passes until convergence)

## Language Specification (言語仕様)

詳細な文法定義や演算子の仕様については、以下のドキュメントを参照してください。

* 👉 **[Language Specification (言語仕様書)](./SPECIFICATION.md)**

## Build & Usage (ビルドと使い方)

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Usage

```bash
./mipsy [options] <input_file>

```

**Options:**

* `-o <file>` : Output file name (Default: `out.s`)
* `-O0,-O1,...`        : Enable optimization(Default: -O1)
* `-a`        : Show Abstract Syntax Tree (AST)
* `-j`        : Show AST as JSON
* `-d`        : Show debug info
* `-h`        : Show help

### Example Code

```
func main() {
    define i, sum;
    sum = 0;
    i = 0;
    while (i <= 10) {
        sum = sum + i;
        i = i + 1;
    }
    // sum is now 55
}
```

### License

This project is licensed under the MIT License - see the [LICENSE](./LICENSE) file for details.
