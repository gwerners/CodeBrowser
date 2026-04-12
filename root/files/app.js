/* CodeBrowser - Consolidated JavaScript
   Merged from: new_utils.js and extracted editor action code from HTML templates.
   Single source of truth for all frontend logic. */

// === Search page utilities ===

function renderResults() {
  const table = document.getElementById('resultsTable');
  if (!table || typeof searchResults === 'undefined') return;

  for (const result of searchResults) {
    const dirRow = document.createElement('tr');
    dirRow.className = 'dir';
    const dirCell = document.createElement('td');
    dirCell.colSpan = 2;
    dirCell.innerHTML = `<a href="${result.url}">${result.dir}</a>`;
    dirRow.appendChild(dirCell);
    table.appendChild(dirRow);

    result.entries.forEach(entry => {
      const row = document.createElement('tr');
      row.className = 'file-row';
      const fileCell = document.createElement('td');
      fileCell.className = 'f';
      fileCell.innerHTML = `<a href="${entry.url}">${entry.file}</a>`;

      const contentCell = document.createElement('td');
      contentCell.className = 'con';
      for (const line of entry.lines) {
        const a = document.createElement('a');
        a.className = 's';
        a.href = line.url;
        a.innerHTML = `<span class="l">${line.num}</span> ${line.content}`;
        contentCell.appendChild(a);
        contentCell.appendChild(document.createElement('br'));
      }

      row.appendChild(fileCell);
      row.appendChild(contentCell);
      table.appendChild(row);
    });
  }
}

function clearForm() {
  document.getElementById('s1').value = '';
  document.getElementById('s2').value = '';
  const sel = document.getElementById('project');
  if (sel) {
    for (const opt of sel.options) opt.selected = false;
  }
}

function goFirstProject() {
  const sel = document.getElementById('project');
  if (!sel) return;
  const selected = Array.from(sel.selectedOptions);
  if (selected.length > 0) {
    window.location = "/folders?project=" + encodeURIComponent(selected[0].text);
  }
}

// === Editor / LSP utilities ===

async function fetchHoverInfo(url, lineNumber, column) {
  try {
    const response = await fetch(`${url}&line=${lineNumber}&column=${column}`);
    if (!response.ok) throw new Error('Network error');
    return await response.json();
  } catch (error) {
    console.error('Failed to fetch hover info:', error);
    return null;
  }
}

async function fetchIdentifier(url, identifier, lineNumber, column, type) {
  try {
    const fullUrl = `${url}&identifier=${identifier}&line=${lineNumber}&column=${column}&type=${type}`;
    const response = await fetch(fullUrl);
    if (!response.ok) throw new Error('Network error');
    const data = await response.json();
    if (data.error) throw new Error(data.error.message);
    return data;
  } catch (error) {
    console.error('Failed to fetch identifier:', error);
    return null;
  }
}

async function fetchHeaderSource(url) {
  try {
    const response = await fetch(url);
    if (!response.ok) throw new Error('Network error');
    const data = await response.json();
    if (data.error) throw new Error(data.error.message);
    return data;
  } catch (error) {
    console.error('Failed to fetch header/source:', error);
    return null;
  }
}

// Get word at cursor from editor, using selection or word-at-position
function getEditorWord(editor) {
  const selection = editor.getSelection();
  const position = selection.getPosition();
  let word = editor.getModel().getValueInRange(selection);
  if (!word) {
    const wordObj = editor.getModel().getWordAtPosition(position);
    if (wordObj) word = wordObj.word;
  }
  return { word, position };
}

// Navigate to a definition/declaration/implementation/typeDefinition
async function navigateToSymbol(editor, identifierUrl, type, openInNewTab) {
  const { word, position } = getEditorWord(editor);
  if (!word || !position) return;

  const target = await fetchIdentifier(
    identifierUrl, word, position.lineNumber, position.column, type
  );

  if (target && target.uri) {
    if (openInNewTab) {
      window.open(target.uri, '_blank');
    } else {
      window.location.href = target.uri;
    }
  } else {
    console.error('Target not found for', type);
  }
}

// Register all editor actions (context menu items)
function registerEditorActions(editor, monaco, identifierUrl, searchUrl, headerSourceUrl) {
  // Ctrl+Click: go to definition (Shift+Ctrl+Click: declaration)
  editor.onMouseDown(async (event) => {
    const { event: { ctrlKey, shiftKey } } = event;
    if (!ctrlKey) return;

    const position = editor.getPosition();
    if (!position) return;

    const wordObj = editor.getModel().getWordAtPosition(position);
    if (!wordObj) return;

    const type = shiftKey ? "declaration" : "definition";
    const target = await fetchIdentifier(
      identifierUrl, wordObj.word, position.lineNumber, position.column, type
    );
    if (target && target.uri) {
      window.location.href = target.uri;
    }
  });

  // Context menu actions
  const actions = [
    { id: 'cmd-switch-header-source', label: 'Switch Header/Source', order: 1, handler: async () => {
      const result = await fetchHeaderSource(headerSourceUrl);
      if (result && result.url) window.location.href = result.url;
    }},
    { id: 'cmd-search-word', label: 'Search Word', order: 2, handler: async () => {
      const { word } = getEditorWord(editor);
      if (word) window.open(`${searchUrl}&q=${word}`, '_blank');
    }},
    { id: 'cmd-definition', label: 'Go to Definition', order: 3, handler: () =>
      navigateToSymbol(editor, identifierUrl, 'definition', true)
    },
    { id: 'cmd-declaration', label: 'Go to Declaration', order: 4, handler: () =>
      navigateToSymbol(editor, identifierUrl, 'declaration', true)
    },
    { id: 'cmd-references', label: 'Show References', order: 5, handler: async () => {
      const { word, position } = getEditorWord(editor);
      if (!word || !position) return;
      const target = await fetchIdentifier(
        identifierUrl, word, position.lineNumber, position.column, 'references'
      );
      if (target && target.references) {
        createReferencesOverlay(target.references);
      }
    }},
    { id: 'cmd-typeHierarchy', label: 'Type Hierarchy', order: 6, handler: () =>
      navigateToSymbol(editor, identifierUrl, 'typeHierarchy', false)
    },
    { id: 'cmd-implementation', label: 'Go to Implementation', order: 7, handler: () =>
      navigateToSymbol(editor, identifierUrl, 'implementation', true)
    },
    { id: 'cmd-type-definition', label: 'Go to Type Definition', order: 8, handler: () =>
      navigateToSymbol(editor, identifierUrl, 'typeDefinition', true)
    },
  ];

  for (const action of actions) {
    editor.addAction({
      id: action.id,
      label: action.label,
      contextMenuGroupId: 'navigation',
      contextMenuOrder: action.order,
      run: action.handler,
    });
  }
}

// Create overlay popup for references
function createReferencesOverlay(urls) {
  const existing = document.getElementById('overlay');
  if (existing) existing.remove();

  const overlay = document.createElement('div');
  overlay.id = 'overlay';

  // Header with close button
  const table = document.createElement('table');
  table.id = 'overlay-table';
  const tr = document.createElement('tr');

  const titleCell = document.createElement('td');
  titleCell.innerHTML = '<h2>References</h2>';
  tr.appendChild(titleCell);

  const closeCell = document.createElement('td');
  const closeBtn = document.createElement('button');
  closeBtn.id = 'close-overlay-btn';
  closeBtn.innerHTML = '&times;';
  closeBtn.addEventListener('click', () => overlay.remove());
  closeCell.appendChild(closeBtn);
  tr.appendChild(closeCell);

  const tbody = document.createElement('tbody');
  tbody.appendChild(tr);
  table.appendChild(tbody);
  overlay.appendChild(table);

  // URL list
  const list = document.createElement('ul');
  urls.forEach(item => {
    const li = document.createElement('li');
    const link = document.createElement('a');
    link.href = item.url;
    link.target = '_blank';
    link.innerText = item.name;
    li.appendChild(link);
    list.appendChild(li);
  });
  overlay.appendChild(list);

  document.getElementById('editor').appendChild(overlay);
}

// Initialize Monaco editor with content and optional line highlight
function initMonacoEditor(container, content, lang, lineNumber, config) {
  require.config({ paths: { 'vs': config.monacoPath } });
  require(['vs/editor/editor.main'], (monaco) => {
    const editor = monaco.editor.create(container, {
      value: content,
      language: lang,
      theme: 'vs-dark',
      readOnly: config.readOnly || false,
      automaticLayout: true,
    });

    if (lineNumber > 0) {
      editor.revealLineInCenter(lineNumber);
      editor.deltaDecorations([], [{
        range: new monaco.Range(lineNumber, 1, lineNumber, 1),
        options: { isWholeLine: true, className: 'myLineHighlight' }
      }]);
    }

    // Register actions if URLs are provided
    if (config.identifierUrl) {
      registerEditorActions(editor, monaco,
        config.identifierUrl, config.searchUrl, config.headerSourceUrl);
    }

    // Register hover provider if URL is provided
    if (config.hoverUrl) {
      monaco.languages.registerHoverProvider(lang, {
        provideHover: async (model, position) => {
          const hoverInfo = await fetchHoverInfo(
            config.hoverUrl, position.lineNumber, position.column
          );
          if (!hoverInfo || !hoverInfo.description) return null;
          return { contents: [{ value: hoverInfo.description }] };
        }
      });
    }

    // Annotate mode: make blame hashes clickable
    if (config.isAnnotate && config.blameBaseUrl) {
      setupBlameLinks(editor, monaco, content, config.blameBaseUrl);
    }

    // Store reference for jumpToLine
    _monacoEditorInstance = editor;

    if (config.onReady) config.onReady(editor, monaco);
  });
}

// Initialize Monaco diff editor
function initMonacoDiffEditor(container, originalContent, modifiedContent, lang, monacoPath) {
  require.config({ paths: { 'vs': monacoPath } });
  require(['vs/editor/editor.main'], function() {
    var originalModel = monaco.editor.createModel(originalContent, lang);
    var modifiedModel = monaco.editor.createModel(modifiedContent, lang);
    var diffEditor = monaco.editor.createDiffEditor(container, {
      language: lang,
      theme: 'vs-dark',
      readOnly: true,
      automaticLayout: true,
    });
    diffEditor.setModel({ original: originalModel, modified: modifiedModel });
  });
}

// === Blame / Annotate links ===

function setupBlameLinks(editor, monaco, content, blameBaseUrl) {
  // Parse blame output: each line starts with "hash (author date line) code"
  // Extract hash per line and add hover + click support
  const lines = content.split('\n');
  const blameHashes = [];  // hash per line number

  for (let i = 0; i < lines.length; i++) {
    const match = lines[i].match(/^([0-9a-f]{6,})\s/);
    blameHashes[i + 1] = match ? match[1] : null;
  }

  // Add decorations to make hash area visually distinct
  const decorations = [];
  for (let i = 1; i <= lines.length; i++) {
    if (blameHashes[i]) {
      const hashLen = blameHashes[i].length;
      decorations.push({
        range: new monaco.Range(i, 1, i, hashLen + 1),
        options: {
          inlineClassName: 'blame-hash-link',
          hoverMessage: { value: `Click to view file at commit **${blameHashes[i]}**` },
        }
      });
    }
  }
  editor.deltaDecorations([], decorations);

  // Click handler: if click is on the hash area, navigate
  editor.onMouseDown((event) => {
    if (!event.target || !event.target.position) return;
    const lineNum = event.target.position.lineNumber;
    const col = event.target.position.column;
    const hash = blameHashes[lineNum];
    if (hash && col <= hash.length + 1) {
      window.open(blameBaseUrl + hash, '_blank');
    }
  });
}

// === Symbol outline panel ===

// Kind icons for symbol types
const SYMBOL_ICONS = {
  'function': 'f()',
  'method': 'm()',
  'class': 'C',
  'struct': 'S',
  'enum': 'E',
  'member': 'M',
  'variable': 'V',
  'macro': '#',
  'typedef': 'T',
  'namespace': 'N',
  'prototype': 'p()',
  'enumerator': 'e',
  'field': 'F',
  'union': 'U',
};

function getSymbolIcon(kind) {
  if (!kind) return '?';
  const k = kind.toLowerCase();
  return SYMBOL_ICONS[k] || kind.charAt(0).toUpperCase();
}

async function toggleSymbolPanel(symbolsUrl) {
  const panel = document.getElementById('symbolPanel');
  const resizer = document.getElementById('symbolResizer');
  if (!panel) return;

  if (panel.style.display !== 'none') {
    panel.style.display = 'none';
    if (resizer) resizer.style.display = 'none';
    return;
  }

  // Fetch symbols if not loaded yet
  if (!panel.dataset.loaded) {
    try {
      const resp = await fetch(symbolsUrl);
      const symbols = await resp.json();
      renderSymbolPanel(panel, symbols);
      panel.dataset.loaded = 'true';
    } catch (e) {
      panel.innerHTML = '<div class="sym-empty">Failed to load symbols</div>';
    }
  }

  panel.style.display = 'block';
  if (resizer) {
    resizer.style.display = 'block';
    initSymbolResizer(panel, resizer);
  }
}

function renderSymbolPanel(panel, symbols) {
  if (!symbols || symbols.length === 0) {
    panel.innerHTML = '<div class="sym-empty">No symbols found</div>';
    return;
  }

  // Group by kind
  const groups = {};
  for (const sym of symbols) {
    const kind = sym.kind || 'other';
    if (!groups[kind]) groups[kind] = [];
    groups[kind].push(sym);
  }

  // Sort each group by line number
  for (const kind in groups) {
    groups[kind].sort((a, b) => a.line - b.line);
  }

  // Preferred display order
  const kindOrder = ['class', 'struct', 'enum', 'function', 'method',
                     'prototype', 'member', 'field', 'variable', 'macro',
                     'typedef', 'namespace', 'enumerator', 'union'];
  const sortedKinds = Object.keys(groups).sort((a, b) => {
    const ia = kindOrder.indexOf(a.toLowerCase());
    const ib = kindOrder.indexOf(b.toLowerCase());
    return (ia === -1 ? 999 : ia) - (ib === -1 ? 999 : ib);
  });

  let html = '<div class="sym-header">Symbols<button class="sym-close" onclick="document.getElementById(\'symbolPanel\').style.display=\'none\'; var r=document.getElementById(\'symbolResizer\'); if(r) r.style.display=\'none\';">&times;</button></div>';
  html += '<div class="sym-list">';

  for (const kind of sortedKinds) {
    const items = groups[kind];
    html += `<div class="sym-group-title">${kind} (${items.length})</div>`;
    for (const sym of items) {
      const icon = getSymbolIcon(kind);
      const label = sym.scope ? `${sym.scope}::${sym.name}` : sym.name;
      const sig = sym.signature || '';
      html += `<a class="sym-item" href="#" onclick="jumpToLine(${sym.line}); return false;" title="Line ${sym.line}">`;
      html += `<span class="sym-icon">${icon}</span>`;
      html += `<span class="sym-name">${label}</span>`;
      html += `<span class="sym-sig">${sig}</span>`;
      html += `<span class="sym-line">${sym.line}</span>`;
      html += `</a>`;
    }
  }

  html += '</div>';
  panel.innerHTML = html;
}

// Resizable symbol panel via drag handle
function initSymbolResizer(panel, resizer) {
  if (resizer.dataset.init) return;
  resizer.dataset.init = 'true';

  let startX, startWidth, dragOverlay;

  resizer.addEventListener('mousedown', (e) => {
    startX = e.clientX;
    startWidth = panel.offsetWidth;
    resizer.classList.add('dragging');

    // Block Monaco from stealing mouse events during drag
    dragOverlay = document.createElement('div');
    dragOverlay.style.cssText = 'position:fixed;top:0;left:0;right:0;bottom:0;z-index:9999;cursor:col-resize;';
    document.body.appendChild(dragOverlay);

    document.addEventListener('mousemove', onDrag);
    document.addEventListener('mouseup', onStopDrag);
    e.preventDefault();
  });

  function onDrag(e) {
    const newWidth = startWidth + (e.clientX - startX);
    panel.style.width = Math.max(120, Math.min(800, newWidth)) + 'px';
    if (_monacoEditorInstance) _monacoEditorInstance.layout();
  }

  function onStopDrag() {
    resizer.classList.remove('dragging');
    if (dragOverlay) { dragOverlay.remove(); dragOverlay = null; }
    document.removeEventListener('mousemove', onDrag);
    document.removeEventListener('mouseup', onStopDrag);
  }
}

// Jump to line in the Monaco editor
var _monacoEditorInstance = null;

function jumpToLine(line) {
  if (_monacoEditorInstance) {
    _monacoEditorInstance.revealLineInCenter(line);
    _monacoEditorInstance.setPosition({ lineNumber: line, column: 1 });
    _monacoEditorInstance.focus();
  }
}

// === History page utilities ===

function toggleAllFileLists(link) {
  const lists = document.querySelectorAll('.filelist-hidden');
  const showing = [...lists].some(div => div.style.display === 'block');
  lists.forEach(div => div.style.display = showing ? 'none' : 'block');
  link.textContent = showing
    ? '(Show modified files >>>)'
    : '(<<< Hide modified files)';
}

function applyRadioLogic() {
  const rows = Array.from(document.querySelectorAll('#revTable tr'));
  if (rows.length === 0) return;

  const leftRadios = rows.map(row => row.querySelector('input[name="r1"]'));
  const rightRadios = rows.map(row => row.querySelector('input[name="r2"]'));

  function getSelectedIndex(radios) {
    return radios.findIndex(r => r && r.checked);
  }

  function updateStates() {
    const leftIndex = getSelectedIndex(leftRadios);
    const rightIndex = getSelectedIndex(rightRadios);

    rows.forEach((_, i) => {
      const left = leftRadios[i];
      const right = rightRadios[i];
      if (!left || !right) return;

      if (left.checked) {
        right.disabled = true;
      } else if (right.checked) {
        left.disabled = true;
      } else {
        left.disabled = rightIndex !== -1 && i <= rightIndex;
        right.disabled = leftIndex !== -1 && i >= leftIndex;
      }
    });
  }

  [...leftRadios, ...rightRadios].forEach(radio => {
    if (radio) radio.addEventListener('change', updateStates);
  });

  // Default selections
  if (rows.length >= 2 && leftRadios[1]) leftRadios[1].checked = true;
  if (rows.length >= 1 && rightRadios[0]) rightRadios[0].checked = true;

  updateStates();
}
