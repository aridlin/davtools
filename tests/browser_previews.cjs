// PLAYWRIGHT_MODULE may point to an existing Playwright installation.
const {firefox} = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
(async () => {
  const browser = await firefox.launch({headless:true, ...(process.env.FIREFOX_PATH ? {executablePath:process.env.FIREFOX_PATH} : {})});
  try {
    for (const width of [1366,390]) {
      const page=await browser.newPage({viewport:{width,height:900}});
      const errors=[];
      page.on('pageerror', e=>errors.push(e.message));
      await page.goto(process.env.TEST_URL || 'http://127.0.0.1:8081/', {waitUntil:'networkidle'});
      for (const op of ['threshold','halftone','bayer','dither']) {
        await page.locator(`[data-operation="${op}"]`).click();
        await page.waitForFunction(op => {
          const img=document.querySelector('[data-result-preview]');
          return img.src.includes('/'+op+'/') && img.complete && img.naturalWidth>0;
        },op);
        if (op==='halftone') {
          await page.locator('[data-density]').fill('70');
          await page.locator('[data-density]').dispatchEvent('input');
          await page.locator('[data-dot-size]').fill('12');
          await page.locator('[data-dot-size]').dispatchEvent('input');
        }
        if (op==='dither') {
          if (!await page.locator('[data-dither-settings]').isVisible()) throw new Error('Dither settings hidden');
          await page.locator('[data-dither-method]').selectOption('2');
          await page.locator('[data-tone]').fill('65');
          await page.locator('[data-tone]').dispatchEvent('input');
          await page.locator('[data-grain]').fill('2');
          await page.locator('[data-grain]').dispatchEvent('input');
        }
        if (op==='bayer') await page.locator('[data-grid]').selectOption('8');
        await page.waitForFunction(op => {
          const img=document.querySelector('[data-result-preview]');
          const query=new URL(img.src).searchParams;
          const matches = op === 'dither' ? query.get('method') === '2' && query.get('tone') === '65' && query.get('grain') === '2'
            : op === 'halftone' ? query.get('density') === '70' && query.get('size') === '12'
            : op === 'bayer' ? query.get('grid') === '8' : true;
          return matches && document.querySelector('[data-preview-status]').textContent.includes('1600 × 1287') && img.complete && img.naturalWidth === 1600;
        },op);
        if (op==='halftone' || op==='dither') {
          await page.waitForTimeout(800);
          await page.screenshot({path:`/tmp/davtools-${op}-${width}.png`,fullPage:true});
        }
        await page.locator('[data-file-input]').setInputFiles(process.env.TEST_IMAGE || 'web/bliss.jpg');
        await page.locator('[data-convert]').click();
        await page.locator('[data-results]').waitFor({state:'visible'});
        if (await page.locator('[data-toast]').textContent() !== 'Conversion finished.') throw new Error('Conversion failed for '+op);
      }
      if (await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth+1)) throw new Error('Horizontal overflow');
      if (errors.length) throw new Error(errors.join('\n'));
      await page.screenshot({path:`/tmp/davtools-previews-${width}.png`,fullPage:true});
      console.log('PASS Firefox',width,'preview controls and all four uploads');
      await page.close();
    }
  } finally { await browser.close(); }
})().catch(e=>{console.error(e);process.exit(1)});
