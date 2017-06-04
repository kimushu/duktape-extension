declare interface Process {
    exit: (exitCode: number) => void;
    nextTick: (callback: Function, ...args) => void;
    readonly arch: string;
    exitCode: number;
}
declare module "process" {
    export = Process;
}
declare var process: Process;
