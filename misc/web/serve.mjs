import { createServer } from 'node:http';
import { stat, readFile, writeFile, mkdir } from 'node:fs/promises';
import { join, extname, resolve } from 'node:path';

const root = resolve(process.argv[2] ?? 'misc/web/dist');
const port = Number(process.argv[3] ?? 8080);

const mime = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript',
  '.mjs': 'text/javascript',
  '.wasm': 'application/wasm',
  '.json': 'application/json',
  '.map': 'application/json',
  '.css': 'text/css',
};

createServer(async (req, res) => {
  const urlPath = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  const headers = {
    'Cross-Origin-Opener-Policy': 'same-origin',
    'Cross-Origin-Embedder-Policy': 'require-corp',
    'Cache-Control': 'no-store',
  };
  if (req.method === 'POST' && urlPath.startsWith('/save/')) {
    const name = urlPath.slice('/save/'.length).replace(/[^\w.-]/g, '_');
    const chunks = [];
    for await (const chunk of req) chunks.push(chunk);
    await mkdir('misc/web/out', { recursive: true });
    await writeFile(join('misc/web/out', name), Buffer.concat(chunks));
    res.writeHead(200, headers).end('saved ' + name);
    return;
  }
  let file = join(root, urlPath);
  if (!file.startsWith(root)) {
    res.writeHead(403).end();
    return;
  }
  try {
    if ((await stat(file)).isDirectory()) file = join(file, 'index.html');
    const body = await readFile(file);
    res.writeHead(200, { ...headers, 'Content-Type': mime[extname(file)] ?? 'application/octet-stream' });
    res.end(body);
  } catch {
    res.writeHead(404).end('not found: ' + urlPath);
  }
}).listen(port, () => console.log(`serving ${root} at http://localhost:${port}`));
