if (typeof WorkerGlobalScope !== 'undefined' && !globalThis.__xrayErrorHook) {
  globalThis.__xrayErrorHook = true;
  addEventListener('error', (e) => {
    const stack = e.error && e.error.stack ? e.error.stack : '(no stack)';
    console.error('[worker trap] ' + e.message + '\n' + stack);
  });
}
