import { readdir, stat, mkdir, copyFile, writeFile, readFile, rm } from 'node:fs/promises';
import { join, relative, dirname, resolve, sep } from 'node:path';
import { createHash } from 'node:crypto';
import { crc32 } from 'node:zlib';

const repo = resolve(import.meta.dirname, '../../..');
const distDir = join(repo, 'misc/web/dist');
const outDir = join(distDir, 'engine');
const gamedata = join(repo, 'res/gamedata');
const PLUS_ARCHIVE = 'patches/xpatch_openxray_plus.db';
const PLUS_EXCLUDE = /\.(txt|gitattributes|py)$/i;
const CHUNK_HEADER = 666;
const CHUNK_DATA = 0;
const CHUNK_FILE_TABLE = 1;

async function walk(dir) {
  const files = [];
  for (const name of (await readdir(dir)).sort()) {
    const full = join(dir, name);
    if ((await stat(full)).isDirectory()) files.push(...(await walk(full)));
    else files.push(full);
  }
  return files;
}

function chunk(id, body) {
  const head = Buffer.alloc(8);
  head.writeUInt32LE(id, 0);
  head.writeUInt32LE(body.length, 4);
  return Buffer.concat([head, body]);
}

function fileHeader(name, size, crc, ptr) {
  const nameBytes = Buffer.from(name, 'latin1');
  const entry = Buffer.alloc(18 + nameBytes.length);
  entry.writeUInt16LE(16 + nameBytes.length, 0);
  entry.writeUInt32LE(size, 2);
  entry.writeUInt32LE(size, 6);
  entry.writeUInt32LE(crc >>> 0, 10);
  nameBytes.copy(entry, 14);
  entry.writeUInt32LE(ptr, 14 + nameBytes.length);
  return entry;
}

function headerSection(ltx) {
  const lines = [];
  let inHeader = false;
  for (const raw of ltx.split(/\r?\n/)) {
    const line = raw.trim();
    if (/^\[.*\]$/.test(line)) {
      inHeader = line.toLowerCase() === '[header]';
      if (inHeader) lines.push(line);
    } else if (inHeader && line && !line.startsWith(';')) {
      lines.push(line.replace(/\s*=\s*/, ' = '));
    }
  }
  const [section, ...keys] = lines;
  return [section, ...keys.sort()].map((line) => line + '\r\n').join('');
}

async function packOpenXRayPlus(files) {
  const header = chunk(CHUNK_HEADER, Buffer.from(headerSection(await readFile(join(repo, 'res/openxray_plus.ltx'), 'latin1')), 'latin1'));
  let offset = header.length + 8;
  const folders = new Set();
  const data = [];
  const table = [];
  const retired = [];
  for (const full of files) {
    const parts = relative(gamedata, full).split(sep);
    for (let i = 1; i < parts.length; i++) folders.add(parts.slice(0, i).join('\\') + '\\');
    const bytes = await readFile(full);
    table.push(fileHeader(parts.join('\\'), bytes.length, crc32(bytes), offset));
    data.push(bytes);
    offset += bytes.length;
    retired.push({
      path: join('gamedata', ...parts).toLowerCase(),
      size: bytes.length,
      sha256: createHash('sha256').update(bytes).digest('hex'),
    });
  }
  const folderEntries = [...folders].sort().map((folder) => fileHeader(folder, 0, 0, 0));
  const archive = Buffer.concat([header, chunk(CHUNK_DATA, Buffer.concat(data)), chunk(CHUNK_FILE_TABLE, Buffer.concat([...folderEntries, ...table]))]);
  return { archive, retired };
}

await rm(outDir, { recursive: true, force: true });
await mkdir(outDir, { recursive: true });
for (const name of await readdir(join(repo, 'misc/web/page')))
  await copyFile(join(repo, 'misc/web/page', name), join(distDir, name));

const files = [];
async function copyLoose(src, rel) {
  const dest = join(outDir, rel);
  await mkdir(dirname(dest), { recursive: true });
  await copyFile(src, dest);
  files.push({ path: rel, size: (await stat(src)).size });
}

await copyLoose(join(repo, 'res/fsgame.ltx'), 'fsgame.ltx');
const plusSources = [];
for (const full of await walk(gamedata)) {
  const rel = relative(gamedata, full);
  if (rel.startsWith('shaders' + sep)) await copyLoose(full, join('gamedata', rel).toLowerCase());
  else if (!PLUS_EXCLUDE.test(rel)) plusSources.push(full);
}

const { archive, retired } = await packOpenXRayPlus(plusSources);
await mkdir(join(outDir, dirname(PLUS_ARCHIVE)), { recursive: true });
await writeFile(join(outDir, PLUS_ARCHIVE), archive);
files.push({ path: PLUS_ARCHIVE, size: archive.length });

await writeFile(join(distDir, 'engine_data.json'), JSON.stringify({ files, retired }));
console.log(`misc/web/dist: page + ${files.length - 1} loose engine files + ${PLUS_ARCHIVE} (${plusSources.length} files)`);
