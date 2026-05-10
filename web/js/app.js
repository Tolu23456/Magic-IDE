class MagicIDE {
    constructor() {
        this.currentDirectory = '.';
        this.currentFileIndex = null;
        this.openFiles = [];
        this.editor = null;
        this.searchMatches = [];
        this.searchIndex = 0;
        this.init();
    }

    init() {
        this.bindEvents();
        this.initializeMonaco()
            .then(() => {
                this.setStatusMessage('Monaco Editor ready');
                this.loadWorkspace();
            })
            .catch(error => {
                console.warn('Monaco failed to initialize:', error);
                this.initializeTextareaFallback();
                this.setStatusMessage('Loaded fallback editor');
                this.loadWorkspace();
            });
    }

    bindEvents() {
        document.getElementById('new-file').addEventListener('click', () => this.newFile());
        document.getElementById('open-file').addEventListener('click', () => this.openFile());
        document.getElementById('save-file').addEventListener('click', () => this.saveFile());
        document.getElementById('find-replace').addEventListener('click', () => this.toggleModal('find-replace-modal', true));
        document.getElementById('settings').addEventListener('click', () => this.toggleModal('settings-modal', true));
        document.getElementById('close-find-replace').addEventListener('click', () => this.toggleModal('find-replace-modal', false));
        document.getElementById('close-settings').addEventListener('click', () => this.toggleModal('settings-modal', false));
        document.getElementById('find-next').addEventListener('click', () => this.findNext());
        document.getElementById('replace').addEventListener('click', () => this.replaceSelection());
        document.getElementById('replace-all').addEventListener('click', () => this.replaceAll());
    }

    waitForMagicIDE() {
        return new Promise(resolve => {
            if (window.magicIDE && window.magicIDE.listDirectory) {
                resolve(window.magicIDE);
                return;
            }

            let attempts = 0;
            const poll = setInterval(() => {
                if (window.magicIDE && window.magicIDE.listDirectory) {
                    clearInterval(poll);
                    resolve(window.magicIDE);
                } else if (++attempts > 50) {
                    clearInterval(poll);
                    resolve(null);
                }
            }, 100);
        });
    }

    initializeMonaco() {
        return new Promise((resolve, reject) => {
            if (!window.require) {
                reject(new Error('Monaco loader not available'));
                return;
            }

            window.require.config({ paths: { vs: 'https://unpkg.com/monaco-editor@0.45.0/min/vs' } });
            window.require(['vs/editor/editor.main'], () => {
                this.editor = monaco.editor.create(document.getElementById('monaco-editor'), {
                    value: '',
                    language: 'javascript',
                    theme: 'vs-dark',
                    automaticLayout: true,
                    minimap: { enabled: false },
                    fontFamily: 'Consolas, Monaco, monospace',
                    fontSize: 14,
                    lineNumbers: 'on',
                    roundedSelection: false
                });

                this.editor.onDidChangeModelContent(() => {
                    if (this.currentFileIndex !== null) {
                        const file = this.openFiles[this.currentFileIndex];
                        if (file) {
                            file.dirty = this.editor.getValue() !== file.content;
                            this.updateTabs();
                        }
                    }
                });

                this.editor.onDidChangeCursorPosition(event => {
                    const position = event.position;
                    this.setCursorPosition(position.lineNumber, position.column);
                });

                resolve();
            }, reject);
        });
    }

    initializeTextareaFallback() {
        const editorWrapper = document.getElementById('monaco-editor');
        editorWrapper.innerHTML = '';
        this.editor = document.createElement('textarea');
        this.editor.id = 'fallback-editor';
        this.editor.style.width = '100%';
        this.editor.style.height = '100%';
        this.editor.style.border = 'none';
        this.editor.style.outline = 'none';
        this.editor.style.resize = 'none';
        this.editor.style.fontFamily = 'Consolas, Monaco, monospace';
        this.editor.style.fontSize = '14px';
        this.editor.style.lineHeight = '1.4';
        this.editor.placeholder = 'Start coding here...';
        this.editor.addEventListener('input', () => {
            if (this.currentFileIndex !== null) {
                const file = this.openFiles[this.currentFileIndex];
                if (file) {
                    file.dirty = this.editor.value !== file.content;
                    this.updateTabs();
                }
            }
        });
        editorWrapper.appendChild(this.editor);
    }

    setEditorContent(value) {
        if (!this.editor) {
            return;
        }

        if (this.editor.getModel && typeof this.editor.getModel === 'function') {
            this.editor.setValue(value);
        } else {
            this.editor.value = value;
        }
    }

    getEditorContent() {
        if (!this.editor) {
            return '';
        }

        if (this.editor.getValue && typeof this.editor.getValue === 'function') {
            return this.editor.getValue();
        }

        return this.editor.value;
    }

    loadWorkspace() {
        this.updateFileTree();
    }

    newFile() {
        const fileName = prompt('Enter new file path:', 'untitled.txt');
        if (!fileName) {
            return;
        }

        const newFile = {
            path: fileName,
            name: fileName.split('/').pop() || fileName,
            content: '',
            dirty: true
        };

        this.openFiles.push(newFile);
        this.currentFileIndex = this.openFiles.length - 1;
        this.updateUI();
        this.setStatusMessage(`New file created: ${fileName}`);
    }

    openFile() {
        const fileName = prompt('Enter file path to open:');
        if (!fileName) {
            return;
        }
        this.openFileFromPath(fileName);
    }

    openFileFromPath(filePath) {
        const existingIndex = this.openFiles.findIndex(file => file.path === filePath);
        if (existingIndex !== -1) {
            this.currentFileIndex = existingIndex;
            this.updateUI();
            return;
        }

        this.waitForMagicIDE().then(api => {
            if (!api) {
                this.setStatusMessage('Backend IPC unavailable');
                return;
            }

            api.readFile(filePath, response => {
                if (response.error) {
                    this.setStatusMessage(`Failed to open ${filePath}: ${response.error}`);
                    return;
                }

                const content = response.result || '';
                this.openFiles.push({
                    path: filePath,
                    name: filePath.split('/').pop() || filePath,
                    content,
                    dirty: false
                });
                this.currentFileIndex = this.openFiles.length - 1;
                this.updateUI();
                this.setStatusMessage(`Opened ${filePath}`);
            });
        });
    }

    saveFile() {
        if (this.currentFileIndex === null) {
            this.setStatusMessage('No file is open');
            return;
        }

        const file = this.openFiles[this.currentFileIndex];
        if (!file) {
            this.setStatusMessage('Current file is invalid');
            return;
        }

        let filePath = file.path;
        if (!filePath) {
            filePath = prompt('Enter file path to save:');
            if (!filePath) {
                this.setStatusMessage('Save canceled');
                return;
            }
            file.path = filePath;
            file.name = filePath.split('/').pop() || filePath;
        }

        const content = this.getEditorContent();
        this.waitForMagicIDE().then(api => {
            if (!api) {
                this.setStatusMessage('Backend IPC unavailable');
                return;
            }

            api.writeFile(filePath, content, response => {
                if (response.error || response.result !== true) {
                    this.setStatusMessage(`Failed to save ${filePath}`);
                    return;
                }

                file.content = content;
                file.dirty = false;
                this.updateTabs();
                this.setStatusMessage(`Saved ${filePath}`);
            });
        });
    }

    updateUI() {
        this.updateTabs();
        this.updateEditor();
        this.updateFileTree();
        this.updateStatusBarInfo();
    }

    updateTabs() {
        const tabsContainer = document.getElementById('editor-tabs');
        tabsContainer.innerHTML = '';

        this.openFiles.forEach((file, index) => {
            const tab = document.createElement('div');
            tab.className = 'tab' + (index === this.currentFileIndex ? ' active' : '');
            if (file.dirty) {
                tab.classList.add('dirty');
            }
            tab.textContent = file.name;
            tab.addEventListener('click', () => {
                this.currentFileIndex = index;
                this.updateUI();
            });

            const closeButton = document.createElement('span');
            closeButton.className = 'close-tab';
            closeButton.textContent = '×';
            closeButton.addEventListener('click', event => {
                event.stopPropagation();
                this.closeTab(index);
            });
            tab.appendChild(closeButton);
            tabsContainer.appendChild(tab);
        });
    }

    closeTab(index) {
        const file = this.openFiles[index];
        if (!file) {
            return;
        }

        if (file.dirty && !confirm(`Close ${file.name} without saving changes?`)) {
            return;
        }

        this.openFiles.splice(index, 1);
        if (this.currentFileIndex === index) {
            this.currentFileIndex = this.openFiles.length - 1;
        } else if (this.currentFileIndex > index) {
            this.currentFileIndex -= 1;
        }

        this.updateUI();
    }

    updateEditor() {
        if (this.currentFileIndex === null || !this.openFiles[this.currentFileIndex]) {
            this.setEditorContent('');
            return;
        }

        const currentFile = this.openFiles[this.currentFileIndex];
        this.setEditorContent(currentFile.content);
        this.updateStatusBarInfo();
    }

    updateFileTree() {
        const fileTree = document.getElementById('file-tree');
        fileTree.innerHTML = '<div>Loading files...</div>';

        this.waitForMagicIDE().then(api => {
            if (!api) {
                fileTree.innerHTML = '<div>Backend unavailable</div>';
                return;
            }

            api.listDirectory(this.currentDirectory, response => {
                if (response.error) {
                    fileTree.innerHTML = `<div>Error: ${response.error}</div>`;
                    return;
                }

                const items = response.result || [];
                fileTree.innerHTML = '';

                if (this.currentDirectory !== '.') {
                    const parent = document.createElement('div');
                    parent.className = 'file-item';
                    parent.textContent = '⬆ ..';
                    parent.addEventListener('click', () => this.goUpDirectory());
                    fileTree.appendChild(parent);
                }

                items.forEach(item => {
                    const path = this.currentDirectory === '.' ? item.name : `${this.currentDirectory}/${item.name}`;
                    const fileItem = document.createElement('div');
                    fileItem.className = 'file-item';
                    fileItem.textContent = item.isDirectory ? `📁 ${item.name}` : `📄 ${item.name}`;
                    fileItem.addEventListener('click', () => {
                        if (item.isDirectory) {
                            this.currentDirectory = path;
                            this.updateFileTree();
                        } else {
                            this.openFileFromPath(path);
                        }
                    });
                    fileTree.appendChild(fileItem);
                });
            });
        });
    }

    goUpDirectory() {
        if (this.currentDirectory === '.' || this.currentDirectory === '/') {
            this.currentDirectory = '.';
            this.updateFileTree();
            return;
        }

        const parts = this.currentDirectory.split('/');
        parts.pop();
        this.currentDirectory = parts.length === 0 ? '.' : parts.join('/');
        this.updateFileTree();
    }

    updateStatusBarInfo() {
        const fileInfo = document.getElementById('file-info');
        const cursorPosition = document.getElementById('cursor-position');
        const file = this.openFiles[this.currentFileIndex];

        if (file) {
            fileInfo.textContent = `${file.path}${file.dirty ? ' • unsaved' : ''}`;
        } else {
            fileInfo.textContent = 'No file open';
        }

        if (!this.editor || !this.editor.getPosition) {
            cursorPosition.textContent = '';
            return;
        }

        const position = this.editor.getPosition();
        if (position) {
            cursorPosition.textContent = `Ln ${position.lineNumber}, Col ${position.column}`;
        }
    }

    setCursorPosition(line, column) {
        const cursorPosition = document.getElementById('cursor-position');
        cursorPosition.textContent = `Ln ${line}, Col ${column}`;
    }

    setStatusMessage(message) {
        document.getElementById('status-message').textContent = message;
    }

    toggleModal(modalId, visible) {
        const modal = document.getElementById(modalId);
        if (!modal) {
            return;
        }
        modal.style.display = visible ? 'flex' : 'none';
    }

    findNext() {
        const query = document.getElementById('find-text').value;
        if (!query || !this.editor || !this.editor.getModel) {
            return;
        }

        const model = this.editor.getModel();
        this.searchMatches = model.findMatches(query, true, false, false, null, true);
        if (this.searchMatches.length === 0) {
            this.setStatusMessage(`No matches for '${query}'`);
            return;
        }

        this.searchIndex = (this.searchIndex + 1) % this.searchMatches.length;
        const match = this.searchMatches[this.searchIndex];
        this.editor.setSelection(match.range);
        this.editor.revealRangeInCenter(match.range);
        this.setStatusMessage(`Match ${this.searchIndex + 1}/${this.searchMatches.length}`);
    }

    replaceSelection() {
        const replacement = document.getElementById('replace-text').value;
        if (!this.editor) {
            return;
        }

        if (this.editor.getSelection) {
            const selection = this.editor.getSelection();
            this.editor.executeEdits('replace', [{ range: selection, text: replacement }]);
            this.setStatusMessage('Replaced selection');
        }
    }

    replaceAll() {
        const query = document.getElementById('find-text').value;
        const replacement = document.getElementById('replace-text').value;
        if (!query || !this.editor || !this.editor.getModel) {
            return;
        }

        const model = this.editor.getModel();
        const matches = model.findMatches(query, true, false, false, null, true);
        if (matches.length === 0) {
            this.setStatusMessage(`No matches for '${query}'`);
            return;
        }

        const edits = matches.map(match => ({ range: match.range, text: replacement }));
        this.editor.executeEdits('replaceAll', edits);
        this.setStatusMessage(`Replaced ${matches.length} occurrences`);
    }
}

const app = new MagicIDE();
