declare module "timers" {
    export function clearInterval(id: number): void;
    export function clearTimeout(id: number): void;
    export function setInterval(callback: Function, delay: number, ...args): number;
    export function setTimeout(callback: Function, delay: number, ...args): number;
}
