# @cloudpss/mmap

[![check](https://img.shields.io/github/actions/workflow/status/CloudPSS/mmap/check.yml?event=push&logo=github)](https://github.com/CloudPSS/mmap/actions/workflows/check.yml)
[![Codacy coverage](https://img.shields.io/codacy/coverage/c0b6811e7e5f45eeb46383607cac81a8?logo=jest)](https://app.codacy.com/gh/CloudPSS/mmap/dashboard)
[![Codacy Badge](https://img.shields.io/codacy/grade/c0b6811e7e5f45eeb46383607cac81a8?logo=codacy)](https://app.codacy.com/gh/CloudPSS/mmap/dashboard)
[![npm version](https://img.shields.io/npm/v/@cloudpss/mmap?logo=npm)](https://npmjs.org/package/@cloudpss/mmap)

This is a [pure esm package](https://gist.github.com/sindresorhus/a39789f98801d908bbc7ff3ecc99d99c) contains the mmap n-api addon for node.js.

## Installation

```bash
npm install @cloudpss/mmap
```

## Usage

```js
import { mmap } from '@cloudpss/mmap';

const buffer = mmap('/file/to/mmap');
```

## API

### `mmap(file: string, length?: number): Buffer`

## License

MIT
