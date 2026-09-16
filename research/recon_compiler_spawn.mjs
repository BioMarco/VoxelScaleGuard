// What, exactly, can and cannot be launched in this sandbox?
//
// Purpose: `RESULTS.md` §8.4 Blocker A. `AGENTS.md` §7.2 says "CMake and Ninja
// cannot be used here". That note is easy to misread — it invites the conclusion
// that installing a newer CMake would help. This probe separates four questions
// that the note conflates:
//
//   1. Do cmake.exe and ninja.exe run at all?                 (yes)
//   2. Can CMake get past its own `cmake_minimum_required`?   (no — version gate)
//   3. Can CMake launch its own subprocesses?                 (no — "Accesso negato")
//   4. Can Ninja execute a build rule that spawns a process?  (no — it hangs)
//
// Output is captured through FILES, never pipes. That is not a style choice: this
// sandbox denies piped stdio between processes (`EPERM` in Node regardless of the
// program, `StandardOutputEncoding…` in PowerShell), and a probe that pipes reports
// "blocked" for a reason that has nothing to do with the tool it is measuring.
// That exact mistake produced a wrong conclusion the first time this was run, and
// it is why `AGENTS.md` §7.1 exists.
//
// Read-only with respect to the repository: writes only under scratch/.
//
// Usage:
//   node research/recon_compiler_spawn.mjs

import { spawnSync } from 'node:child_process';
import { mkdirSync, writeFileSync, readFileSync, openSync, closeSync, existsSync } from 'node:fs';
import { join, resolve } from 'node:path';

const VS = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community';
const CMAKE = `${VS}\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe`;
const NINJA = `${VS}\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\Ninja\\ninja.exe`;

const dir = resolve('scratch/recon_spawn');
mkdirSync(dir, { recursive: true });

let counter = 0;

/** Run `exe` with stdio on files, so nothing depends on a pipe. */
function run(label, exe, args, timeout = 30000) {
  const outPath = join(dir, `out_${counter}.txt`);
  const errPath = join(dir, `err_${counter}.txt`);
  counter += 1;
  let outFd;
  let errFd;
  try {
    outFd = openSync(outPath, 'w');
    errFd = openSync(errPath, 'w');
  } catch (e) {
    console.log(`?    | ${label} | could not open capture files: ${e.message}`);
    return;
  }
  let r;
  try {
    r = spawnSync(exe, args, { cwd: dir, timeout, stdio: ['ignore', outFd, errFd] });
  } finally {
    closeSync(outFd);
    closeSync(errFd);
  }

  const out = existsSync(outPath) ? readFileSync(outPath, 'utf8') : '';
  const err = existsSync(errPath) ? readFileSync(errPath, 'utf8') : '';
  const timedOut = r.error && r.error.code === 'ETIMEDOUT';
  const status = r.status === null ? (timedOut ? 'HUNG (killed)' : 'no-exit-code') : r.status;
  const why = r.error && !timedOut ? r.error.code : '';
  const firstErr = err.trim().split('\n').filter(Boolean)[0] || '';
  const firstOut = out.trim().split('\n').filter(Boolean)[0] || '';

  console.log(`${r.status === 0 ? 'OK  ' : 'FAIL'} | ${label} | status=${status}${why ? ' | ' + why : ''}`);
  if (firstOut) console.log(`       stdout: ${firstOut}`);
  if (firstErr) console.log(`       stderr: ${firstErr}`);
  return r;
}

console.log('== 1. do the tools start? ==');
run('cmake --version', CMAKE, ['--version']);
run('ninja --version', NINJA, ['--version']);

console.log('\n== 2/3. can CMake configure a project? ==');
mkdirSync(join(dir, 'needs328'), { recursive: true });
writeFileSync(join(dir, 'needs328', 'CMakeLists.txt'),
  'cmake_minimum_required(VERSION 3.28)\nproject(probe CXX)\n');
run('project requires 3.28 (what upstream declares)', CMAKE,
  ['-S', join(dir, 'needs328'), '-B', join(dir, 'needs328', 'build'), '-G', 'Ninja',
   `-DCMAKE_MAKE_PROGRAM=${NINJA}`]);

mkdirSync(join(dir, 'anyver'), { recursive: true });
writeFileSync(join(dir, 'anyver', 'CMakeLists.txt'),
  'cmake_minimum_required(VERSION 3.20)\nproject(probe CXX)\n');
run('project version-agnostic (isolates the version gate)', CMAKE,
  ['-S', join(dir, 'anyver'), '-B', join(dir, 'anyver', 'build'), '-G', 'Ninja',
   `-DCMAKE_MAKE_PROGRAM=${NINJA}`]);

console.log('\n== 4. can Ninja execute a rule? ==');
mkdirSync(join(dir, 'ninja'), { recursive: true });
writeFileSync(join(dir, 'ninja', 'hello.txt'), 'hello\n');
writeFileSync(join(dir, 'ninja', 'build.ninja'),
  'rule copy\n  command = cmd /c copy /y hello.txt out.txt\n  description = copying\n' +
  'build out.txt: copy hello.txt\n');
run('ninja executing a rule (30 s bound)', NINJA, ['-C', join(dir, 'ninja')]);
console.log(`       out.txt produced: ${existsSync(join(dir, 'ninja', 'out.txt'))}`);
