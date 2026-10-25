// Minimal service worker: only there so browsers that still ask for one offer to install the app.
// Everything goes to the network; the controller has to be reachable to be controlled anyway.
// Browsers only run it over HTTPS (see main.ts), so plain http://matrixcontroller.local ignores it.
self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', event => event.waitUntil(self.clients.claim()));
self.addEventListener('fetch', () => {});
