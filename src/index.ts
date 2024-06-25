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

/**
 * create mmap
 * @param path - path to file
 * @param length - length of mmap, if not provided, the whole file will be mapped, if length is larger than file size, the file will be extended with zeros
 */
export function mmap(path: string, length?: number): Buffer {
    if (typeof path !== 'string') throw new TypeError('path must be a string');
    length = Number(length);
    if (!Number.isSafeInteger(length) || length <= 0) length = -1;
    path = resolve(path);
    return bindings.mmap(path, length);
}
