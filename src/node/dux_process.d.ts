declare namespace Dux {
    interface ProcessVersions {
        duktape: string;
        dux: string;
    }
    interface Process {
        readonly arch: string;
        exit: (exitCode: number) => void;
        exitCode: number;
        nextTick: (callback: Function, ...args) => void;
        readonly version: string;
        readonly versions: ProcessVersions;
    }
}
declare var process: Dux.Process;
