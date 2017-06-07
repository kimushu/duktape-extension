/**
 * Stop a periodic timer
 * @param id ID of periodic timer
 */
declare function clearInterval(id: number): void;

/**
 * Stop an oneshot timer
 * @param id ID of oneshot timer
 */
declare function clearTimeout(id: number): void;

/**
 * Start a new periodic timer
 * @param callback Callback function
 * @param delay Delay (Unit is usually "milliseconds" but it is dependent on running platform)
 * @param args Arguments to be passed to callback function
 */
declare function setInterval(callback: Function, delay: number, ...args): number;

/**
 * Start a new oneshot timer
 * @param callback Callback function
 * @param delay Delay (Unit is usually "milliseconds" but it is dependent on running platform)
 * @param args Arguments to be passed to callback function
 */
declare function setTimeout(callback: Function, delay: number, ...args): number;
