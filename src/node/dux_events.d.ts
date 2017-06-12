declare module "events" {
    class EventEmitter {
        /**
         * Circular reference to EventEmitter
         */
        static "EventEmitter": typeof EventEmitter;

        /**
         * Default max listeners
         */
        static defaultMaxListeners: number;

        /**
         * Adds the listener function to the end of the array of
         * listeners for the event. This is an alias of on().
         * @param eventName Name of the event
         * @param listener Listener function
         */
        addListener(eventName: any, listener: Function): EventEmitter;

        /**
         * Calls event listeners for the event
         * @param eventName Name of the event
         * @return true if the event had listeners
         */
        emit(eventName: any, ...args: any[]): boolean;

        /**
         * Get the list of event names
         */
        eventNames(): string[];

        /**
         * Get current maximum number of listeners
         */
        getMaxListeners(): number;

        /**
         * Get the number of listeners for the event
         * @param eventName Name of the event
         */
        listenerCount(eventName: any): number;

        /**
         * Get a copy of the list of listeners for the event
         * @param eventName Name of the event
         */
        listeners(eventName: any): Function[];

        /**
         * Adds the listener function to the end of the array of
         * listeners for the event.
         * @param eventName Name of the event
         * @param listener Listener function
         */
        on(eventName: any, listener: Function): EventEmitter;

        /**
         * Adds a one-time listener function to the end of the array of
         * listeners for the event.
         * @param eventName Name of the event
         * @param listener Listener function
         */
        once(eventName: any, listener: Function): EventEmitter;

        /**
         * Adds the listener function to the beginning of the array of
         * listeners for the event.
         * @param eventName Name of the event
         * @param listener Listener function
         */
        prependListener(eventName: any, listener: Function): EventEmitter;

        /**
         * Adds a one-time listener function to the beginning of
         * the array of listeners for the event.
         * @param eventName Name of the event
         * @param listener Listener function
         */
        prependOnceListener(eventName: any, listener: Function): EventEmitter;

        /**
         * Remove all listeners, or those of the specified event
         */
        removeAllListeners(eventName?: any): EventEmitter;

        /**
         * Remove the specified listener
         */
        removeListener(eventName: any, listener: Function): EventEmitter;

        /**
         * Set maximum number of listeners
         * @param n Max number of listeners
         */
        setMaxListeners(n: number): EventEmitter;
    }

    module EventEmitter {
    }

    export = EventEmitter;
}
