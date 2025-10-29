import { createRequire } from 'node:module';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
// eslint-disable-next-line unicorn/prefer-import-meta-properties
const __dirname = import.meta.dirname || dirname(fileURLToPath(import.meta.url));
const rootDir = resolve(__dirname, './../');

// eslint-disable-next-line @typescript-eslint/no-unsafe-call
const bindings = require('node-gyp-build')(rootDir) as {
    /** Create memory-mapped buffer from a file */
    mmap(path: string, length: number): Buffer;
};

/** Constructor of an ArrayBufferView, i.e., a typed array or {@link DataView} */
type ArrayBufferViewConstructor<T> = {
    /** Creates a new ArrayBufferView object using the passed ArrayBuffer for its storage */
    new (buffer: ArrayBufferLike, byteOffset?: number, length?: number): T;
    /** The size in bytes of each element in the array */
    readonly BYTES_PER_ELEMENT?: number;
};

/**
 * Create a memory-mapped buffer from a file
 *
 * Maps a file into memory and returns a Buffer representing the mapped region.
 * If the file doesn't exist, an error will be thrown.
 * @param path - Path to the file to be memory-mapped
 * @param byteLength - Length of the memory mapping in bytes. If not provided,
 *                     the entire file will be mapped. If the length is larger
 *                     than the file size, the file will be extended with zeros.
 * @returns A Buffer representing the memory-mapped file
 * @throws {TypeError} When path is not a string
 * @throws {Error} When the file cannot be opened or mapped
 * @example
 * ```js
 * import { mmap } from '@cloudpss/mmap';
 *
 * // Map entire file
 * const buffer = mmap('/path/to/file.dat');
 *
 * // Map first 1024 bytes
 * const partialBuffer = mmap('/path/to/file.dat', 1024);
 * ```
 */
export function mmap(path: string, byteLength?: number): Buffer;

/**
 * Create a memory-mapped typed array or view from a file
 *
 * Maps a file into memory and returns a typed array or view of the specified type.
 * This is useful when you want to work with binary data in a specific format.
 * @template T - The type of the returned array buffer view
 * @param view - Constructor for the desired {@link ArrayBufferView},
 *               (e.g., {@link Int32Array}, {@link Float64Array}, {@link DataView})
 * @param path - Path to the file to be memory-mapped
 * @param byteLength - Length of the memory mapping in bytes. If not provided,
 *                     the entire file will be mapped. If the length is larger
 *                     than the file size, the file will be extended with zeros.
 * @returns A typed array or buffer view of the specified type
 * @throws {TypeError} When path is not a string or view is not a constructor
 * @throws {Error} When the file cannot be opened or mapped
 * @example
 * ```js
 * import { mmap } from '@cloudpss/mmap';
 *
 * // Create a Float32Array view of the file
 * const floats = mmap(Float32Array, '/path/to/floats.bin');
 *
 * // Create an Int32Array with specific byte length
 * const ints = mmap(Int32Array, '/path/to/data.bin', 1024);
 *
 * // Create a DataView for mixed data types
 * const view = mmap(DataView, '/path/to/mixed.dat');
 * ```
 */
export function mmap<T>(view: ArrayBufferViewConstructor<T>, path: string, byteLength?: number): T;

/**
 * Implementation of the mmap function with overloaded signatures
 *
 * This function handles both Buffer and typed array creation from memory-mapped files.
 * @internal
 */
export function mmap(a0: string | ArrayBufferViewConstructor<unknown>, a1?: string | number, a2?: number): unknown {
    let path: string;
    let byteLength: number;
    let view: ArrayBufferViewConstructor<unknown> | undefined;

    if (typeof a0 == 'function') {
        view = a0;
        path = a1 as string;
        byteLength = a2 ?? -1;
    } else {
        path = a0;
        byteLength = (a1 as number) ?? -1;
    }

    if (typeof path !== 'string') throw new TypeError('path must be a string');
    byteLength = Math.trunc(Number(byteLength));
    if (!Number.isSafeInteger(byteLength) || byteLength <= 0) byteLength = -1;
    path = path.startsWith('/dev/shm/') ? path : resolve(path);

    const map = bindings.mmap(path, byteLength);

    if (view == null) {
        return map;
    }

    if (view.BYTES_PER_ELEMENT) {
        const finalLength = Math.trunc(map.length / view.BYTES_PER_ELEMENT);
        return new view(map.buffer, 0, finalLength);
    }

    return new view(map.buffer);
}
