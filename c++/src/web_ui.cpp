#include "web_ui.hpp"

std::string_view davtools_web_ui() {
    return R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>davtools · Image threshold</title>
  <style>
    :root { color-scheme: dark; font-family: Inter, ui-sans-serif, system-ui, sans-serif; }
    * { box-sizing: border-box; }
    body { margin: 0; min-height: 100vh; background: #0b0d10; color: #f4f5f7; }
    main { width: min(960px, calc(100% - 32px)); margin: 0 auto; padding: 56px 0 72px; }
    header { display: flex; justify-content: space-between; gap: 24px; align-items: end; margin-bottom: 28px; }
    h1 { margin: 0; font-size: clamp(2rem, 5vw, 4rem); letter-spacing: -.055em; }
    header p { max-width: 430px; margin: 0; color: #aeb4bd; line-height: 1.5; }
    .panel { background: #15181d; border: 1px solid #2a2f37; border-radius: 22px; padding: 22px; box-shadow: 0 24px 70px #0008; }
    .drop { display: grid; place-items: center; min-height: 190px; border: 1px dashed #505866; border-radius: 16px; cursor: pointer; text-align: center; padding: 24px; transition: .18s ease; }
    .drop:hover, .drop.drag { border-color: #d8ff59; background: #d8ff5909; }
    .drop strong { font-size: 1.15rem; }
    .drop span { display: block; margin-top: 8px; color: #929aa6; }
    input[type=file] { position: absolute; opacity: 0; pointer-events: none; }
    .controls { display: grid; grid-template-columns: 1fr auto; gap: 18px; align-items: end; margin-top: 20px; }
    label { display: block; color: #c5cad1; font-size: .9rem; }
    .value { color: #d8ff59; font-variant-numeric: tabular-nums; }
    input[type=range] { width: 100%; accent-color: #d8ff59; margin-top: 12px; }
    button, .download { border: 0; border-radius: 12px; padding: 13px 20px; background: #d8ff59; color: #111; font-weight: 750; cursor: pointer; text-decoration: none; }
    button:disabled { opacity: .4; cursor: not-allowed; }
    .status { min-height: 24px; margin: 18px 0 0; color: #aeb4bd; }
    .status.error { color: #ff8c8c; }
    .previews { display: grid; grid-template-columns: 1fr 1fr; gap: 18px; margin-top: 22px; }
    figure { margin: 0; min-height: 240px; border-radius: 16px; background: #0e1013; border: 1px solid #292e35; overflow: hidden; }
    figcaption { padding: 12px 14px; color: #aeb4bd; border-bottom: 1px solid #292e35; }
    figure img { display: block; width: 100%; height: 330px; object-fit: contain; image-rendering: auto; }
    .actions { display: flex; justify-content: flex-end; margin-top: 16px; }
    .download[hidden], .previews[hidden] { display: none; }
    @media (max-width: 680px) { header { display: block; } header p { margin-top: 14px; } .controls, .previews { grid-template-columns: 1fr; } button { width: 100%; } }
  </style>
</head>
<body>
<main>
  <header>
    <h1>Image threshold</h1>
    <p>Turn any image into a crisp black-and-white PNG. The converter is available through this page and the same davtools WebDAV endpoint.</p>
  </header>
  <section class="panel">
    <label class="drop" id="drop" for="file">
      <div><strong id="file-name">Choose an image</strong><span>or drop one here · up to 50 MB</span></div>
    </label>
    <input id="file" type="file" accept="image/*">
    <div class="controls">
      <label>Threshold <span class="value" id="threshold-value">50%</span>
        <input id="threshold" type="range" min="1" max="100" value="50">
      </label>
      <button id="convert" type="button" disabled>Convert image</button>
    </div>
    <p class="status" id="status" aria-live="polite">Select an image to begin.</p>
    <div class="previews" id="previews" hidden>
      <figure><figcaption>Original</figcaption><img id="original" alt="Original image preview"></figure>
      <figure><figcaption>Threshold result</figcaption><img id="result" alt="Threshold conversion result"></figure>
    </div>
    <div class="actions"><a class="download" id="download" hidden>Download PNG</a></div>
  </section>
</main>
<script>
(() => {
  const fileInput = document.querySelector('#file');
  const drop = document.querySelector('#drop');
  const button = document.querySelector('#convert');
  const slider = document.querySelector('#threshold');
  const value = document.querySelector('#threshold-value');
  const status = document.querySelector('#status');
  const previews = document.querySelector('#previews');
  const original = document.querySelector('#original');
  const result = document.querySelector('#result');
  const download = document.querySelector('#download');
  let selected = null;
  let originalUrl = null;
  let resultUrl = null;

  const outputName = name => `${name.replace(/\.[^.]*$/, '')}_threshold.png`;
  const setStatus = (message, error = false) => { status.textContent = message; status.classList.toggle('error', error); };
  const selectFile = file => {
    if (!file) return;
    selected = file;
    document.querySelector('#file-name').textContent = file.name;
    button.disabled = false;
    if (originalUrl) URL.revokeObjectURL(originalUrl);
    originalUrl = URL.createObjectURL(file);
    original.src = originalUrl;
    previews.hidden = false;
    result.removeAttribute('src');
    download.hidden = true;
    setStatus('Ready to convert.');
  };

  slider.addEventListener('input', () => value.textContent = `${slider.value}%`);
  fileInput.addEventListener('change', () => selectFile(fileInput.files[0]));
  ['dragenter', 'dragover'].forEach(type => drop.addEventListener(type, event => { event.preventDefault(); drop.classList.add('drag'); }));
  ['dragleave', 'drop'].forEach(type => drop.addEventListener(type, event => { event.preventDefault(); drop.classList.remove('drag'); }));
  drop.addEventListener('drop', event => selectFile(event.dataTransfer.files[0]));

  button.addEventListener('click', async () => {
    if (!selected) return;
    button.disabled = true;
    download.hidden = true;
    setStatus('Applying threshold…');
    try {
      const setting = await fetch(`/convert/threshold/settings/value/${slider.value}`, { method: 'DELETE' });
      if (!setting.ok) throw new Error(`Could not set threshold (${setting.status})`);
      const inputPath = `/convert/threshold/in/${encodeURIComponent(selected.name)}`;
      const upload = await fetch(inputPath, { method: 'PUT', body: selected });
      if (!upload.ok) throw new Error((await upload.text()) || `Conversion failed (${upload.status})`);
      const name = outputName(selected.name);
      const response = await fetch(`/convert/threshold/out/${encodeURIComponent(name)}`);
      if (!response.ok) throw new Error(`Could not fetch result (${response.status})`);
      const blob = await response.blob();
      if (resultUrl) URL.revokeObjectURL(resultUrl);
      resultUrl = URL.createObjectURL(blob);
      result.src = resultUrl;
      download.href = resultUrl;
      download.download = name;
      download.hidden = false;
      setStatus(`Done · ${slider.value}% threshold`);
    } catch (error) {
      setStatus(error.message || 'Conversion failed.', true);
    } finally {
      button.disabled = false;
    }
  });
})();
</script>
</body>
</html>)HTML";
}
