declare namespace Dux {
    interface Immediate {
    }
}

/**
 * Cancel the "immediate" action for the current event-loop
 * @param immediate An Immediate object returned by setImmediate()
 */
declare function clearImmediate(immediate: Dux.Immediate): void;

/**
 * Schedules the "immediate" action for the current event loop
 * @param callback Callback function to call at the end of this event loop
 * @param args Arguments to pass to the callback
 */
declare function setImmediate(callback: Function, ...args: any[]): Dux.Immediate;
