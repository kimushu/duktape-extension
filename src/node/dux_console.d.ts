declare namespace Dux {
    interface Console {
        /**
         * A simple assertion test which verifies whether value is truthy.
         * @param value Value to test
         * @param message Message will be thrown if assertion failed
         * @param args Substitutions for message format (see util.format)
         */
        assert(value: any, message?: string, ...args: any[]): void;

        /**
         * Prints error message to stderr
         * @param message Message string or object
         * @param args Substitutions for message format (see util.format)
         */
        error(message?: any, ...args: any[]): void;

        /**
         * Prints information message to stdout
         * @param message Message string or object
         * @param args Substitutions for message format (see util.format)
         */
        info(message?: any, ...args: any[]): void;

        /**
         * Prints log message to stdout
         * @param message Message string or object
         * @param args Substitutions for message format (see util.format)
         */
        log(message?: any, ...args: any[]): void;

        /**
         * Prints warning message to stderr
         * @param message Message string or object
         * @param args Substitutions for message format (see util.format)
         */
        warn(message?: any, ...args: any[]): void;
    }

    interface ConsoleConstructor {
        new (stdout/*: Dux.WritableStream*/, stderr?/*: Dux.WritableStream*/): Console;
        prototype: Console;
    }

    interface GlobalConsole extends Console {
        Console: ConsoleConstructor;
    }
}

declare var console: Dux.GlobalConsole;

declare module "console" {
    export = console;
}
