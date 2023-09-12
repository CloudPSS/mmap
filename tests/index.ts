import { mmap } from '@cloudpss/mmap';

describe('should reject bad input', () => {
    // @ts-expect-error - bad input
    expect(() => mmap(1)).toThrowError(TypeError);
    // @ts-expect-error - bad input
    expect(() => mmap()).toThrowError(TypeError);
});

describe('should work', () => {
    expect(mmap('/dev/shm/xx')).toBeInstanceOf(Buffer);
});
