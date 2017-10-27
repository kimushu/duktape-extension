declare namespace Dux {
    interface RequireFunction {
        /**
         * Load dux internal module
         */
        (id: string): any;
    }
    interface Module {
        id: string;
        parent: Module;
        filename: string;
    }
}
declare const require: Dux.RequireFunction;
declare const module: Dux.Module;
declare const __filename: string;
declare const __dirname: string;