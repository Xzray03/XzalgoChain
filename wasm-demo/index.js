// XzalgoChain Demo Application
// Copyright 2026 Xzrayツ

let xzalgochain = null;
let wasmReady = false;

// Recent hashes storage
let recentHashes = JSON.parse(localStorage.getItem('xzalgochain_recent') || '[]');

// DOM Elements
const elements = {
    wasmStatus: document.getElementById('wasm-status'),
    version: document.getElementById('version'),

    // String hash
    stringInput: document.getElementById('string-input'),
    hashStringBtn: document.getElementById('hash-string-btn'),
    stringHashOutput: document.getElementById('string-hash-output'),

    // File hash
    fileInput: document.getElementById('file-input'),
    fileInfo: document.getElementById('file-info'),
    hashFileBtn: document.getElementById('hash-file-btn'),
    fileHashOutput: document.getElementById('file-hash-output'),

    // Verify string
    verifyStringInput: document.getElementById('verify-string-input'),
    verifyStringHash: document.getElementById('verify-string-hash'),
    verifyStringBtn: document.getElementById('verify-string-btn'),
    verifyStringResult: document.getElementById('verify-string-result'),

    // Verify file
    verifyFileInput: document.getElementById('verify-file-input'),
    verifyFileInfo: document.getElementById('verify-file-info'),
    verifyFileHash: document.getElementById('verify-file-hash'),
    verifyFileBtn: document.getElementById('verify-file-btn'),
    verifyFileResult: document.getElementById('verify-file-result'),

    // Recent
    recentList: document.getElementById('recent-list'),
    clearRecent: document.getElementById('clear-recent')
};

// Initialize WASM module
async function initWASM() {
    try {
        if (typeof XzalgoChain === 'undefined') {
            throw new Error('XzalgoChain module not loaded');
        }

        console.log('Loading XzalgoChain module...');
        xzalgochain = await XzalgoChain();

        // Debug: lihat struktur object
        console.log('XzalgoChain module loaded:', xzalgochain);
        console.log('Available functions:', Object.keys(xzalgochain).filter(key => key.startsWith('_')).sort());

        // Get version
        if (typeof xzalgochain._xzalgochain_version_wasm === 'function') {
            const versionPtr = xzalgochain._xzalgochain_version_wasm();
            const version = xzalgochain.UTF8ToString(versionPtr);
            elements.version.textContent = version;
        } else {
            elements.version.textContent = 'unknown';
        }

        // Update status
        elements.wasmStatus.textContent = 'Ready';
        elements.wasmStatus.classList.remove('loading');
        elements.wasmStatus.classList.add('ready');

        wasmReady = true;

        // Enable buttons
        enableButtons(true);

        console.log('XzalgoChain WASM initialized successfully');
    } catch (error) {
        console.error('Failed to initialize WASM:', error);
        elements.wasmStatus.textContent = 'Failed to load';
        elements.wasmStatus.classList.remove('loading');
        elements.wasmStatus.classList.add('error');
        enableButtons(false);
    }
}

// Enable/disable buttons based on WASM readiness
function enableButtons(enable) {
    const buttons = [
        elements.hashStringBtn,
        elements.hashFileBtn,
        elements.verifyStringBtn,
        elements.verifyFileBtn
    ];

    buttons.forEach(btn => {
        if (btn) btn.disabled = !enable;
    });
}

// Helper function to convert string to UTF8 array
function stringToUTF8Array(str) {
    const utf8 = unescape(encodeURIComponent(str));
    const arr = new Uint8Array(utf8.length);
    for (let i = 0; i < utf8.length; i++) {
        arr[i] = utf8.charCodeAt(i);
    }
    return arr;
}

// ==================== Clipboard Helper ====================

// Copy text to clipboard
async function copyToClipboard(text) {
    try {
        // Modern approach
        if (navigator.clipboard && navigator.clipboard.writeText) {
            await navigator.clipboard.writeText(text);
            return true;
        }

        // Fallback for older browsers
        const textArea = document.createElement('textarea');
        textArea.value = text;
        document.body.appendChild(textArea);
        textArea.select();
        document.execCommand('copy');
        document.body.removeChild(textArea);
        return true;
    } catch (err) {
        console.error('Failed to copy:', err);
        return false;
    }
}

// Show copy notification
function showCopyNotification(element, message = '✓ Copied!') {
    const originalText = element.textContent;
    const originalColor = element.style.backgroundColor;

    element.textContent = message;
    element.style.backgroundColor = '#4CAF50';
    element.style.color = 'white';
    element.style.transition = 'all 0.3s ease';

    setTimeout(() => {
        element.textContent = originalText;
        element.style.backgroundColor = originalColor;
        element.style.color = '';
    }, 2000);
}

// Calculate hash from string using cwrap
async function calculateHash(str) {
    if (!wasmReady || !xzalgochain) {
        throw new Error('WASM not ready');
    }

    if (typeof xzalgochain._xzalgochain_wasm !== 'function') {
        throw new Error('Hash function not found in module');
    }

    // Convert string to UTF8 array
    const data = stringToUTF8Array(str);
    const output = new Uint8Array(40); // 320 bits = 40 bytes

    // Allocate memory
    const dataPtr = xzalgochain._malloc(data.length);
    const outputPtr = xzalgochain._malloc(40);

    if (!dataPtr || !outputPtr) {
        throw new Error('Failed to allocate memory');
    }

    try {
        // Write data to WASM memory using stringToUTF8
        for (let i = 0; i < data.length; i++) {
            xzalgochain.setValue(dataPtr + i, data[i], 'i8');
        }

        // Call hash function
        xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);

        // Read result back
        for (let i = 0; i < 40; i++) {
            output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
        }

        // Convert to hex string
        return Array.from(output)
        .map(b => (b + 256) % 256) // Handle negative values
        .map(b => b.toString(16).padStart(2, '0'))
        .join('');
    } finally {
        // Free memory
        xzalgochain._free(dataPtr);
        xzalgochain._free(outputPtr);
    }
}

// Calculate hash from file
async function calculateFileHash(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();

        reader.onload = async function(e) {
            try {
                const arrayBuffer = e.target.result;
                const data = new Uint8Array(arrayBuffer);

                if (!wasmReady || !xzalgochain) {
                    throw new Error('WASM not ready');
                }

                if (typeof xzalgochain._xzalgochain_wasm !== 'function') {
                    throw new Error('Hash function not found in module');
                }

                const output = new Uint8Array(40);

                // Allocate memory
                const dataPtr = xzalgochain._malloc(data.length);
                const outputPtr = xzalgochain._malloc(40);

                if (!dataPtr || !outputPtr) {
                    throw new Error('Failed to allocate memory');
                }

                try {
                    // Write data to WASM memory using setValue
                    for (let i = 0; i < data.length; i++) {
                        xzalgochain.setValue(dataPtr + i, data[i], 'i8');
                    }

                    // Call hash function
                    xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);

                    // Read result back
                    for (let i = 0; i < 40; i++) {
                        output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
                    }

                    // Convert to hex string
                    const hashHex = Array.from(output)
                    .map(b => (b + 256) % 256) // Handle negative values
                    .map(b => b.toString(16).padStart(2, '0'))
                    .join('');

                    resolve(hashHex);
                } finally {
                    // Free memory
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

// Add hash to recent list
function addToRecent(type, input, hash) {
    const timestamp = new Date().toLocaleTimeString();
    const preview = input.length > 30 ? input.substring(0, 30) + '...' : input;

    recentHashes.unshift({
        type,
        input: preview,
        fullInput: input,
        hash,
        timestamp
    });

    // Keep only last 10
    if (recentHashes.length > 10) {
        recentHashes.pop();
    }

    // Save to localStorage
    localStorage.setItem('xzalgochain_recent', JSON.stringify(recentHashes));

    updateRecentList();
}

// Update recent list UI
function updateRecentList() {
    if (recentHashes.length === 0) {
        elements.recentList.innerHTML = '<div class="recent-empty">No hashes generated yet</div>';
        return;
    }

    elements.recentList.innerHTML = recentHashes.map(item => `
    <div class="recent-item" data-hash="${item.hash}" data-input="${item.fullInput || item.input}">
    <span class="recent-preview">${item.input}</span>
    <span class="recent-type">${item.type === 'text' ? '📝' : '📁'}</span>
    </div>
    `).join('');

    // Add click handlers
    document.querySelectorAll('.recent-item').forEach(el => {
        el.addEventListener('click', () => {
            const hash = el.dataset.hash;
            const input = el.dataset.input;

            // Fill appropriate fields based on type
            if (el.querySelector('.recent-type').textContent === '📝') {
                elements.verifyStringInput.value = input;
                elements.verifyStringHash.value = hash;
            } else {
                elements.verifyFileHash.value = hash;
            }
        });
    });
}

// Show verification result
function showVerifyResult(element, success, message) {
    element.textContent = message;
    element.className = 'verify-result ' + (success ? 'success' : 'error');
}

// ==================== Event Handlers ====================

// Hash string button
elements.hashStringBtn.addEventListener('click', async () => {
    const input = elements.stringInput.value.trim();
    if (!input) {
        alert('Please enter a string to hash');
        return;
    }

    try {
        elements.hashStringBtn.disabled = true;
        elements.hashStringBtn.textContent = 'Hashing...';

        const hash = await calculateHash(input);
        elements.stringHashOutput.textContent = hash;

        // Auto copy to clipboard
        const copied = await copyToClipboard(hash);
        if (copied) {
            showCopyNotification(elements.stringHashOutput);
        }

        addToRecent('text', input, hash);
    } catch (error) {
        console.error('Hash error:', error);
        alert('Error calculating hash: ' + error.message);
    } finally {
        elements.hashStringBtn.disabled = false;
        elements.hashStringBtn.textContent = 'Generate Hash';
    }
});

// File input change
elements.fileInput.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (file) {
        elements.fileInfo.textContent = `📄 ${file.name} (${(file.size / 1024).toFixed(2)} KB)`;
        elements.hashFileBtn.disabled = false;
    } else {
        elements.fileInfo.textContent = 'No file selected';
        elements.hashFileBtn.disabled = true;
    }
});

// Hash file button
elements.hashFileBtn.addEventListener('click', async () => {
    const file = elements.fileInput.files[0];
    if (!file) return;

    try {
        elements.hashFileBtn.disabled = true;
        elements.hashFileBtn.textContent = 'Hashing...';

        const hash = await calculateFileHash(file);
        elements.fileHashOutput.textContent = hash;

        // Auto copy to clipboard
        const copied = await copyToClipboard(hash);
        if (copied) {
            showCopyNotification(elements.fileHashOutput);
        }

        addToRecent('file', file.name, hash);
    } catch (error) {
        console.error('File hash error:', error);
        alert('Error calculating file hash: ' + error.message);
    } finally {
        elements.hashFileBtn.disabled = false;
        elements.hashFileBtn.textContent = 'Generate Hash';
    }
});

// Verify string button
elements.verifyStringBtn.addEventListener('click', async () => {
    const input = elements.verifyStringInput.value.trim();
    const hashToVerify = elements.verifyStringHash.value.trim();

    if (!input || !hashToVerify) {
        showVerifyResult(elements.verifyStringResult, false, 'Please enter both string and hash');
        return;
    }

    try {
        elements.verifyStringBtn.disabled = true;
        elements.verifyStringBtn.textContent = 'Verifying...';

        const calculatedHash = await calculateHash(input);
        const match = calculatedHash.toLowerCase() === hashToVerify.toLowerCase();

        if (match) {
            showVerifyResult(elements.verifyStringResult, true, '✓ Hash matches!');
        } else {
            showVerifyResult(elements.verifyStringResult, false, '✗ Hash does not match');

            // Optionally show the calculated hash for debugging
            console.log('Calculated hash:', calculatedHash);
            console.log('Provided hash:  ', hashToVerify.toLowerCase());
        }
    } catch (error) {
        console.error('Verify error:', error);
        showVerifyResult(elements.verifyStringResult, false, 'Error: ' + error.message);
    } finally {
        elements.verifyStringBtn.disabled = false;
        elements.verifyStringBtn.textContent = 'Verify';
    }
});

// Verify file input change
elements.verifyFileInput.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (file) {
        elements.verifyFileInfo.textContent = `📄 ${file.name} (${(file.size / 1024).toFixed(2)} KB)`;
        elements.verifyFileBtn.disabled = false;
    } else {
        elements.verifyFileInfo.textContent = 'No file selected';
        elements.verifyFileBtn.disabled = true;
    }
});

// Verify file button
elements.verifyFileBtn.addEventListener('click', async () => {
    const file = elements.verifyFileInput.files[0];
    const hashToVerify = elements.verifyFileHash.value.trim();

    if (!file || !hashToVerify) {
        showVerifyResult(elements.verifyFileResult, false, 'Please select file and enter hash');
        return;
    }

    // Validate hash format
    if (!/^[0-9a-fA-F]{80}$/.test(hashToVerify)) {
        showVerifyResult(elements.verifyFileResult, false, 'Invalid hash format (should be 80 hex characters)');
        return;
    }

    try {
        elements.verifyFileBtn.disabled = true;
        elements.verifyFileBtn.textContent = 'Verifying...';

        const calculatedHash = await calculateFileHash(file);
        const match = calculatedHash.toLowerCase() === hashToVerify.toLowerCase();

        if (match) {
            showVerifyResult(elements.verifyFileResult, true, '✓ Hash matches!');
        } else {
            showVerifyResult(elements.verifyFileResult, false, '✗ Hash does not match');

            // Optionally show the calculated hash for debugging
            console.log('Calculated hash:', calculatedHash);
            console.log('Provided hash:  ', hashToVerify.toLowerCase());
        }
    } catch (error) {
        console.error('Verify error:', error);
        showVerifyResult(elements.verifyFileResult, false, 'Error: ' + error.message);
    } finally {
        elements.verifyFileBtn.disabled = false;
        elements.verifyFileBtn.textContent = 'Verify';
    }
});

// Clear recent button
elements.clearRecent.addEventListener('click', () => {
    recentHashes = [];
    localStorage.removeItem('xzalgochain_recent');
    updateRecentList();
});

// Initialize on page load
window.addEventListener('load', initWASM);
