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
});
