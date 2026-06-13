// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Tuomas Airaksinen

/* Clay custom component "widgetList": a reorderable list of 0-6 widget slots,
   replacing the six fixed `select` dropdowns. Clay serializes this whole object
   via toSource() and re-evals it inside the config webview, so — exactly like
   config_clay_custom.ts — every function here MUST be self-contained: no
   require()/import referenced at runtime, no TS downlevel helpers
   (__spreadArray/_this), no spread/destructuring, no closure over module scope.
   Native DOM + native array methods only. The component value is an array of
   widget IDs (ints) in slot order; the row <select>s are the source of truth.
   See docs/superpowers/specs/2026-06-11-timestyle-widget-list-reorder-design.md */

// `initialize` runs before `set` (clay-config.js build order), so it stashes the
// render/read closures on the item instance for the manipulator to call.
function widgetListInitialize(this: any, _minified: any, _clayConfig: any): void {
  const self = this;
  const root: HTMLElement = self.$element[0];
  const MAX = 16;   // matches MAX_WIDGET_LIST in src/c/sidebar_widgets.h

  // Static widget options, embedded here (cannot reference module scope at
  // runtime — this function is re-eval'd in isolation). id -> label.
  // Crypto coin entries are NOT listed here; they are read live from the
  // cryptoList component's DOM and appended dynamically by currentOptions().
  // KEEP IN SYNC with the widget ids/labels in config_clay.ts and
  // src/c/sidebar_widgets.c.
  const STATIC_OPTIONS: { id: number; label: string }[] = [
    { id: 0, label: 'Empty' },
    { id: 3, label: 'Alternate Time Zone' },
    { id: 5, label: 'Seconds' },
    { id: 11, label: 'Swatch Internet Time' },
    { id: 4, label: "Today's Date" },
    { id: 6, label: 'Week Number' },
    { id: 7, label: 'Current Weather' },
    { id: 8, label: "Today's Forecast" },
    { id: 13, label: 'UV Index' },
    { id: 14, label: 'Porssisahko' },
    { id: 18, label: 'Seuraava halpa sahko' },
    { id: 19, label: 'Halvin sahkotunti' },
    { id: 9, label: 'Sleep' },
    { id: 10, label: 'Steps' },
    { id: 12, label: 'Heart Rate' },
    { id: 2, label: 'Battery' },
  ];

  // Build the option list live: static widgets + one per cryptoList row.
  // Read the crypto rows straight from the cryptoList component's DOM (same
  // config page), so a coin added there appears here without a save/reopen.
  function readCryptoRows(): { wid: number; coin: string; label: string }[] {
    const out: { wid: number; coin: string; label: string }[] = [];
    const rowEls = document.querySelectorAll('.cl-root .cl-row');
    for (let i = 0; i < rowEls.length; i++) {
      const el = rowEls[i] as HTMLElement;
      const wid = parseInt(el.getAttribute('data-wid') || '0', 10) || 0;
      const coinSel = el.querySelector('.cl-coin') as HTMLSelectElement;
      let coin = coinSel ? coinSel.value : '';
      if (coin === 'custom') {
        const cust = el.querySelector('.cl-custom') as HTMLInputElement;
        coin = cust ? cust.value : '';
      }
      const labelIn = el.querySelector('.cl-label') as HTMLInputElement;
      out.push({ wid: wid, coin: coin, label: labelIn ? labelIn.value : '' });
    }
    return out;
  }
  function currentOptions(): { id: number; label: string }[] {
    const out: { id: number; label: string }[] = STATIC_OPTIONS.slice();
    const arr = readCryptoRows();
    for (let i = 0; i < arr.length; i++) {
      const r = arr[i];
      if (!r) { continue; }
      const wid = r.wid;
      if (isNaN(wid) || wid === 0) { continue; }
      const coin = (typeof r.coin === 'string') ? r.coin : '';
      const label = (typeof r.label === 'string' && r.label !== '')
        ? r.label : coin.toUpperCase();
      out.push({ id: wid, label: label });
    }
    return out;
  }

  function optionsHtml(selected: number): string {
    const opts = currentOptions();
    let html = '';
    for (let i = 0; i < opts.length; i++) {
      const o = opts[i];
      html += '<option value="' + o.id + '"' +
        (o.id === selected ? ' selected' : '') + '>' + o.label + '</option>';
    }
    return html;
  }

  function rowHtml(selected: number): string {
    return '<div class="wl-row">' +
      '<select class="wl-sel">' + optionsHtml(selected) + '</select>' +
      '<button type="button" class="wl-up" title="Move up">&#9650;</button>' +
      '<button type="button" class="wl-down" title="Move down">&#9660;</button>' +
      '<button type="button" class="wl-del" title="Remove">&#10005;</button>' +
      '</div>';
  }

  function currentIds(): number[] {
    const ids: number[] = [];
    const selects = root.querySelectorAll('.wl-row .wl-sel');
    for (let i = 0; i < selects.length; i++) {
      const sel = selects[i] as HTMLSelectElement;
      ids.push(parseInt(sel.value, 10) || 0);
    }
    return ids;
  }

  function updateButtons(): void {
    const rows = root.querySelectorAll('.wl-row');
    for (let i = 0; i < rows.length; i++) {
      (rows[i].querySelector('.wl-up') as HTMLButtonElement).disabled = (i === 0);
      (rows[i].querySelector('.wl-down') as HTMLButtonElement).disabled =
        (i === rows.length - 1);
    }
    const add = root.querySelector('.wl-add') as HTMLButtonElement;
    if (add) { add.style.display = (rows.length >= MAX) ? 'none' : ''; }
  }

  function rebuild(ids: number[]): void {
    const list = root.querySelector('.wl-list') as HTMLElement;
    let html = '';
    for (let i = 0; i < ids.length && i < MAX; i++) {
      html += rowHtml(parseInt(ids[i] as any, 10) || 0);
    }
    list.innerHTML = html;
    updateButtons();
  }

  // expose for the manipulator (set runs after initialize)
  self._wlCurrentIds = currentIds;
  self._wlRebuild = rebuild;

  function rowIndexOf(node: Node | null): number {
    const rows = root.querySelectorAll('.wl-row');
    for (let i = 0; i < rows.length; i++) {
      if (rows[i] === node) { return i; }
    }
    return -1;
  }

  function swap(arr: number[], a: number, b: number): void {
    const tmp = arr[a]; arr[a] = arr[b]; arr[b] = tmp;
  }

  root.addEventListener('click', function(ev: Event) {
    let target = ev.target as HTMLElement;
    if (!target) { return; }
    if (target.tagName !== 'BUTTON') {
      target = target.closest ? (target.closest('button') as HTMLElement) : null as any;
    }
    if (!target) { return; }

    if (target.classList.contains('wl-add')) {
      const ids = currentIds();
      if (ids.length < MAX) {
        ids.push(0);
        rebuild(ids);
        self.trigger('change');
      }
      return;
    }

    const rowEl = target.parentNode as HTMLElement; // the .wl-row
    const idx = rowIndexOf(rowEl);
    if (idx === -1) { return; }
    const ids = currentIds();

    if (target.classList.contains('wl-del')) {
      ids.splice(idx, 1);
      rebuild(ids);
      self.trigger('change');
    } else if (target.classList.contains('wl-up') && idx > 0) {
      swap(ids, idx, idx - 1);
      rebuild(ids);
      self.trigger('change');
    } else if (target.classList.contains('wl-down') && idx < ids.length - 1) {
      swap(ids, idx, idx + 1);
      rebuild(ids);
      self.trigger('change');
    }
  });

  // a row <select> changed: re-notify so the visibility fn re-runs
  root.addEventListener('change', function(ev: Event) {
    const t = ev.target as HTMLElement;
    if (t && t.tagName === 'SELECT') { self.trigger('change'); }
  });

  // Rebuild a row <select>'s options from the live crypto rows when the user
  // opens it (focus/mousedown), preserving the current selection.
  root.addEventListener('mousedown', function(ev: Event) {
    const t = ev.target as HTMLElement;
    if (t && t.tagName === 'SELECT' && t.classList.contains('wl-sel')) {
      const sel = t as HTMLSelectElement;
      const cur = parseInt(sel.value, 10) || 0;
      sel.innerHTML = optionsHtml(cur);
    }
  });
}

const widgetListComponent = {
  name: 'widgetList',
  template:
    '<div class="wl-root">' +
    '<div class="wl-list"></div>' +
    '<button type="button" class="wl-add">+ Add widget</button>' +
    '</div>',
  // NOTE: Clay's base theme styles `button { min-width: 12rem; margin: 0 auto }`
  // (elements/_button.scss). Our row buttons MUST override min-width (else each is
  // forced to 12rem, overflowing the row and squeezing the select to zero width)
  // and neutralize the auto margins. `.wl-row button` (specificity 0,1,1) beats
  // Clay's `button` (0,0,1), so these win regardless of stylesheet order.
  style:
    '.wl-row{display:flex;align-items:center;margin:0 0 8px 0}' +
    // Native <select> defaults to a light OS theme; match Clay's dark controls
    // (body bg gray-2 #333, buttons gray-7 #767676, white text). color-scheme:dark
    // nudges the OS-rendered option popup toward dark too where supported.
    '.wl-row .wl-sel{flex:1 1 auto;min-width:0;height:2.8rem;margin:0;' +
      'background-color:#767676;color:#fff;border:none;border-radius:0.3rem;' +
      'padding:0 0.5rem;color-scheme:dark}' +
    '.wl-row button{flex:0 0 auto;min-width:0;width:2.8rem;height:2.8rem;' +
      'margin:0 0 0 6px;padding:0}' +
    '.wl-row button[disabled]{opacity:.35}' +
    '.wl-add{margin:8px 0 10px 0}',
  manipulator: {
    get: function(this: any): number[] {
      return this._wlCurrentIds ? this._wlCurrentIds() : [];
    },
    set: function(this: any, value: any) {
      // Inlined normalization — this function is serialized via toSource and
      // re-eval'd in the webview, so it must NOT reference any module-scope
      // helper (a referenced sibling fn would be undefined there). Accepts a
      // bare array (normal), a JSON string, or a {value:X} wrapper; anything
      // else -> empty list.
      let ids: any[] = [];
      let v: any = value;
      if (v && typeof v === 'object' && !Array.isArray(v) && v.value !== undefined) {
        v = v.value;
      }
      if (Array.isArray(v)) {
        ids = v;
      } else if (typeof v === 'string' && v !== '') {
        try {
          const parsed = JSON.parse(v);
          if (Array.isArray(parsed)) { ids = parsed; }
        } catch (e) { ids = []; }
      }
      if (this._wlRebuild) { this._wlRebuild(ids); }
      return this;
    },
  },
  defaults: { label: '' },
  initialize: widgetListInitialize,
};

export = widgetListComponent;
