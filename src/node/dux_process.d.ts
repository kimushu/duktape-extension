declare namespace Dux {
    interface ProcessVersions {
        /** Duktape version in semver format (X.X.X) */
        duktape: string;

        /** Dux (duktape-extension) version in semver format (X.X.X) */
        dux: string;
    }

    interface Process {
        /** Running architecture */
        readonly arch: string;

        /** Exit Duktape runtime at the end of this tick */
        exit: (exitCode: number) => void;

        /** Current exit code */
        exitCode: number;

        /**
         * Add callback to next tick queue
         * @param callback Callback function to be called in next tick
         * @param args Arguments to be passed to callback function
         */
        nextTick: (callback: Function, ...args) => void;

        /** Duktape version (vX.X.X) */
        readonly version: string;

        /** Object which stores versions of Duktape related modules */
        readonly versions: ProcessVersions;
    }
}

declare var process: Dux.Process;
