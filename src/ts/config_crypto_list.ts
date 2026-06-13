// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Clay custom component "cryptoList": a reorderable list of crypto/currency
   coins. Value = array of { wid, coin, vs, p, label }. Each row has a popular-
   coin <select> (+ "Custom…" revealing a free-text CoinGecko id), a vs-currency
   <select> (USD/EUR), a numeric precision input `p`, an optional label input,
   and up/down/remove buttons. Each row carries a STABLE wid (data-wid): legacy
   migrated rows keep 15/16/17; new rows get the smallest free id in [200,216).
   The widget-list component reads these rows live from the DOM to build its
   crypto options (see config_widget_list.ts).

   Serialized via toSource() + re-eval'd in the webview -> fully self-contained:
   no imports at runtime, no spread/destructuring, no module-scope helpers, no
   TS downlevel helpers. Native DOM + native array methods only. The row
   controls are the source of truth, read back on `get`. See
   docs/superpowers/specs/2026-06-13-timestyle-generic-crypto-widgets-design.md */

function cryptoListInitialize(this: any, _minified: any, _clayConfig: any): void {
  const self = this;
  const root: HTMLElement = self.$element[0];
  const CRYPTO_BASE = 200;
  const MAX_CRYPTO = 16;   // matches MAX_CRYPTO in src/c/crypto.h

  const COINS: { id: string; label: string; p: number }[] = [
    { id: 'bitcoin', label: 'BTC', p: -3 },
    { id: 'ethereum', label: 'ETH', p: 0 },
    { id: 'tether', label: 'USDT', p: 2 },
    { id: 'binancecoin', label: 'BNB', p: 0 },
    { id: 'solana', label: 'SOL', p: 1 },
    { id: 'ripple', label: 'XRP', p: 4 },
    { id: 'cardano', label: 'ADA', p: 4 },
    { id: 'dogecoin', label: 'DOGE', p: 4 },
    { id: 'tron', label: 'TRX', p: 4 },
    { id: 'polkadot', label: 'DOT', p: 2 },
    { id: 'chainlink', label: 'LINK', p: 2 },
    { id: 'matic-network', label: 'MATIC', p: 4 },
    { id: 'litecoin', label: 'LTC', p: 1 },
    { id: 'avalanche-2', label: 'AVAX', p: 1 },
    { id: 'monero', label: 'XMR', p: 0 },
    { id: 'stellar', label: 'XLM', p: 4 },
    { id: 'cosmos', label: 'ATOM', p: 2 },
    { id: 'uniswap', label: 'UNI', p: 2 },
    { id: 'shiba-inu', label: 'SHIB', p: 8 },
    { id: 'euro-coin', label: 'EUR', p: 3 },
  ];

  function escAttr(s: string): string {
    return String(s).replace(/&/g, '&amp;').replace(/"/g, '&quot;')
      .replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  function coinOptionsHtml(selected: string): string {
    let html = '';
    let known = false;
    for (let i = 0; i < COINS.length; i++) {
      const c = COINS[i];
      if (c.id === selected) { known = true; }
      html += '<option value="' + escAttr(c.id) + '"' +
        (c.id === selected ? ' selected' : '') + '>' + escAttr(c.label) + '</option>';
    }
    const isCustom = (!known && selected !== '');
    html += '<option value="custom"' + (isCustom ? ' selected' : '') + '>Custom…</option>';
    return html;
  }

  function rowHtml(row: any): string {
    const coin = (row && typeof row.coin === 'string') ? row.coin : 'bitcoin';
    const vs = (row && row.vs === 'eur') ? 'eur' : 'usd';
    const p = (row && row.p !== undefined && row.p !== null) ? row.p : -3;
    const label = (row && typeof row.label === 'string') ? row.label : '';
    const wid = (row && row.wid !== undefined) ? parseInt(row.wid, 10) : 0;
    let isCustom = true;
    for (let i = 0; i < COINS.length; i++) { if (COINS[i].id === coin) { isCustom = false; } }
    return '<div class="cl-row" data-wid="' + wid + '">' +
      '<div class="cl-line">' +
        '<select class="cl-coin">' + coinOptionsHtml(coin) + '</select>' +
        '<select class="cl-vs">' +
          '<option value="usd"' + (vs === 'usd' ? ' selected' : '') + '>USD</option>' +
          '<option value="eur"' + (vs === 'eur' ? ' selected' : '') + '>EUR</option>' +
        '</select>' +
      '</div>' +
      '<div class="cl-line">' +
        '<input class="cl-custom" type="text" placeholder="coingecko id" value="' +
          (isCustom ? escAttr(coin) : '') + '"' + (isCustom ? '' : ' style="display:none"') + '>' +
        '<input class="cl-p" type="number" step="1" title="precision" value="' + escAttr(String(p)) + '">' +
        '<input class="cl-label" type="text" placeholder="label" maxlength="5" value="' + escAttr(label) + '">' +
      '</div>' +
      '<div class="cl-line">' +
        '<button type="button" class="cl-up" title="Move up">&#9650;</button>' +
        '<button type="button" class="cl-down" title="Move down">&#9660;</button>' +
        '<button type="button" class="cl-del" title="Remove">&#10005;</button>' +
      '</div>' +
      '</div>';
  }

  function defaultPFor(coinId: string): number {
    for (let i = 0; i < COINS.length; i++) { if (COINS[i].id === coinId) { return COINS[i].p; } }
    return 2;
  }

  function rowCoinId(rowEl: HTMLElement): string {
    const sel = rowEl.querySelector('.cl-coin') as HTMLSelectElement;
    if (sel && sel.value === 'custom') {
      const cust = rowEl.querySelector('.cl-custom') as HTMLInputElement;
      return cust ? cust.value : '';
    }
    return sel ? sel.value : '';
  }

  function currentRows(): any[] {
    const rows: any[] = [];
    const rowEls = root.querySelectorAll('.cl-row');
    for (let i = 0; i < rowEls.length; i++) {
      const el = rowEls[i] as HTMLElement;
      const vsSel = el.querySelector('.cl-vs') as HTMLSelectElement;
      const pIn = el.querySelector('.cl-p') as HTMLInputElement;
      const labelIn = el.querySelector('.cl-label') as HTMLInputElement;
      const wid = parseInt(el.getAttribute('data-wid') || '0', 10) || 0;
      const p = parseInt(pIn ? pIn.value : '0', 10);
      rows.push({
        wid: wid,
        coin: rowCoinId(el),
        vs: (vsSel && vsSel.value === 'eur') ? 'eur' : 'usd',
        p: isNaN(p) ? 0 : p,
        label: labelIn ? labelIn.value : '',
      });
    }
    return rows;
  }

  function usedWids(): number[] {
    const out: number[] = [];
    const rowEls = root.querySelectorAll('.cl-row');
    for (let i = 0; i < rowEls.length; i++) {
      out.push(parseInt((rowEls[i] as HTMLElement).getAttribute('data-wid') || '0', 10) || 0);
    }
    return out;
  }

  function nextFreeWid(): number {
    const used = usedWids();
    for (let w = CRYPTO_BASE; w < CRYPTO_BASE + MAX_CRYPTO; w++) {
      if (used.indexOf(w) === -1) { return w; }
    }
    return CRYPTO_BASE;
  }

  function updateButtons(): void {
    const rowEls = root.querySelectorAll('.cl-row');
    for (let i = 0; i < rowEls.length; i++) {
      (rowEls[i].querySelector('.cl-up') as HTMLButtonElement).disabled = (i === 0);
      (rowEls[i].querySelector('.cl-down') as HTMLButtonElement).disabled =
        (i === rowEls.length - 1);
    }
    const add = root.querySelector('.cl-add') as HTMLButtonElement;
    if (add) { add.style.display = (rowEls.length >= MAX_CRYPTO) ? 'none' : ''; }
  }

  function rebuild(rows: any[]): void {
    const list = root.querySelector('.cl-list') as HTMLElement;
    let html = '';
    for (let i = 0; i < rows.length && i < MAX_CRYPTO; i++) { html += rowHtml(rows[i]); }
    list.innerHTML = html;
    updateButtons();
  }

  self._clCurrentRows = currentRows;
  self._clRebuild = rebuild;

  function rowIndexOf(node: Node | null): number {
    const rowEls = root.querySelectorAll('.cl-row');
    for (let i = 0; i < rowEls.length; i++) { if (rowEls[i] === node) { return i; } }
    return -1;
  }
  function swap(arr: any[], a: number, b: number): void {
    const tmp = arr[a]; arr[a] = arr[b]; arr[b] = tmp;
  }

  root.addEventListener('click', function(ev: Event) {
    let target = ev.target as HTMLElement;
    if (!target) { return; }
    if (target.tagName !== 'BUTTON') {
      target = target.closest ? (target.closest('button') as HTMLElement) : null as any;
    }
    if (!target) { return; }

    if (target.classList.contains('cl-add')) {
      const rows = currentRows();
      if (rows.length < MAX_CRYPTO) {
        rows.push({ wid: nextFreeWid(), coin: 'bitcoin', vs: 'usd', p: defaultPFor('bitcoin'), label: '' });
        rebuild(rows);
        self.trigger('change');
      }
      return;
    }

    const rowEl = (target.closest ? target.closest('.cl-row') : null) as HTMLElement;
    const idx = rowIndexOf(rowEl);
    if (idx === -1) { return; }
    const rows = currentRows();
    if (target.classList.contains('cl-del')) {
      rows.splice(idx, 1);
      rebuild(rows);
      self.trigger('change');
    } else if (target.classList.contains('cl-up') && idx > 0) {
      swap(rows, idx, idx - 1);
      rebuild(rows);
      self.trigger('change');
    } else if (target.classList.contains('cl-down') && idx < rows.length - 1) {
      swap(rows, idx, idx + 1);
      rebuild(rows);
      self.trigger('change');
    }
  });

  root.addEventListener('change', function(ev: Event) {
    const t = ev.target as HTMLElement;
    if (!t) { return; }
    if (t.classList.contains('cl-coin')) {
      const rowEl = (t.closest ? t.closest('.cl-row') : null) as HTMLElement;
      if (rowEl) {
        const sel = t as HTMLSelectElement;
        const cust = rowEl.querySelector('.cl-custom') as HTMLInputElement;
        const pIn = rowEl.querySelector('.cl-p') as HTMLInputElement;
        if (sel.value === 'custom') {
          if (cust) { cust.style.display = ''; }
        } else {
          if (cust) { cust.style.display = 'none'; }
          if (pIn) { pIn.value = String(defaultPFor(sel.value)); }
        }
      }
    }
    self.trigger('change');
  });
}

const cryptoListComponent = {
  name: 'cryptoList',
  template:
    '<div class="cl-root">' +
    '<div class="cl-list"></div>' +
    '<button type="button" class="cl-add">+ Add crypto</button>' +
    '</div>',
  style:
    '.cl-row{border-bottom:1px solid #555;padding:6px 0;margin:0 0 6px 0}' +
    '.cl-line{display:flex;align-items:center;margin:0 0 4px 0}' +
    '.cl-row select,.cl-row input{flex:1 1 auto;min-width:0;height:2.6rem;margin:0 4px 0 0;' +
      'background-color:#767676;color:#fff;border:none;border-radius:0.3rem;' +
      'padding:0 0.4rem;color-scheme:dark}' +
    '.cl-row .cl-p{flex:0 0 4rem}' +
    '.cl-row button{flex:0 0 auto;min-width:0;width:2.6rem;height:2.6rem;margin:0 6px 0 0;padding:0}' +
    '.cl-row button[disabled]{opacity:.35}' +
    '.cl-add{margin:8px 0 10px 0}',
  manipulator: {
    get: function(this: any): any[] {
      return this._clCurrentRows ? this._clCurrentRows() : [];
    },
    set: function(this: any, value: any) {
      let rows: any[] = [];
      let v: any = value;
      if (v && typeof v === 'object' && !Array.isArray(v) && v.value !== undefined) {
        v = v.value;
      }
      if (Array.isArray(v)) {
        rows = v;
      } else if (typeof v === 'string' && v !== '') {
        try { const parsed = JSON.parse(v); if (Array.isArray(parsed)) { rows = parsed; } }
        catch (e) { rows = []; }
      }
      if (this._clRebuild) { this._clRebuild(rows); }
      return this;
    },
  },
  defaults: { label: '' },
  initialize: cryptoListInitialize,
};

export = cryptoListComponent;
