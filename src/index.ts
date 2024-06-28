import { createRequire } from 'node:module';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const rootDir = resolve(dirname(fileURLToPath(import.meta.url)), './../');

// eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call, @typescript-eslint/no-var-requires
const bindings = require('node-gyp-build')(rootDir) as {
    /** create mmap */
    mmap(path: string, length: number): Buffer;
};

/** Constructor of an ArrayBufferView */
type ArrayBufferViewConstructor<T> = {
    /** Creates a new ArrayBufferView object using the passed ArrayBuffer for its storage. */
    new (buffer: ArrayBufferLike, byteOffset?: number, length?: number): T;
    /**
     * The size in bytes of each element in the array.
     */
    readonly BYTES_PER_ELEMENT?: number;
};

/**
 * create mmap
 * @param path - path to file
 * @param byteLength - length of mmap, if not provided, the whole file will be mapped, if length is larger than file size, the file will be extended with zeros
 */
export function mmap(path: string, byteLength?: number): Buffer;

/**
 * create mmap
 * @param view - TypedArray constructor
 * @param path - path to file
 * @param byteLength - length of mmap, if not provided, the whole file will be mapped, if length is larger than file size, the file will be extended with zeros
 */
export function mmap<T>(view: ArrayBufferViewConstructor<T>, path: string, byteLength?: number): T;

/** create mmap */
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
    path = resolve(path);

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
