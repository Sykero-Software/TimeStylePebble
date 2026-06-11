// Minimal type surface for pebble-clay as used by this watchface. Clay ships no
// types of its own. We only construct a Clay instance and call generateUrl /
// getSettings.

declare module 'pebble-clay' {
  type ClayConfig = unknown[];
  type ClayCustomFn = (this: unknown, minified: unknown) => void;

  interface ClayOptions {
    autoHandleEvents?: boolean;
    userData?: unknown;
  }

  class Clay {
    constructor(config: ClayConfig, customFn?: ClayCustomFn | null, options?: ClayOptions);
    generateUrl(): string;
    /**
     * @param response the webviewclosed response string
     * @param convert when false, returns the unflattened `{ key: { value: X } }`
     *   shape (Clay's serialize form) rather than `{ key: X }`.
     */
    getSettings(response: string, convert?: boolean): Record<string, any>;
  }

  export = Clay;
}
