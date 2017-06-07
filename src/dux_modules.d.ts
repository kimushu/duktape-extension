interface DuxRequireFunction {
    /**
     * Load dux internal module
     */
    (id: string): any;
}

declare const require: DuxRequireFunction;
