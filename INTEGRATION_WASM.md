# XzalgoChain WebAssembly Integration Guide

This guide explains how to integrate XzalgoChain WebAssembly module into various JavaScript/TypeScript projects.

## Installation

### Copy Files Manually
Copy the generated files to your project:
```
your-project/
├── public/
│   ├── wasm/
│   │   ├── XzalgoChain.js
│   │   └── XzalgoChain.wasm
│   └── ...
└── src/
    └── ...
```

## Basic Integration

### Utility Functions
Create a utility file `xzalgochain-utils.js`:

```javascript
// xzalgochain-utils.js
let xzalgochain = null;
let wasmReady = false;

/**
 * Initialize WASM module
 * @returns {Promise<boolean>} - True if initialized successfully
 */
export async function initXzalgoChain() {
    if (wasmReady) return true;
    
    try {
        if (typeof XzalgoChain === 'undefined') {
            throw new Error('XzalgoChain module not loaded');
        }
        
        xzalgochain = await XzalgoChain();
        wasmReady = true;
        console.log('XzalgoChain WASM initialized');
        return true;
    } catch (error) {
        console.error('Failed to initialize XzalgoChain:', error);
        return false;
    }
}

/**
 * Convert string to UTF8 byte array
 * @param {string} str - Input string
 * @returns {Uint8Array} - UTF8 bytes
 */
export function stringToUTF8Array(str) {
    const utf8 = unescape(encodeURIComponent(str));
    const arr = new Uint8Array(utf8.length);
    for (let i = 0; i < utf8.length; i++) {
        arr[i] = utf8.charCodeAt(i);
    }
    return arr;
}

/**
 * Convert byte array to hex string
 * @param {Uint8Array} bytes - Byte array
 * @returns {string} - Hex string
 */
export function bytesToHex(bytes) {
    return Array.from(bytes)
        .map(b => (b + 256) % 256)
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
}

/**
 * Convert hex string to byte array
 * @param {string} hex - Hex string
 * @returns {Uint8Array} - Byte array
 */
export function hexToBytes(hex) {
    if (hex.length !== 80) throw new Error('Invalid hash length (must be 80 hex characters)');
    const bytes = new Uint8Array(40);
    for (let i = 0; i < 40; i++) {
        bytes[i] = parseInt(hex.substr(i * 2, 2), 16);
    }
    return bytes;
}

/**
 * Calculate hash from string
 * @param {string} input - Input string
 * @returns {Promise<string>} - Hex hash
 */
export async function hashString(input) {
    if (!wasmReady || !xzalgochain) {
        throw new Error('WASM not initialized. Call initXzalgoChain() first.');
    }
    
    const data = stringToUTF8Array(input);
    const output = new Uint8Array(40);
    
    const dataPtr = xzalgochain._malloc(data.length);
    const outputPtr = xzalgochain._malloc(40);
    
    try {
        // Write data to WASM memory
        for (let i = 0; i < data.length; i++) {
            xzalgochain.setValue(dataPtr + i, data[i], 'i8');
        }
        
        // Call hash function
        xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);
        
        // Read result
        for (let i = 0; i < 40; i++) {
            output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
        }
        
        return bytesToHex(output);
    } finally {
        xzalgochain._free(dataPtr);
        xzalgochain._free(outputPtr);
    }
}

/**
 * Calculate hash from file
 * @param {File} file - Input file
 * @returns {Promise<string>} - Hex hash
 */
export async function hashFile(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        
        reader.onload = async (e) => {
            try {
                const data = new Uint8Array(e.target.result);
                
                if (!wasmReady || !xzalgochain) {
                    throw new Error('WASM not initialized');
                }
                
                const output = new Uint8Array(40);
                const dataPtr = xzalgochain._malloc(data.length);
                const outputPtr = xzalgochain._malloc(40);
                
                try {
                    // Write data to WASM memory
                    for (let i = 0; i < data.length; i++) {
                        xzalgochain.setValue(dataPtr + i, data[i], 'i8');
                    }
                    
                    // Call hash function
                    xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);
                    
                    // Read result
                    for (let i = 0; i < 40; i++) {
                        output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
                    }
                    
                    resolve(bytesToHex(output));
                } finally {
                    xzalgochain._free(dataPtr);
                    xzalgochain._free(outputPtr);
                }
            } catch (error) {
                reject(error);
            }
        };
        
        reader.onerror = () => reject(reader.error);
        reader.readAsArrayBuffer(file);
    });
}

/**
 * Verify hash matches string
 * @param {string} input - Input string
 * @param {string} expectedHash - Expected hash (hex)
 * @returns {Promise<boolean>} - True if matches
 */
export async function verifyString(input, expectedHash) {
    const calculated = await hashString(input);
    return calculated.toLowerCase() === expectedHash.toLowerCase();
}

/**
 * Verify hash matches file
 * @param {File} file - Input file
 * @param {string} expectedHash - Expected hash (hex)
 * @returns {Promise<boolean>} - True if matches
 */
export async function verifyFile(file, expectedHash) {
    const calculated = await hashFile(file);
    return calculated.toLowerCase() === expectedHash.toLowerCase();
}

/**
 * Compare two hashes in constant time
 * @param {string} hash1 - First hash (hex)
 * @param {string} hash2 - Second hash (hex)
 * @returns {Promise<boolean>} - True if equal
 */
export async function compareHashes(hash1, hash2) {
    if (!wasmReady || !xzalgochain) {
        throw new Error('WASM not initialized');
    }
    
    const bytes1 = hexToBytes(hash1);
    const bytes2 = hexToBytes(hash2);
    
    const ptr1 = xzalgochain._malloc(40);
    const ptr2 = xzalgochain._malloc(40);
    
    try {
        // Copy bytes to WASM memory
        for (let i = 0; i < 40; i++) {
            xzalgochain.setValue(ptr1 + i, bytes1[i], 'i8');
            xzalgochain.setValue(ptr2 + i, bytes2[i], 'i8');
        }
        
        // Compare (returns 1 if equal)
        return xzalgochain._xzalgochain_equals_wasm(ptr1, ptr2) === 1;
    } finally {
        xzalgochain._free(ptr1);
        xzalgochain._free(ptr2);
    }
}

/**
 * Get library version
 * @returns {Promise<string>} - Version string
 */
export async function getVersion() {
    if (!wasmReady || !xzalgochain) {
        throw new Error('WASM not initialized');
    }
    
    const versionPtr = xzalgochain._xzalgochain_version_wasm();
    return xzalgochain.UTF8ToString(versionPtr);
}

// Export the module instance for advanced usage
export function getWasmModule() {
    return xzalgochain;
}
```

## Framework Integrations

### Vanilla JavaScript

```html
<!DOCTYPE html>
<html>
<head>
    <title>XzalgoChain Integration</title>
    <script src="wasm/XzalgoChain.js"></script>
    <script src="xzalgochain-utils.js"></script>
</head>
<body>
    <input type="text" id="input" value="Hello World">
    <button onclick="calculateHash()">Hash</button>
    <div id="output"></div>
    
    <script>
        async function calculateHash() {
            await initXzalgoChain();
            const input = document.getElementById('input').value;
            const hash = await hashString(input);
            document.getElementById('output').textContent = hash;
        }
    </script>
</body>
</html>
```

### React

Create `useXzalgoChain` hook:

```javascript
// hooks/useXzalgoChain.js
import { useState, useEffect } from 'react';
import { initXzalgoChain, hashString, hashFile, verifyString, verifyFile, getVersion } from '../utils/xzalgochain-utils';

export function useXzalgoChain() {
    const [isReady, setIsReady] = useState(false);
    const [version, setVersion] = useState('');
    const [error, setError] = useState(null);

    useEffect(() => {
        initialize();
    }, []);

    const initialize = async () => {
        try {
            const success = await initXzalgoChain();
            if (success) {
                const ver = await getVersion();
                setVersion(ver);
                setIsReady(true);
            } else {
                setError('Failed to initialize');
            }
        } catch (err) {
            setError(err.message);
        }
    };

    return {
        isReady,
        version,
        error,
        hashString,
        hashFile,
        verifyString,
        verifyFile
    };
}
```

React component example:

```jsx
// components/HashComponent.jsx
import React, { useState } from 'react';
import { useXzalgoChain } from '../hooks/useXzalgoChain';

export function HashComponent() {
    const { isReady, version, error, hashString, verifyString } = useXzalgoChain();
    const [input, setInput] = useState('');
    const [hash, setHash] = useState('');
    const [verifyHash, setVerifyHash] = useState('');
    const [verifyResult, setVerifyResult] = useState(null);

    const handleHash = async () => {
        if (!isReady) return;
        const result = await hashString(input);
        setHash(result);
    };

    const handleVerify = async () => {
        if (!isReady) return;
        const matches = await verifyString(input, verifyHash);
        setVerifyResult(matches);
    };

    if (error) return <div>Error: {error}</div>;
    if (!isReady) return <div>Loading XzalgoChain...</div>;

    return (
        <div>
            <h2>XzalgoChain Hash (v{version})</h2>
            
            <div>
                <input 
                    value={input}
                    onChange={(e) => setInput(e.target.value)}
                    placeholder="Enter text"
                />
                <button onClick={handleHash}>Generate Hash</button>
                {hash && <pre>{hash}</pre>}
            </div>
            
            <div>
                <input 
                    value={verifyHash}
                    onChange={(e) => setVerifyHash(e.target.value)}
                    placeholder="Enter hash to verify"
                />
                <button onClick={handleVerify}>Verify</button>
                {verifyResult !== null && (
                    <div style={{ color: verifyResult ? 'green' : 'red' }}>
                        {verifyResult ? '✓ Hash matches!' : '✗ Hash does not match'}
                    </div>
                )}
            </div>
        </div>
    );
}
```

### Vue.js

```vue
<!-- components/XzalgoHash.vue -->
<template>
  <div>
    <div v-if="error">Error: {{ error }}</div>
    <div v-else-if="!isReady">Loading XzalgoChain...</div>
    <div v-else>
      <h2>XzalgoChain Hash (v{{ version }})</h2>
      
      <div>
        <input v-model="input" placeholder="Enter text">
        <button @click="handleHash">Generate Hash</button>
        <pre v-if="hash">{{ hash }}</pre>
      </div>
      
      <div>
        <input v-model="verifyHash" placeholder="Enter hash to verify">
        <button @click="handleVerify">Verify</button>
        <div v-if="verifyResult !== null" :class="verifyResult ? 'success' : 'error'">
          {{ verifyResult ? '✓ Hash matches!' : '✗ Hash does not match' }}
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { initXzalgoChain, hashString, verifyString, getVersion } from '../utils/xzalgochain-utils';

export default {
  name: 'XzalgoHash',
  data() {
    return {
      isReady: false,
      version: '',
      error: null,
      input: '',
      hash: '',
      verifyHash: '',
      verifyResult: null
    };
  },
  async created() {
    try {
      const success = await initXzalgoChain();
      if (success) {
        this.version = await getVersion();
        this.isReady = true;
      } else {
        this.error = 'Failed to initialize';
      }
    } catch (err) {
      this.error = err.message;
    }
  },
  methods: {
    async handleHash() {
      this.hash = await hashString(this.input);
    },
    async handleVerify() {
      this.verifyResult = await verifyString(this.input, this.verifyHash);
    }
  }
};
</script>

<style scoped>
.success { color: green; }
.error { color: red; }
</style>
```

### Angular

Create a service:

```typescript
// services/xzalgochain.service.ts
import { Injectable } from '@angular/core';
import { initXzalgoChain, hashString, hashFile, verifyString, verifyFile, getVersion } from '../utils/xzalgochain-utils';

@Injectable({
  providedIn: 'root'
})
export class XzalgoChainService {
  private initialized = false;
  private version = '';

  async initialize(): Promise<boolean> {
    if (this.initialized) return true;
    
    try {
      const success = await initXzalgoChain();
      if (success) {
        this.version = await getVersion();
        this.initialized = true;
      }
      return success;
    } catch (error) {
      console.error('Failed to initialize XzalgoChain:', error);
      return false;
    }
  }

  async hashString(input: string): Promise<string> {
    await this.ensureInitialized();
    return hashString(input);
  }

  async hashFile(file: File): Promise<string> {
    await this.ensureInitialized();
    return hashFile(file);
  }

  async verifyString(input: string, hash: string): Promise<boolean> {
    await this.ensureInitialized();
    return verifyString(input, hash);
  }

  async verifyFile(file: File, hash: string): Promise<boolean> {
    await this.ensureInitialized();
    return verifyFile(file, hash);
  }

  getVersion(): string {
    return this.version;
  }

  private async ensureInitialized() {
    if (!this.initialized) {
      await this.initialize();
    }
  }
}
```

Angular component:

```typescript
// components/hash/hash.component.ts
import { Component } from '@angular/core';
import { XzalgoChainService } from '../../services/xzalgochain.service';

@Component({
  selector: 'app-hash',
  template: `
    <div *ngIf="!ready">Loading XzalgoChain...</div>
    <div *ngIf="ready">
      <h2>XzalgoChain Hash (v{{ version }})</h2>
      
      <div>
        <input [(ngModel)]="input" placeholder="Enter text">
        <button (click)="handleHash()">Generate Hash</button>
        <pre *ngIf="hash">{{ hash }}</pre>
      </div>
      
      <div>
        <input [(ngModel)]="verifyHash" placeholder="Enter hash to verify">
        <button (click)="handleVerify()">Verify</button>
        <div *ngIf="verifyResult !== null" [ngClass]="{'success': verifyResult, 'error': !verifyResult}">
          {{ verifyResult ? '✓ Hash matches!' : '✗ Hash does not match' }}
        </div>
      </div>
    </div>
  `,
  styles: ['.success { color: green; } .error { color: red; }']
})
export class HashComponent {
  ready = false;
  version = '';
  input = '';
  hash = '';
  verifyHash = '';
  verifyResult: boolean | null = null;

  constructor(private xzalgochain: XzalgoChainService) {}

  async ngOnInit() {
    await this.xzalgochain.initialize();
    this.ready = true;
    this.version = this.xzalgochain.getVersion();
  }

  async handleHash() {
    this.hash = await this.xzalgochain.hashString(this.input);
  }

  async handleVerify() {
    this.verifyResult = await this.xzalgochain.verifyString(this.input, this.verifyHash);
  }
}
```

### Node.js

```javascript
// xzalgochain-node.js
const fs = require('fs');
const path = require('path');

// Load WASM module
let xzalgochain = null;

async function initXzalgoChain() {
    if (xzalgochain) return xzalgochain;
    
    // Load the WASM file
    const wasmPath = path.join(__dirname, 'wasm', 'XzalgoChain.wasm');
    const wasmBuffer = fs.readFileSync(wasmPath);
    
    // Create WebAssembly module
    const wasmModule = await WebAssembly.compile(wasmBuffer);
    const wasmInstance = await WebAssembly.instantiate(wasmModule, {
        env: {
            memory: new WebAssembly.Memory({ initial: 256, maximum: 512 }),
            table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' }),
            __indirect_function_table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' }),
            emscripten_notify_memory_growth: () => {}
        }
    });
    
    xzalgochain = wasmInstance.exports;
    return xzalgochain;
}

// Utility functions (similar to browser version)
function stringToUTF8Array(str) {
    const encoder = new TextEncoder();
    return encoder.encode(str);
}

function bytesToHex(bytes) {
    return Array.from(bytes)
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
}

async function hashString(input) {
    const wasm = await initXzalgoChain();
    const data = stringToUTF8Array(input);
    const output = new Uint8Array(40);
    
    const dataPtr = wasm.malloc(data.length);
    const outputPtr = wasm.malloc(40);
    
    try {
        // Write data to WASM memory
        const memory = new Uint8Array(wasm.memory.buffer);
        memory.set(data, dataPtr);
        
        // Call hash function
        wasm._xzalgochain_wasm(dataPtr, data.length, outputPtr);
        
        // Read result
        output.set(memory.subarray(outputPtr, outputPtr + 40));
        
        return bytesToHex(output);
    } finally {
        wasm.free(dataPtr);
        wasm.free(outputPtr);
    }
}

async function hashFile(filePath) {
    const data = fs.readFileSync(filePath);
    const wasm = await initXzalgoChain();
    const output = new Uint8Array(40);
    
    const dataPtr = wasm.malloc(data.length);
    const outputPtr = wasm.malloc(40);
    
    try {
        const memory = new Uint8Array(wasm.memory.buffer);
        memory.set(data, dataPtr);
        
        wasm._xzalgochain_wasm(dataPtr, data.length, outputPtr);
        output.set(memory.subarray(outputPtr, outputPtr + 40));
        
        return bytesToHex(output);
    } finally {
        wasm.free(dataPtr);
        wasm.free(outputPtr);
    }
}

// Usage
async function main() {
    const hash = await hashString('Hello Node.js!');
    console.log('Hash:', hash);
    
    const fileHash = await hashFile('./test.txt');
    console.log('File hash:', fileHash);
}

main().catch(console.error);
```

### TypeScript

```typescript
// types/xzalgochain.d.ts
declare module 'xzalgochain' {
    export interface XzalgoChainModule {
        _malloc(size: number): number;
        _free(ptr: number): void;
        _xzalgochain_wasm(dataPtr: number, length: number, outputPtr: number): void;
        _xzalgochain_version_wasm(): number;
        _xzalgochain_equals_wasm(ptr1: number, ptr2: number): number;
        UTF8ToString(ptr: number): string;
        setValue(ptr: number, value: number, type: string): void;
        getValue(ptr: number, type: string): number;
    }

    export default function XzalgoChain(): Promise<XzalgoChainModule>;
}

// utils/xzalgochain-utils.ts
import type { XzalgoChainModule } from 'xzalgochain';

let xzalgochain: XzalgoChainModule | null = null;
let wasmReady = false;

export async function initXzalgoChain(): Promise<boolean> {
    if (wasmReady) return true;
    
    try {
        const XzalgoChain = (await import('xzalgochain')).default;
        xzalgochain = await XzalgoChain();
        wasmReady = true;
        return true;
    } catch (error) {
        console.error('Failed to initialize XzalgoChain:', error);
        return false;
    }
}

export function stringToUTF8Array(str: string): Uint8Array {
    const encoder = new TextEncoder();
    return encoder.encode(str);
}

export function bytesToHex(bytes: Uint8Array): string {
    return Array.from(bytes)
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
}

export function hexToBytes(hex: string): Uint8Array {
    if (hex.length !== 80) {
        throw new Error('Invalid hash length (must be 80 hex characters)');
    }
    const bytes = new Uint8Array(40);
    for (let i = 0; i < 40; i++) {
        bytes[i] = parseInt(hex.substr(i * 2, 2), 16);
    }
    return bytes;
}

export async function hashString(input: string): Promise<string> {
    if (!wasmReady || !xzalgochain) {
        throw new Error('WASM not initialized');
    }
    
    const data = stringToUTF8Array(input);
    const output = new Uint8Array(40);
    
    const dataPtr = xzalgochain._malloc(data.length);
    const outputPtr = xzalgochain._malloc(40);
    
    try {
        for (let i = 0; i < data.length; i++) {
            xzalgochain.setValue(dataPtr + i, data[i], 'i8');
        }
        
        xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);
        
        for (let i = 0; i < 40; i++) {
            output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
        }
        
        return bytesToHex(output);
    } finally {
        xzalgochain._free(dataPtr);
        xzalgochain._free(outputPtr);
    }
}

// ... other functions with TypeScript types
```

## Advanced Usage

### Context API for Large Data

```javascript
// xzalgochain-context.js
export class XzalgoChainHasher {
    constructor() {
        this.ctxPtr = null;
        this.initialized = false;
    }

    async init() {
        if (!wasmReady) await initXzalgoChain();
        this.ctxPtr = xzalgochain._malloc(256); // Allocate context
        xzalgochain._xzalgochain_init_wasm(this.ctxPtr);
        this.initialized = true;
    }

    async update(data) {
        if (!this.initialized) throw new Error('Hasher not initialized');
        
        const bytes = data instanceof Uint8Array ? data : stringToUTF8Array(data);
        const dataPtr = xzalgochain._malloc(bytes.length);
        
        try {
            for (let i = 0; i < bytes.length; i++) {
                xzalgochain.setValue(dataPtr + i, bytes[i], 'i8');
            }
            xzalgochain._xzalgochain_update_wasm(this.ctxPtr, dataPtr, bytes.length);
        } finally {
            xzalgochain._free(dataPtr);
        }
    }

    async final() {
        if (!this.initialized) throw new Error('Hasher not initialized');
        
        const output = new Uint8Array(40);
        const outputPtr = xzalgochain._malloc(40);
        
        try {
            xzalgochain._xzalgochain_final_wasm(this.ctxPtr, outputPtr);
            
            for (let i = 0; i < 40; i++) {
                output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
            }
            
            return bytesToHex(output);
        } finally {
            xzalgochain._free(outputPtr);
            xzalgochain._free(this.ctxPtr);
            this.initialized = false;
        }
    }

    async reset() {
        if (!this.initialized) throw new Error('Hasher not initialized');
        xzalgochain._xzalgochain_ctx_reset_wasm(this.ctxPtr);
    }

    async wipe() {
        if (!this.initialized) throw new Error('Hasher not initialized');
        xzalgochain._xzalgochain_ctx_wipe_wasm(this.ctxPtr);
    }
}

// Usage
async function hashLargeFile(file) {
    const hasher = new XzalgoChainHasher();
    await hasher.init();
    
    const reader = file.stream().getReader();
    const chunkSize = 64 * 1024; // 64KB chunks
    
    while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        
        // Process chunk
        for (let i = 0; i < value.length; i += chunkSize) {
            const chunk = value.slice(i, i + chunkSize);
            await hasher.update(chunk);
        }
    }
    
    return await hasher.final();
}
```

### Worker Threads

```javascript
// xzalgochain.worker.js
importScripts('./wasm/XzalgoChain.js');

let xzalgochain = null;

async function init() {
    xzalgochain = await XzalgoChain();
    postMessage({ type: 'ready' });
}

init();

self.onmessage = async (e) => {
    const { id, type, data } = e.data;
    
    try {
        let result;
        
        if (type === 'hash') {
            const { input, isFile } = data;
            
            if (isFile) {
                // Handle file data (base64 or array buffer)
                const bytes = new Uint8Array(input);
                result = await calculateFileHash(bytes);
            } else {
                result = await calculateStringHash(input);
            }
        } else if (type === 'verify') {
            const { input, hash, isFile } = data;
            const calculated = isFile 
                ? await calculateFileHash(new Uint8Array(input))
                : await calculateStringHash(input);
            result = calculated.toLowerCase() === hash.toLowerCase();
        }
        
        postMessage({ id, type: 'success', result });
    } catch (error) {
        postMessage({ id, type: 'error', error: error.message });
    }
};

async function calculateStringHash(str) {
    const data = stringToUTF8Array(str);
    return calculateHash(data);
}

async function calculateFileHash(bytes) {
    return calculateHash(bytes);
}

async function calculateHash(data) {
    const output = new Uint8Array(40);
    const dataPtr = xzalgochain._malloc(data.length);
    const outputPtr = xzalgochain._malloc(40);
    
    try {
        for (let i = 0; i < data.length; i++) {
            xzalgochain.setValue(dataPtr + i, data[i], 'i8');
        }
        
        xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);
        
        for (let i = 0; i < 40; i++) {
            output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
        }
        
        return bytesToHex(output);
    } finally {
        xzalgochain._free(dataPtr);
        xzalgochain._free(outputPtr);
    }
}

function stringToUTF8Array(str) {
    const utf8 = unescape(encodeURIComponent(str));
    const arr = new Uint8Array(utf8.length);
    for (let i = 0; i < utf8.length; i++) {
        arr[i] = utf8.charCodeAt(i);
    }
    return arr;
}

function bytesToHex(bytes) {
    return Array.from(bytes)
        .map(b => (b + 256) % 256)
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
}
```

### Stream Processing

```javascript
// xzalgochain-stream.js
import { Readable, Writable, Transform } from 'stream';
import { XzalgoChainHasher } from './xzalgochain-context.js';

export class XzalgoChainHashStream extends Transform {
    constructor() {
        super();
        this.hasher = new XzalgoChainHasher();
        this.initialized = false;
        this.finalized = false;
    }

    async _construct(callback) {
        try {
            await this.hasher.init();
            this.initialized = true;
            callback();
        } catch (err) {
            callback(err);
        }
    }

    async _transform(chunk, encoding, callback) {
        try {
            if (!this.initialized) {
                throw new Error('Stream not initialized');
            }
            
            if (this.finalized) {
                throw new Error('Hash already finalized');
            }
            
            await this.hasher.update(chunk);
            callback();
        } catch (err) {
            callback(err);
        }
    }

    async _flush(callback) {
        try {
            if (!this.initialized) {
                throw new Error('Stream not initialized');
            }
            
            const hash = await this.hasher.final();
            this.finalized = true;
            this.push(hash);
            callback();
        } catch (err) {
            callback(err);
        }
    }
}

// Usage with file streams
import { createReadStream } from 'fs';
import { pipeline } from 'stream/promises';

async function hashFileStream(filePath) {
    const hashStream = new XzalgoChainHashStream();
    
    await pipeline(
        createReadStream(filePath),
        hashStream
    );
    
    return hashStream.read();
}
```

## Performance Optimization

### Batch Processing

```javascript
// Process multiple inputs efficiently
export async function hashBatch(inputs) {
    if (!wasmReady || !xzalgochain) {
        throw new Error('WASM not initialized');
    }
    
    const results = [];
    
    // Pre-allocate output buffer once
    const output = new Uint8Array(40);
    const outputPtr = xzalgochain._malloc(40);
    
    try {
        for (const input of inputs) {
            const data = stringToUTF8Array(input);
            const dataPtr = xzalgochain._malloc(data.length);
            
            try {
                for (let i = 0; i < data.length; i++) {
                    xzalgochain.setValue(dataPtr + i, data[i], 'i8');
                }
                
                xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);
                
                for (let i = 0; i < 40; i++) {
                    output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
                }
                
                results.push(bytesToHex(output));
            } finally {
                xzalgochain._free(dataPtr);
            }
        }
    } finally {
        xzalgochain._free(outputPtr);
    }
    
    return results;
}
```

### Memory Pool

```javascript
// Memory pool for frequent allocations
export class MemoryPool {
    constructor(wasm, blockSize, maxBlocks = 10) {
        this.wasm = wasm;
        this.blockSize = blockSize;
        this.pool = [];
        this.maxBlocks = maxBlocks;
    }
    
    allocate() {
        if (this.pool.length > 0) {
            return this.pool.pop();
        }
        return this.wasm._malloc(this.blockSize);
    }
    
    release(ptr) {
        if (this.pool.length < this.maxBlocks) {
            this.pool.push(ptr);
        } else {
            this.wasm._free(ptr);
        }
    }
    
    cleanup() {
        while (this.pool.length > 0) {
            this.wasm._free(this.pool.pop());
        }
    }
}

// Usage
const outputPool = new MemoryPool(xzalgochain, 40);
const dataPtr = xzalgochain._malloc(data.length);
const outputPtr = outputPool.allocate();

try {
    // ... use pointers
} finally {
    xzalgochain._free(dataPtr);
    outputPool.release(outputPtr);
}
```

## Troubleshooting

### Common Issues and Solutions

1. **WASM not loading**
```javascript
// Check if file exists
fetch('./wasm/XzalgoChain.wasm')
    .then(response => {
        if (!response.ok) {
            throw new Error('WASM file not found');
        }
        return response;
    })
    .catch(error => {
        console.error('Failed to load WASM:', error);
    });
```

2. **Memory access errors**
```javascript
// Verify memory is accessible
function testMemoryAccess() {
    const testPtr = xzalgochain._malloc(1);
    try {
        xzalgochain.setValue(testPtr, 42, 'i8');
        const value = xzalgochain.getValue(testPtr, 'i8');
        console.assert(value === 42, 'Memory access working');
    } finally {
        xzalgochain._free(testPtr);
    }
}
```

3. **Initialization timeout**
```javascript
// Add timeout to initialization
async function initWithTimeout(timeout = 10000) {
    const initPromise = initXzalgoChain();
    const timeoutPromise = new Promise((_, reject) => {
        setTimeout(() => reject(new Error('Initialization timeout')), timeout);
    });
    
    return Promise.race([initPromise, timeoutPromise]);
}
```

### Debug Mode

```javascript
// Enable debug logging
const DEBUG = true;

function debugLog(...args) {
    if (DEBUG) {
        console.log('[XzalgoChain Debug]', ...args);
    }
}

// Monitor memory usage
function logMemoryUsage() {
    if (xzalgochain && xzalgochain.HEAP8) {
        debugLog('Memory used:', xzalgochain.HEAP8.length, 'bytes');
    }
}
```

## Examples

### Complete Web Application Example

```html
<!-- index.html -->
<!DOCTYPE html>
<html>
<head>
    <title>XzalgoChain File Integrity Checker</title>
    <style>
        body { font-family: Arial; margin: 20px; }
        .container { max-width: 800px; margin: 0 auto; }
        .card { border: 1px solid #ccc; padding: 20px; margin: 20px 0; }
        .hash { word-break: break-all; background: #f5f5f5; padding: 10px; }
        .success { color: green; }
        .error { color: red; }
    </style>
</head>
<body>
    <div class="container">
        <h1>XzalgoChain File Integrity Checker</h1>
        
        <div class="card">
            <h2>Generate File Hash</h2>
            <input type="file" id="fileInput">
            <button onclick="generateHash()">Generate</button>
            <div id="fileHash" class="hash"></div>
        </div>
        
        <div class="card">
            <h2>Verify File Integrity</h2>
            <input type="file" id="verifyFileInput">
            <input type="text" id="expectedHash" placeholder="Expected hash">
            <button onclick="verifyFile()">Verify</button>
            <div id="verifyResult"></div>
        </div>
        
        <div class="card">
            <h2>Batch Generate</h2>
            <input type="file" id="batchFileInput" multiple>
            <button onclick="batchGenerate()">Generate All</button>
            <div id="batchResults"></div>
        </div>
    </div>
    
    <script src="wasm/XzalgoChain.js"></script>
    <script src="xzalgochain-utils.js"></script>
    <script>
        let ready = false;
        
        window.onload = async () => {
            await initXzalgoChain();
            ready = true;
        };
        
        async function generateHash() {
            if (!ready) return alert('Please wait for initialization');
            
            const file = document.getElementById('fileInput').files[0];
            if (!file) return alert('Select a file');
            
            const hash = await hashFile(file);
            document.getElementById('fileHash').textContent = hash;
        }
        
        async function verifyFile() {
            if (!ready) return alert('Please wait for initialization');
            
            const file = document.getElementById('verifyFileInput').files[0];
            const expected = document.getElementById('expectedHash').value;
            
            if (!file || !expected) {
                return alert('Select file and enter hash');
            }
            
            const matches = await verifyFile(file, expected);
            const resultDiv = document.getElementById('verifyResult');
            
            resultDiv.innerHTML = matches 
                ? '<span class="success">✓ File integrity verified</span>'
                : '<span class="error">✗ File has been modified</span>';
        }
        
        async function batchGenerate() {
            if (!ready) return alert('Please wait for initialization');
            
            const files = document.getElementById('batchFileInput').files;
            if (files.length === 0) return alert('Select files');
            
            const results = [];
            for (let file of files) {
                const hash = await hashFile(file);
                results.push(`${file.name}: ${hash}`);
            }
            
            document.getElementById('batchResults').innerHTML = 
                results.map(r => `<div class="hash">${r}</div>`).join('');
        }
    </script>
</body>
</html>
```

### Command Line Tool

```javascript
#!/usr/bin/env node
// xzalgochain-cli.js

const fs = require('fs');
const path = require('path');
const { program } = require('commander');

program
    .description('XzalgoChain CLI tool');

program
    .command('hash <file>')
    .description('Calculate hash of a file')
    .action(async (file) => {
        try {
            const { hashFile } = require('./xzalgochain-node');
            const hash = await hashFile(file);
            console.log(hash);
        } catch (error) {
            console.error('Error:', error.message);
        }
    });

program
    .command('verify <file> <hash>')
    .description('Verify file integrity')
    .action(async (file, hash) => {
        try {
            const { hashFile } = require('./xzalgochain-node');
            const calculated = await hashFile(file);
            
            if (calculated.toLowerCase() === hash.toLowerCase()) {
                console.log('✓ File integrity verified');
                process.exit(0);
            } else {
                console.log('✗ File has been modified');
                console.log('Expected:', hash);
                console.log('Calculated:', calculated);
                process.exit(1);
            }
        } catch (error) {
            console.error('Error:', error.message);
            process.exit(1);
        }
    });

program.parse(process.argv);
```

---

## Support

For issues and questions:
- GitHub: https://github.com/Xzray03/XzalgoChain
- Email: Xzray@proton.me

## License

Apache 2.0 - Copyright 2026 Xzrayツ
