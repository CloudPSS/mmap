import { mmap } from '@cloudpss/mmap';
import fs from 'node:fs/promises';
import { randomBytes } from 'node:crypto';

describe('mmap', () => {
    it('should reject bad input', () => {
        // @ts-expect-error - bad input
        expect(() => mmap(1)).toThrow(TypeError);
        // @ts-expect-error - bad input
        expect(() => mmap()).toThrow(TypeError);
    });

    it('should work', async () => {
        const data = randomBytes(1024);
        await fs.writeFile('/dev/shm/xx', data);
        const mapped = mmap('/dev/shm/xx');
        expect(mapped).toBeInstanceOf(Buffer);
        expect(mapped).toHaveLength(1024);
        expect(mapped).toEqual(data);
    });

    it('should work with empty file', async () => {
        const data = randomBytes(0);
        await fs.writeFile('/dev/shm/xx', data);
        const mapped = mmap('/dev/shm/xx');
        expect(mapped).toBeInstanceOf(Buffer);
        expect(mapped).toHaveLength(0);
        expect(mapped).toEqual(data);
    });

    it('should work with smaller size', async () => {
        const data = randomBytes(1024);
        await fs.writeFile('/dev/shm/yy', data);
        const mapped = mmap('/dev/shm/yy', 128);
        expect(mapped).toBeInstanceOf(Buffer);
        expect(mapped).toHaveLength(128);
        expect(mapped).toEqual(data.subarray(0, 128));
    });

    it('should work with lager size', async () => {
        const data = randomBytes(1024);
        await fs.writeFile('/dev/shm/zz', data);
        const mapped = mmap('/dev/shm/zz', 2048);
        expect(mapped).toBeInstanceOf(Buffer);
        expect(mapped).toHaveLength(2048);
        expect(mapped.subarray(0, 1024)).toEqual(data);
        expect(mapped.subarray(1024)).toEqual(Buffer.alloc(1024));
        expect((await fs.stat('/dev/shm/zz')).size).toBe(2048);
    });

    it('should accept custom view', async () => {
        const data = randomBytes(1024);
        await fs.writeFile('/dev/shm/xx', data);
        const mapped = mmap(DataView, '/dev/shm/xx');
        expect(mapped).toBeInstanceOf(DataView);
        expect(mapped.byteLength).toBe(1024);
        expect(mapped.buffer).toEqual(data.buffer);
    });

    it('should accept custom view', async () => {
        const data = randomBytes(1024);
        await fs.writeFile('/dev/shm/xx', data);
        const mapped = mmap(Int32Array, '/dev/shm/xx', 6);
        expect(mapped).toBeInstanceOf(Int32Array);
        expect(mapped).toHaveLength(1);
        expect(mapped.byteLength).toBe(4);
    });
});
