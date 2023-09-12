import { createRequire } from 'node:module';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const rootDir = resolve(dirname(fileURLToPath(import.meta.url)), './../');

// eslint-disable-next-line @typescript-eslint/no-unsafe-assignment, @typescript-eslint/no-unsafe-call, @typescript-eslint/no-var-requires
const bindings = require('node-gyp-build')(rootDir) as {
    /** create mmap */
    mmap(path: string): Buffer;
};

/** create mmap */
export function mmap(path: string): Buffer {
    if (typeof path !== 'string') throw new TypeError('path must be a string');
    path = resolve(path);
    return bindings.mmap(path);
}
