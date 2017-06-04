declare interface Console {
    assert(value: any, message?: string, ...args: any[]): void;
    error(message?: any, ...args: any[]): void;
    info(message?: any, ...args: any[]): void;
    log(message?: any, ...args: any[]): void;
    warn(message?: any, ...args: any[]): void;
}
declare var console: Console;
