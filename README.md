# @cloudpss/mmap

[![check](https://img.shields.io/github/actions/workflow/status/CloudPSS/mmap/check.yml?event=push&logo=github)](https://github.com/CloudPSS/mmap/actions/workflows/check.yml)
[![Codacy coverage](https://img.shields.io/codacy/coverage/c0b6811e7e5f45eeb46383607cac81a8?logo=jest)](https://app.codacy.com/gh/CloudPSS/mmap/dashboard)
[![Codacy Badge](https://img.shields.io/codacy/grade/c0b6811e7e5f45eeb46383607cac81a8?logo=codacy)](https://app.codacy.com/gh/CloudPSS/mmap/dashboard)
[![npm version](https://img.shields.io/npm/v/@cloudpss/mmap?logo=npm)](https://npmjs.org/package/@cloudpss/mmap)

A high-performance memory mapping library for Node.js that provides efficient file I/O through native N-API bindings.

This is a [pure ESM package](https://gist.github.com/sindresorhus/a39789f98801d908bbc7ff3ecc99d99c) that offers memory-mapped file access for optimal performance when working with large files or binary data.

## Installation

```bash
npm install @cloudpss/mmap
```

## Requirements

- **Node.js**: >= 14.16
- **OS**: Linux (currently supported platform)

## Usage

### Basic Usage

```js
import { mmap } from '@cloudpss/mmap';

// Map an entire file into memory
const buffer = mmap('/path/to/your/file.dat');
console.log(`Mapped ${buffer.length} bytes`);

// Read data from the mapped buffer
const firstByte = buffer[0];
const firstInt32 = buffer.readInt32LE(0);
```

### Partial File Mapping

```js
import { mmap } from '@cloudpss/mmap';

// Map only the first 1024 bytes of a file
const buffer = mmap('/path/to/large/file.dat', 1024);

// If the file is smaller than 1024 bytes, it will be extended with zeros
const extendedBuffer = mmap('/path/to/small/file.dat', 2048);
```

### Typed Array Views

The library supports creating typed array views directly from memory-mapped files:

```js
import { mmap } from '@cloudpss/mmap';

// Create a Float32Array view for efficient floating-point operations
const floats = mmap(Float32Array, '/path/to/floats.bin');
for (let i = 0; i < floats.length; i++) {
  console.log(`Float at index ${i}: ${floats[i]}`);
}

// Create an Int32Array view for 32-bit integers
const ints = mmap(Int32Array, '/path/to/integers.bin');
const sum = ints.reduce((acc, val) => acc + val, 0);

// Create a DataView for mixed data types
const view = mmap(DataView, '/path/to/mixed.dat');
const header = view.getUint32(0, true); // little-endian
const timestamp = view.getBigUint64(4, true);

// Work with specific byte lengths
const limitedInts = mmap(Int32Array, '/path/to/data.bin', 1024); // 256 integers
```

### Advanced Examples

```js
import { mmap } from '@cloudpss/mmap';
import { writeFileSync } from 'fs';

// Create a binary file with sample data
const sampleData = new Uint8Array(1024);
for (let i = 0; i < sampleData.length; i++) {
  sampleData[i] = i % 256;
}
writeFileSync('/tmp/sample.bin', sampleData);

// Map as different view types
const bytes = mmap(Uint8Array, '/tmp/sample.bin');
const shorts = mmap(Uint16Array, '/tmp/sample.bin');
const words = mmap(Uint32Array, '/tmp/sample.bin');

console.log(`File contains ${bytes.length} bytes`);
console.log(`File contains ${shorts.length} 16-bit integers`);
console.log(`File contains ${words.length} 32-bit integers`);

// Modify data through memory mapping (changes are reflected in the file)
bytes[0] = 255;
console.log(`Modified first byte to ${bytes[0]}`);
```

## API

### `mmap(path: string, byteLength?: number): Buffer`

Creates a memory-mapped Buffer from a file.

**Parameters:**

- `path` - Path to the file to be memory-mapped
- `byteLength` (optional) - Number of bytes to map. If not provided, maps the entire file. If larger than file size, extends the file with zeros.

**Returns:** A Buffer representing the memory-mapped file

**Throws:**

- `TypeError` - When path is not a string
- `Error` - When the file cannot be opened or mapped

### `mmap<T>(view: ArrayBufferViewConstructor<T>, path: string, byteLength?: number): T`

Creates a memory-mapped typed array or view from a file.

**Parameters:**

- `view` - Constructor for the desired typed array (Int8Array, Uint8Array, Int16Array, Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array, BigInt64Array, BigUint64Array, DataView)
- `path` - Path to the file to be memory-mapped
- `byteLength` (optional) - Number of bytes to map

**Returns:** A typed array or DataView of the specified type

**Throws:**

- `TypeError` - When path is not a string or view is not a valid constructor
- `Error` - When the file cannot be opened or mapped

## Performance Benefits

Memory mapping provides several advantages over traditional file I/O:

- **Zero-copy operations**: Data is accessed directly from memory without copying
- **Lazy loading**: Only accessed portions of the file are loaded into RAM
- **Shared memory**: Multiple processes can efficiently share the same mapped file
- **Automatic caching**: The OS handles caching and memory management
- **Large file support**: Work with files larger than available RAM

## Platform Support

Currently, this package is optimized for and supports:

- **Linux** (all architectures)

Support for additional platforms may be added in future versions.

## Error Handling

```js
import { mmap } from '@cloudpss/mmap';

try {
  const buffer = mmap('/path/to/file.dat');
  // Work with the buffer
} catch (error) {
  if (error instanceof TypeError) {
    console.error('Invalid path provided');
  } else {
    console.error('Failed to map file:', error.message);
  }
}
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

MIT License - see the [LICENSE](LICENSE) file for details.
