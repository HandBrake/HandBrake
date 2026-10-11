// Runs OpenDLSS-NR's WebGPU port on one set of input features, in headless Chromium on this machine's GPU,
// and writes the head it computes. The half of `opendlss_reference.py` that has to live in a browser.
//
//   node opendlss_reference.mjs <port root> <model dir> <io dir> <valid width> <valid height> [capture]
//
// <port root> is ports/browser-webgpu of a clone of maanHimself/OpenDLSS-NR, served as it is: the page below
// imports its modules and shaders straight from there. <io dir> holds features.bin (f32, [field rows][16])
// and receives head.bin (f32, [field rows][4]) and result.json. With `capture` it also receives every block
// boundary the port records, boundary-<name>.bin (E4M3 bytes, [rows][channels]), listed with their shapes in
// boundaries.json. CHROME names the browser (/usr/bin/chromium).
//
// Headless Chromium picks SwiftShader, a CPU implementation of WebGPU, unless it is told the GPU is allowed:
// with --ignore-gpu-blocklist and the Vulkan backend it reports "intel / xe-2lpg". (Adding
// VulkanFromANGLE to the features leaves it with no adapter at all.)

import { createServer } from 'node:http';
import { readFile, writeFile, mkdtemp, rm } from 'node:fs/promises';
import { spawn } from 'node:child_process';
import { tmpdir } from 'node:os';
import { extname, join, normalize, resolve, sep } from 'node:path';

const [portRoot, modelDir, ioDir, width, height, mode] = process.argv.slice(2);
const capture = mode === 'capture';
const boundaries = [];
if (!height) {
  console.error('usage: opendlss_reference.mjs <port root> <model dir> <io dir> <width> <height>');
  process.exit(2);
}

const page = `<!doctype html><meta charset="utf-8"><title>reference</title><script type="module">
import { Network } from '/src/network.js';
const done = (body) => fetch('/done', { method: 'POST', body: JSON.stringify(body) });
try {
  const network = await Network.create({ weights: '/weights', width: ${Number(width)}, height: ${Number(height)},
                                          captureBoundaries: ${capture} });
  const g = network.geometry;
  const features = new Float32Array(await (await fetch('/io/features.bin')).arrayBuffer());
  if (features.length !== g.fullRows * 16)
    throw new Error('features hold ' + features.length / 16 + ' rows; the field ' + g.fullWidth + 'x' +
                    g.fullHeight + ' has ' + g.fullRows);
  network.writeFeatures(features);
  const times = [];
  for (let run = 0; run < 3; ++run) {
    const start = performance.now();
    await network.run();
    times.push(performance.now() - start);
  }
  const head = await network.readHead();
  await fetch('/head', { method: 'POST', body: head.buffer });
  for (const name of network.boundaryNames) {
    const tensor = network.graph.boundaries.get(name);
    const bytes = await network.readBoundary(name);
    await fetch('/boundary?name=' + name + '&rows=' + tensor.rows + '&channels=' + tensor.channels +
                '&format=' + tensor.format, { method: 'POST', body: bytes });
  }
  const info = network.adapterInfo ?? {};
  await done({ ok: true, field: [g.fullWidth, g.fullHeight], times,
               adapter: (info.vendor ?? '') + ' / ' + (info.architecture ?? '') });
} catch (error) {
  await done({ ok: false, error: String(error && error.stack || error) });
}
</script>`;

const types = { '.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript',
                '.json': 'application/json', '.wgsl': 'text/plain', '.bin': 'application/octet-stream' };

function inside(base, relative) {
  const target = join(base, normalize(relative));
  return target === base || target.startsWith(base + sep) ? target : null;
}

let finished = false;
const server = createServer(async (request, response) => {
  try {
    const url = new URL(request.url, 'http://localhost');
    if (request.method === 'POST') {
      const chunks = [];
      for await (const chunk of request) chunks.push(chunk);
      const body = Buffer.concat(chunks);
      // written before the answer: the page reports done as soon as this returns, and the process exits
      if (url.pathname === '/head') await writeFile(join(ioDir, 'head.bin'), body);
      if (url.pathname === '/boundary') {
        const name = url.searchParams.get('name');
        if (!/^[a-z0-9-]+$/.test(name)) throw new Error('bad boundary name');
        await writeFile(join(ioDir, `boundary-${name}.bin`), body);
        boundaries.push({ name, rows: Number(url.searchParams.get('rows')),
                          channels: Number(url.searchParams.get('channels')),
                          format: url.searchParams.get('format'), file: `boundary-${name}.bin` });
      }
      response.writeHead(204).end();
      if (url.pathname === '/done') finish(body.toString());
      return;
    }
    if (url.pathname === '/io/reference.html') {
      response.writeHead(200, { 'content-type': 'text/html' }).end(page);
      return;
    }
    const path = decodeURIComponent(url.pathname);
    const target = path.startsWith('/weights/') ? inside(resolve(modelDir), path.slice(9))
                 : path.startsWith('/io/') ? inside(resolve(ioDir), path.slice(4))
                 : inside(resolve(portRoot), path.slice(1));
    if (!target) { response.writeHead(403).end(); return; }
    const body = await readFile(target);
    response.writeHead(200, { 'content-type': types[extname(target)] ?? 'application/octet-stream',
                              'cache-control': 'no-store' }).end(body);
  } catch (error) {
    response.writeHead(error.code === 'ENOENT' ? 404 : 500).end(String(error));
  }
});
await new Promise((ready) => server.listen(0, '127.0.0.1', ready));
const port = server.address().port;
const profile = await mkdtemp(join(tmpdir(), 'opendlss-'));

const browser = spawn(process.env.CHROME ?? '/usr/bin/chromium', [
  '--headless=new', '--enable-unsafe-webgpu', '--ignore-gpu-blocklist', '--use-angle=vulkan',
  '--use-vulkan=native', '--enable-features=Vulkan',
  `--user-data-dir=${profile}`, '--no-first-run', '--no-default-browser-check', '--disable-extensions',
  `http://127.0.0.1:${port}/io/reference.html`,
], { stdio: ['ignore', 'ignore', 'ignore'] });

const timeout = setTimeout(() => finish(JSON.stringify({ ok: false, error: 'timed out' })),
                           Number(process.env.OPENDLSS_TIMEOUT_MS ?? 900000));

async function finish(body) {
  if (finished) return;
  finished = true;
  clearTimeout(timeout);
  if (capture) await writeFile(join(ioDir, 'boundaries.json'), JSON.stringify(boundaries, null, 1));
  await writeFile(join(ioDir, 'result.json'), body);
  console.log(body);
  try { browser.kill(); } catch { /* already gone */ }
  server.close();
  await rm(profile, { recursive: true, force: true }).catch(() => {});
  process.exit(JSON.parse(body).ok ? 0 : 1);
}
