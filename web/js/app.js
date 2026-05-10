// Magic IDE Application Logic

class MagicIDE {
    constructor() {
        this.currentFile = null;
        this.openFiles = [];
        this.init();
    }

    init() {
        this.initializeMonaco().then(() => {
            this.bindEvents();
            this.loadSettings();
            this.updateFileTree();
            this.initializeGit();
            console.log('Magic IDE initialized with Monaco Editor');
        });
    }

    bindEvents() {
        document.getElementById('new-file').addEventListener('click', () => this.newFile());
        document.getElementById('open-file').addEventListener('click', () => this.openFile());
        document.getElementById('save-file').addEventListener('click', () => this.saveFile());
    }

    initializeEditor() {
        // Initialize Monaco Editor or basic textarea for now
        const editorElement = document.getElementById('editor');
        this.editor = document.createElement('textarea');
        this.editor.style.width = '100%';
        this.editor.style.height = '100%';
        this.editor.style.border = 'none';
        this.editor.style.outline = 'none';
        this.editor.style.resize = 'none';
        this.editor.style.fontFamily = 'Consolas, Monaco, monospace';
        this.editor.style.fontSize = '14px';
        this.editor.style.lineHeight = '1.4';
        this.editor.placeholder = 'Start coding here...';
        editorElement.appendChild(this.editor);
    }

    newFile() {
        const fileName = prompt('Enter file name:');
        if (fileName) {
            this.openFiles.push({ name: fileName, content: '' });
            this.currentFile = this.openFiles.length - 1;
            this.updateUI();
            this.setStatusMessage(`New file: ${fileName}`);
        }
    }

    openFile() {
        // For now, simulate file opening
        const fileName = prompt('Enter file path:');
        if (fileName) {
            // In a real implementation, this would read from the file system
            this.openFiles.push({ name: fileName, content: '// File content would be loaded here' });
            this.currentFile = this.openFiles.length - 1;
            this.updateUI();
            this.setStatusMessage(`Opened: ${fileName}`);
        }
    }

    saveFile() {
        if (this.currentFile !== null) {
            const content = this.editor.value;
            this.openFiles[this.currentFile].content = content;
            this.setStatusMessage(`Saved: ${this.openFiles[this.currentFile].name}`);
        } else {
            this.setStatusMessage('No file to save');
        }
    }

    updateUI() {
        if (this.currentFile !== null) {
            this.editor.value = this.openFiles[this.currentFile].content;
        }
        this.updateTabs();
        this.updateFileTree();
    }

    updateTabs() {
        const tabsContainer = document.getElementById('editor-tabs');
        tabsContainer.innerHTML = '';
        this.openFiles.forEach((file, index) => {
            const tab = document.createElement('div');
            tab.className = 'tab' + (index === this.currentFile ? ' active' : '');
            tab.textContent = file.name;
            tab.addEventListener('click', () => {
                this.currentFile = index;
                this.updateUI();
            });
            tabsContainer.appendChild(tab);
        });
    }

    updateFileTree() {
        // Populate file tree from current directory
        const fileTree = document.getElementById('file-tree');
        fileTree.innerHTML = '<div>Loading files...</div>';

        // In a real implementation, this would call the backend
        // For now, show a basic structure
        setTimeout(() => {
            fileTree.innerHTML = `
                <div class="file-item" data-path=".">📁 . (root)</div>
                <div class="file-item" data-path="src">📁 src/</div>
                <div class="file-item" data-path="include">📁 include/</div>
                <div class="file-item" data-path="web">📁 web/</div>
                <div class="file-item" data-path="CMakeLists.txt">📄 CMakeLists.txt</div>
                <div class="file-item" data-path="PLAN.md">📄 PLAN.md</div>
            `;

            // Add click handlers
            document.querySelectorAll('.file-item').forEach(item => {
                item.addEventListener('click', () => {
                    const path = item.getAttribute('data-path');
                    if (path.endsWith('/')) {
                        // Directory - could expand
                        console.log('Directory clicked:', path);
                    } else {
                        // File - open it
                        this.simulateFileOpen(path);
                    }
                });
            });
        }, 100);
    }

    setStatusMessage(message) {
        document.getElementById('status-message').textContent = message;
    }
}

// Initialize the application
const app = new MagicIDE();