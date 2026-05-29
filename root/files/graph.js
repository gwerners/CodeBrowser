/* CodeBrowser - Graph panel (Cytoscape.js)
   Renders call graphs, inheritance hierarchies and member relationships.
   Depends on: cytoscape.min.js (loaded before this script) */

// ---------------------------------------------------------------------------
// Node colour by symbol kind
// ---------------------------------------------------------------------------

const GRAPH_KIND_COLORS = {
  function:    '#89b4fa',  // blue
  method:      '#a6e3a1',  // green
  constructor: '#a6e3a1',
  destructor:  '#a6e3a1',
  class:       '#fab387',  // orange
  struct:      '#f9e2af',  // yellow
  union:       '#f9e2af',
  enum:        '#cba6f7',  // purple
  enum_value:  '#cba6f7',
  namespace:   '#94e2d5',  // teal
  field:       '#89dceb',  // sky
  global_var:  '#eba0ac',  // pink-red
  typedef:     '#b4befe',
  type_alias:  '#b4befe',
  macro:       '#f38ba8',
};

function _kindColor(kind) {
  if (!kind) return '#cdd6f4';
  return GRAPH_KIND_COLORS[kind.toLowerCase()] || '#cdd6f4';
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

let _cy         = null;
let _graphPanel = null;
let _graphResizer = null;

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

function initGraphPanel(panel, resizer) {
  _graphPanel   = panel;
  _graphResizer = resizer;
  if (resizer) _initGraphResizer(panel, resizer);
}

// ---------------------------------------------------------------------------
// Show / hide
// ---------------------------------------------------------------------------

function showGraph(data, title) {
  if (!_graphPanel) return;

  // Title
  const titleEl = _graphPanel.querySelector('.graph-title');
  if (titleEl) titleEl.textContent = title;

  // Clear tooltip
  const tip = _graphPanel.querySelector('.graph-tooltip');
  if (tip) tip.textContent = 'Double-click a node to navigate';

  // Show panel + resizer
  _graphPanel.style.display = 'flex';
  if (_graphResizer) _graphResizer.style.display = 'block';

  // Trigger Monaco relayout so it shrinks to make room
  if (typeof _monacoEditorInstance !== 'undefined' && _monacoEditorInstance)
    _monacoEditorInstance.layout();

  // Defer Cytoscape init until the flex layout has settled — otherwise the
  // canvas reports 0×0 dimensions and nodes are rendered at wrong positions.
  requestAnimationFrame(() => {
    requestAnimationFrame(() => {
      _renderCytoscape(data);
    });
  });
}

function hideGraph() {
  if (!_graphPanel) return;
  _graphPanel.style.display = 'none';
  if (_graphResizer) _graphResizer.style.display = 'none';
  if (typeof _monacoEditorInstance !== 'undefined' && _monacoEditorInstance)
    _monacoEditorInstance.layout();
  if (_cy) { _cy.destroy(); _cy = null; }
}

// ---------------------------------------------------------------------------
// Cytoscape rendering
// ---------------------------------------------------------------------------

function _renderCytoscape(data) {
  const container = _graphPanel.querySelector('.graph-canvas');
  if (!container) return;

  if (_cy) { _cy.destroy(); _cy = null; }

  // Convert server graph JSON → Cytoscape elements
  const elements = [];

  for (const node of (data.nodes || [])) {
    // Use the last segment of a qualified name as the visible label
    const fullName  = node.name || '?';
    const label     = fullName.includes('::')
        ? fullName.split('::').pop()
        : fullName;

    elements.push({ data: {
      id:       String(node.id),
      label,
      fullName,
      kind:     node.kind  || '',
      file:     node.file  || '',
      line:     node.line  || 0,
      url:      node.url   || '',
      isCenter: (node.id === 0),
    }});
  }

  for (const edge of (data.edges || [])) {
    elements.push({ data: {
      id:     `e${edge.from}-${edge.to}`,
      source: String(edge.from),
      target: String(edge.to),
    }});
  }

  _cy = cytoscape({
    container,
    elements,
    style: [
      {
        selector: 'node',
        style: {
          'background-color':   (ele) => _kindColor(ele.data('kind')),
          'label':              'data(label)',
          'text-valign':        'center',
          'text-halign':        'center',
          'color':              '#1e1e2e',
          'font-size':          '11px',
          'font-weight':        'bold',
          // auto-size: ~7px per char, 20px padding, clamped to [80, 200]
          'width':  (ele) => Math.max(80, Math.min(200, (ele.data('label')||'').length * 7 + 20)),
          'height': 34,
          'shape':              (ele) => ele.data('isCenter') ? 'round-rectangle' : 'ellipse',
          'border-width':       (ele) => ele.data('isCenter') ? 3 : 1,
          'border-color':       (ele) => ele.data('isCenter') ? '#f9e2af' : '#585b70',
          'text-overflow-wrap': 'anywhere',
          'text-max-width':     (ele) => Math.max(72, Math.min(190, (ele.data('label')||'').length * 7 + 10)) + 'px',
          'cursor':             'pointer',
        }
      },
      {
        selector: 'edge',
        style: {
          'width':              2,
          'line-color':         '#585b70',
          'target-arrow-color': '#585b70',
          'target-arrow-shape': 'triangle',
          'curve-style':        'bezier',
          'arrow-scale':        1.2,
        }
      },
      {
        selector: 'node.highlighted',
        style: { 'border-color': '#89b4fa', 'border-width': 2 }
      },
      {
        selector: 'node:selected',
        style: { 'border-color': '#f9e2af', 'border-width': 3 }
      },
    ],
    layout: {
      name:           'cose',
      animate:        false,
      randomize:      false,
      nodeRepulsion:  8000,
      idealEdgeLength: 120,
      gravity:        1.2,
      numIter:        1000,
    },
    minZoom: 0.15,
    maxZoom: 5,
    wheelSensitivity: 0.3,
  });

  // Single click: open in new tab if URL exists, otherwise show tooltip
  _cy.on('tap', 'node', (evt) => {
    const n   = evt.target;
    const url = n.data('url');
    const tip = _graphPanel.querySelector('.graph-tooltip');

    if (url) {
      window.open(url, '_blank');
    } else {
      // No URL (e.g. symbol from un-indexed system headers) — show info
      if (tip) {
        let msg = `${n.data('kind')}: ${n.data('fullName')}`;
        if (n.data('file')) msg += `  (line ${n.data('line')})`;
        msg += ' — no source location indexed';
        tip.textContent = msg;
      }
    }
  });

  // Hover → show full name in tooltip (without navigating)
  _cy.on('mouseover', 'node', (evt) => {
    const n   = evt.target;
    const tip = _graphPanel.querySelector('.graph-tooltip');
    if (!tip) return;
    let msg = `${n.data('kind')}: ${n.data('fullName')}`;
    if (n.data('file')) msg += `  → line ${n.data('line')}`;
    tip.textContent = msg;
  });

  // Resize + fit after a short delay to handle any remaining layout settling
  _cy.resize();
  _cy.fit(undefined, 30);
  setTimeout(() => { if (_cy) { _cy.resize(); _cy.fit(undefined, 30); } }, 150);
}

// ---------------------------------------------------------------------------
// Graph data fetch
// ---------------------------------------------------------------------------

async function fetchAndShowGraph(baseUrl, line, col, title) {
  const url = `${baseUrl}&line=${line}&col=${col}`;
  try {
    const resp = await fetch(url);
    if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
    const data = await resp.json();

    if (!data || !data.nodes || data.nodes.length === 0) {
      // Build debug message from server-side debug fields
      let msg = 'No symbol found at this position.';
      const lines = [];

      if (data._debug_query)
        lines.push(`Query: ${data._debug_query}`);

      if (data._debug_atOutput) {
        const out = data._debug_atOutput.trim();
        lines.push(`cxxidx at: ${out || '(empty)'}`);
      }

      if (data._debug_graphOutput) {
        const out = data._debug_graphOutput.trim();
        if (out) lines.push(`cxxidx ${title.toLowerCase()}: ${out}`);
      }

      if (lines.length > 0)
        msg = lines.join('\n');

      _showGraphMsg(title, msg);
      return;
    }
    showGraph(data, title);
  } catch (e) {
    console.error('Graph fetch failed:', e);
    _showGraphMsg(title, `Error: ${e.message}`);
  }
}

function _showGraphMsg(title, msg) {
  if (!_graphPanel) return;
  const titleEl = _graphPanel.querySelector('.graph-title');
  if (titleEl) titleEl.textContent = title;
  const tip = _graphPanel.querySelector('.graph-tooltip');
  if (tip) tip.textContent = '';
  _graphPanel.style.display = 'flex';
  if (_graphResizer) _graphResizer.style.display = 'block';
  if (typeof _monacoEditorInstance !== 'undefined' && _monacoEditorInstance)
    _monacoEditorInstance.layout();
  const canvas = _graphPanel.querySelector('.graph-canvas');
  if (canvas) {
    // Show each line of the debug message as a separate <p>
    const lines = msg.split('\n').map(l =>
      `<p class="graph-debug-line">${l}</p>`).join('');
    canvas.innerHTML = `<div class="graph-debug-box">${lines}</div>`;
  }
}

// ---------------------------------------------------------------------------
// Resizable panel
// ---------------------------------------------------------------------------

function _initGraphResizer(panel, resizer) {
  if (resizer.dataset.init) return;
  resizer.dataset.init = 'true';

  let startX, startWidth, dragOverlay;

  resizer.addEventListener('mousedown', (e) => {
    startX     = e.clientX;
    startWidth = panel.offsetWidth;
    resizer.classList.add('dragging');

    dragOverlay = document.createElement('div');
    dragOverlay.style.cssText =
      'position:fixed;top:0;left:0;right:0;bottom:0;z-index:9999;cursor:col-resize;';
    document.body.appendChild(dragOverlay);

    document.addEventListener('mousemove', onDrag);
    document.addEventListener('mouseup', onStop);
    e.preventDefault();
  });

  function onDrag(e) {
    const newW = startWidth - (e.clientX - startX); // resizer is on the LEFT of the panel
    panel.style.width = Math.max(180, Math.min(1000, newW)) + 'px';
    if (_cy) _cy.resize();
    if (typeof _monacoEditorInstance !== 'undefined' && _monacoEditorInstance)
      _monacoEditorInstance.layout();
  }

  function onStop() {
    resizer.classList.remove('dragging');
    if (dragOverlay) { dragOverlay.remove(); dragOverlay = null; }
    document.removeEventListener('mousemove', onDrag);
    document.removeEventListener('mouseup', onStop);
  }
}
