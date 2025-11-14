# Firebird LTRIM_ZERO Collation Plugin

A custom collation for Firebird that removes leading zeros and spaces before comparing strings (case-insensitive).

## What It Does

This collation makes these strings **equal**:
```
'00001' = '1' = '    1'
'00000A' = '0A' = 'A' = '    A' = 'a'
```

Perfect for comparing product codes, invoice numbers, or any field where leading zeros should be ignored.

## Features

- ✅ **Removes leading zeros** (`'00001'` = `'1'`)
- ✅ **Removes leading spaces** (`'   A'` = `'A'`)
- ✅ **Case-insensitive** (`'a'` = `'A'`)
- ✅ **Works with indexes** - creates and uses indexes normally
- ✅ **Multiple charsets** - WIN1252, ISO8859_1, UTF8, DOS850, NONE

## Quick Start

### 1. Build the Plugin

**Windows (Visual Studio):**
```batch
compile.bat
```

**CMake (Cross-platform):**
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 2. Install

Copy the files to Firebird's `intl` directory:
- `fbltrimzero.dll` (or `.so` on Linux)
- `fbltrimzero.conf`

The plugin will be automatically loaded by Firebird (no need to edit `fbintl.conf`).

### 3. Use in Database

```sql
-- Create the collation
CREATE COLLATION WIN1252_LTRIM_ZERO_AI
    FOR WIN1252
    FROM EXTERNAL ('WIN1252_LTRIM_ZERO');

-- Use in a table
CREATE TABLE PRODUCTS (
    CODE VARCHAR(20) CHARACTER SET WIN1252
        COLLATE WIN1252_LTRIM_ZERO_AI,
    NAME VARCHAR(100)
);

-- Test it
SELECT '00001' COLLATE WIN1252_LTRIM_ZERO_AI = '1' FROM RDB$DATABASE;
-- Returns TRUE
```

## Supported Charsets

- `WIN1252` - Windows Latin-1
- `ISO8859_1` - ISO Latin-1
- `UTF8` - Unicode (single-byte chars only)
- `DOS850` - DOS Latin-1
- `NONE` - No conversion

## Documentation

- **[Quick Start Guide](QUICK_START.md)** - Get started in 5 minutes

## Requirements

- **Firebird:** 3.0, 4.0, or 5.0+
- **Windows:** Visual Studio 2019+ or MinGW
- **Linux:** GCC 7+ or Clang 6+
- **CMake:** 3.12+ (optional, for cross-platform builds)

## Example Usage

```sql
-- Create a test table
CREATE TABLE TEST_CODES (
    CODE VARCHAR(20) COLLATE WIN1252_LTRIM_ZERO_AI
);

INSERT INTO TEST_CODES VALUES ('00001');
INSERT INTO TEST_CODES VALUES ('1');
INSERT INTO TEST_CODES VALUES ('0000A');
INSERT INTO TEST_CODES VALUES ('A');
INSERT INTO TEST_CODES VALUES ('   B');
INSERT INTO TEST_CODES VALUES ('B');

-- Returns 3 distinct values (1, A, B)
SELECT DISTINCT CODE FROM TEST_CODES ORDER BY CODE;

-- Returns 2 rows (both versions of '1')
SELECT * FROM TEST_CODES WHERE CODE = '1';
```

## Project Structure

```
firebird-ltrim-zero-collation/
├── src/
│   └── fbltrimzero.cpp          # Main source code
├── config/
│   └── fbltrimzero.conf         # Plugin configuration
├── test/
│   └── test_collation.sql       # Test scripts
├── CMakeLists.txt               # CMake build config
├── compile.bat                  # Windows build script
└── README.md                    # This file
```

## Building from Source

### Windows

```batch
cd firebird-ltrim-zero-collation
compile.bat
```

### Linux/macOS with CMake

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## Compatibility

Tested with:
- Firebird 3.0
- Firebird 4.0
- Firebird 5.0

Platforms:
- Windows (x64)
- Linux (x64)

## License

TBD

## Author

Lucas Rubian Schatz (arvanus)

## Links

- [Firebird Project](https://firebirdsql.org/)

