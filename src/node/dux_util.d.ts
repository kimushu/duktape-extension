declare namespace Dux {
    interface PromisifyFunction {
        /**
         * Convert function with callback to promisifed function
         * @param original Function to convert
         */
        <T>(original: Function): Promise<T>;

        /**
         * Symbol to store pre-promisified functions
         */
        custom: any; /* FIXME: Symbol */
    }
}

declare module "util" {
    /**
     * Generate string from printf-like string and objects
     * @param format Format string
     * @param args Substitutions
     */
    export function format(format: any, ...args: any[]): string;

    // export function inspect();

    export var promisify: Dux.PromisifyFunction;
}
