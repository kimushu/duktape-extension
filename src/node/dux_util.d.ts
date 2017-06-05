interface PromisifyFunction {
    <T>(original: Function): Promise<T>;
    custom: any; /* FIXME: Symbol */
}

declare module "util" {
    export function format(format: any, ...args: any[]): string;
    // export function inspect();
    export var promisify: PromisifyFunction;
}
