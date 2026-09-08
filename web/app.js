(() => {
  'use strict';

  const definitions = {
    'threshold': {title: 'Image threshold', kind: 'IMAGE', icon: '◩', short: 'black & white split', description: 'Turn an image into a crisp black-and-white PNG with an adjustable cutoff.', accept: 'image/*', drop: 'drop an image here', result: 'the result will be a black-and-white PNG'},
    'bayer': {title: 'Bayer dither', kind: 'IMAGE', icon: '▦', short: 'ordered pixel patterns', description: 'Create black-and-white ordered dithering with a selectable Bayer grid. Transparency is flattened onto white; animated inputs use the first frame.', accept: 'image/*', drop: 'drop an image here', result: 'the result will be a Bayer-dithered PNG'},
    'dither': {title: 'Dither image', kind: 'IMAGE', icon: '░', short: 'fine black & white grain', description: 'Reproduce image shading with Floyd–Steinberg black-and-white dithering. Transparency is flattened onto white; animated inputs use the first frame.', accept: 'image/*', drop: 'drop an image here', result: 'the result will be a dithered PNG'},
    'halftone': {title: 'Halftone image', kind: 'IMAGE', icon: '⠿', short: 'newspaper-style dots', description: 'Turn image shading into a black-and-white pattern of clustered dots with adjustable dot size and density. Transparency is flattened onto white; animated inputs use the first frame.', accept: 'image/*', drop: 'drop an image here', result: 'the result will be a halftone PNG'},
    'png-jpg': {title: 'PNG → JPG', kind: 'IMAGE', icon: '▧', short: 'flatten & compress', description: 'Flatten transparency onto white and make a clean JPEG.', accept: 'image/png', drop: 'drop a PNG here', result: 'the result will be a JPEG'},
    'invert': {title: 'Invert image', kind: 'IMAGE', icon: '◐', short: 'reverse every color', description: 'Invert the colors while keeping the original image format.', accept: 'image/*', drop: 'drop an image here', result: 'the result keeps its image format'},
    'img-gif': {title: 'Image → GIF', kind: 'IMAGE', icon: '◇', short: 'make a GIF', description: 'Turn a still image into a broadly compatible GIF.', accept: 'image/*', drop: 'drop an image here', result: 'the result will be a GIF'},
    'pdf-png': {title: 'PDF → PNG', kind: 'DOCUMENT', icon: '▤', short: 'render every page', description: 'Render every PDF page at useful resolution as separate PNG files.', accept: 'application/pdf,.pdf', drop: 'drop a PDF here', result: 'each page becomes a PNG'},
    'mp4-gif': {title: 'MP4 → GIF', kind: 'VIDEO', icon: '▶', short: '12 fps animated GIF', description: 'Turn an MP4 into a smooth 12 fps GIF using FFmpeg.', accept: 'video/mp4,.mp4', drop: 'drop an MP4 here', result: 'the result will be an animated GIF'},
    'md5': {title: 'MD5 digest', kind: 'CHECKSUM', icon: '#', short: 'legacy checksum', description: 'Calculate the file’s MD5 digest and return it as plain text.', accept: '*/*', drop: 'drop any file here', result: 'the result will be an MD5 text file'},
    'sha256': {title: 'SHA-256 digest', kind: 'CHECKSUM', icon: '#', short: 'strong checksum', description: 'Calculate the file’s SHA-256 digest and return it as plain text.', accept: '*/*', drop: 'drop any file here', result: 'the result will be a SHA-256 text file'},
    'base64': {title: 'Base64 encode', kind: 'TEXT / DATA', icon: '64', short: 'encode any file', description: 'Encode the file as Base64 text without changing its source bytes.', accept: '*/*', drop: 'drop any file here', result: 'the result will be Base64 text'},
    'json-min': {title: 'Minify JSON', kind: 'TEXT / DATA', icon: '{}', short: 'remove whitespace', description: 'Validate JSON and remove formatting whitespace for a compact result.', accept: 'application/json,.json', drop: 'drop a JSON file here', result: 'the result will be compact JSON'},
    'virustest': {title: 'Virus report', kind: 'SECURITY', icon: '⌁', short: 'ClamAV scan image', description: 'Scan a file with ClamAV and return the report as a PNG.', accept: '*/*', drop: 'drop a file to scan', result: 'the result will be a PNG scan report'}
  };

  const fallbackOrder = Object.keys(definitions);
  const state = {operation: 'threshold', file: null, operations: fallbackOrder};
  const $ = selector => document.querySelector(selector);
  const toolList = $('[data-tool-list]');
  const dropzone = $('[data-dropzone]');
  const fileInput = $('[data-file-input]');
  const convertButton = $('[data-convert]');
  const toast = $('[data-toast]');
  let toastTimer;

  function showToast(message, error = false) {
    clearTimeout(toastTimer);
    toast.textContent = message;
    toast.classList.toggle('is-error', error);
    toast.classList.add('is-visible');
    toastTimer = setTimeout(() => toast.classList.remove('is-visible'), 3200);
  }

  function humanSize(bytes) {
    if (bytes < 1024) return `${bytes} B`;
    if (bytes < 1024 ** 2) return `${(bytes / 1024).toFixed(1)} KiB`;
    return `${(bytes / 1024 ** 2).toFixed(1)} MiB`;
  }

  function safeName(name) {
    return name.replace(/[\\/]/g, '_').replace(/[?#]/g, '_') || 'input.bin';
  }

  function hrefName(href) {
    const clean = href.replace(/\/$/, '');
    const leaf = clean.slice(clean.lastIndexOf('/') + 1);
    try { return decodeURIComponent(leaf); } catch { return leaf; }
  }

  function parseDav(xmlText, collectionPath) {
    const xml = new DOMParser().parseFromString(xmlText, 'application/xml');
    if (xml.querySelector('parsererror')) throw new Error('The converter returned an invalid directory listing.');
    return [...xml.getElementsByTagNameNS('DAV:', 'response')].map(response => {
      const href = response.getElementsByTagNameNS('DAV:', 'href')[0]?.textContent || '';
      const sizeText = response.getElementsByTagNameNS('DAV:', 'getcontentlength')[0]?.textContent || '0';
      return {href, name: hrefName(href), size: Number(sizeText) || 0};
    }).filter(item => item.href && item.href.replace(/\/$/, '') !== collectionPath.replace(/\/$/, ''));
  }

  async function propfind(path, depth = '1') {
    const response = await fetch(path, {method: 'PROPFIND', headers: {Depth: depth}, cache: 'no-store'});
    if (response.status !== 207) throw new Error(`Directory listing failed (HTTP ${response.status}).`);
    return parseDav(await response.text(), path);
  }

  function renderTools() {
    toolList.replaceChildren();
    state.operations.forEach(operation => {
      const def = definitions[operation] || {title: operation, kind: 'TOOL', icon: '→', short: 'server converter', description: `Convert with ${operation}.`, accept: '*/*', drop: 'drop a file here', result: 'the result will appear below'};
      const button = document.createElement('button');
      button.type = 'button';
      button.className = `tool-button${operation === state.operation ? ' is-active' : ''}`;
      button.dataset.operation = operation;
      button.setAttribute('aria-pressed', String(operation === state.operation));

      const icon = document.createElement('span'); icon.className = 'tool-icon'; icon.textContent = def.icon;
      const copy = document.createElement('span'); copy.className = 'tool-copy';
      const title = document.createElement('strong'); title.textContent = def.title;
      const short = document.createElement('span'); short.textContent = def.short;
      copy.append(title, short);
      const arrow = document.createElement('span'); arrow.className = 'tool-arrow'; arrow.textContent = '›';
      button.append(icon, copy, arrow);
      button.addEventListener('click', () => selectOperation(operation));
      toolList.append(button);
    });
    $('[data-tool-count]').textContent = `${state.operations.length} tools`;
  }

  function selectOperation(operation) {
    state.operation = operation;
    const def = definitions[operation] || {title: operation, kind: 'TOOL', description: `Convert with ${operation}.`, accept: '*/*', drop: 'drop a file here', result: 'the result will appear below'};
    document.querySelectorAll('[data-operation]').forEach(button => {
      const active = button.dataset.operation === operation;
      button.classList.toggle('is-active', active);
      button.setAttribute('aria-pressed', String(active));
    });
    $('[data-selected-kind]').textContent = def.kind;
    $('[data-selected-title]').textContent = def.title;
    $('[data-selected-description]').textContent = def.description;
    $('[data-selected-code]').textContent = operation;
    $('[data-drop-title]').textContent = def.drop;
    $('[data-convert-hint]').textContent = def.result;
    fileInput.accept = def.accept;
    $('[data-threshold-settings]').hidden = operation !== 'threshold';
    $('[data-halftone-settings]').hidden = operation !== 'halftone';
    $('[data-bayer-settings]').hidden = operation !== 'bayer';
    $('[data-setting-preview]').hidden = !['threshold', 'halftone', 'bayer', 'dither'].includes(operation);
    if (!$('[data-setting-preview]').hidden) {
      $('[data-source-preview]').src = `/convert/${operation}/settings/source.png`;
      updatePreview();
    }
    $('[data-results]').hidden = true;
  }

  let settingsQueue = Promise.resolve();
  let previewTimer;
  function saveSettings() {
    const operation = state.operation;
    const fields = operation === 'threshold' ? [['value', $('[data-threshold]').value]]
      : operation === 'halftone' ? [['density', $('[data-density]').value], ['size', $('[data-dot-size]').value]]
      : operation === 'bayer' ? [['grid', $('[data-grid]').value]] : [];
    const task = settingsQueue.catch(() => {}).then(async () => {
      for (const [field, value] of fields) {
        const response = await fetch(`/convert/${operation}/settings/${field}/${value}.png`, {method: 'DELETE'});
        if (!response.ok) throw new Error(`Setting failed (HTTP ${response.status}).`);
      }
    });
    settingsQueue = task;
    return task;
  }
  async function updatePreview() {
    const operation = state.operation;
    try {
      await saveSettings();
      if (operation === state.operation) {
        $('[data-result-preview]').src = `/convert/${operation}/settings/current.png?revision=${Date.now()}`;
      }
    } catch (error) { showToast(error.message, true); }
  }
  function schedulePreview() {
    $('[data-density-value]').textContent = $('[data-density]').value;
    $('[data-dot-size-value]').textContent = `${$('[data-dot-size]').value} px`;
    clearTimeout(previewTimer);
    previewTimer = setTimeout(updatePreview, 180);
  }
  ['[data-threshold]', '[data-density]', '[data-dot-size]', '[data-grid]'].forEach(selector => {
    $(selector).addEventListener('input', schedulePreview);
  });

  function chooseFile(file) {
    if (!file) return;
    if (file.size > 50 * 1024 * 1024) {
      showToast('That file is over the 50 MiB converter limit.', true);
      return;
    }
    state.file = file;
    $('[data-file-name]').textContent = file.name;
    $('[data-file-size]').textContent = `${humanSize(file.size)} · ${file.type || 'unknown type'}`;
    $('[data-chosen-file]').hidden = false;
    $('[data-convert-label]').textContent = 'convert now';
    convertButton.disabled = false;
  }

  function setProgress(percent, label = 'uploading') {
    $('[data-progress]').hidden = false;
    $('[data-progress-label]').textContent = label;
    $('[data-progress-percent]').textContent = `${percent}%`;
    $('[data-progress-bar]').style.width = `${percent}%`;
  }

  async function listResults() {
    const path = `/convert/${encodeURIComponent(state.operation)}/out/`;
    const files = await propfind(path);
    const list = $('[data-result-list]');
    list.replaceChildren();
    $('[data-results]').hidden = false;
    if (!files.length) {
      const empty = document.createElement('div'); empty.className = 'empty-results'; empty.textContent = 'nothing here yet — outputs expire after about 10 minutes';
      list.append(empty);
      return;
    }
    files.forEach(file => {
      const row = document.createElement('div'); row.className = 'result-row';
      const nameBox = document.createElement('div'); nameBox.className = 'result-name';
      const name = document.createElement('strong'); name.textContent = file.name;
      const meta = document.createElement('span'); meta.textContent = file.size ? humanSize(file.size) : 'ready to download';
      nameBox.append(name, meta);
      const download = document.createElement('a'); download.className = 'result-download'; download.href = file.href; download.download = file.name; download.textContent = 'download';
      const remove = document.createElement('button'); remove.type = 'button'; remove.className = 'result-delete'; remove.textContent = 'delete';
      remove.addEventListener('click', async () => {
        remove.disabled = true;
        const response = await fetch(file.href, {method: 'DELETE'});
        if (!response.ok && response.status !== 204) showToast(`Delete failed (HTTP ${response.status}).`, true);
        await listResults();
      });
      row.append(nameBox, download, remove);
      list.append(row);
    });
  }

  function uploadWithProgress(file, url) {
    return new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.open('PUT', url);
      xhr.setRequestHeader('Content-Type', file.type || 'application/octet-stream');
      xhr.upload.addEventListener('progress', event => {
        if (event.lengthComputable) setProgress(Math.min(92, Math.round(event.loaded / event.total * 92)), 'uploading');
      });
      xhr.addEventListener('load', () => {
        if (xhr.status >= 200 && xhr.status < 300) resolve();
        else reject(new Error(xhr.responseText.trim() || `Conversion failed (HTTP ${xhr.status}).`));
      });
      xhr.addEventListener('error', () => reject(new Error('The upload connection failed.')));
      xhr.send(file);
    });
  }

  async function convert() {
    if (!state.file) return;
    convertButton.disabled = true;
    $('[data-convert-label]').textContent = 'working…';
    setProgress(2);
    try {
      await saveSettings();
      const uploadName = encodeURIComponent(safeName(state.file.name));
      await uploadWithProgress(state.file, `/convert/${encodeURIComponent(state.operation)}/in/${uploadName}`);
      setProgress(96, 'collecting output');
      await listResults();
      setProgress(100, 'done');
      showToast('Conversion finished.');
      $('[data-results]').scrollIntoView({behavior: 'smooth', block: 'nearest'});
    } catch (error) {
      setProgress(0, 'failed');
      showToast(error.message || 'Conversion failed.', true);
    } finally {
      convertButton.disabled = false;
      $('[data-convert-label]').textContent = 'convert again';
      setTimeout(() => { $('[data-progress]').hidden = true; }, 1200);
    }
  }

  async function discover() {
    const stateBox = $('[data-service-state]');
    try {
      const found = (await propfind('/convert/')).map(item => item.name).filter(Boolean);
      if (found.length) {
        state.operations = found;
        if (!found.includes(state.operation)) state.operation = found[0];
      }
      renderTools();
      selectOperation(state.operation);
      stateBox.classList.add('is-online');
      stateBox.querySelector('b').textContent = 'converter online';
    } catch (error) {
      renderTools();
      stateBox.classList.add('is-offline');
      stateBox.querySelector('b').textContent = 'converter unavailable';
      showToast(error.message, true);
    }
  }

  fileInput.addEventListener('change', () => chooseFile(fileInput.files[0]));
  ['dragenter', 'dragover'].forEach(type => dropzone.addEventListener(type, event => { event.preventDefault(); dropzone.classList.add('is-dragging'); }));
  ['dragleave', 'drop'].forEach(type => dropzone.addEventListener(type, event => { event.preventDefault(); dropzone.classList.remove('is-dragging'); }));
  dropzone.addEventListener('drop', event => chooseFile(event.dataTransfer.files[0]));
  convertButton.addEventListener('click', convert);
  $('[data-threshold]').addEventListener('input', event => {
    $('[data-threshold-value]').textContent = `${event.target.value}%`;
  });
  $('[data-refresh-results]').addEventListener('click', () => listResults().catch(error => showToast(error.message, true)));
  document.querySelectorAll('[data-copy-url]').forEach(button => button.addEventListener('click', async () => {
    try { await navigator.clipboard.writeText(button.dataset.copyUrl); showToast('WebDAV address copied.'); }
    catch { showToast('Could not access the clipboard.', true); }
  }));

  discover();
})();
