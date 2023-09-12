import { mmap } from '@cloudpss/mmap';
import fs from 'node:fs/promises';
import { randomBytes } from 'node:crypto';

describe('mmap', () => {
    it('should reject bad input', () => {
        // @ts-expect-error - bad input
        expect(() => mmap(1)).toThrowError(TypeError);
        // @ts-expect-error - bad input
        expect(() => mmap()).toThrowError(TypeError);
    });

    it('should work', async () => {
        const data = randomBytes(1024);
        await fs.writeFile('/dev/shm/xx', data);
        const mapped = mmap('/dev/shm/xx');
        expect(mapped).toBeInstanceOf(Buffer);
        expect(mapped).toHaveLength(1024);
        expect(mapped).toEqual(data);
    });
});
