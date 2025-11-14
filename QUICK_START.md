# Quick Start - LTRIM_ZERO Collation

## Build and Install in 5 Minutes

### Step 1: Build

**If you have Visual Studio:**
```batch
cd firebird-ltrim-zero-collation
compile.bat
```

**Using CMake (cross-platform):**
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

You should see: "BUILD COMPLETED SUCCESSFULLY!" (or similar message)

### Step 2: Install

**Windows (Run as Administrator):**

Manually copy to Firebird's `intl` directory:
- `dist\fbltrimzero.dll` → `<firebird>\intl\`
- `dist\fbltrimzero.conf` → `<firebird>\intl\`

**Linux/macOS:**
```bash
sudo cp build/fbltrimzero.so /opt/firebird/intl/
sudo cp config/fbltrimzero.conf /opt/firebird/intl/
```

### Step 3: Use in Database

Connect to your database and execute:

```sql
-- 1. Create the collation
CREATE COLLATION WIN1252_LTRIM_ZERO_AI
    FOR WIN1252
    FROM EXTERNAL ('WIN1252_LTRIM_ZERO');

-- 2. Apply to your domain (example)
CREATE DOMAIN YOUR_CODE_DOMAIN AS VARCHAR(10)
    CHARACTER SET  WIN1252
    COLLATE WIN1252_LTRIM_ZERO_AI;

-- 3. Test it
SELECT '00001' COLLATE WIN1252_LTRIM_ZERO_AI = '1' FROM RDB$DATABASE;
-- Should return TRUE
```

### Step 4: Verify

```sql
-- All these queries should return TRUE
SELECT '00000A' COLLATE WIN1252_LTRIM_ZERO_AI = 'A' FROM RDB$DATABASE;
SELECT '0A' COLLATE WIN1252_LTRIM_ZERO_AI = 'A' FROM RDB$DATABASE;
SELECT '    A' COLLATE WIN1252_LTRIM_ZERO_AI = 'A' FROM RDB$DATABASE;
SELECT 'a' COLLATE WIN1252_LTRIM_ZERO_AI = 'A' FROM RDB$DATABASE;
```

## Done!

Now all columns using `YOUR_CODE_DOMAIN` will automatically ignore leading zeros and spaces!

## Troubleshooting

1. **Build error**: Check BUILD.md for detailed instructions
2. **Install error**: Run as administrator (Windows) or use sudo (Linux)
3. **Collation not working**: Check firebird.log for errors
4. **Questions**: Read README.md and test/test_collation.sql

## Available Charsets

```sql
-- WIN1252 (Windows Latin-1)
CREATE COLLATION WIN1252_LTRIM_ZERO_AI FOR WIN1252 FROM EXTERNAL ('WIN1252_LTRIM_ZERO');

-- ISO8859_1 (ISO Latin-1)
CREATE COLLATION ISO8859_1_LTRIM_ZERO_AI FOR ISO8859_1 FROM EXTERNAL ('ISO8859_1_LTRIM_ZERO');

-- UTF8
CREATE COLLATION UTF8_LTRIM_ZERO_AI FOR UTF8 FROM EXTERNAL ('UTF8_LTRIM_ZERO');

-- DOS850
CREATE COLLATION DOS850_LTRIM_ZERO_AI FOR DOS850 FROM EXTERNAL ('DOS850_LTRIM_ZERO');

-- NONE
CREATE COLLATION NONE_LTRIM_ZERO_AI FOR NONE FROM EXTERNAL ('NONE_LTRIM_ZERO');
```

## Next Steps

- Read the [README](README.md) for more details
- Run tests from `test/test_collation.sql`
